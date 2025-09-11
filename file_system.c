#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BLOCK_SIZE 256
#define MAX_BLOCKS 4096
#define MAX_FILES 128

// Allocation type
#define ALLOC_CONTIGUOUS 0

// File entry structure
typedef struct {
	char name[32];
	int start_block;
	int size_bytes; // actual content length
	int blocks_used;
	int allocation_type;
	bool used;
} FileEntry;

// File system structure
typedef struct {
	FileEntry files[MAX_FILES];
	bool block_map[MAX_BLOCKS];
} FileSystem;

FileSystem fs;
FILE *virtual_disk;

void init_file_system() {
	memset(&fs, 0, sizeof(fs));
	virtual_disk = fopen("virtual_disk.img", "r+b");
	if (!virtual_disk) {
    	virtual_disk = fopen("virtual_disk.img", "w+b");
    	fseek(virtual_disk, BLOCK_SIZE * MAX_BLOCKS - 1, SEEK_SET);
    	fputc('\0', virtual_disk);
    	fflush(virtual_disk);
	}
}

void shutdown_file_system() {
	fclose(virtual_disk);
}

int find_free_blocks(int blocks_needed) {
	int count = 0;
	for (int i = 0; i <= MAX_BLOCKS - blocks_needed; i++) {
    	bool found = true;
    	for (int j = 0; j < blocks_needed; j++) {
        	if (fs.block_map[i + j]) {
            	found = false;
            	break;
        	}
    	}
    	if (found) return i;
	}
	return -1;
}

void create_file(const char *name, const char *content) {
	for (int i = 0; i < MAX_FILES; i++) {
    	if (fs.files[i].used && strcmp(fs.files[i].name, name) == 0) {
        	printf("File with this name already exists.\n");
        	return;
    	}
	}

	int content_length = strlen(content);
	int blocks_needed = (content_length + BLOCK_SIZE - 1) / BLOCK_SIZE;

	int start_block = find_free_blocks(blocks_needed);
	if (start_block == -1) {
    	printf("Not enough contiguous space available.\n");
    	return;
	}

	for (int i = start_block; i < start_block + blocks_needed; i++) {
    	fs.block_map[i] = true;
	}

	for (int i = 0; i < MAX_FILES; i++) {
    	if (!fs.files[i].used) {
        	strcpy(fs.files[i].name, name);
        	fs.files[i].start_block = start_block;
        	fs.files[i].size_bytes = content_length;
        	fs.files[i].blocks_used = blocks_needed;
        	fs.files[i].allocation_type = ALLOC_CONTIGUOUS;
        	fs.files[i].used = true;
        	break;
    	}
	}

	fseek(virtual_disk, start_block * BLOCK_SIZE, SEEK_SET);
	fwrite(content, 1, content_length, virtual_disk);
	fflush(virtual_disk);

	printf("File '%s' created successfully.\n", name);
}

void list_files() {
	printf("Files:\n");
	for (int i = 0; i < MAX_FILES; i++) {
    	if (fs.files[i].used) {
        	printf("- %s (%d bytes, %d blocks used)\n",
               	fs.files[i].name,
               	fs.files[i].size_bytes,
               	fs.files[i].blocks_used);
    	}
	}
}

void view_file(const char *name) {
	for (int i = 0; i < MAX_FILES; i++) {
    	if (fs.files[i].used && strcmp(fs.files[i].name, name) == 0) {
        	char *buffer = malloc(fs.files[i].size_bytes + 1);
        	fseek(virtual_disk, fs.files[i].start_block * BLOCK_SIZE, SEEK_SET);
        	fread(buffer, 1, fs.files[i].size_bytes, virtual_disk);
        	buffer[fs.files[i].size_bytes] = '\0';
        	printf("Content of '%s':\n%s\n", name, buffer);
        	free(buffer);
        	return;
    	}
	}
	printf("File not found.\n");
}

void delete_file(const char *name) {
	for (int i = 0; i < MAX_FILES; i++) {
    	if (fs.files[i].used && strcmp(fs.files[i].name, name) == 0) {
        	for (int j = fs.files[i].start_block;
             	j < fs.files[i].start_block + fs.files[i].blocks_used; j++) {
            	fs.block_map[j] = false;
        	}
        	fs.files[i].used = false;
        	printf("File '%s' deleted.\n", name);
        	return;
    	}
	}
	printf("File not found.\n");
}

void edit_file(const char *name, const char *new_content) {
	for (int i = 0; i < MAX_FILES; i++) {
		if (fs.files[i].used && strcmp(fs.files[i].name, name) == 0) {
			int new_length = strlen(new_content);
			int new_blocks = (new_length + BLOCK_SIZE - 1) / BLOCK_SIZE;

			// Free old blocks
			for (int j = fs.files[i].start_block; 
			     j < fs.files[i].start_block + fs.files[i].blocks_used; j++) {
				fs.block_map[j] = false;
			}

			// Find new space
			int new_start_block = find_free_blocks(new_blocks);
			if (new_start_block == -1) {
				printf("Not enough contiguous space available for updated content.\n");

				// Reallocate the old blocks back if new space can't be found
				for (int j = fs.files[i].start_block;
				     j < fs.files[i].start_block + fs.files[i].blocks_used; j++) {
					fs.block_map[j] = true;
				}

				return;
			}

			for (int j = new_start_block; j < new_start_block + new_blocks; j++) {
				fs.block_map[j] = true;
			}

			fs.files[i].start_block = new_start_block;
			fs.files[i].size_bytes = new_length;
			fs.files[i].blocks_used = new_blocks;

			fseek(virtual_disk, new_start_block * BLOCK_SIZE, SEEK_SET);
			fwrite(new_content, 1, new_length, virtual_disk);
			fflush(virtual_disk);

			printf("File '%s' updated successfully.\n", name);
			return;
		}
	}
	printf("File not found.\n");
}




int main() {
	init_file_system();

	int choice;
	char name[32];
	char content[1024];

	while (1) {
    	printf("\n1. Create file\n2. List files\n3. View file\n4. Delete file\n5. Edit file\n6. Exit\nChoose an option: ");
    	scanf("%d", &choice);
    	getchar(); 

    	switch (choice) {
        	case 1:
            	printf("Enter file name: ");
            	fgets(name, sizeof(name), stdin);
            	name[strcspn(name, "\n")] = '\0';
            	printf("Enter file content: ");
            	fgets(content, sizeof(content), stdin);
            	content[strcspn(content, "\n")] = '\0';
            	create_file(name, content);
            	break;
        	case 2:
            	list_files();
            	break;
        	case 3:
            	printf("Enter file name to view: ");
            	fgets(name, sizeof(name), stdin);
            	name[strcspn(name, "\n")] = '\0';
            	view_file(name);
            	break;
        	case 4:
            	printf("Enter file name to delete: ");
            	fgets(name, sizeof(name), stdin);
            	name[strcspn(name, "\n")] = '\0';
            	delete_file(name);
            	break;
        	case 5:
            	printf("Enter file name to edit: ");
            	fgets(name, sizeof(name), stdin);
            	name[strcspn(name, "\n")] = '\0';
            	printf("Enter new content: ");
            	fgets(content, sizeof(content), stdin);
            	content[strcspn(content, "\n")] = '\0';
            	edit_file(name, content);
            	break;
    	case 6:
            	shutdown_file_system();
            	return 0;
        	default:
            	printf("Invalid option.\n");
    	}
	}
}


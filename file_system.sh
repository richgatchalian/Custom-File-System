#!/bin/bash

VIRTUAL_DISK="virtual_disk.img"
DISK_SIZE=1048576  # 1MB virtual disk

# Initialize Virtual Disk
init_disk() {
    if [ ! -f $VIRTUAL_DISK ]; then
        dd if=/dev/zero of=$VIRTUAL_DISK bs=1 count=$DISK_SIZE
        echo "Virtual disk initialized: $VIRTUAL_DISK"
    else
        echo "Virtual disk already exists."
    fi
}

# Run File System Operations
run_fs() {
    gcc file_system.c -o file_system
    ./file_system
}

# Main Menu
echo "Custom File System"
echo "1. Initialize Virtual Disk"
echo "2. Run File System"
echo "3. Exit"
read -p "Choose an option: " option

case $option in
    1) init_disk ;;
    2) run_fs ;;
    3) exit ;;
    *) echo "Invalid option";;
esac

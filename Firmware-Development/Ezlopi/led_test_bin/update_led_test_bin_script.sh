#!/bin/bash

# Get the directory of the script
script_dir=$(dirname "$0")

# Destination directory where the files will be copied (same directory as the script)
destination_dir="$script_dir"

# Array of source directories where the files are located (led_test_binaries)
source_dirs=("../build/bootloader" "../build" "../build/partition_table" "../build")

# List of files to be copied from each source directory (led_test_binaries)
files=("bootloader.bin" "Ezlo_Pi_v3x.bin" "partition-table.bin" "ota_data_initial.bin")

# Loop through each source directory and copy files to destination directory
for ((i=0; i<${#source_dirs[@]}; i++)); do
    source_dir="${source_dirs[i]}"
    file="${files[i]}"
    cp "$source_dir/$file" "$destination_dir"
done

echo "Files copied successfully to destination directory: $destination_dir."

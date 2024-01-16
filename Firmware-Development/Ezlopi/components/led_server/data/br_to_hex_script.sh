#!/bin/bash

input_folder="brotli"
output_folder="br_to_hex_files"

# Check if input folder exists
if [ ! -d "$input_folder" ]; then
    echo "Error: Input folder not found."
    exit 1
fi

# Create the output folder if it doesn't exist
mkdir -p "$output_folder"

# Process each .br file in the input folder
for input_file in "$input_folder"/*.br; do
    if [ -e "$input_file" ]; then
        # Extract the base name without extension and replace dot with underscore
        output_base_name=$(basename "${input_file%.*}" | tr '.' '_')

        # Generate output file name
        output_file="$output_folder/compressed_${output_base_name}.h"

        # Create the C hexdump using xxd with const unsigned char and const unsigned int
        xxd -i "$input_file" | sed -e 's/unsigned char/const unsigned char/' -e 's/unsigned int/const unsigned int/' > "$output_file"

        echo "C hexdump saved to: $output_file"
    fi
done

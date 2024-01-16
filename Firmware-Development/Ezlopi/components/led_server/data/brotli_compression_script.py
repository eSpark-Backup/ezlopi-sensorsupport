import os
import brotli

def compress_file(input_path, output_folder):
    # Create the output folder if it doesn't exist
    os.makedirs(output_folder, exist_ok=True)

    # Build the output file path
    output_path = os.path.join(output_folder, os.path.basename(input_path) + '.br')

    try:
        with open(input_path, 'rb') as input_file:
            with open(output_path, 'wb') as output_file:
                output_file.write(brotli.compress(input_file.read()))
        print(f"File compressed and saved to: {output_path}")
    except Exception as e:
        print(f"Error compressing file: {e}")

# Example usage
compress_file('index.htm', 'brotli')
compress_file('index.css', 'brotli')
compress_file('index.js', 'brotli')
compress_file('iro.js', 'brotli')
compress_file('rangetouch.js', 'brotli')

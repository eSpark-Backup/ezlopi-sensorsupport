#!/bin/bash

# Define custom variable names
INDEX_HTML_VAR="compressed_index_html"
INDEX_CSS_VAR="compressed_index_css"
INDEX_JS_VAR="compressed_index_js"
IRO_JS_VAR="compressed_iro_js"
RANGETOUCH_JS_VAR="compressed_rangetouch_js"

# Define output folders
COMPRESSED_FOLDER="gzip"
HEADER_FOLDER="gzip_to_hex_files"
FINAL_HEADER_PATH="../compressed_webpage_files"  # Adjust the destination folder as needed

# Create output folders if they don't exist
mkdir -p "$COMPRESSED_FOLDER"
mkdir -p "$HEADER_FOLDER"

# Compress HTML, CSS, and JS files using 7-Zip and move them to the compressed folder
"C:\Program Files\7-Zip\7z.exe" a -tgzip "$COMPRESSED_FOLDER/index.html.gz" "index.htm"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "$COMPRESSED_FOLDER/index.css.gz" "index.css"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "$COMPRESSED_FOLDER/index.js.gz" "index.js"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "$COMPRESSED_FOLDER/iro.js.gz" "iro.js"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "$COMPRESSED_FOLDER/rangetouch.js.gz" "rangetouch.js"

# Convert compressed files to C-style arrays using xxd and declare as const arrays
xxd -i -g 1 "$COMPRESSED_FOLDER/index.html.gz" | sed "s/^unsigned/const unsigned/; s/^unsigned/const unsigned/; s/^\(\s*\)unsigned char.*/\1const unsigned char $INDEX_HTML_VAR\[\] = {/" > "$HEADER_FOLDER/$INDEX_HTML_VAR.h"
xxd -i -g 1 "$COMPRESSED_FOLDER/index.css.gz" | sed "s/^unsigned/const unsigned/; s/^unsigned/const unsigned/; s/^\(\s*\)unsigned char.*/\1const unsigned char $INDEX_CSS_VAR\[\] = {/" > "$HEADER_FOLDER/$INDEX_CSS_VAR.h"
xxd -i -g 1 "$COMPRESSED_FOLDER/index.js.gz" | sed "s/^unsigned/const unsigned/; s/^unsigned/const unsigned/; s/^\(\s*\)unsigned char.*/\1const unsigned char $INDEX_JS_VAR\[\] = {/" > "$HEADER_FOLDER/$INDEX_JS_VAR.h"
xxd -i -g 1 "$COMPRESSED_FOLDER/iro.js.gz" | sed "s/^unsigned/const unsigned/; s/^unsigned/const unsigned/; s/^\(\s*\)unsigned char.*/\1const unsigned char $IRO_JS_VAR\[\] = {/" > "$HEADER_FOLDER/$IRO_JS_VAR.h"
xxd -i -g 1 "$COMPRESSED_FOLDER/rangetouch.js.gz" | sed "s/^unsigned/const unsigned/; s/^unsigned/const unsigned/; s/^\(\s*\)unsigned char.*/\1const unsigned char $RANGETOUCH_JS_VAR\[\] = {/" > "$HEADER_FOLDER/$RANGETOUCH_JS_VAR.h"

# Copy header files to the specified path
cp "$HEADER_FOLDER/$INDEX_HTML_VAR.h" "$FINAL_HEADER_PATH"
cp "$HEADER_FOLDER/$INDEX_CSS_VAR.h" "$FINAL_HEADER_PATH"
cp "$HEADER_FOLDER/$INDEX_JS_VAR.h" "$FINAL_HEADER_PATH"
cp "$HEADER_FOLDER/$IRO_JS_VAR.h" "$FINAL_HEADER_PATH"
cp "$HEADER_FOLDER/$RANGETOUCH_JS_VAR.h" "$FINAL_HEADER_PATH"

# Remove the compressed folder
rm -r "$COMPRESSED_FOLDER"
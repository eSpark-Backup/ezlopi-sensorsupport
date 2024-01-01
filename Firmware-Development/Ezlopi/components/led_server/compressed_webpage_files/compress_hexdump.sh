#!/bin/bash

# Compress HTML, CSS, and JS files using 7-Zip
"C:\Program Files\7-Zip\7z.exe" a -tgzip "index.html.gz" "index.htm"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "index.css.gz" "index.css"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "index.js.gz" "index.js"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "iro.js.gz" "iro.js"
"C:\Program Files\7-Zip\7z.exe" a -tgzip "rangetouch.js.gz" "rangetouch.js"

# Convert compressed files to C-style arrays using xxd
xxd -i -g 1 "index.html.gz" > "compressed_index_html.h"
xxd -i -g 1 "index.css.gz" > "compressed_index_css.h"
xxd -i -g 1 "index.js.gz" > "compressed_index_js.h"
xxd -i -g 1 "iro.js.gz" > "compressed_iro_js.h"
xxd -i -g 1 "rangetouch.js.gz" > "compressed_rangetouch_js.h"

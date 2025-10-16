#!/usr/bin/env python3
import gzip
import re

# Read the file and extract bytes
with open('.pio/libdeps/esp32-s3-devkitc-1/ElegantOTA/src/elop.cpp', 'r') as f:
    content = f.read()

# Extract the array data
match = re.search(r'const uint8_t ELEGANT_HTML\[\d+\] PROGMEM = \{(.*?)\};', content, re.DOTALL)
if match:
    array_content = match.group(1)
    # Split by comma and convert to integers
    byte_values = []
    for val in array_content.split(','):
        val = val.strip()
        if val.isdigit():
            byte_values.append(int(val))

    print(f'Extracted {len(byte_values)} bytes')

    # Create gz file
    with open('elegant_html.gz', 'wb') as f:
        f.write(bytes(byte_values))

    print('Created elegant_html.gz')

    # Try to decompress
    try:
        with gzip.open('elegant_html.gz', 'rt', encoding='utf-8') as f:
            html_content = f.read()
        print('Successfully decompressed!')

        # Save HTML file
        with open('elegant_ota.html', 'w', encoding='utf-8') as f:
            f.write(html_content)
        print('Created elegant_ota.html')
        print(f'HTML length: {len(html_content)} characters')

        # Show first few lines
        lines = html_content.split('\n')[:10]
        print('\nFirst 10 lines of HTML:')
        for i, line in enumerate(lines, 1):
            print(f'{i:2}: {line}')

    except Exception as e:
        print(f'Decompression failed: {e}')
        print('This might not be gzip compressed data, or it might be corrupted.')
else:
    print('Could not find the ELEGANT_HTML array in the file')

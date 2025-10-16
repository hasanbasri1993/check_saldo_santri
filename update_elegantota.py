#!/usr/bin/env python3
"""
Script to compress HTML and update both elop.h and elop.cpp with new ElegantOTA data
"""

import gzip
import os

def compress_html_to_c_array(input_file, output_file):
    """Compress HTML file to gzipped C byte array format"""

    # Read the HTML file
    with open(input_file, 'rb') as f:
        html_content = f.read()

    print(f"Original HTML size: {len(html_content)} bytes")

    # Compress with gzip
    compressed_data = gzip.compress(html_content)

    print(f"Compressed size: {len(compressed_data)} bytes")
    print(f"Compression ratio: {len(compressed_data)/len(html_content)*100:.1f}%")

    # Convert to C array format
    array_lines = []
    array_lines.append("const uint8_t ELEGANT_HTML[{}] PROGMEM = {{".format(len(compressed_data)))

    # Process bytes in chunks of 16 for readability
    for i in range(0, len(compressed_data), 16):
        chunk = compressed_data[i:i+16]
        hex_values = [str(b) for b in chunk]
        if i + 16 >= len(compressed_data):
            # Last line, no trailing comma
            array_lines.append("  " + ", ".join(hex_values))
        else:
            array_lines.append("  " + ", ".join(hex_values) + ",")

    array_lines.append("};")
    array_lines.append("")

    # Write to output file
    with open(output_file, 'w') as f:
        f.write('\n'.join(array_lines))

    print(f"C array written to {output_file}")
    print(f"Array contains {len(compressed_data)} bytes")

    return len(compressed_data)

def update_elop_h(array_size):
    """Update elop.h with new array size"""

    elop_h_path = '.pio/libdeps/esp32-s3-devkitc-1/ElegantOTA/src/elop.h'

    # Read the current content
    with open(elop_h_path, 'r') as f:
        content = f.read()

    # Update the array size declaration
    old_line = 'extern const uint8_t ELEGANT_HTML[10667];'
    new_line = f'extern const uint8_t ELEGANT_HTML[{array_size}];'

    if old_line in content:
        content = content.replace(old_line, new_line)
    else:
        # If the exact line isn't found, try a more flexible replacement
        import re
        content = re.sub(r'extern const uint8_t ELEGANT_HTML\[\d+\];',
                        f'extern const uint8_t ELEGANT_HTML[{array_size}];', content)

    # Write back to file
    with open(elop_h_path, 'w') as f:
        f.write(content)

    print(f"✅ Updated elop.h with array size: {array_size}")
    return True

def update_elop_cpp():
    """Replace the ELEGANT_HTML array in elop.cpp with new compressed data"""

    elop_cpp_path = '.pio/libdeps/esp32-s3-devkitc-1/ElegantOTA/src/elop.cpp'

    # Read the original file
    with open(elop_cpp_path, 'r') as f:
        original_content = f.read()

    # Read the new compressed array
    with open('compressed_array.txt', 'r') as f:
        new_array = f.read()

    # Replace the old array with the new one
    # Find the start and end of the old array
    start_marker = 'const uint8_t ELEGANT_HTML['
    end_marker = '};'

    start_idx = original_content.find(start_marker)
    if start_idx == -1:
        print("❌ Could not find ELEGANT_HTML array in elop.cpp")
        return False

    # Find where the array ends (the closing brace and semicolon)
    end_idx = original_content.rfind(end_marker)
    if end_idx == -1:
        print("❌ Could not find end of ELEGANT_HTML array")
        return False

    # Extract the end of the array (including the closing brace and semicolon)
    end_idx += len(end_marker)

    # Replace the old array with the new one
    new_content = original_content[:start_idx] + new_array + original_content[end_idx:]

    # Write back to file
    with open(elop_cpp_path, 'w') as f:
        f.write(new_content)

    print("✅ Successfully updated elop.cpp with new compressed array")
    return True

def main():
    """Main function to compress HTML and update both header and source files"""

    input_file = "elegant_ota.html"
    output_file = "compressed_array.txt"

    # Check if input file exists
    if not os.path.exists(input_file):
        print(f"❌ Error: Could not find input file '{input_file}'")
        return False

    try:
        # Step 1: Compress the HTML
        print("🔄 Step 1: Compressing HTML...")
        array_size = compress_html_to_c_array(input_file, output_file)

        # Step 2: Update elop.h
        print("\n🔄 Step 2: Updating elop.h...")
        update_elop_h(array_size)

        # Step 3: Update elop.cpp
        print("\n🔄 Step 3: Updating elop.cpp...")
        update_elop_cpp()

        print("\n🎉 All files updated successfully!")
        print(f"📊 New compressed size: {array_size} bytes")
        print("✅ ElegantOTA HTML has been updated with your changes")
        return True

    except Exception as e:
        print(f"❌ Error during update process: {e}")
        return False

if __name__ == "__main__":
    success = main()
    if not success:
        exit(1)

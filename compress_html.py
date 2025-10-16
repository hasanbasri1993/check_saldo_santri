#!/usr/bin/env python3
import gzip
import io

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

if __name__ == "__main__":
    input_file = "elegant_ota.html"
    output_file = "compressed_array.txt"

    try:
        final_size = compress_html_to_c_array(input_file, output_file)
        print(f"\n✅ Successfully compressed {input_file} to {output_file}")
        print(f"📊 Final compressed size: {final_size} bytes")
    except FileNotFoundError:
        print(f"❌ Error: Could not find input file '{input_file}'")
    except Exception as e:
        print(f"❌ Error during compression: {e}")

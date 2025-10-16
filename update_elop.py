#!/usr/bin/env python3
"""
Script to update the ELEGANT_HTML array in elop.cpp with newly compressed data
"""

def update_elop_cpp():
    """Replace the ELEGANT_HTML array in elop.cpp with new compressed data"""

    # Read the original file
    with open('.pio/libdeps/esp32-s3-devkitc-1/ElegantOTA/src/elop.cpp', 'r') as f:
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
    with open('.pio/libdeps/esp32-s3-devkitc-1/ElegantOTA/src/elop.cpp', 'w') as f:
        f.write(new_content)

    print("✅ Successfully updated elop.cpp with new compressed array")
    return True

if __name__ == "__main__":
    try:
        success = update_elop_cpp()
        if success:
            print("🎉 elop.cpp has been updated with the new compressed HTML data!")
        else:
            print("❌ Failed to update elop.cpp")
    except Exception as e:
        print(f"❌ Error updating elop.cpp: {e}")

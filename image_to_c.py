from PIL import Image
import os
import sys
import re

def sanitize_name(filename):
    name = os.path.splitext(os.path.basename(filename))[0]
    return re.sub(r'\W|^(?=\d)', '_', name)

def convert_image_to_rgb3bit_array(image_path):
    img = Image.open(image_path).convert('RGB')
    img = img.resize((128, 75), Image.LANCZOS)

    data = []
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = img.getpixel((x, y))
            r_bit = 1 if r > 127 else 0
            g_bit = 1 if g > 127 else 0
            b_bit = 1 if b > 127 else 0
            rgb3bit = (b_bit << 2) | (g_bit << 1) | r_bit  # BGR → 0b00000BGR
            data.append(rgb3bit)

    return data

def write_c_file(images_data, output_path):
    with open(output_path, 'w') as f:
        f.write('#include "hello_world.h"\n\n')
        f.write('// Auto-generated from input folder\n')
        f.write('// Format: 1-bit per R, G, B channel packed into 3 LSBs of each byte\n\n')

        array_names = []

        for name, data in images_data.items():
            array_name = f'image_{name}'
            array_names.append(array_name)

            f.write(f'const uint8_t {array_name}[{len(data)}] = {{\n')
            for i, byte in enumerate(data):
                f.write(f'0x{byte:02X}, ')
                if (i + 1) % 16 == 0:
                    f.write('\n')
            f.write('\n};\n\n')

        # Write the all_frames pointer array
        f.write(f'const uint8_t* const all_frames[{len(array_names)}] = {{\n')
        for name in array_names:
            f.write(f'    {name},\n')
        f.write('};\n')

def is_image_file(filename):
    return filename.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp', '.gif'))

def main(input_folder, output_c_path):
    images_data = {}
    for filename in sorted(os.listdir(input_folder)):
        if is_image_file(filename):
            path = os.path.join(input_folder, filename)
            print(f"🖼️  Processing: {filename}")
            data = convert_image_to_rgb3bit_array(path)
            sanitized = sanitize_name(filename)
            images_data[sanitized] = data

    write_c_file(images_data, output_c_path)
    print(f"\n✅ All images written to {output_c_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python batch_image_to_c.py <image_folder> <output.c>")
        sys.exit(1)

    folder = sys.argv[1]
    output_file = sys.argv[2]
    main(folder, output_file)

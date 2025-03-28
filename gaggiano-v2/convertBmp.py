import argparse
from PIL import Image

def convert_to_rgb565(image_path):
    try:
        # Open the bitmap image
        bitmap = Image.open(image_path)

        # Convert image to RGB mode (in case it's in a different mode)
        bitmap = bitmap.convert("RGB")

        # Get image dimensions
        width, height = bitmap.size

        # Open a file to write RGB565 data
        output_file = image_path.rsplit('.', 1)[0] + '.rgb565'
        with open(output_file, 'wb') as f:
            for y in range(height):
                for x in range(width):
                    # Get RGB color tuple at pixel (x, y)
                    r, g, b = bitmap.getpixel((x, y))

                    # Convert RGB to RGB565
                    rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

                    # Write the RGB565 value as 2 bytes (big-endian)
                    f.write(rgb565.to_bytes(2, byteorder='big'))

        print(f"RGB565 data saved to {output_file}")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert a bitmap image to RGB565 format.')
    parser.add_argument('image_path', type=str, help='Path to the bitmap image')
    args = parser.parse_args()
    convert_to_rgb565(args.image_path);

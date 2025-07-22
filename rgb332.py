from PIL import Image

def rgb24_to_rgb332(r, g, b):
    r3 = (r >> 5) & 0x07
    g3 = (g >> 5) & 0x07
    b2 = (b >> 6) & 0x03
    
    return (r3 << 5) | (g3 << 2) | b2

def rgb24_to_rgb332_from_hex(hex_color):
    if isinstance(hex_color, str):
        hex_color = int(hex_color, 16)

    r = (hex_color >> 16) & 0xff
    g = (hex_color >> 8) & 0xff
    b = hex_color & 0xff

    r3 = r >> 5
    g3 = g >> 5
    b2 = b >> 6
    rgb332 = (r3 << 5) | (g3 << 2) | b2

    return rgb332

def convert_image_to_rgb332(image_path, output_bin_path):
    img = Image.open(image_path).convert('RGB')
    img = img.resize((320, 200))
    pixels = list(img.getdata())

    words = bytearray()

    for i in range(0, len(pixels), 2):
        rgb1 = rgb24_to_rgb332(*pixels[i])
        rgb2 = rgb24_to_rgb332(*pixels[i+1]) if i+1 < len(pixels) else 0
        word = (rgb2 << 8) | rgb1
        words.append(word & 0xFF)
        words.append((word >> 8) & 0xFF)

    with open(output_bin_path, 'wb') as f:
        f.write(words)

    print(f"Written {len(words)} bytes to {output_bin_path}")

width, height = 320, 200

def make_rgb332(r, g, b):
    r3 = r >> 5
    g3 = g >> 5
    b2 = b >> 6
    return (r3 << 5) | (g3 << 2) | b2

red = make_rgb332(255, 0, 0)
black = make_rgb332(0, 0, 0)

image_data = bytearray()

tile_size = 20

for y in range(height):
    for x in range(width):
        if ((x // tile_size) + (y // tile_size)) % 2 == 0:
            image_data.append(red)
        else:
            image_data.append(black)

with open("checkerboard.rgb332", "wb") as f:
    f.write(image_data)

hex_color = 0x2b66bc
rgb332 = rgb24_to_rgb332_from_hex(hex_color)

print(f"RGB332 value: 0x{rgb332:02X}")
# Generate Multi-Resolution Tray Icons for OpenKey
# Uses Pillow to create .ico files with multiple sizes for crisp display at any DPI

import os
import io
import struct
import urllib.request
from PIL import Image, ImageDraw, ImageFont

# Configuration
SIZES = [16, 20, 24, 32, 48, 64, 128, 256]
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))  # tools/ directory
OUTPUT_DIR = os.path.join(SCRIPT_DIR, "..", "Sources", "OpenKey", "win32", "OpenKey", "OpenKey")
# Arial Rounded MT Bold - copy from C:\Windows\Fonts\ARLRDBD.TTF or download manually
# Place the font file in tools/ directory as "ArialRoundedMTBold.ttf"
FONT_PATH = os.path.join(SCRIPT_DIR, "ArialRoundedMTBold.ttf")

# Colors (RGBA)
COLORS = {
    "V": (243, 98, 103, 255),     # Red #f36267 for Vietnamese
    "E": (47, 175, 218, 255),     # Blue #2fafda for English
    "V_gray": (255, 255, 255, 255),  # White for gray theme
    "E_gray": (255, 255, 255, 255),  # White for gray theme
    "V_black": (0, 0, 0, 255),       # Black for light theme
    "E_black": (0, 0, 0, 255),       # Black for light theme
}

# Background - transparent
BG_COLOR = (0, 0, 0, 0)

def download_font():
    """Check if font exists, provide instructions if not"""
    if not os.path.exists(FONT_PATH):
        print("ERROR: Font not found!")
        print(f"Please copy Arial Rounded MT Bold to: {FONT_PATH}")
        print("On Windows, the font is at: C:\\Windows\\Fonts\\ARLRDBD.TTF")
        exit(1)
    return FONT_PATH

def create_icon_image(letter: str, size: int, color: tuple) -> Image.Image:
    """Create a single icon image with the given letter, size, and color"""
    # Create transparent image
    img = Image.new('RGBA', (size, size), BG_COLOR)
    draw = ImageDraw.Draw(img)
    
    # Calculate font size (120% of icon size - font metrics don't fill full space)
    font_size = int(size * 1.2)
    
    try:
        font = ImageFont.truetype(FONT_PATH, font_size)
    except:
        # Fallback to default font
        font = ImageFont.load_default()
    
    # Get text bounding box for centering
    bbox = draw.textbbox((0, 0), letter, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    # Center the text
    x = (size - text_width) // 2 - bbox[0]
    y = (size - text_height) // 2 - bbox[1]
    
    # stroke_width = max(1, size // 16)
    # draw.text((x, y), letter, font=font, fill=color, stroke_width=stroke_width, stroke_fill=color)
    # Draw the letter with anti-aliasing
    draw.text((x, y), letter, font=font, fill=color)
    
    return img

def create_ico_manually(images: list, output_path: str):
    """Create ICO file manually with proper multi-size embedding"""
    # ICO file structure:
    # ICONDIR header (6 bytes)
    # ICONDIRENTRY for each image (16 bytes each)
    # Image data (PNG format for each)
    
    num_images = len(images)
    
    # Create PNG data for each image
    png_data_list = []
    for img in images:
        buffer = io.BytesIO()
        img.save(buffer, format='PNG')
        png_data_list.append(buffer.getvalue())
    
    # Calculate offsets
    header_size = 6 + 16 * num_images
    
    # Build ICONDIR header
    # Reserved (2 bytes) = 0
    # Type (2 bytes) = 1 for ICO
    # Count (2 bytes) = number of images
    header = struct.pack('<HHH', 0, 1, num_images)
    
    # Build ICONDIRENTRY for each image
    entries = b''
    offset = header_size
    
    for i, (img, png_data) in enumerate(zip(images, png_data_list)):
        width = img.width if img.width < 256 else 0  # 0 means 256
        height = img.height if img.height < 256 else 0
        
        # ICONDIRENTRY structure:
        # Width (1 byte)
        # Height (1 byte)
        # ColorCount (1 byte) = 0 for >256 colors
        # Reserved (1 byte) = 0
        # Planes (2 bytes) = 1
        # BitCount (2 bytes) = 32 for RGBA
        # SizeInBytes (4 bytes)
        # FileOffset (4 bytes)
        
        entry = struct.pack('<BBBBHHII', 
                           width, height, 0, 0, 1, 32, 
                           len(png_data), offset)
        entries += entry
        offset += len(png_data)
    
    # Write the ICO file
    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(entries)
        for png_data in png_data_list:
            f.write(png_data)

def create_ico_file(letter: str, color: tuple, output_filename: str):
    """Create an .ico file with multiple sizes"""
    images = []
    
    for size in SIZES:
        img = create_icon_image(letter, size, color)
        images.append(img)
    
    output_path = os.path.join(OUTPUT_DIR, output_filename)
    
    # Use manual ICO creation for proper multi-size embedding
    create_ico_manually(images, output_path)
    
    print(f"Created: {output_filename}")

def main():
    # Download font
    download_font()
    
    # Create color icons
    create_ico_file("V", COLORS["V"], "StatusViet.ico")
    create_ico_file("E", COLORS["E"], "StatusEng.ico")
    
    # Create gray/white icons (for Win10 style)
    create_ico_file("V", COLORS["V_gray"], "StatusViet10.ico")
    create_ico_file("E", COLORS["E_gray"], "StatusEng10.ico")
    
    # Create black icons (for light theme)
    create_ico_file("V", COLORS["V_black"], "StatusVietBlack.ico")
    create_ico_file("E", COLORS["E_black"], "StatusEngBlack.ico")
    
    print("\nAll icons generated successfully!")
    print(f"Output directory: {OUTPUT_DIR}")
    print(f"Sizes included: {SIZES}")

if __name__ == "__main__":
    main()

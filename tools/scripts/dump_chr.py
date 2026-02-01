#!/usr/bin/env python3
"""
Dump CHR ROM data from NES ROMs to PNG images.

Usage:
    ./dump_chr.py <rom.nes> [output.png] [--palette COLORS]

Examples:
    ./dump_chr.py ../../roms/smb.nes
    ./dump_chr.py ../../roms/smb.nes smb_tiles.png
    ./dump_chr.py ../../roms/smb.nes smb_tiles.png --palette 000000 7c7c7c bcbcbc ffffff

Requires: pip install Pillow
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow is required. Install with: pip install Pillow")
    sys.exit(1)


# iNES header constants
INES_HEADER_SIZE = 16
PRG_ROM_UNIT = 16384  # 16 KB
CHR_ROM_UNIT = 8192   # 8 KB
TILE_SIZE = 16        # bytes per tile
TILE_WIDTH = 8        # pixels
TILE_HEIGHT = 8       # pixels
TILES_PER_ROW = 16    # tiles per row in output image

# Default grayscale palette
DEFAULT_PALETTE = [
    (0x00, 0x00, 0x00),  # Black
    (0x55, 0x55, 0x55),  # Dark gray
    (0xAA, 0xAA, 0xAA),  # Light gray
    (0xFF, 0xFF, 0xFF),  # White
]


def parse_ines_header(data: bytes) -> dict:
    """Parse iNES header and return ROM info."""
    if len(data) < INES_HEADER_SIZE:
        raise ValueError("File too small to contain iNES header")

    # Check magic number "NES\x1a"
    if data[0:4] != b'NES\x1a':
        raise ValueError("Not a valid iNES ROM (missing NES header)")

    prg_rom_size = data[4] * PRG_ROM_UNIT
    chr_rom_size = data[5] * CHR_ROM_UNIT

    # Check for trainer (512 bytes before PRG ROM)
    has_trainer = bool(data[6] & 0x04)
    trainer_size = 512 if has_trainer else 0

    chr_rom_offset = INES_HEADER_SIZE + trainer_size + prg_rom_size

    return {
        'prg_rom_size': prg_rom_size,
        'chr_rom_size': chr_rom_size,
        'chr_rom_offset': chr_rom_offset,
        'has_trainer': has_trainer,
    }


def decode_tile(tile_data: bytes) -> list:
    """
    Decode a single 8x8 NES tile from 16 bytes of CHR data.

    NES tiles use 2 bitplanes:
    - First 8 bytes: low bit of each pixel
    - Last 8 bytes: high bit of each pixel

    Returns 8x8 list of palette indices (0-3).
    """
    pixels = []
    for y in range(8):
        row = []
        low_byte = tile_data[y]
        high_byte = tile_data[y + 8]
        for x in range(7, -1, -1):  # MSB is leftmost pixel
            low_bit = (low_byte >> x) & 1
            high_bit = (high_byte >> x) & 1
            color_index = (high_bit << 1) | low_bit
            row.append(color_index)
        pixels.append(row)
    return pixels


def chr_to_image(chr_data: bytes, palette: list) -> Image.Image:
    """Convert CHR ROM data to a PIL Image."""
    num_tiles = len(chr_data) // TILE_SIZE
    if num_tiles == 0:
        raise ValueError("No tile data found")

    # Calculate image dimensions
    tiles_wide = TILES_PER_ROW
    tiles_high = (num_tiles + tiles_wide - 1) // tiles_wide

    img_width = tiles_wide * TILE_WIDTH
    img_height = tiles_high * TILE_HEIGHT

    img = Image.new('RGB', (img_width, img_height), palette[0])
    pixels = img.load()

    for tile_idx in range(num_tiles):
        tile_offset = tile_idx * TILE_SIZE
        tile_data = chr_data[tile_offset:tile_offset + TILE_SIZE]

        if len(tile_data) < TILE_SIZE:
            break

        tile_pixels = decode_tile(tile_data)

        # Calculate tile position in image
        tile_x = (tile_idx % tiles_wide) * TILE_WIDTH
        tile_y = (tile_idx // tiles_wide) * TILE_HEIGHT

        # Draw tile
        for y, row in enumerate(tile_pixels):
            for x, color_idx in enumerate(row):
                pixels[tile_x + x, tile_y + y] = palette[color_idx]

    return img


def parse_hex_color(hex_str: str) -> tuple:
    """Parse a hex color string (RRGGBB) to RGB tuple."""
    hex_str = hex_str.lstrip('#')
    if len(hex_str) != 6:
        raise ValueError(f"Invalid color format: {hex_str}")
    return (
        int(hex_str[0:2], 16),
        int(hex_str[2:4], 16),
        int(hex_str[4:6], 16),
    )


def main():
    parser = argparse.ArgumentParser(
        description='Dump CHR ROM data from NES ROMs to PNG images.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('rom', help='Input NES ROM file (.nes)')
    parser.add_argument('output', nargs='?', help='Output PNG file (default: <rom>_chr.png)')
    parser.add_argument('--palette', '-p', nargs=4, metavar='COLOR',
                        help='Four hex colors (RRGGBB) for the palette')
    parser.add_argument('--scale', '-s', type=int, default=1,
                        help='Scale factor for output image (default: 1)')
    parser.add_argument('--info', '-i', action='store_true',
                        help='Print ROM info and exit')

    args = parser.parse_args()

    rom_path = Path(args.rom)
    if not rom_path.exists():
        print(f"Error: File not found: {rom_path}")
        sys.exit(1)

    # Read ROM
    rom_data = rom_path.read_bytes()

    try:
        header = parse_ines_header(rom_data)
    except ValueError as e:
        print(f"Error: {e}")
        sys.exit(1)

    if args.info:
        print(f"ROM: {rom_path.name}")
        print(f"PRG ROM: {header['prg_rom_size'] // 1024} KB")
        print(f"CHR ROM: {header['chr_rom_size'] // 1024} KB")
        print(f"Trainer: {'Yes' if header['has_trainer'] else 'No'}")
        print(f"Tiles: {header['chr_rom_size'] // TILE_SIZE}")
        sys.exit(0)

    if header['chr_rom_size'] == 0:
        print("Error: ROM has no CHR ROM (uses CHR RAM)")
        print("This tool only works with ROMs that have CHR ROM data.")
        sys.exit(1)

    # Extract CHR data
    chr_start = header['chr_rom_offset']
    chr_end = chr_start + header['chr_rom_size']
    chr_data = rom_data[chr_start:chr_end]

    # Parse palette
    if args.palette:
        try:
            palette = [parse_hex_color(c) for c in args.palette]
        except ValueError as e:
            print(f"Error: {e}")
            sys.exit(1)
    else:
        palette = DEFAULT_PALETTE

    # Generate image
    img = chr_to_image(chr_data, palette)

    # Scale if requested
    if args.scale > 1:
        img = img.resize(
            (img.width * args.scale, img.height * args.scale),
            Image.NEAREST
        )

    # Determine output path (default to tools/dumps/)
    if args.output:
        output_path = Path(args.output)
    else:
        dumps_dir = Path(__file__).parent.parent / 'dumps'
        dumps_dir.mkdir(exist_ok=True)
        output_path = dumps_dir / f"{rom_path.stem}_chr.png"

    img.save(output_path)
    print(f"Saved: {output_path}")
    print(f"Tiles: {header['chr_rom_size'] // TILE_SIZE} ({header['chr_rom_size'] // 1024} KB)")
    print(f"Image: {img.width}x{img.height}")


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Build macOS plugin bitmaps from the source assets.

The Windows build uses the C# resource_builder which scales the PNGs and
rewrites Resource.rc/resource.h. On macOS the bitmaps are shipped next to the
plugin dylib as bmp<id>.png (id taken from the checked-in resource.h), which is
what the patched VSTGUI loader looks for.

This script intentionally depends only on the Python standard library.

Usage:
    python3 tools/build_resources.py -in assets -out dist/BENDY -scale 3
"""

import argparse
import os
import re
import struct
import sys
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def read_png(path):
    """Decode an 8-bit RGB/RGBA non-interlaced PNG into (width, height, rgba)."""
    with open(path, "rb") as f:
        data = f.read()

    if data[:8] != PNG_SIGNATURE:
        raise ValueError("not a PNG file: %s" % path)

    pos = 8
    width = height = bit_depth = color_type = interlace = None
    idat = b""
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length

        if ctype == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif ctype == b"IDAT":
            idat += chunk
        elif ctype == b"IEND":
            break

    if bit_depth != 8:
        raise ValueError("only 8-bit PNGs are supported (%s)" % path)
    if interlace != 0:
        raise ValueError("interlaced PNGs are not supported (%s)" % path)
    if color_type == 2:
        channels = 3
    elif color_type == 6:
        channels = 4
    else:
        raise ValueError("unsupported PNG color type %d (%s)" % (color_type, path))

    raw = zlib.decompress(idat)
    stride = width * channels
    out = bytearray(width * height * 4)
    prev = bytearray(stride)

    pos = 0
    for y in range(height):
        filter_type = raw[pos]
        pos += 1
        line = bytearray(raw[pos:pos + stride])
        pos += stride

        if filter_type == 1:  # Sub
            for x in range(channels, stride):
                line[x] = (line[x] + line[x - channels]) & 0xFF
        elif filter_type == 2:  # Up
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 0xFF
        elif filter_type == 3:  # Average
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                line[x] = (line[x] + ((left + prev[x]) >> 1)) & 0xFF
        elif filter_type == 4:  # Paeth
            for x in range(stride):
                a = line[x - channels] if x >= channels else 0
                b = prev[x]
                c = prev[x - channels] if x >= channels else 0
                p = a + b - c
                pa = abs(p - a)
                pb = abs(p - b)
                pc = abs(p - c)
                if pa <= pb and pa <= pc:
                    pr = a
                elif pb <= pc:
                    pr = b
                else:
                    pr = c
                line[x] = (line[x] + pr) & 0xFF
        elif filter_type != 0:
            raise ValueError("unknown PNG filter %d (%s)" % (filter_type, path))

        for x in range(width):
            si = x * channels
            di = (y * width + x) * 4
            out[di] = line[si]
            out[di + 1] = line[si + 1]
            out[di + 2] = line[si + 2]
            out[di + 3] = line[si + 3] if channels == 4 else 255

        prev = line

    return width, height, out


def write_png(path, width, height, rgba):
    """Write an 8-bit RGBA PNG."""
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)  # filter: None
        raw += rgba[y * stride:(y + 1) * stride]

    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload +
                struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as f:
        f.write(PNG_SIGNATURE)
        f.write(chunk(b"IHDR", ihdr))
        f.write(chunk(b"IDAT", zlib.compress(bytes(raw), 9)))
        f.write(chunk(b"IEND", b""))


def scale_nearest(width, height, rgba, scale):
    new_width = max(1, int(width * scale))
    new_height = max(1, int(height * scale))
    out = bytearray(new_width * new_height * 4)
    for y in range(new_height):
        sy = min(height - 1, int(y / scale))
        for x in range(new_width):
            sx = min(width - 1, int(x / scale))
            si = (sy * width + sx) * 4
            di = (y * new_width + x) * 4
            out[di:di + 4] = rgba[si:si + 4]
    return new_width, new_height, out


def replace_magenta(rgba):
    """Replace opaque magenta (255,0,255,255) with fully transparent pixels."""
    for i in range(0, len(rgba), 4):
        if rgba[i] == 255 and rgba[i + 1] == 0 and rgba[i + 2] == 255 and rgba[i + 3] == 255:
            rgba[i] = 0
            rgba[i + 1] = 0
            rgba[i + 2] = 0
            rgba[i + 3] = 0


def parse_resource_ids(resource_header):
    ids = {}
    with open(resource_header, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r"\s*#define\s+(PNG_\w+)\s+(\d+)", line)
            if m:
                ids[m.group(1)] = int(m.group(2))
    return ids


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-in", dest="input_dir", required=True)
    parser.add_argument("-out", dest="output_dir", required=True)
    parser.add_argument("-scale", dest="scale", type=float, default=3.0)
    parser.add_argument("-metadata", dest="metadata", default=None)
    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    resource_header = os.path.join(repo_root, "resource.h")
    ids = parse_resource_ids(resource_header)

    os.makedirs(args.output_dir, exist_ok=True)

    built = 0
    for name in sorted(os.listdir(args.input_dir)):
        if not name.lower().endswith(".png"):
            continue

        short_name = name[:-4]
        macro = "PNG_" + short_name
        if macro not in ids:
            print("warning: %s has no entry in resource.h, skipping" % name, file=sys.stderr)
            continue

        scale = args.scale
        if short_name.endswith("_halfres"):
            scale /= 2.0

        src = os.path.join(args.input_dir, name)
        width, height, rgba = read_png(src)
        width, height, rgba = scale_nearest(width, height, rgba, scale)
        replace_magenta(rgba)

        dst = os.path.join(args.output_dir, "bmp%05d.png" % ids[macro])
        write_png(dst, width, height, rgba)
        built += 1

    print("built %d bitmaps into %s" % (built, args.output_dir))


if __name__ == "__main__":
    main()

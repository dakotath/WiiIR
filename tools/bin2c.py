#!/usr/bin/env python3
import sys, os

def sanitize(name: str) -> str:
    out = []
    for c in name:
        if c.isalnum():
            out.append(c)
        else:
            out.append('_')
    return ''.join(out)

def main():
    if len(sys.argv) != 4:
        print("Usage: bin2c.py <input.bin> <output.c> <output.h>")
        return 1

    input_file = sys.argv[1]
    out_c = sys.argv[2]
    out_h = sys.argv[3]

    if not os.path.exists(input_file):
        print(f"Error: input file '{input_file}' not found")
        return 1

    # Create base name from file
    base = sanitize(os.path.basename(input_file))

    with open(input_file, "rb") as f:
        data = f.read()

    size = len(data)

    # Write header file
    with open(out_h, "w") as h:
        h.write(f"#pragma once\n\n")
        h.write(f"extern const unsigned char {base}[];\n")
        h.write(f"extern const unsigned int {base}_size;\n")

    # Write C file
    with open(out_c, "w") as c:
        c.write(f"#include \"{os.path.basename(out_h)}\"\n\n")
        c.write(f"const unsigned char {base}[] = {{\n")

        # Format data as hex
        for i, b in enumerate(data):
            if i % 16 == 0:
                c.write("    ")
            c.write(f"0x{b:02x}, ")
            if i % 16 == 15:
                c.write("\n")

        c.write("\n};\n")
        c.write(f"const unsigned int {base}_size = {size};\n")

    return 0

if __name__ == "__main__":
    sys.exit(main())

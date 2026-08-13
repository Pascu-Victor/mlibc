#!/usr/bin/env python3

import pathlib
import struct
import sys


def main() -> int:
    if len(sys.argv) != 3:
        raise SystemExit("usage: make-malformed-elf.py INPUT OUTPUT")

    data = bytearray(pathlib.Path(sys.argv[1]).read_bytes())
    if data[:4] != b"\x7fELF" or len(data) < 16:
        raise SystemExit("fixture input must be an ELF object")

    endian = {1: "<", 2: ">"}.get(data[5])
    layout = {
        1: (28, "I", 42, 44, 32, 24),
        2: (32, "Q", 54, 56, 56, 4),
    }.get(data[4])
    if endian is None or layout is None:
        raise SystemExit("fixture input has an unsupported ELF class or byte order")

    phoff_field, phoff_format, phentsize_field, phnum_field, expected_size, flags_field = layout
    program_header_offset = struct.unpack_from(endian + phoff_format, data, phoff_field)[0]
    program_header_size = struct.unpack_from(endian + "H", data, phentsize_field)[0]
    program_header_count = struct.unpack_from(endian + "H", data, phnum_field)[0]
    if program_header_size != expected_size:
        raise SystemExit("fixture input has an unexpected program-header size")

    for index in range(program_header_count):
        offset = program_header_offset + index * program_header_size
        if offset + program_header_size > len(data):
            raise SystemExit("fixture input has a truncated program-header table")
        if struct.unpack_from(endian + "I", data, offset)[0] != 1:  # PT_LOAD
            continue
        flags = struct.unpack_from(endian + "I", data, offset + flags_field)[0]
        struct.pack_into(endian + "I", data, offset + flags_field, flags | 0x3)  # PF_W | PF_X
        pathlib.Path(sys.argv[2]).write_bytes(data)
        return 0

    raise SystemExit("fixture input has no PT_LOAD program header")


if __name__ == "__main__":
    raise SystemExit(main())

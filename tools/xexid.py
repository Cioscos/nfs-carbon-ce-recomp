"""Print the identity of an Xbox 360 executable: title ID, version and the digest of the first
page descriptor, which starts the SHA-1 chain over all pages and so identifies the exact file.
The build uses it to tell the supported game editions apart (nfsc/cmake/editions.cmake).

Usage: python tools/xexid.py <default.xex>
Prints: title=454107EC version=0000000B digest=<40 hex digits>
"""
import struct
import sys

HEADER_LIMIT = 64 * 1024  # as in nfsc/src/game_path.cpp
EXECUTION_INFO = 0x00040006


def _be32(data, offset):
    """Big endian u32, or 0 past the end (like LoadBE32 in game_path.cpp)."""
    if offset + 4 > len(data):
        return 0
    return struct.unpack_from(">I", data, offset)[0]


def read_identity(data):
    """(title_id, version, digest) from the start of an XEX2 file, or None."""
    if len(data) < 0x18 or data[:4] != b"XEX2":
        return None
    security = _be32(data, 0x10)
    count = _be32(data, 0x14)
    title = version = 0
    for i in range(min(count, 64)):
        key = _be32(data, 0x18 + i * 8)
        if key == EXECUTION_INFO:
            info = _be32(data, 0x18 + i * 8 + 4)
            version = _be32(data, info + 4)
            title = _be32(data, info + 12)
    if not title:
        return None
    pages = _be32(data, security + 0x180)
    digest = data[security + 0x188:security + 0x188 + 20]
    if not pages or len(digest) != 20:
        return None
    return title, version, bytes(digest)


def main(argv):
    if len(argv) != 1:
        print(__doc__.strip(), file=sys.stderr)
        return 1
    try:
        with open(argv[0], "rb") as f:
            data = f.read(HEADER_LIMIT)
    except OSError as e:
        print(f"xexid: {e}", file=sys.stderr)
        return 1
    identity = read_identity(data)
    if identity is None:
        print(f"xexid: {argv[0]} is not an XEX2 file with execution and security info",
              file=sys.stderr)
        return 1
    title, version, digest = identity
    print(f"title={title:08X} version={version:08X} digest={digest.hex()}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

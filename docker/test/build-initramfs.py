#!/usr/bin/env python3
"""打包最小 initramfs（newc cpio + gzip），纯 Python 无需 WSL。
用法: python build-initramfs.py <hack.ko> [输出.cpio.gz] [init二进制]"""
import gzip
import io
import os
import sys

def cpio_entry(name, mode, data=b""):
    ino = abs(hash(name)) & 0xFFFFFFFF
    fields = [ino, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(name) + 1, 0]
    hdr = ("070701" + "".join(f"{f:08X}" for f in fields)).encode() + name.encode() + b"\0"
    hdr += b"\0" * ((4 - len(hdr) % 4) % 4)
    data += b"\0" * ((4 - len(data) % 4) % 4)
    return hdr + data

def main():
    ko = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else "rootfs.cpio.gz"
    init_path = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(__file__), "init")
    buf = io.BytesIO()
    for d in ("proc", "sys", "dev"):
        buf.write(cpio_entry(d, 0o040755))
    buf.write(cpio_entry("init", 0o100755, open(init_path, "rb").read()))
    buf.write(cpio_entry("hack.ko", 0o100644, open(ko, "rb").read()))
    buf.write(cpio_entry("TRAILER!!!", 0, b""))
    with gzip.open(out, "wb", compresslevel=1) as f:
        f.write(buf.getvalue())
    print(f"built: {out} ({os.path.getsize(out)} bytes)")

if __name__ == "__main__":
    main()

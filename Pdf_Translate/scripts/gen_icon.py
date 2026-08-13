# -*- coding: utf-8 -*-
"""生成应用图标 app.ico（32x32 + 48x48，PNG 内嵌，纯标准库）。

设计：蓝色圆角底 + 白色文档 + 右向箭头（寓意"PDF → 翻译"）
用法: python scripts/gen_icon.py [输出路径]
默认输出: src/resources/app.ico
"""
import struct
import sys
import zlib

SIZE_LIST = [16, 32, 48]


def make_png(size, pixels):
    """pixels: 每行 list[(r,g,b,a)]，自顶向下。"""
    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    raw = b"".join(
        b"\x00" + b"".join(struct.pack("4B", *px) for px in row)
        for row in pixels)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def draw(size):
    BG = (43, 108, 176, 255)        # 蓝色圆角底
    DOC = (255, 255, 255, 255)      # 白色文档
    DOC_FOLD = (205, 215, 230, 255) # 文档折角
    LINE = (150, 180, 215, 255)     # 文档上的文字线条
    ARROW = (255, 255, 255, 255)    # 右向箭头

    def in_rounded(x, y, x0, y0, x1, y1, r):
        if x < x0 or x > x1 or y < y0 or y > y1:
            return False
        cx = min(max(x, x0 + r), x1 - r)
        cy = min(max(y, y0 + r), y1 - r)
        return (x - cx) ** 2 + (y - cy) ** 2 <= r * r

    px = [[(0, 0, 0, 0)] * size for _ in range(size)]
    u = size / 48.0  # 以 48px 设计为基准缩放

    def S(v):
        return max(0, min(size - 1, int(v * u)))

    r = S(9)
    for y in range(size):
        for x in range(size):
            if in_rounded(x, y, S(1), S(1), S(47), S(47), r):
                px[y][x] = BG

    # 文档（左侧）
    dx0, dy0, dx1, dy1 = S(8), S(13), S(29), S(38)
    dr = S(2)
    for y in range(size):
        for x in range(size):
            if in_rounded(x, y, dx0, dy0, dx1, dy1, dr):
                px[y][x] = DOC
    # 文档折角（右上）
    fx, fy = dx1 - S(6), dy0
    for y in range(size):
        for x in range(size):
            if fx <= x <= dx1 and dy0 <= y <= fy + (x - fx):
                if in_rounded(x, y, dx0, dy0, dx1, dy1, dr):
                    px[y][x] = DOC_FOLD
    # 文档文字线条
    for (ly0, ly1) in ((S(18), S(20)), (S(23), S(25)), (S(28), S(30))):
        for y in range(ly0, ly1 + 1):
            for x in range(S(11), S(26)):
                px[y][x] = LINE

    # 右向箭头（右侧）
    ax = S(33)
    for y in range(S(19), S(30)):
        for x in range(ax, S(36)):
            px[y][x] = ARROW
    # 箭头三角
    for i in range(S(8)):
        for y in range(S(24) - i, S(25) + i + 1):
            x = S(36) + i
            px[y][x] = ARROW

    return px


def build_ico(pngs):
    header = struct.pack("<HHH", 0, 1, len(pngs))
    entries = b""
    blobs = b""
    offset = 6 + 16 * len(pngs)
    for (size, png) in pngs:
        entries += struct.pack("<BBBBHHII", size % 256, size % 256,
                               0, 0, 1, 32, len(png), offset)
        blobs += png
        offset += len(png)
    return header + entries + blobs


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "src/resources/app.ico"
    pngs = [(s, make_png(s, draw(s))) for s in SIZE_LIST]
    with open(out, "wb") as f:
        f.write(build_ico(pngs))
    print("已生成: %s (%d 字节)" % (out, len(build_ico(pngs))))

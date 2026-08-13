# -*- coding: utf-8 -*-
"""生成测试用多页文本 PDF。

用 Edge 无头模式把 HTML 打印成 PDF（浏览器生成的 PDF 字体信息完整，
渲染和文字提取均可用）。仅支持 Windows。

用法: python scripts/gen_test_pdf.py [输出路径]
默认输出: testdata/sample_en.pdf（3 页英文段落文本，含空行分段）
"""
import os
import subprocess
import sys
import tempfile

EDGE_PATHS = [
    r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    r"C:\Program Files\Microsoft\Edge\Application\msedge.exe",
]

PAGES = [
    [
        "Effects of Soil Moisture on Root Length Density in Winter Wheat",
        "",
        "Root length density (RLD) is a key parameter for describing root",
        "distribution in the soil profile. Understanding how soil moisture",
        "regulates root growth is essential for improving water management",
        "strategies in semiarid regions.",
        "",
        "In this study, we measured RLD at three growth stages under four",
        "irrigation treatments. Soil samples were collected at 10 cm depth",
        "intervals down to 100 cm using a soil auger.",
        "",
        "The results showed that RLD decreased with increasing soil depth.",
        "The highest RLD values were observed in the topsoil layer.",
    ],
    [
        "Materials and Methods",
        "",
        "The field experiment was conducted at the experimental farm of the",
        "university. The soil type was classified as a silt loam with a bulk",
        "density of 1.35 g cm-3. Four irrigation treatments were arranged in a",
        "randomized complete block design with three replications.",
        "",
        "Root samples were washed over a 0.5 mm sieve and scanned with a root",
        "scanner. Root length was calculated using image analysis software.",
        "",
        "Daily soil moisture was monitored using time domain reflectometry",
        "sensors installed at depths of 10, 30, 50, 70 and 90 cm.",
    ],
    [
        "Results and Discussion",
        "",
        "Root length density varied significantly among irrigation treatments.",
        "Deficit irrigation promoted deeper root growth, while full irrigation",
        "concentrated roots in the upper soil layers.",
        "",
        "A positive correlation was found between soil moisture and RLD in",
        "the 0-30 cm layer, but the correlation became negative below 60 cm.",
        "",
        "These findings suggest that moderate water stress may enhance root",
        "exploration of deep soil layers and improve drought resistance.",
    ],
]


def find_edge():
    for p in EDGE_PATHS:
        if os.path.isfile(p):
            return p
    raise FileNotFoundError("未找到 Edge 浏览器")


def build_html(pages):
    page_divs = []
    for lines in pages:
        body = "\n".join(
            "<p>%s</p>" % l if l else "<p><br/></p>" for l in lines)
        page_divs.append(
            '<div class="page">%s</div>' % body)
    css = (
        "@page { size: Letter; margin: 50px; }"
        ".page { page-break-after: always; }"
        ".page:last-child { page-break-after: auto; }"
        "body { font-family: Georgia, serif; font-size: 13px; "
        "line-height: 1.7; margin: 0; }"
        "h2 { font-size: 17px; }"
    )
    return ("<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<style>%s</style></head><body>%s</body></html>"
            % (css, "\n".join(page_divs)))


def make_pdf(pages, path):
    edge = find_edge()
    with tempfile.NamedTemporaryFile("w", suffix=".html",
                                     delete=False, encoding="utf-8") as f:
        f.write(build_html(pages))
        html_path = f.name
    try:
        subprocess.run(
            [edge, "--headless", "--disable-gpu", "--no-pdf-header-footer",
             "--print-to-pdf=" + os.path.abspath(path),
             "file:///" + html_path.replace("\\", "/")],
            check=True, capture_output=True, timeout=60)
    finally:
        os.unlink(html_path)

    print("已生成: %s (%d 页, %d 字节)"
          % (path, len(pages), os.path.getsize(path)))


if __name__ == "__main__":
    out_path = sys.argv[1] if len(sys.argv) > 1 else "testdata/sample_en.pdf"
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    make_pdf(PAGES, out_path)

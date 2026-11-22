from PIL import Image
import io

src = Image.open(r"D:\DESKTOP\GK4.0\_garrison\logo_coop.png").convert("RGBA")
alpha = src.split()[3]

def emit(name, w, h, subset=None):
    m = alpha.resize((w, h), Image.LANCZOS)
    px = list(m.getdata())
    lines = []
    lines.append("static const uint8_t %sMap[] = {" % name)
    for y in range(h):
        row = ", ".join("0x%02X" % px[y * w + x] for x in range(w))
        lines.append("    " + row + ("," if y + 1 < h else ""))
    lines.append("};")
    lines.append("static const lv_img_dsc_t %s = {" % name)
    lines.append("    {LV_IMG_CF_ALPHA_8BIT, 0, 0, %d, %d}," % (w, h))
    lines.append("    sizeof(%sMap)," % name)
    lines.append("    %sMap," % name)
    lines.append("};")
    return "\n".join(lines), w * h

badge, nb = emit("garrisonBadge", 21, 24)
mark, nm = emit("garrisonEmblem", 104, 116)

head = """// Arknights garrison emblem, decoded from the PRTS wiki asset
// (\\u536b\\u620d\\u534f\\u8bae_Logo_\\u534f\\u9632\\u5e72\\u5458.png) as ALPHA_8BIT masks so the
// page can tint them like the other glyphs in the project.
"""
out = head + "\n" + badge + "\n\n" + mark + "\n"
path = r"D:\DESKTOP\GK4.0\esp32_screen_module\main\apps\GarrisonEmblem.inc"
io.open(path, "w", encoding="ascii", newline="\n").write(out)
print("badge", nb, "mark", nm, "file", len(out))

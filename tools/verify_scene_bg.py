import re, io, os
from PIL import Image, ImageDraw

p = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                 "main", "scene_home_bg.c")
s = io.open(p, encoding="utf-8").read()

names = ["scene_clear_bg","scene_cloudy_bg","scene_rain_bg","scene_snow_bg","scene_dusk_bg","scene_night_bg"]
out = []
for n in names:
    m = re.search(re.escape(n) + r"_map\[\]\s*=\s*\{(.*?)\};", s, re.S)
    if not m:
        print("MISS", n); continue
    data = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", m.group(1)))
    if len(data) != 153600:
        print(n, "bad len", len(data)); continue
    im = Image.new("RGB", (320, 240))
    px = im.load()
    for y in range(240):
        for x in range(320):
            i = (y*320+x)*2
            v = data[i] | (data[i+1] << 8)
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            px[x, y] = (r, g, b)
    im.save(r"D:\DESKTOP\GK4.0\_scene\gen_%s.png" % n)
    out.append((n, im))
    print(n, "ok")

sheet = Image.new("RGB", (3*330, 2*265), (18,22,26))
dr = ImageDraw.Draw(sheet)
for i, (n, im) in enumerate(out):
    x = (i % 3) * 330 + 5
    y = (i // 3) * 265 + 20
    sheet.paste(im, (x, y))
    dr.text((x, y-14), n, fill=(180,255,180))
sheet.save(r"D:\DESKTOP\GK4.0\_scene\gen_sheet.png")
print("done")

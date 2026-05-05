#!/usr/bin/env python3
"""
Skaliert piep-design1.png auf Displaygrösse und speichert als logo.png
in data/ (LittleFS-Ordner).
Benötigt: pip install Pillow
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Pillow fehlt. Installieren mit:  pip install Pillow")
    sys.exit(1)

SRC  = Path(__file__).parent.parent / "datasheets" / "piep-design1.png"
DST  = Path(__file__).parent.parent / "data" / "logo.png"
SIZE = (260, 158)   # Zielgrösse: passt zentriert in 320×170

DST.parent.mkdir(exist_ok=True)

img = Image.open(SRC).convert("RGBA")

# Weisser Hintergrund (Display ist schwarz → Logo auf schwarz optimieren)
bg = Image.new("RGBA", img.size, (0, 0, 0, 255))
bg.paste(img, mask=img.split()[3])
img = bg.convert("RGB")

# Proportional skalieren, dann in Zielgrösse einpassen
img.thumbnail(SIZE, Image.LANCZOS)
canvas = Image.new("RGB", SIZE, (0, 0, 0))
x = (SIZE[0] - img.width)  // 2
y = (SIZE[1] - img.height) // 2
canvas.paste(img, (x, y))

canvas.save(DST, "PNG", optimize=True)
print(f"Gespeichert: {DST}  ({canvas.width}×{canvas.height} px)")

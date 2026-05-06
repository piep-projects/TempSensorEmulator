#!/usr/bin/env python3
"""
Konvertiert piep-design1.png → data/logo.png (weiß auf schwarz, 260x158 px).
Muss einmalig vor dem ersten 'pio run -t uploadfs' ausgeführt werden.

Aufruf: python3 tools/png_to_littlefs.py
Benötigt: pip install Pillow
"""

import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Pillow fehlt. Installieren mit:  pip install Pillow")
    sys.exit(1)

SRC  = Path(__file__).parent.parent / "datasheets" / "piep-logo-projects1.png"
DST  = Path(__file__).parent.parent / "data" / "logo.png"
SIZE = (260, 158)   # passt zentriert in 320×170 Display

DST.parent.mkdir(exist_ok=True)

# Logo-Form (Alpha-Kanal) als weiße Maske auf schwarzem Hintergrund
logo = Image.open(SRC).convert("RGBA")
white  = Image.new("RGB", logo.size, (255, 255, 255))
canvas = Image.new("RGB", logo.size, (0, 0, 0))
canvas.paste(white, mask=logo.split()[3])   # Alpha → weiße Pixel

# Proportional auf Zielgröße skalieren
canvas.thumbnail(SIZE, Image.LANCZOS)

# In exakten Rahmen einpassen (schwarz auffüllen)
result = Image.new("RGB", SIZE, (0, 0, 0))
x = (SIZE[0] - canvas.width)  // 2
y = (SIZE[1] - canvas.height) // 2
result.paste(canvas, (x, y))

result.save(DST, "PNG", optimize=True)
print(f"OK: {DST}  ({result.width}×{result.height} px, weiß auf schwarz)")
print("Jetzt flashen:  pio run -t uploadfs && pio run -t upload")

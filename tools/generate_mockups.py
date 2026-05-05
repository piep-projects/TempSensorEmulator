#!/usr/bin/env python3
"""
Erzeugt PNG-Mockups der drei Display-Screens (320x170, Skala 3x → 960x510).
Aufruf: python3 tools/generate_mockups.py
"""

from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

# ── Pfade ─────────────────────────────────────────────────────
ROOT     = Path(__file__).parent.parent
LOGO_SRC = ROOT / "datasheets" / "piep-design1.png"
OUT_DIR  = ROOT / "mockups"
OUT_DIR.mkdir(exist_ok=True)

# ── Display-Dimensionen ───────────────────────────────────────
DW, DH   = 320, 170   # physisches Display
SCALE    = 3
W, H     = DW * SCALE, DH * SCALE   # 960 × 510

# ── Farben ────────────────────────────────────────────────────
BLACK      = (0,   0,   0)
WHITE      = (255, 255, 255)
CYAN       = (0,   220, 220)
YELLOW     = (255, 220, 0)
GREEN      = (0,   200, 80)
RED        = (220, 50,  50)
DARKGREY   = (80,  80,  80)
MIDGREY    = (140, 140, 140)
ORANGE     = (255, 160, 0)
DARKBG     = (18,  18,  18)   # leicht aufgehellt damit Rahmen sichtbar

def s(v):
    """Skaliert einen Wert oder ein Tupel."""
    if isinstance(v, (int, float)):
        return int(v * SCALE)
    return tuple(int(x * SCALE) for x in v)

# ── Fonts laden (System-Fallback) ────────────────────────────
def load_font(size):
    for name in ["/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
                 "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                 "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"]:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()

def load_font_regular(size):
    for name in ["/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                 "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"]:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return ImageFont.load_default()

def load_font_mono(size):
    for name in ["/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
                 "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"]:
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            pass
    return load_font(size)

F_TINY   = load_font_regular(s(5.5))
F_SMALL  = load_font_regular(s(6.5))
F_MED    = load_font(s(8))
F_LARGE  = load_font_mono(s(30))
F_UNIT   = load_font(s(14))
F_TITLE  = load_font(s(7))

def centered_text(draw, y, text, font, color, width=W):
    bbox = draw.textbbox((0, 0), text, font=font)
    x = (width - (bbox[2] - bbox[0])) // 2
    draw.text((x, y), text, font=font, fill=color)
    return bbox[3] - bbox[1]

def new_screen():
    img = Image.new("RGB", (W, H), DARKBG)
    draw = ImageDraw.Draw(img)
    # Displayrahmen
    draw.rectangle([0, 0, W-1, H-1], outline=(50, 50, 50), width=2)
    return img, draw

def battery_icon(draw, x, y, pct, charging=False, color=WHITE):
    """Zeichnet ein Batterie-Icon an Position x,y."""
    bw, bh = s(16), s(8)
    nub = s(2)
    # Gehäuse
    draw.rectangle([x, y, x+bw, y+bh], outline=color, width=s(1))
    # Nub
    draw.rectangle([x+bw, y+s(2), x+bw+nub, y+bh-s(2)], fill=color)
    # Füllstand
    fill_w = int((bw - s(2)) * pct / 100)
    fill_color = RED if pct < 15 else (ORANGE if pct < 30 else GREEN)
    if fill_w > 0:
        draw.rectangle([x+s(1), y+s(1), x+s(1)+fill_w, y+bh-s(1)], fill=fill_color)
    # Blitz beim Laden
    if charging:
        lx, ly = x + bw//2, y - s(2)
        draw.text((lx - s(3), ly - s(7)), "⚡", font=F_TINY, fill=YELLOW)

def wifi_bars(draw, x, y, strength=3, color=WHITE):
    """Zeichnet 3-stufige WLAN-Balken."""
    heights = [s(4), s(7), s(10)]
    widths  = s(3)
    gap     = s(2)
    for i in range(3):
        bx = x + i * (widths + gap)
        bh = heights[i]
        by = y + s(10) - bh
        c = color if i < strength else DARKGREY
        draw.rectangle([bx, by, bx+widths, y+s(10)], fill=c)

# ══════════════════════════════════════════════════════════════
# SCREEN 1: SPLASH
# ══════════════════════════════════════════════════════════════
def make_splash():
    img, draw = new_screen()

    # Logo laden: Alpha-Kanal als Maske → weißes Logo auf Schwarz
    logo = Image.open(LOGO_SRC).convert("RGBA")
    white = Image.new("RGB", logo.size, (255, 255, 255))
    bg    = Image.new("RGB", logo.size, (0, 0, 0))
    bg.paste(white, mask=logo.split()[3])
    logo  = bg

    # Proportional auf max. 70% der Display-Fläche skalieren
    max_w, max_h = int(W * 0.70), int(H * 0.75)
    logo.thumbnail((max_w, max_h), Image.LANCZOS)

    lx = (W - logo.width) // 2
    ly = (H - logo.height) // 2 - s(6)
    img.paste(logo, (lx, ly))

    # Versionsnummer unten rechts
    ver = "v1.1.0"
    bbox = draw.textbbox((0, 0), ver, font=F_SMALL)
    tw = bbox[2] - bbox[0]
    draw.text((W - tw - s(6), H - s(12)), ver, font=F_SMALL, fill=DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 2: MAIN (aktiv)
# ══════════════════════════════════════════════════════════════
def make_main():
    img, draw = new_screen()

    # ── Trennlinien ───────────────────────────────────────────
    draw.line([(0, s(20)), (W, s(20))], fill=(40, 40, 40), width=1)
    draw.line([(0, s(148)), (W, s(148))], fill=(40, 40, 40), width=1)
    draw.line([(0, s(160)), (W, s(160))], fill=(30, 30, 30), width=1)

    # ── Statusleiste (y=4..18) ────────────────────────────────
    draw.text((s(4), s(4)), "Wolf CHA-07", font=F_TITLE, fill=MIDGREY)

    # I2C OK
    draw.text((s(115), s(4)), "I²C OK", font=F_TINY, fill=GREEN)

    # WiFi-Balken + IP
    wifi_bars(draw, s(175), s(5), strength=3, color=WHITE)
    draw.text((s(192), s(4)), "192.168.1.42", font=F_TINY, fill=WHITE)

    # Batterie 78%
    battery_icon(draw, s(280), s(6), pct=78, charging=False)
    draw.text((s(299), s(4)), "78%", font=F_TINY, fill=WHITE)

    # ── Temperatur (y=30..120) ────────────────────────────────
    temp_str = "-5.5"
    bbox = draw.textbbox((0, 0), temp_str, font=F_LARGE)
    tw = bbox[2] - bbox[0]
    tx = (W - tw) // 2 - s(18)
    ty = s(34)
    draw.text((tx, ty), temp_str, font=F_LARGE, fill=WHITE)

    # Einheit °C
    unit_x = tx + tw + s(4)
    draw.text((unit_x, ty + s(4)), "°C", font=F_UNIT, fill=CYAN)

    # ── Info-Zeile (y=134..148) ───────────────────────────────
    draw.text((s(4), s(135)), "R = 22 237 Ω", font=F_SMALL, fill=YELLOW)
    step_str = "Schritt  57 / 127"
    bbox = draw.textbbox((0, 0), step_str, font=F_SMALL)
    draw.text((W - bbox[2] - s(4), s(135)), step_str, font=F_SMALL, fill=YELLOW)

    # ── Tasten-Hints (y=162..170) ─────────────────────────────
    draw.text((s(4),  s(162)), "BOOT  ▼  (−)", font=F_TINY, fill=DARKGREY)
    hint2 = "(+)  ▲  KEY"
    bbox = draw.textbbox((0, 0), hint2, font=F_TINY)
    draw.text((W - bbox[2] - s(4), s(162)), hint2, font=F_TINY, fill=DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 3: SHUTDOWN
# ══════════════════════════════════════════════════════════════
def make_shutdown():
    img, draw = new_screen()

    # Roter Rahmen als Warnung
    draw.rectangle([s(4), s(4), W-s(4), H-s(4)], outline=(180, 30, 30), width=s(2))

    # Warnsymbol
    centered_text(draw, s(18), "⚠  Gerät wird ausgeschaltet  ⚠", F_MED, RED)

    # Trennlinie
    draw.line([(s(20), s(54)), (W-s(20), s(54))], fill=(60, 30, 30), width=1)

    # Info-Zeilen
    centered_text(draw, s(62),  "Letzte Temperatur:  −5.5 °C", F_SMALL, MIDGREY)
    centered_text(draw, s(80),  "Wert gespeichert — wird beim", F_SMALL, MIDGREY)
    centered_text(draw, s(94),  "nächsten Start wiederhergestellt.", F_SMALL, MIDGREY)

    # Trennlinie
    draw.line([(s(20), s(112)), (W-s(20), s(112))], fill=(60, 30, 30), width=1)

    # Wichtiger Hinweis
    centered_text(draw, s(120), "Bitte echten Fühler wieder", F_SMALL, ORANGE)
    centered_text(draw, s(134), "an Heizung anschliessen!", F_SMALL, ORANGE)

    # Countdown
    centered_text(draw, s(152), "Ausschalten in  2 s …", F_TINY, DARKGREY)

    return img

# ── Erzeugen & Speichern ──────────────────────────────────────
screens = {
    "screen_splash.png":   make_splash(),
    "screen_main.png":     make_main(),
    "screen_shutdown.png": make_shutdown(),
}

for fname, img in screens.items():
    out = OUT_DIR / fname
    img.save(out, "PNG")
    print(f"Gespeichert: {out}")

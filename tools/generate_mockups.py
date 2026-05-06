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
LOGO_SRC = ROOT / "datasheets" / "piep-logo-projects1.png"
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
F_LARGE  = load_font_mono(s(42))   # Temperatur
F_UNIT   = load_font(s(34))        # °C — proportional zu F_LARGE
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

    # Logo laden: weißes RGBA-Bild mit Original-Alpha → transparent auf Hintergrund
    logo_src = Image.open(LOGO_SRC).convert("RGBA")
    white_logo = Image.new("RGBA", logo_src.size, (0, 0, 0, 0))
    white_logo.paste(Image.new("RGBA", logo_src.size, (255, 255, 255, 255)),
                     mask=logo_src.split()[3])

    # Logo auf max. 65 % Breite / 55 % Höhe begrenzen (Platz für Text unten)
    max_w, max_h = int(W * 0.65), int(H * 0.55)
    white_logo.thumbnail((max_w, max_h), Image.LANCZOS)

    # Logo vertikal im oberen 60 % zentrieren
    lx = (W - white_logo.width) // 2
    ly = (int(H * 0.60) - white_logo.height) // 2
    img.paste(white_logo, (lx, ly), mask=white_logo.split()[3])

    # "TempSensorEmulator" in Gelb unterhalb des Logos
    title = "TempSensorEmulator"
    title_y = ly + white_logo.height + s(8)
    centered_text(draw, title_y, title, F_MED, YELLOW)

    # Versionsnummer in Dunkelgrau darunter
    ver = "v1.2.0"
    bbox = draw.textbbox((0, 0), ver, font=F_SMALL)
    ver_h = bbox[3] - bbox[1]
    centered_text(draw, title_y + s(14) + ver_h // 2, ver, F_SMALL, DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 2: MAIN (aktiv)
# ══════════════════════════════════════════════════════════════
def make_main():
    img, draw = new_screen()

    # ── Trennlinien ───────────────────────────────────────────
    draw.line([(0, s(26)), (W, s(26))], fill=(40, 40, 40), width=1)
    draw.line([(0, s(148)), (W, s(148))], fill=(40, 40, 40), width=1)
    draw.line([(0, s(158)), (W, s(158))], fill=(30, 30, 30), width=1)

    # ── Statusleiste: Logo + Titel links ─────────────────────
    logo_src = Image.open(LOGO_SRC).convert("RGBA")
    white_logo = Image.new("RGBA", logo_src.size, (0, 0, 0, 0))
    white_logo.paste(Image.new("RGBA", logo_src.size, (255, 255, 255, 255)),
                     mask=logo_src.split()[3])
    logo_h = s(20)
    logo_w = int(logo_src.width * logo_h / logo_src.height)
    logo_small = white_logo.resize((logo_w, logo_h), Image.LANCZOS)
    ly_logo = (s(26) - logo_h) // 2
    img.paste(logo_small, (s(4), ly_logo), mask=logo_small.split()[3])

    title_x = s(4) + logo_w + s(4)
    bbox_t  = draw.textbbox((0, 0), "TempSensorEmulator", font=F_TINY)
    ty_title = (s(26) - (bbox_t[3] - bbox_t[1])) // 2
    draw.text((title_x, ty_title), "TempSensorEmulator", font=F_TINY, fill=YELLOW)

    # WiFi-Balken + IP
    wifi_bars(draw, s(162), s(8), strength=3, color=WHITE)
    draw.text((s(178), s(7)), "192.168.1.42", font=F_TINY, fill=WHITE)

    # Batterie — weiter links, Platz für Knopf-Beschriftung rechts
    battery_icon(draw, s(248), s(9), pct=78, charging=False)
    draw.text((s(267), s(7)), "78%", font=F_TINY, fill=WHITE)

    # ── Knopf "Temp +" — oben rechts (physisch oben rechts am Gerät) ──
    for i, (txt, fnt, col) in enumerate([
        ("Temp +",    F_SMALL, YELLOW),
        ("3.5s: AUS", F_TINY,  WHITE),
    ]):
        bb = draw.textbbox((0, 0), txt, font=fnt)
        draw.text((W - (bb[2]-bb[0]) - s(3), s(30) + i * s(11)), txt, font=fnt, fill=col)

    # ── Temperatur: gleichmässige Grösse für alle Ziffern ────────
    temp_str = "-5.5"
    bb_temp = draw.textbbox((0, 0), temp_str, font=F_LARGE)
    tw, th  = bb_temp[2]-bb_temp[0], bb_temp[3]-bb_temp[1]
    ty = s(26) + (s(122) - th) // 2
    tx = (W - tw) // 2 - s(20)
    draw.text((tx, ty), temp_str, font=F_LARGE, fill=WHITE)

    # "°C" in Gelb, an Temperaturziffern angepasste Grösse
    bb_unit = draw.textbbox((0, 0), "°C", font=F_UNIT)
    unit_h  = bb_unit[3] - bb_unit[1]
    draw.text((tx + tw + s(3), ty + th - unit_h - s(2)), "°C", font=F_UNIT, fill=YELLOW)

    # ── Knopf "Temp −" — unten rechts (physisch unten rechts am Gerät) ──
    for i, (txt, fnt, col) in enumerate([
        ("Temp −",     F_SMALL, YELLOW),
        ("3.5s: WLAN", F_TINY,  WHITE),
    ]):
        bb = draw.textbbox((0, 0), txt, font=fnt)
        draw.text((W - (bb[2]-bb[0]) - s(3), s(127) + i * s(11)), txt, font=fnt, fill=col)

    # ── Info-Zeile: R-Wert links, I²C-Status direkt daneben ──────
    r_str = "R = 22 237 Ω"
    bb_r  = draw.textbbox((0, 0), r_str, font=F_SMALL)
    rw, rh = bb_r[2]-bb_r[0], bb_r[3]-bb_r[1]
    info_y = s(148) + (s(10) - rh) // 2
    draw.text((s(4), info_y), r_str, font=F_SMALL, fill=YELLOW)

    # I²C Statusblock direkt rechts neben R=Ω (grün = OK; auf Gerät rot)
    blk  = s(6)
    bx   = s(4) + rw + s(8)
    by   = s(148) + (s(10) - blk) // 2
    draw.rectangle([bx, by, bx + blk, by + blk], fill=GREEN)
    draw.text((bx + blk + s(2), info_y), "I²C", font=F_TINY, fill=YELLOW)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 3: SHUTDOWN
# ══════════════════════════════════════════════════════════════
def make_shutdown():
    img, draw = new_screen()

    # Gelber Rahmen
    draw.rectangle([s(4), s(4), W-s(4), H-s(4)], outline=YELLOW, width=s(2))

    # ── Logo + Titel (wie Main-Screen) ────────────────────────
    logo_src = Image.open(LOGO_SRC).convert("RGBA")
    white_logo = Image.new("RGBA", logo_src.size, (0, 0, 0, 0))
    white_logo.paste(Image.new("RGBA", logo_src.size, (255, 255, 255, 255)),
                     mask=logo_src.split()[3])
    logo_h = s(14)
    logo_w = int(logo_src.width * logo_h / logo_src.height)
    logo_small = white_logo.resize((logo_w, logo_h), Image.LANCZOS)
    img.paste(logo_small, (s(10), s(8)), mask=logo_small.split()[3])
    bbox_t = draw.textbbox((0, 0), "TempSensorEmulator", font=F_TINY)
    ty_t = s(8) + (logo_h - (bbox_t[3]-bbox_t[1])) // 2
    draw.text((s(10) + logo_w + s(4), ty_t), "TempSensorEmulator", font=F_TINY, fill=YELLOW)

    # Trennlinie
    draw.line([(s(10), s(30)), (W-s(10), s(30))], fill=(80, 70, 0), width=1)

    # "Gerät wird ausgeschaltet"
    centered_text(draw, s(36), "Gerät wird ausgeschaltet", F_MED, YELLOW)

    # Trennlinie
    draw.line([(s(20), s(60)), (W-s(20), s(60))], fill=(80, 70, 0), width=1)

    # Info-Zeilen
    centered_text(draw, s(68),  "Letzte Temperatur:  −5.5 °C", F_SMALL, MIDGREY)
    centered_text(draw, s(84),  "Wert gespeichert — wird beim", F_SMALL, MIDGREY)
    centered_text(draw, s(98),  "nächsten Start wiederhergestellt.", F_SMALL, MIDGREY)

    # Trennlinie
    draw.line([(s(20), s(114)), (W-s(20), s(114))], fill=(80, 70, 0), width=1)

    # Wichtiger Hinweis in Gelb
    centered_text(draw, s(120), "Bitte echten Fühler wieder", F_SMALL, YELLOW)
    centered_text(draw, s(134), "an Heizung anschliessen!", F_SMALL, YELLOW)

    # Countdown
    centered_text(draw, s(153), "Ausschalten in  10 s …", F_TINY, DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 4: WIFI SETUP
# ══════════════════════════════════════════════════════════════
def make_wifi_setup():
    img, draw = new_screen()

    # Blauer Rahmen für WiFi-Thema
    draw.rectangle([s(4), s(4), W-s(4), H-s(4)], outline=(30, 80, 180), width=s(2))

    # ── Logo + Titel (wie Main/Shutdown) ─────────────────────
    logo_src = Image.open(LOGO_SRC).convert("RGBA")
    white_logo = Image.new("RGBA", logo_src.size, (0, 0, 0, 0))
    white_logo.paste(Image.new("RGBA", logo_src.size, (255, 255, 255, 255)),
                     mask=logo_src.split()[3])
    logo_h = s(14)
    logo_w = int(logo_src.width * logo_h / logo_src.height)
    logo_small = white_logo.resize((logo_w, logo_h), Image.LANCZOS)
    img.paste(logo_small, (s(10), s(8)), mask=logo_small.split()[3])
    bbox_t = draw.textbbox((0, 0), "TempSensorEmulator", font=F_TINY)
    ty_t = s(8) + (logo_h - (bbox_t[3]-bbox_t[1])) // 2
    draw.text((s(10) + logo_w + s(4), ty_t), "TempSensorEmulator", font=F_TINY, fill=YELLOW)

    # Trennlinie
    draw.line([(s(10), s(28)), (W-s(10), s(28))], fill=(30, 60, 120), width=1)

    # AP-Info (Icon + SSID + Passwort)
    wifi_bars(draw, s(16), s(36), strength=3, color=(80, 160, 255))
    draw.text((s(34), s(34)), "AP:  CHA-Emulator", font=F_SMALL, fill=WHITE)
    draw.text((s(34), s(50)), "PW:  wolf1234",     font=F_SMALL, fill=MIDGREY)

    # Trennlinie
    draw.line([(s(16), s(66)), (W-s(16), s(66))], fill=(30, 60, 120), width=1)

    # Schritte
    steps = [
        "1.  Handy mit  \"CHA-Emulator\"  verbinden",
        "2.  Browser oeffnet sich automatisch",
        "3.  Heimnetz waehlen + Passwort eingeben",
    ]
    for i, step in enumerate(steps):
        draw.text((s(16), s(74) + i * s(22)), step, font=F_SMALL, fill=MIDGREY)

    # Warte-Animation
    centered_text(draw, s(152), "Warte auf Verbindung …", F_TINY, DARKGREY)

    return img

# ── Erzeugen & Speichern ──────────────────────────────────────
screens = {
    "screen_splash.png":     make_splash(),
    "screen_main.png":       make_main(),
    "screen_shutdown.png":   make_shutdown(),
    "screen_wifi_setup.png": make_wifi_setup(),
}

for fname, img in screens.items():
    out = OUT_DIR / fname
    img.save(out, "PNG")
    print(f"Gespeichert: {out}")

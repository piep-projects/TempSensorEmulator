#!/usr/bin/env python3
"""
Erzeugt PNG-Mockups der vier Display-Screens (320x170, Skala 3x → 960x510).
Font-Größen entsprechen exakt den LovyanGFX-Bitmap-Font-Höhen:
  Font2 chr_hgt=16  · Font4 chr_hgt=26  · Font7 chr_hgt=48
Alle Koordinaten entsprechen 1:1 dem display.cpp-Code.

Aufruf: python3 tools/generate_mockups.py
"""

from PIL import Image, ImageDraw, ImageFont
from pathlib import Path

ROOT     = Path(__file__).parent.parent
LOGO_SRC = ROOT / "datasheets" / "piep-logo-projects1.png"
OUT_DIR  = ROOT / "mockups"
OUT_DIR.mkdir(exist_ok=True)

# ── Display-Dimensionen ───────────────────────────────────────
DW, DH = 320, 170
SCALE  = 3
W, H   = DW * SCALE, DH * SCALE   # 960 × 510

# ── Farben (direkt aus config.h / display.cpp) ────────────────
BLACK    = (0,   0,   0)
WHITE    = (255, 255, 255)
YELLOW   = (255, 220, 0)
GREEN    = (0,   200, 80)
RED      = (220, 50,  50)
ORANGE   = (255, 150, 0)
DARKGREY = (80,  80,  80)
MIDGREY  = (140, 140, 140)
LTBLUE   = (80,  160, 255)
DARKBG   = (0,   0,   0)
SEP      = (40,  40,  40)
SEP_YEL  = (80,  64,  0)
SEP_BLU  = (30,  60,  120)

def s(v):
    """Skaliert einen Wert oder ein Tupel (real Display-Pixel → Canvas-Pixel)."""
    if isinstance(v, (int, float)):
        return int(v * SCALE)
    return tuple(int(x * SCALE) for x in v)

# ── Fonts — exakte LovyanGFX-Font-Höhen ──────────────────────
# Font2: chr_hgt=16  → alle kleinen Texte (Status, Hints, Info)
# Font4: chr_hgt=26  → Überschriften (WiFi-Konfiguration, Shutdown-Titel)
# Font4×size2: Großbuchstaben-Höhe ≈ baseline×2 = 38px → "oC"-Einheit
# Font7: chr_hgt=48  → Temperatur-Ziffer (7-Segment-Stil, Fallback: Mono)

def load_bold(size):
    for p in ["/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"]:
        try: return ImageFont.truetype(p, size)
        except OSError: pass
    return ImageFont.load_default()

def load_regular(size):
    for p in ["/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"]:
        try: return ImageFont.truetype(p, size)
        except OSError: pass
    return ImageFont.load_default()

def load_mono(size):
    for p in ["/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf",
              "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"]:
        try: return ImageFont.truetype(p, size)
        except OSError: pass
    return load_bold(size)

# Font sizes are pre-computed to simultaneously satisfy:
#   1. Pixel height proportional to the real LovyanGFX bitmap font height
#   2. All strings for a given font class fit within the 960 px canvas
#
# LovyanGFX bitmap heights:  Font2=16px  Font4=26px  Font7=48px
# TrueType cap-height / em ≈ 0.72 (bold), digit / em ≈ 0.70 (mono)
# TrueType chars are ~1.3× wider per pixel-height than the narrow bitmap fonts,
# so we reduce from the ideal s(N) until the longest string in that class fits.
#
# F_F2  = 41 px (ideal s(16)=48, widest body string limits to 41)
# F_F4  = 62 px (ideal s(26)=78, widest heading limits to 62)
# F_TEMP= s(69)=207 (digit height ≈ 48×3 canvas px — Font7 fills full chr_hgt)
# F_UNIT= s(53)=159 (cap-height ≈ 38×3 canvas px — Font4×2 cap = baseline×2)

F_F2   = load_regular(41)      # Font2: body text (16 px real bitmap)
F_F4   = load_bold(62)         # Font4 ×1: headings (26 px real bitmap)
F_TEMP = load_mono(s(69))      # Font7: temperature digits (48 px real)
F_UNIT = load_bold(s(53))      # Font4 ×2 cap-height: 38 px real

# ── Hilfsfunktionen ───────────────────────────────────────────

def new_screen():
    img  = Image.new("RGB", (W, H), DARKBG)
    draw = ImageDraw.Draw(img)
    return img, draw

def separator(draw, y, col=SEP):
    draw.line([(0, s(y)), (W, s(y))], fill=col, width=1)

def draw_text(draw, x, y, text, font, color):
    draw.text((s(x), s(y)), text, font=font, fill=color)

def draw_text_right(draw, right_x, y, text, font, color):
    bb = draw.textbbox((0, 0), text, font=font)
    tw = bb[2] - bb[0]
    draw.text((s(right_x) - tw, s(y)), text, font=font, fill=color)

def draw_centered(draw, y, text, font, color):
    bb = draw.textbbox((0, 0), text, font=font)
    tw = bb[2] - bb[0]
    draw.text(((W - tw) // 2, s(y)), text, font=font, fill=color)


def text_width(draw, text, font):
    bb = draw.textbbox((0, 0), text, font=font)
    return bb[2] - bb[0]

def logo_paste(img, x, y, max_w, max_h):
    """Klebt das Logo weiß-auf-schwarz an (x,y), begrenzt auf max_w×max_h real px."""
    src = Image.open(LOGO_SRC).convert("RGBA")
    canvas = Image.new("RGBA", src.size, (0, 0, 0, 0))
    canvas.paste(Image.new("RGBA", src.size, (255, 255, 255, 255)),
                 mask=src.split()[3])
    # Auf Zielgröße skalieren (größte Seite limitiert)
    mw, mh = s(max_w), s(max_h)
    ratio  = min(mw / canvas.width, mh / canvas.height)
    lw     = max(1, int(canvas.width  * ratio))
    lh     = max(1, int(canvas.height * ratio))
    scaled = canvas.resize((lw, lh), Image.LANCZOS)
    img.paste(scaled, (s(x), s(y)), mask=scaled.split()[3])
    return lw, lh

def bat_icon(draw, x, y, pct, charging=False):
    """Batterie-Icon exakt wie display.cpp: drawRect(x,y,20,10) + fillRect(x+20,y+3,2,4)."""
    col_fill = RED if pct < 15 else (ORANGE if pct < 30 else GREEN)
    # Gehäuse 20×10
    draw.rectangle([s(x), s(y), s(x)+s(20), s(y)+s(10)], outline=MIDGREY, width=s(1))
    # Nub 2×4 bei x+20, y+3
    draw.rectangle([s(x)+s(20), s(y)+s(3), s(x)+s(22), s(y)+s(7)], fill=MIDGREY)
    # Füllstand
    fw = max(0, min(18, (pct * 18) // 100))
    if fw:
        draw.rectangle([s(x)+s(1), s(y)+s(1),
                        s(x)+s(1)+int(fw*SCALE), s(y)+s(9)], fill=col_fill)
    if charging:
        draw.text((s(x)+s(23), s(y)+s(1)), "+", font=F_F2, fill=YELLOW)

def wifi_icon(draw, x, y, connected=True):
    """WiFi-Balken wie display.cpp: 3 Rechtecke."""
    c = WHITE if connected else DARKGREY
    draw.rectangle([s(x),    s(y)+s(6), s(x)+s(3),  s(y)+s(10)], fill=c)
    draw.rectangle([s(x)+s(4), s(y)+s(3), s(x)+s(7),  s(y)+s(10)], fill=c)
    draw.rectangle([s(x)+s(8), s(y),      s(x)+s(11), s(y)+s(10)], fill=c)

# ══════════════════════════════════════════════════════════════
# SCREEN 1: SPLASH
# ══════════════════════════════════════════════════════════════
def make_splash():
    img, draw = new_screen()

    # Logo: data/logo.png is 260×158 px; on screen drawn at scale 0.46
    # → rendered 120×73 px, centered at x=100, y=8.
    # The source here is the original 1838×888 PNG — resize directly to
    # the known rendered size to avoid applying 0.46 to the wrong base.
    src = Image.open(LOGO_SRC).convert("RGBA")
    wl  = Image.new("RGBA", src.size, (0, 0, 0, 0))
    wl.paste(Image.new("RGBA", src.size, (255, 255, 255, 255)), mask=src.split()[3])
    render_w, render_h = 120, 73          # real pixels as rendered on display
    scaled = wl.resize((render_w * SCALE, render_h * SCALE), Image.LANCZOS)
    img.paste(scaled, (s(100), s(8)), mask=scaled.split()[3])

    # "TempSensorEmulator" — Font4 (26 px), y=92
    draw_centered(draw, 92, "TempSensorEmulator", F_F4, YELLOW)

    # "v1.3.0" — Font2 (16 px), y=112
    draw_centered(draw, 112, "v1.3.0", F_F2, DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 2: MAIN
# ══════════════════════════════════════════════════════════════
def make_main():
    img, draw = new_screen()

    # ── Separatoren ──────────────────────────────────────────
    separator(draw, 26)
    separator(draw, 148)

    # ── Statusleiste (y 0–26) ─────────────────────────────────
    # Logo x=2, y=3, maxW=28, maxH=20
    logo_paste(img, 2, 3, 28, 20)

    # "TempSensorEmulator" gelb, Font2, x=34, y=9
    draw_text(draw, 34, 9, "TempSensorEmulator", F_F2, YELLOW)

    # WiFi-Icon x=162, y=8; IP x=176, y=9
    wifi_icon(draw, 162, 8, connected=True)
    draw_text(draw, 176, 9, "192.168.1.42", F_F2, WHITE)

    # Batterie x=248, y=8 (8 im Icon, 9 für Text)
    bat_icon(draw, 248, 8, pct=78, charging=False)
    draw_text(draw, 273, 9, "78%", F_F2, WHITE)

    # ── Tasten-Hint oben rechts (y 30 + 40) ──────────────────
    draw_text_right(draw, 316, 30, "Temp +",    F_F2, YELLOW)
    draw_text_right(draw, 316, 40, "3.5s: AUS", F_F2, WHITE)

    # ── Temperatur (Font7 ≈ 48 px), zentriert in 260px-Zone ──
    temp_str = "27.0"
    tw = text_width(draw, temp_str, F_TEMP)
    tx_real = (260 - tw // SCALE) // 2          # Mitte der linken 260-px-Zone
    ty_real = 63
    draw.text((s(tx_real), s(ty_real)), temp_str, font=F_TEMP, fill=WHITE)

    # "oC" gelb, Font4×2 (cap-h ≈38 px), rechts an Temp, +16 px tiefer
    draw.text((s(tx_real) + tw + s(4), s(ty_real) + s(16)),
              "oC", font=F_UNIT, fill=YELLOW)

    # ── Tasten-Hint unten rechts (y 120 + 130) ───────────────
    draw_text_right(draw, 316, 120, "Temp −",     F_F2, YELLOW)
    draw_text_right(draw, 316, 130, "3.5s: WLAN", F_F2, WHITE)

    # ── Info-Zeile (y 148–170) ────────────────────────────────
    r_str = "R = 22 237 Ohm"
    rw    = text_width(draw, r_str, F_F2)
    draw_text(draw, 4, 153, r_str, F_F2, YELLOW)

    # I²C-Block (7×7) + "I2C"
    blk_size = s(7)
    bx = s(4) + rw + s(6)
    by = s(152)
    draw.rectangle([bx, by, bx + blk_size, by + blk_size], fill=RED)
    draw.text((bx + blk_size + s(2), s(153)), "I2C", font=F_F2, fill=YELLOW)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 3: SHUTDOWN
# ══════════════════════════════════════════════════════════════
def make_shutdown():
    img, draw = new_screen()

    # Gelber Rahmen: drawRect(4,4,312,162)
    draw.rectangle([s(4), s(4), s(4)+s(312), s(4)+s(162)],
                   outline=YELLOW, width=s(1))

    # Logo x=8, y=8, maxW=24, maxH=16
    logo_paste(img, 8, 8, 24, 16)

    # "TempSensorEmulator" — Font2, x=36, y=10
    draw_text(draw, 36, 10, "TempSensorEmulator", F_F2, YELLOW)

    separator(draw, 28, SEP_YEL)

    # "Geraet wird ausgeschaltet" — Font4, zentriert, y=34
    draw_centered(draw, 34, "Geraet wird ausgeschaltet", F_F4, YELLOW)

    separator(draw, 56, SEP_YEL)

    # Letzte Temperatur — Font2, y=62
    draw_centered(draw, 62, "Letzte Temperatur:  27.0 oC", F_F2, MIDGREY)
    draw_centered(draw, 76, "Wert gespeichert.",            F_F2, MIDGREY)

    separator(draw, 92, SEP_YEL)

    # Hinweis — Font2, y=98 + y=112
    draw_centered(draw, 98,  "Bitte echten Fuehler wieder",  F_F2, YELLOW)
    draw_centered(draw, 112, "an Heizung anschliessen!",     F_F2, YELLOW)

    separator(draw, 128, SEP_YEL)

    # Countdown — Font2, y=136
    draw_centered(draw, 136, "Ausschalten in  10 s ...", F_F2, DARKGREY)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 4: WIFI SETUP
# ══════════════════════════════════════════════════════════════
def make_wifi_setup():
    img, draw = new_screen()

    # Blauer Rahmen: drawRect(4,4,312,162)
    draw.rectangle([s(4), s(4), s(4)+s(312), s(4)+s(162)],
                   outline=LTBLUE, width=s(1))

    # Logo x=8, y=8, maxW=24, maxH=16
    logo_paste(img, 8, 8, 24, 16)

    # "TempSensorEmulator" — Font2, x=36, y=10
    draw_text(draw, 36, 10, "TempSensorEmulator", F_F2, YELLOW)

    separator(draw, 28, SEP_BLU)

    # "WiFi-Konfiguration" — Font4, zentriert, y=34
    draw_centered(draw, 34, "WiFi-Konfiguration", F_F4, LTBLUE)

    separator(draw, 54, SEP_BLU)

    # WiFi-Icon + AP/PW — y=60 / y=74
    wifi_icon(draw, 14, 62, connected=True)
    draw_text(draw, 30, 60, 'AP:  CHA-Emulator', F_F2, WHITE)
    draw_text(draw, 30, 74, 'PW:  wolf1234',     F_F2, MIDGREY)

    separator(draw, 88, SEP_BLU)

    # Schritte — Font2, y=96 / y=110 / y=124
    draw_text(draw, 12, 96,  '1.  Handy mit  "CHA-Emulator"  verbinden', F_F2, MIDGREY)
    draw_text(draw, 12, 110, '2.  Browser oeffnet sich automatisch',      F_F2, MIDGREY)
    draw_text(draw, 12, 124, '3.  Heimnetz waehlen + Passwort eingeben',  F_F2, MIDGREY)

    separator(draw, 140, SEP_BLU)

    # "Warte auf Verbindung ..." — Font2, y=146
    draw_centered(draw, 146, "Warte auf Verbindung ...", F_F2, DARKGREY)

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
    print(f"Gespeichert: {out}  ({img.width}×{img.height})")

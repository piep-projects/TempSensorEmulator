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
F_TEMP = load_mono(s(50))      # Font7: temperature digits — etwas kleiner
F_UNIT = load_bold(s(50))      # gleiche em-Größe wie F_TEMP → gleiche Höhe

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
    """Batterie-Icon exakt wie display.cpp: drawRect(x,y,24,14) + fillRect(x+24,y+4,3,6)."""
    col_fill = RED if pct < 15 else (ORANGE if pct < 30 else GREEN)
    # Gehäuse 24×14
    draw.rectangle([s(x), s(y), s(x)+s(24), s(y)+s(14)], outline=MIDGREY, width=s(1))
    # Nub 3×6 bei x+24, y+4
    draw.rectangle([s(x)+s(24), s(y)+s(4), s(x)+s(27), s(y)+s(10)], fill=MIDGREY)
    # Füllstand
    fw = max(0, min(22, (pct * 22) // 100))
    if fw:
        draw.rectangle([s(x)+s(1), s(y)+s(1),
                        s(x)+s(1)+int(fw*SCALE), s(y)+s(13)], fill=col_fill)
    if charging:
        draw.text((s(x)+s(28), s(y)+s(3)), "+", font=F_F2, fill=YELLOW)

def wifi_icon(draw, x, y, connected=True):
    """WiFi-Balken wie display.cpp: 3 Rechtecke, Höhe 14px."""
    c = WHITE if connected else DARKGREY
    draw.rectangle([s(x),      s(y)+s(8), s(x)+s(3),  s(y)+s(14)], fill=c)
    draw.rectangle([s(x)+s(4), s(y)+s(4), s(x)+s(7),  s(y)+s(14)], fill=c)
    draw.rectangle([s(x)+s(8), s(y),      s(x)+s(11), s(y)+s(14)], fill=c)

# ══════════════════════════════════════════════════════════════
# SCREEN 1: SPLASH
# ══════════════════════════════════════════════════════════════
def make_splash():
    img, draw = new_screen()

    # Logo: originale Proportionen beibehalten (1838×888 = AR 2.07:1)
    # Breite 120 px real → Höhe = round(120 * 888/1838) = 58 px
    src = Image.open(LOGO_SRC).convert("RGBA")
    wl  = Image.new("RGBA", src.size, (0, 0, 0, 0))
    wl.paste(Image.new("RGBA", src.size, (255, 255, 255, 255)), mask=src.split()[3])
    render_w = 120
    render_h = round(render_w * src.height / src.width)   # 58 px
    scaled = wl.resize((render_w * SCALE, render_h * SCALE), Image.LANCZOS)
    # x=100 → (320-120)/2 = 100 zentriert; y=12 → etwas Abstand oben
    img.paste(scaled, (s(100), s(12)), mask=scaled.split()[3])

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
    # Kein Logo — "TempSensorEmulator" direkt links, x=4
    draw_text(draw, 4, 9, "TempSensorEmulator", F_F2, YELLOW)

    # WiFi + Batterie: y=6 → vertikal zentriert mit F_F2-Text bei y=9 (h=16, Icons h=14)
    wifi_icon(draw, 175, 6, connected=True)
    bat_icon(draw, 191, 6, pct=78, charging=False)
    draw_text(draw, 220, 9, "78%", F_F2, WHITE)

    # ── Tasten-Hint oben rechts (y 28 + 47, Abstand 19 px) ──
    draw_text_right(draw, 316, 28, "Temp +",    F_F2, YELLOW)
    draw_text_right(draw, 316, 47, "lang: AUS", F_F2, WHITE)

    # ── Temperatur (Font7 ≈ 48 px), zentriert in 260px-Zone ──
    temp_str = "27.0"
    tw = text_width(draw, temp_str, F_TEMP)
    tx_real = (260 - tw // SCALE) // 2          # Mitte der linken 260-px-Zone
    ty_real = 63
    draw.text((s(tx_real), s(ty_real)), temp_str, font=F_TEMP, fill=WHITE)

    # Grad-Symbol: nur kleines "o" (F_F4), höher als Ziffernoberkante
    draw.text((s(tx_real) + tw + s(6), s(ty_real - 10)), "o", font=F_F4, fill=YELLOW)

    # ── Tasten-Hint unten rechts (y 110 + 129, Abstand 19 px) ──
    draw_text_right(draw, 316, 110, "Temp −",     F_F2, YELLOW)
    draw_text_right(draw, 316, 129, "lang: WLAN", F_F2, WHITE)

    # ── Info-Zeile (y 148–170) ────────────────────────────────
    r_str = "R = 22 237 Ohm"
    rw    = text_width(draw, r_str, F_F2)
    draw_text(draw, 4, 153, r_str, F_F2, YELLOW)

    # I²C-Status: kein Block, Textfarbe grün=OK / rot=Fehler
    bx = s(4) + rw + s(6)
    draw.text((bx, s(153)), "I2C", font=F_F2, fill=GREEN)

    return img

# ══════════════════════════════════════════════════════════════
# SCREEN 3: SHUTDOWN
# ══════════════════════════════════════════════════════════════
def make_shutdown():
    img, draw = new_screen()

    # Gelber Rahmen: drawRect(4,4,312,162)
    draw.rectangle([s(4), s(4), s(4)+s(312), s(4)+s(162)],
                   outline=YELLOW, width=s(1))

    # Kein Logo — "TempSensorEmulator" direkt links (wie Main-Screen)
    draw_text(draw, 10, 10, "TempSensorEmulator", F_F2, YELLOW)

    separator(draw, 28, SEP_YEL)

    # "... wird ausgeschaltet" — Font4, zentriert, y=34
    draw_centered(draw, 34, "... wird ausgeschaltet", F_F4, YELLOW)

    separator(draw, 56, SEP_YEL)

    # Letzte Temperatur — Text + "o" separat für Superscript-Position
    temp_line = "Letzte Temperatur:  27.0"
    lw = text_width(draw, temp_line, F_F2)
    lx = (W - lw) // 2
    draw.text((lx, s(62)), temp_line, font=F_F2, fill=MIDGREY)
    draw.text((lx + lw + s(2), s(58)), "o", font=F_F2, fill=MIDGREY)
    draw_centered(draw, 76, "wurde gespeichert.",           F_F2, MIDGREY)

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

    # Kein Logo — "TempSensorEmulator" direkt links
    draw_text(draw, 10, 10, "TempSensorEmulator", F_F2, YELLOW)

    separator(draw, 28, SEP_BLU)

    # "WiFi-Konfiguration" — Font4, linksbündig, y=34
    draw_text(draw, 10, 34, "WiFi-Konfiguration", F_F4, LTBLUE)

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

    # IP-Adresse (nach Verbindung) — Font2, y=146, gelb, linksbündig
    draw_text(draw, 10, 146, "Verbunden  ·  192.168.1.42", F_F2, YELLOW)

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

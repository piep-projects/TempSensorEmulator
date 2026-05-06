#include "display.h"
#include "config.h"
#include <LovyanGFX.hpp>
#include <LittleFS.h>

// ── LovyanGFX: T-Display-S3, ST7789V, 8080-Parallelbus ────────
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789   _panel;
    lgfx::Bus_Parallel8  _bus;
    lgfx::Light_PWM      _light;
public:
    LGFX() {
        {
            auto cfg       = _bus.config();
            cfg.pin_wr     = PIN_LCD_WR;
            cfg.pin_rd     = PIN_LCD_RD;
            cfg.pin_rs     = PIN_LCD_DC;
            cfg.pin_d0     = PIN_LCD_D0;
            cfg.pin_d1     = PIN_LCD_D1;
            cfg.pin_d2     = PIN_LCD_D2;
            cfg.pin_d3     = PIN_LCD_D3;
            cfg.pin_d4     = PIN_LCD_D4;
            cfg.pin_d5     = PIN_LCD_D5;
            cfg.pin_d6     = PIN_LCD_D6;
            cfg.pin_d7     = PIN_LCD_D7;
            cfg.freq_write = 20000000;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg          = _panel.config();
            cfg.pin_cs        = PIN_LCD_CS;
            cfg.pin_rst       = PIN_LCD_RST;
            cfg.pin_busy      = -1;
            cfg.panel_width   = 170;
            cfg.panel_height  = 320;
            cfg.offset_x      = 35;
            cfg.offset_y      = 0;
            cfg.invert        = true;
            _panel.config(cfg);
        }
        {
            auto cfg        = _light.config();
            cfg.pin_bl      = PIN_LCD_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};

static LGFX tft;

// ── Farben ────────────────────────────────────────────────────
static constexpr uint32_t COL_BG       = 0x000000u;
static constexpr uint32_t COL_WHITE    = 0xFFFFFFu;
static constexpr uint32_t COL_YELLOW   = 0xFFDC00u;
static constexpr uint32_t COL_GREEN    = 0x00C850u;
static constexpr uint32_t COL_RED      = 0xDC3232u;
static constexpr uint32_t COL_ORANGE   = 0xFF9600u;
static constexpr uint32_t COL_DKGREY   = 0x505050u;
static constexpr uint32_t COL_MIDGREY  = 0x8C8C8Cu;
static constexpr uint32_t COL_LTBLUE   = 0x50A0FFu;
static constexpr uint32_t COL_SEP      = 0x282828u;
static constexpr uint32_t COL_SEP_YEL  = 0x504000u;
static constexpr uint32_t COL_SEP_BLU  = 0x1E3C78u;

// ── Hilfsfunktionen ──────────────────────────────────────────

static void drawSeparator(int y, uint32_t col = COL_SEP) {
    tft.drawFastHLine(0, y, 320, col);
}

// Batterie-Icon bei (x,y), Breite 22px, Höhe 10px
static void drawBatIcon(int x, int y, int pct, bool charging) {
    uint32_t fillColor = pct < BAT_WARN_PCT ? COL_RED :
                         pct < 30           ? COL_ORANGE : COL_GREEN;
    tft.drawRect(x, y, 20, 10, COL_MIDGREY);
    tft.fillRect(x + 20, y + 3, 2, 4, COL_MIDGREY);
    int fw = constrain((pct * 18) / 100, 0, 18);
    if (fw > 0) tft.fillRect(x + 1, y + 1, fw, 8, fillColor);
    if (charging) {
        tft.setTextColor(COL_YELLOW);
        tft.setFont(&fonts::Font0);
        tft.setCursor(x + 23, y + 1);
        tft.print("+");
    }
}

// WiFi-Balken (3 Stufen) bei (x,y), Breite ~11px, Höhe 10px
static void drawWifiIcon(int x, int y, bool connected) {
    uint32_t c = connected ? COL_WHITE : COL_DKGREY;
    tft.fillRect(x,      y + 6, 3, 4,  c);
    tft.fillRect(x + 4,  y + 3, 3, 7,  c);
    tft.fillRect(x + 8,  y,     3, 10, c);
}

// Logo aus LittleFS zeichnen (PNG, weiß auf schwarz)
static void drawLogo(int x, int y, int maxW, int maxH) {
    File f = LittleFS.open("/logo.png");
    if (!f) return;
    size_t sz = f.size();
    uint8_t* buf = (uint8_t*)malloc(sz);
    if (!buf) { f.close(); return; }
    f.read(buf, sz);
    f.close();
    tft.drawPng(buf, sz, x, y, maxW, maxH);
    free(buf);
}

// Zentrierten Text zeichnen; gibt die tatsächliche Textbreite zurück
static int drawCentered(const char* txt, int y,
                         const lgfx::IFont* font, uint32_t col) {
    tft.setFont(font);
    tft.setTextColor(col);
    int tw = tft.textWidth(txt);
    tft.drawString(txt, (320 - tw) / 2, y);
    return tw;
}

// ── Öffentliche API ───────────────────────────────────────────

void displayBegin() {
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    tft.init();
    tft.setRotation(1);   // Querformat 320×170
    tft.setBrightness(180);
    tft.fillScreen(COL_BG);

    LittleFS.begin(true);
}

void displayBrightness(uint8_t val) {
    tft.setBrightness(val);
}

// ── SCREEN 1: Splash ─────────────────────────────────────────
void drawSplash() {
    tft.fillScreen(COL_BG);

    // Logo in oberer Hälfte zentriert
    drawLogo(10, 5, 300, 80);

    // "TempSensorEmulator" in Gelb
    tft.setFont(&fonts::Font4);
    tft.setTextSize(1);
    tft.setTextColor(COL_YELLOW);
    String title = "TempSensorEmulator";
    int tw = tft.textWidth(title);
    tft.drawString(title, (320 - tw) / 2, 92);

    // Firmware-Version in Dunkelgrau
    tft.setFont(&fonts::Font2);
    tft.setTextSize(1);
    tft.setTextColor(COL_DKGREY);
    String ver = FW_VERSION;
    tw = tft.textWidth(ver);
    tft.drawString(ver, (320 - tw) / 2, 112);
}

// ── SCREEN 2: Hauptscreen ─────────────────────────────────────
void drawMain(float tempC, float ohm,
              int batPct, bool charging,
              const char* wifiIP, bool i2cOk) {
    tft.fillScreen(COL_BG);
    tft.setTextSize(1);

    // ── Statusleiste (y 0–25) ────────────────────────────────
    drawLogo(2, 3, 28, 20);

    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    tft.drawString("TempSensorEmulator", 34, 9);

    bool connected = (wifiIP[0] != '-' && wifiIP[0] != 'A');
    drawWifiIcon(162, 8, connected);
    tft.setTextColor(connected ? COL_WHITE : COL_DKGREY);
    tft.drawString(wifiIP, 176, 9);

    drawBatIcon(248, 8, batPct, charging);
    tft.setTextColor(batPct < BAT_WARN_PCT ? COL_RED : COL_WHITE);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", batPct);
    tft.drawString(buf, 273, 9);

    drawSeparator(26);

    // ── Tasten-Hint oben rechts: KEY = Temp + ────────────────
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    const char* hP1 = "Temp +";
    tft.drawString(hP1, 320 - tft.textWidth(hP1) - 4, 30);
    tft.setTextColor(COL_WHITE);
    const char* hP2 = "3.5s: AUS";
    tft.drawString(hP2, 320 - tft.textWidth(hP2) - 4, 40);

    // ── Temperatur zentriert (y 63–111) ──────────────────────
    char tmpBuf[10];
    snprintf(tmpBuf, sizeof(tmpBuf), "%.1f", tempC);

    tft.setFont(&fonts::Font7);
    int tw = tft.textWidth(tmpBuf);
    int tx = (260 - tw) / 2;   // in 260px-Zone links (rechts 60px für Hints)
    if (tx < 4) tx = 4;
    int ty = 63;

    tft.setTextColor(COL_WHITE);
    tft.drawString(tmpBuf, tx, ty);

    // "oC" in Gelb, Font6 (24px) — bottom-aligned mit Temperatur
    tft.setFont(&fonts::Font6);
    tft.setTextColor(COL_YELLOW);
    tft.drawString("oC", tx + tw + 4, ty + 22);

    // ── Tasten-Hint unten rechts: BOOT = Temp − ──────────────
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    const char* hM1 = "Temp -";
    tft.drawString(hM1, 320 - tft.textWidth(hM1) - 4, 120);
    tft.setTextColor(COL_WHITE);
    const char* hM2 = "3.5s: WLAN";
    tft.drawString(hM2, 320 - tft.textWidth(hM2) - 4, 130);

    drawSeparator(148);

    // ── Info-Zeile: R-Wert + I²C-Statusblock ─────────────────
    tft.setFont(&fonts::Font2);
    char info[24];
    snprintf(info, sizeof(info), "R = %.0f Ohm", ohm);
    tft.setTextColor(COL_YELLOW);
    tft.drawString(info, 4, 153);

    int rw = tft.textWidth(info);
    int bx = 4 + rw + 6;
    int by = 152;
    tft.fillRect(bx, by, 7, 7, i2cOk ? COL_GREEN : COL_RED);
    tft.setTextColor(COL_YELLOW);
    tft.drawString("I2C", bx + 9, 153);
}

// ── SCREEN 3: WiFi-Setup ─────────────────────────────────────
void drawWifiSetup() {
    tft.fillScreen(COL_BG);
    tft.setTextSize(1);

    // Blauer Rahmen
    tft.drawRect(4, 4, 312, 162, COL_LTBLUE);

    // ── Logo + Titel Header ───────────────────────────────────
    drawLogo(8, 8, 24, 16);

    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    tft.drawString("TempSensorEmulator", 36, 10);

    drawSeparator(28, COL_SEP_BLU);

    // ── Inhalt ───────────────────────────────────────────────
    tft.setFont(&fonts::Font4);
    tft.setTextColor(COL_LTBLUE);
    tft.setTextSize(1);
    drawCentered("WiFi-Konfiguration", 34, &fonts::Font4, COL_LTBLUE);

    drawSeparator(54, COL_SEP_BLU);

    // SSID und Passwort
    drawWifiIcon(14, 62, true);
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_WHITE);
    tft.drawString("AP:  " WIFI_AP_SSID, 30, 60);
    tft.setTextColor(COL_MIDGREY);
    tft.drawString("PW:  " WIFI_AP_PASS, 30, 74);

    drawSeparator(88, COL_SEP_BLU);

    // Schritt-für-Schritt
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_MIDGREY);
    tft.drawString("1.  Handy mit  \"" WIFI_AP_SSID "\"  verbinden", 12, 96);
    tft.drawString("2.  Browser oeffnet sich automatisch",          12, 110);
    tft.drawString("3.  Heimnetz waehlen + Passwort eingeben",      12, 124);

    drawSeparator(140, COL_SEP_BLU);
    tft.setTextColor(COL_DKGREY);
    drawCentered("Warte auf Verbindung ...", 146, &fonts::Font2, COL_DKGREY);
}

// ── SCREEN 4: Shutdown (mit Countdown) ───────────────────────
void drawShutdown(float tempC, int countdownSec) {
    tft.fillScreen(COL_BG);
    tft.setTextSize(1);

    // Gelber Rahmen
    tft.drawRect(4, 4, 312, 162, COL_YELLOW);

    // ── Logo + Titel Header ───────────────────────────────────
    drawLogo(8, 8, 24, 16);

    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    tft.drawString("TempSensorEmulator", 36, 10);

    drawSeparator(28, COL_SEP_YEL);

    // "Wird ausgeschaltet"
    drawCentered("Geraet wird ausgeschaltet", 34, &fonts::Font4, COL_YELLOW);

    drawSeparator(56, COL_SEP_YEL);

    // Letzte Temperatur
    tft.setFont(&fonts::Font2);
    char buf[40];
    snprintf(buf, sizeof(buf), "Letzte Temperatur:  %.1f oC", tempC);
    drawCentered(buf, 62, &fonts::Font2, COL_MIDGREY);
    drawCentered("Wert gespeichert.", 76, &fonts::Font2, COL_MIDGREY);

    drawSeparator(92, COL_SEP_YEL);

    // Wichtiger Hinweis
    drawCentered("Bitte echten Fuehler wieder",  98, &fonts::Font2, COL_YELLOW);
    drawCentered("an Heizung anschliessen!", 112, &fonts::Font2, COL_YELLOW);

    drawSeparator(128, COL_SEP_YEL);

    // Countdown
    char cdBuf[24];
    snprintf(cdBuf, sizeof(cdBuf), "Ausschalten in  %d s ...", countdownSec);
    drawCentered(cdBuf, 136, &fonts::Font2, COL_DKGREY);
}

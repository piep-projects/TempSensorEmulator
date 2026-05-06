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
static constexpr uint32_t COL_CYAN     = 0x00DCDCu;
static constexpr uint32_t COL_YELLOW   = 0xFFDC00u;
static constexpr uint32_t COL_GREEN    = 0x00C850u;
static constexpr uint32_t COL_RED      = 0xDC3232u;
static constexpr uint32_t COL_ORANGE   = 0xFF9600u;
static constexpr uint32_t COL_DKGREY   = 0x505050u;
static constexpr uint32_t COL_MIDGREY  = 0x8C8C8Cu;
static constexpr uint32_t COL_BLUE     = 0x2050C8u;
static constexpr uint32_t COL_LTBLUE   = 0x50A0FFu;
static constexpr uint32_t COL_SEP      = 0x282828u;

// ── Hilfsfunktionen ──────────────────────────────────────────

static void drawSeparator(int y) {
    tft.drawFastHLine(0, y, 320, COL_SEP);
}

// Batterie-Icon bei (x,y), Breite 22, Höhe 10
static void drawBatIcon(int x, int y, int pct, bool charging) {
    uint32_t fillColor = pct < BAT_WARN_PCT ? COL_RED :
                         pct < 30           ? COL_ORANGE : COL_GREEN;
    tft.drawRect(x, y, 20, 10, COL_MIDGREY);
    tft.fillRect(x + 20, y + 3, 2, 4, COL_MIDGREY);  // Pol
    int fw = constrain((pct * 18) / 100, 0, 18);
    if (fw > 0) tft.fillRect(x + 1, y + 1, fw, 8, fillColor);
    if (charging) {
        tft.setTextColor(COL_YELLOW);
        tft.setFont(&fonts::Font0);
        tft.setCursor(x + 23, y + 1);
        tft.print("+");
    }
}

// WiFi-Balken (3 Stufen) bei (x,y)
static void drawWifiIcon(int x, int y, bool connected) {
    uint32_t c = connected ? COL_WHITE : COL_DKGREY;
    // Drei Balken, steigend
    tft.fillRect(x,      y + 6, 3, 4,  c);
    tft.fillRect(x + 4,  y + 3, 3, 7,  c);
    tft.fillRect(x + 8,  y,     3, 10, c);
}

// Logo aus LittleFS zeichnen (weiß auf schwarz)
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
    drawLogo(30, 6, 260, 148);

    // Firmware-Version unten rechts
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_DKGREY);
    tft.setTextSize(1);
    String ver = FW_VERSION;
    int tw = tft.textWidth(ver);
    tft.drawString(ver, 320 - tw - 4, 156);
}

// ── SCREEN 2: Hauptscreen ─────────────────────────────────────
void drawMain(float tempC, float ohm, uint8_t step,
              int batPct, bool charging,
              const char* wifiIP, bool i2cOk) {
    tft.fillScreen(COL_BG);

    // ── Statusleiste (y 0–19) ────────────────────────────────
    // piep design Logo klein links
    drawLogo(2, 2, 28, 16);

    // I2C-Status
    tft.setFont(&fonts::Font2);
    tft.setTextSize(1);
    tft.setTextColor(i2cOk ? COL_GREEN : COL_RED);
    tft.drawString(i2cOk ? "I2C" : "I2C!", 36, 4);

    // WiFi-Icon + IP
    bool connected = (wifiIP[0] != '-' && wifiIP[0] != 'A');
    drawWifiIcon(100, 4, connected);
    tft.setTextColor(connected ? COL_WHITE : COL_DKGREY);
    tft.drawString(wifiIP, 114, 4);

    // Batterie-Icon + Prozent
    drawBatIcon(268, 5, batPct, charging);
    tft.setTextColor(batPct < BAT_WARN_PCT ? COL_RED : COL_WHITE);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", batPct);
    tft.drawString(buf, 294, 4);

    drawSeparator(20);

    // ── Temperatur zentriert (y 22–130) ──────────────────────
    char tmpBuf[10];
    snprintf(tmpBuf, sizeof(tmpBuf), "%.1f", tempC);

    tft.setFont(&fonts::Font7);
    tft.setTextSize(1);
    int tw = tft.textWidth(tmpBuf);
    int tx = (320 - tw) / 2 - 18;
    int ty = 28;
    tft.setTextColor(COL_WHITE);
    tft.drawString(tmpBuf, tx, ty);

    // °C rechts daneben
    tft.setFont(&fonts::Font4);
    tft.setTextColor(COL_CYAN);
    tft.drawString("oC", tx + tw + 4, ty + 20);  // 'o' als Grad-Ersatz

    drawSeparator(132);

    // ── Info-Zeile (y 134–148) ────────────────────────────────
    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_YELLOW);
    char info[32];
    snprintf(info, sizeof(info), "R = %.0f Ohm", ohm);
    tft.drawString(info, 4, 136);

    char step_str[20];
    snprintf(step_str, sizeof(step_str), "Schritt %d / 127", step);
    int stw = tft.textWidth(step_str);
    tft.drawString(step_str, 320 - stw - 4, 136);

    drawSeparator(150);

    // ── Tasten-Hints (y 153–168) ──────────────────────────────
    tft.setTextColor(COL_DKGREY);
    tft.drawString("BOOT -(", 4, 154);
    const char* hint2 = "+) KEY";
    int hw = tft.textWidth(hint2);
    tft.drawString(hint2, 320 - hw - 4, 154);
}

// ── SCREEN 3: WiFi-Setup ─────────────────────────────────────
void drawWifiSetup() {
    tft.fillScreen(COL_BG);
    tft.drawRect(4, 4, 312, 162, COL_BLUE);

    tft.setFont(&fonts::Font4);
    tft.setTextColor(COL_LTBLUE);
    int tw = tft.textWidth("WiFi-Konfiguration");
    tft.drawString("WiFi-Konfiguration", (320 - tw) / 2, 8);

    drawSeparator(36);

    tft.setFont(&fonts::Font2);
    // AP-Name und Passwort
    drawWifiIcon(14, 46, true);
    tft.setTextColor(COL_WHITE);
    tft.drawString("AP:  " WIFI_AP_SSID, 30, 44);
    tft.setTextColor(COL_MIDGREY);
    tft.drawString("PW:  " WIFI_AP_PASS, 30, 60);

    drawSeparator(76);

    // Schritt-für-Schritt-Anleitung
    tft.setTextColor(COL_MIDGREY);
    tft.drawString("1.  Handy mit  \"" WIFI_AP_SSID "\"  verbinden", 12, 84);
    tft.drawString("2.  Browser oeffnet sich automatisch", 12, 102);
    tft.drawString("3.  Heimnetz waehlen + Passwort eingeben", 12, 120);

    drawSeparator(140);
    tft.setTextColor(COL_DKGREY);
    int ww = tft.textWidth("Warte auf Verbindung ...");
    tft.drawString("Warte auf Verbindung ...", (320 - ww) / 2, 146);
}

// ── SCREEN 4: Shutdown ───────────────────────────────────────
void drawShutdown(float tempC) {
    tft.fillScreen(COL_BG);
    tft.drawRect(4, 4, 312, 162, COL_RED);

    tft.setFont(&fonts::Font4);
    tft.setTextColor(COL_RED);
    int tw = tft.textWidth("Wird ausgeschaltet");
    tft.drawString("Wird ausgeschaltet", (320 - tw) / 2, 10);

    drawSeparator(40);

    tft.setFont(&fonts::Font2);
    tft.setTextColor(COL_MIDGREY);
    char buf[40];
    snprintf(buf, sizeof(buf), "Letzte Temperatur:  %.1f oC", tempC);
    tw = tft.textWidth(buf);
    tft.drawString(buf, (320 - tw) / 2, 50);
    tw = tft.textWidth("Wert gespeichert.");
    tft.drawString("Wert gespeichert.", (320 - tw) / 2, 68);

    drawSeparator(88);

    tft.setTextColor(COL_ORANGE);
    tw = tft.textWidth("Bitte echten Fuhler wieder");
    tft.drawString("Bitte echten Fuhler wieder", (320 - tw) / 2, 96);
    tw = tft.textWidth("an Heizung anschliessen!");
    tft.drawString("an Heizung anschliessen!", (320 - tw) / 2, 114);

    drawSeparator(136);
    tft.setTextColor(COL_DKGREY);
    tw = tft.textWidth("Ausschalten in  2 s ...");
    tft.drawString("Ausschalten in  2 s ...", (320 - tw) / 2, 142);
}

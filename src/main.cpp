// Wolf CHA-07 Außenfühler-Emulator
//
// Hardware : LilyGo T-Display-S3
//            Soldered DIGIPOT 50 kΩ (MCP4018T-503, I²C 0x2F)
// Sensor   : NTC 5 kΩ @ 25 °C, B = 3977 K  (Wolf Art. 2748916)
//
// Verdrahtung MCP4018 → T-Display-S3:
//   VCC → 3,3 V      GND → GND
//   SDA → GPIO 18    SCL → GPIO 17   (QWIIC-Buchse)
//   PW  → Fühleranschluss 1 (Heizung)
//   PB  → Fühleranschluss 2 (Heizung)
//   PA  → offen lassen
//
// B-Wert verifizieren: Sensor bei bekannter Temperatur messen,
//   Seriellen Monitor öffnen, NTC_B anpassen bis R_soll passt.

#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <math.h>

// ── Pins T-Display-S3 ─────────────────────────────────────────
static constexpr int PIN_POWER_ON  = 15;   // Display-Versorgung
static constexpr int PIN_BTN_MINUS =  0;   // BOOT-Taste  = Temp ↓
static constexpr int PIN_BTN_PLUS  = 14;   // rechte Taste = Temp ↑
static constexpr int PIN_I2C_SDA   = 18;   // QWIIC SDA
static constexpr int PIN_I2C_SCL   = 17;   // QWIIC SCL

// ── MCP4018T-503 (50 kΩ, I²C, Adresse fest 0x2F) ─────────────
static constexpr uint8_t MCP4018_ADDR  = 0x2F;
static constexpr float   R_AB          = 50000.0f;  // Nennwiderstand Ende-Ende
static constexpr float   R_WIPER       = 75.0f;     // Wiper-Widerstand (typ.)
static constexpr int     N_STEPS       = 128;        // Schritte (0 … 127)

// ── NTC-Kennlinie Wolf CHA-07 (Art. 2748916) ──────────────────
// B-Wert vom Hersteller nicht veröffentlicht → mit Messung prüfen!
static constexpr float NTC_R25  = 5000.0f;   // 5 kΩ bei 25 °C
static constexpr float NTC_B    = 3977.0f;   // B₂₅/₈₅  (Schätzwert)
static constexpr float T_REF_K  = 298.15f;   // 25 °C in Kelvin

// ── Simulationsbereich ────────────────────────────────────────
static constexpr float T_MIN  = -15.0f;   // °C
static constexpr float T_MAX  =  30.0f;   // °C
static constexpr float T_STEP =   0.5f;   // °C pro Tastendruck

// ── Display: LilyGo T-Display-S3, ST7789, 170×320, 8080-Bus ──
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789   _panel;
    lgfx::Bus_Parallel8  _bus;
    lgfx::Light_PWM      _light;

public:
    LGFX() {
        {
            auto cfg      = _bus.config();
            cfg.pin_wr    = 8;
            cfg.pin_rd    = 9;
            cfg.pin_rs    = 7;    // DC
            cfg.pin_d0    = 39;
            cfg.pin_d1    = 40;
            cfg.pin_d2    = 41;
            cfg.pin_d3    = 42;
            cfg.pin_d4    = 45;
            cfg.pin_d5    = 46;
            cfg.pin_d6    = 47;
            cfg.pin_d7    = 48;
            cfg.freq_write = 20000000;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg         = _panel.config();
            cfg.pin_cs       = 6;
            cfg.pin_rst      = 5;
            cfg.pin_busy     = -1;
            cfg.panel_width  = 170;
            cfg.panel_height = 320;
            cfg.offset_x     = 35;
            cfg.offset_y     = 0;
            cfg.offset_rotation = 0;
            cfg.invert       = true;
            _panel.config(cfg);
        }
        {
            auto cfg        = _light.config();
            cfg.pin_bl      = 38;
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

// ── Zustand ───────────────────────────────────────────────────
static float   g_tempC        = 10.0f;
static float   g_lastDrawnTemp = -999.0f;
static bool    g_i2cOk        = false;

// ── NTC-Berechnung ────────────────────────────────────────────

static float ntcOhm(float tempC) {
    float T = tempC + 273.15f;
    return NTC_R25 * expf(NTC_B * (1.0f / T - 1.0f / T_REF_K));
}

// Ziel-Widerstand → MCP4018-Schritt (0…127)
// R_WB(step) = step * R_AB/N_STEPS + R_WIPER
static uint8_t ohmToStep(float R) {
    float s = (R - R_WIPER) * (float)N_STEPS / R_AB;
    return (uint8_t)constrain((int)roundf(s), 0, N_STEPS - 1);
}

// ── MCP4018 setzen ────────────────────────────────────────────

static bool mcp4018Set(uint8_t step) {
    Wire.beginTransmission(MCP4018_ADDR);
    Wire.write(step & 0x7F);
    return Wire.endTransmission() == 0;
}

static void applyTemperature(float tempC) {
    float   R    = ntcOhm(tempC);
    uint8_t step = ohmToStep(R);
    g_i2cOk = mcp4018Set(step);
    Serial.printf("T = %+.1f °C   R = %.0f Ω   Schritt = %d/127   I²C: %s\n",
                  tempC, R, step, g_i2cOk ? "OK" : "FEHLER");
}

// ── Display ───────────────────────────────────────────────────

static void drawScreen() {
    float   R    = ntcOhm(g_tempC);
    uint8_t step = ohmToStep(R);

    tft.fillScreen(TFT_BLACK);

    // Kopfzeile
    tft.setFont(&fonts::Font2);
    tft.setTextColor(0x4208u);   // dunkelgrau
    tft.drawString("Wolf CHA-07  Aussenfuhler-Emulator", 4, 4);

    // I²C-Status (rechts oben)
    tft.setTextColor(g_i2cOk ? TFT_GREEN : TFT_RED);
    tft.drawString(g_i2cOk ? "I2C OK" : "I2C!!", 262, 4);

    // Temperatur groß (Font7 = 7-Segment-Stil, 48 px hoch)
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f", g_tempC);

    tft.setFont(&fonts::Font7);
    tft.setTextColor(TFT_WHITE);
    int16_t tw = tft.textWidth(buf);
    int16_t tx = (320 - tw) / 2 - 16;   // etwas links, Platz für °C
    tft.drawString(buf, tx, 32);

    // °C rechts neben der Zahl
    tft.setFont(&fonts::Font4);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("oC", tx + tw + 4, 44);   // 'o' als Grad-Ersatz

    // Info-Zeile
    tft.setFont(&fonts::Font2);
    tft.setTextColor(TFT_YELLOW);
    char info[52];
    snprintf(info, sizeof(info), "R = %.0f Ohm     Schritt %d / 127", R, step);
    tft.drawString(info, 4, 134);

    // Tasten-Hinweise
    tft.setTextColor(0x4208u);
    tft.drawString("BOOT -", 4, 155);
    tft.drawString("+ KEY", 268, 155);

    g_lastDrawnTemp = g_tempC;
}

// ── Tasten mit Auto-Repeat ────────────────────────────────────

static constexpr uint32_t HOLD_DELAY_MS  = 600;
static constexpr uint32_t REPEAT_RATE_MS = 130;

struct Button {
    int      pin;
    uint32_t pressedAt;
    uint32_t lastRepeat;
    bool     edgeFired;
};

static Button btnMinus{ PIN_BTN_MINUS, 0, 0, false };
static Button btnPlus { PIN_BTN_PLUS,  0, 0, false };

// gibt +T_STEP, -T_STEP oder 0 zurück
static float pollButton(Button &b, float delta) {
    bool pressed = (digitalRead(b.pin) == LOW);
    uint32_t now = millis();

    if (!pressed) {
        b.pressedAt = 0;
        b.edgeFired = false;
        return 0.0f;
    }

    if (b.pressedAt == 0) {
        b.pressedAt = now;
        b.lastRepeat = now;
    }

    if (!b.edgeFired) {
        b.edgeFired = true;
        return delta;
    }

    if ((now - b.pressedAt > HOLD_DELAY_MS) &&
        (now - b.lastRepeat > REPEAT_RATE_MS)) {
        b.lastRepeat = now;
        return delta;
    }

    return 0.0f;
}

// ── Setup ─────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Wolf CHA-07 Außenfühler-Emulator ===");
    Serial.printf("NTC 5kΩ, B = %.0f K  |  MCP4018 50kΩ, 0x%02X\n",
                  NTC_B, MCP4018_ADDR);

    // Display-Versorgung einschalten
    pinMode(PIN_POWER_ON, OUTPUT);
    digitalWrite(PIN_POWER_ON, HIGH);

    pinMode(PIN_BTN_MINUS, INPUT_PULLUP);
    pinMode(PIN_BTN_PLUS,  INPUT_PULLUP);

    // I²C auf QWIIC-Pins, nicht Default (8/9 = LCD-Bus!)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(100000);

    // MCP4018 erreichbar?
    Wire.beginTransmission(MCP4018_ADDR);
    g_i2cOk = (Wire.endTransmission() == 0);
    if (!g_i2cOk) {
        Serial.printf("WARNUNG: MCP4018 nicht an 0x%02X – Verdrahtung prüfen!\n",
                      MCP4018_ADDR);
    }

    tft.init();
    tft.setRotation(1);   // Querformat: 320×170
    tft.setBrightness(180);

    applyTemperature(g_tempC);
    drawScreen();

    // Referenztabelle auf serieller Konsole ausgeben
    Serial.println("\nKennlinien-Tabelle (B-Wert prüfen!):");
    Serial.println("Temp(°C) | R(Ohm) | Schritt");
    for (int t = -15; t <= 30; t += 5) {
        float R = ntcOhm((float)t);
        Serial.printf("  %+3d°C  |  %5.0f  |  %3d\n", t, R, ohmToStep(R));
    }
}

// ── Loop ──────────────────────────────────────────────────────

void loop() {
    float delta = pollButton(btnMinus, -T_STEP)
                + pollButton(btnPlus,  +T_STEP);

    if (delta != 0.0f) {
        g_tempC = constrain(g_tempC + delta, T_MIN, T_MAX);
        applyTemperature(g_tempC);
    }

    if (g_tempC != g_lastDrawnTemp) {
        drawScreen();
    }

    delay(20);
}

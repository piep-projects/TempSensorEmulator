# Wolf CHA-07 Außenfühler-Emulator — Projektübersicht

**Hersteller:** piep design  
**Projekt:** TempSensorEmulator  
**Hardware:** LilyGo T-Display-S3 · Soldered DIGIPOT 50 kΩ · LiPo 700 mAh  
**Firmware:** Arduino / PlatformIO  
**Repository:** https://github.com/piep-projects/TempSensorEmulator

---

## Wozu dient dieses Gerät?

Der Außenfühler der Wolf CHA-07/10 Monoblock-Wärmepumpe ist ein passiver NTC-Widerstand (5 kΩ bei 25 °C). Der Regler liest dessen Widerstand und steuert darüber die Heizkurve.

Dieses Gerät ersetzt den echten Sensor durch ein per Hand einstellbares **digitales Potentiometer**. Damit lässt sich die Wärmepumpe im Labor oder bei der Inbetriebnahme mit beliebigen Außentemperaturen testen — unabhängig vom tatsächlichen Wetter.

**Typische Anwendungsfälle:**
- Heizkurve bei Extremtemperaturen prüfen (z. B. −10 °C im Sommer)
- Regler-Reaktion auf Temperaturänderungen beobachten
- Inbetriebnahme ohne Außenfühler-Anschluss
- Firmware-Entwicklung und Systemtests

---

## Hardware-Übersicht

| Komponente | Modell | Funktion |
|---|---|---|
| Mikrocontroller | LilyGo T-Display-S3 (ESP32-S3) | Steuerung, Display, WiFi |
| Digitalpotentiometer | Soldered DIGIPOT 50 kΩ (MCP4018T-503) | NTC-Widerstand simulieren |
| Akku | LiPo 3,7 V / 700 mAh | Mobiler Betrieb |
| Verbindung zum Regler | 2-Draht, an Fühleranschluss Wolf | Ersatz des echten Sensors |

### Pinbelegung (T-Display-S3 → MCP4018)

```
T-Display-S3          MCP4018 (DIGIPOT 50K)      Wolf CHA-07
─────────────         ─────────────────────      ─────────────
3,3 V      ────────── VCC
GND        ────────── GND
GPIO 18    ────────── SDA  (QWIIC)
GPIO 17    ────────── SCL  (QWIIC)
                       PW ─────────────────────── Fühler Klemme 1
                       PB ─────────────────────── Fühler Klemme 2
                       PA  offen
```

---

## Features auf einen Blick

| Feature | Details |
|---|---|
| Splash-Screen | piep design Logo + Firmware-Version beim Start |
| Temperaturanzeige | −15 °C bis +30 °C, Schritte 0,5 °C |
| Tastensteuerung | BOOT = −0,5 °C · KEY = +0,5 °C · Halten = schnell |
| Deep Sleep | Beide Tasten 2 s → Stromsparmodus; Taste → Aufwachen |
| Batterieanzeige | Ladestand in % + Icon; Blitz beim Laden |
| Warnung | < 15 % blinkend; < 5 % → automatisch Sleep |
| WiFi | Verbindet mit heimem WLAN; Fallback auf eigenen AP |
| Web-Interface | Temperatur vom Handy/PC aus einstellen |
| OTA-Update | Firmware drahtlos über Browser einspielen |
| NVS-Speicher | Temperatur + WLAN-Daten bleiben nach Neustart erhalten |

---

## Schnellstart

### 1. Hardware verbinden

Gemäß Verdrahtungstabelle oben. MCP4018 an QWIIC-Buchse des T-Display-S3.  
PW und PB an die Fühleranschlussklemmen der Wolf-Steuerung.

### 2. Firmware flashen

```bash
# Repository klonen
git clone https://github.com/piep-projects/TempSensorEmulator
cd TempSensorEmulator

# Logo für LittleFS vorbereiten (einmalig, benötigt Pillow)
pip install Pillow
python3 tools/png_to_littlefs.py

# LittleFS-Image hochladen
pio run -t uploadfs

# Firmware kompilieren und flashen
pio run -t upload

# Seriellen Monitor (optional, Diagnose)
pio device monitor
```

### 3. WLAN einrichten

Beim ersten Start (kein WLAN konfiguriert) öffnet das Gerät einen Access Point:

| Parameter | Wert |
|---|---|
| SSID | `CHA-Emulator` |
| Passwort | `wolf1234` |
| Konfigurations-URL | `http://192.168.4.1` |

WLAN-Daten über das Web-Interface eingeben → Gerät verbindet sich beim nächsten Start automatisch.

### 4. Temperatur einstellen

- **Am Gerät:** BOOT-Taste (−) oder KEY-Taste (+), halten für schnellen Lauf
- **Im Browser:** `http://<IP-Adresse>` → Temperatur per Webseite einstellen

---

## Projektstruktur

```
TempSensorEmulator/
├── src/
│   ├── main.cpp          # Haupt-Loop, Deep-Sleep
│   ├── display.h/.cpp    # Splash, Hauptscreen, Icons
│   ├── ntc.h             # NTC-Kennlinie, Formel
│   ├── mcp4018.h/.cpp    # I²C-Treiber MCP4018
│   ├── battery.h/.cpp    # ADC, Ladestand, Laden
│   ├── wifi_mgr.h/.cpp   # WiFi, AP-Fallback, Webserver, OTA
│   └── prefs.h/.cpp      # NVS-Persistenz
├── data/
│   └── logo.png          # piep design Logo (LittleFS)
├── datasheets/
│   ├── piep-design1.png
│   ├── DIGIPOT-10K-MCP4018.pdf
│   └── SOLDERED_MCP4018_DATASHEET.pdf
├── tools/
│   └── png_to_littlefs.py
├── platformio.ini
├── PROJECT_OVERVIEW.md   # dieses Dokument
├── FDS.md                # Functional Design Specification
└── FEATURES.md           # Feature-Liste (Planungsphase)
```

---

## Kompatibilität

| Wärmepumpe | Baujahr | Fühler-Art.-Nr. |
|---|---|---|
| Wolf CHA-07 | ab 10/2018 | 2748916 |
| Wolf CHA-10 | ab 10/2018 | 2748916 |
| Wolf CHA-16 | ab 05/2023 | 2748916 |
| Wolf CHA-20 | ab 05/2023 | 2748916 |

---

## Dokumente

| Dokument | Inhalt |
|---|---|
| [FDS.md](FDS.md) | Functional Design Specification — vollständige technische Spezifikation |
| [FEATURES.md](FEATURES.md) | Feature-Liste der Planungsphase |
| [datasheets/](datasheets/) | Datenblätter MCP4018 |

---

## Versionsverlauf

| Version | Datum | Änderungen |
|---|---|---|
| 1.0.0 | 2026-05-06 | Initiale Implementierung (Grundfunktion, kein WiFi) |
| 1.1.0 | — | Batterie, Deep Sleep, WiFi, OTA, Web-Interface |

---

*piep design · wolfgang@v-online.me*

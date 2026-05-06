# Wolf CHA-07 Außenfühler-Emulator — Projektübersicht

**Hersteller:** piep design  
**Projekt:** TempSensorEmulator  
**Hardware:** LilyGo T-Display-S3 · Soldered DIGIPOT 50 kΩ · LiPo 700 mAh  
**Firmware:** Arduino / PlatformIO  
**Repository:** https://github.com/piep-projects/TempSensorEmulator

---

## Wozu dient dieses Gerät?

Der Außenfühler der Wolf CHA-07/10 Monoblock-Wärmepumpe ist ein passiver NTC-Widerstand (5 kΩ bei 25 °C, Art.-Nr. 2748916). Der Regler liest dessen Widerstand und steuert darüber die Heizkurve.

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
                       PA  offen lassen
```

---

## Display-Screens

Das Gerät hat vier Display-Zustände (Mockups in `mockups/`):

| Screen | Datei | Beschreibung |
|---|---|---|
| **Splash** | `screen_splash.png` | piep design Logo weiß auf schwarz, Firmware-Version |
| **Main** | `screen_main.png` | Temperatur groß, Logo + Status oben, NTC-Info unten |
| **WiFi-Setup** | `screen_wifi_setup.png` | Captive-Portal-Anleitung (SSID, PW, 3 Schritte) |
| **Shutdown** | `screen_shutdown.png` | Roter Warnrahmen, Hinweis auf echten Fühler, Countdown |

---

## Features auf einen Blick

| Feature | Details |
|---|---|
| **Splash-Screen** | piep design Logo (weiß) + Firmware-Version beim Start |
| **Temperaturanzeige** | −15 °C bis +30 °C, große Anzeige, Schritte 0,5 °C |
| **Tastensteuerung** | Auslösung beim Loslassen · BOOT loslassen < 3,5 s = −0,5 °C · KEY loslassen < 3,5 s = +0,5 °C · kein Auto-Repeat |
| **Deep Sleep** | KEY ≥ 3,5 s halten + loslassen → Stromsparmodus; beliebige Taste → Aufwachen |
| **WiFi on Demand** | BOOT ≥ 3,5 s halten + loslassen → Captive Portal; beim Start nur stilles Reconnect |
| **Batterieanzeige** | Ladestand in % + Icon; Blitz beim Laden |
| **Warnung** | < 15 % rot; < 5 % → automatisch Sleep |
| **Web-Interface** | Temperatur vom Handy/PC einstellen, Status, WLAN-Config |
| **OTA-Update** | Firmware drahtlos über Browser (`/update`) einspielen |
| **NVS-Speicher** | Temperatur + WLAN-Daten bleiben nach Neustart erhalten |

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

### 3. WLAN einrichten — WiFiManager Captive Portal

Das WLAN-Portal wird **auf Knopfdruck** gestartet (nicht automatisch beim Boot):

| Parameter | Wert |
|---|---|
| Auslöser | BOOT-Taste **3,5 s** halten |
| SSID | `CHA-Emulator` |
| Passwort | `wolf1234` |

**Ablauf:**
1. BOOT-Taste 3,5 s halten → Display wechselt auf WiFi-Setup-Screen
2. Handy mit `CHA-Emulator` verbinden
3. Browser öffnet sich **automatisch** (Captive Portal, wie Hotel-WLAN)
4. Heimnetz aus der Liste wählen + Passwort eingeben → Gerät verbindet sich

Beim nächsten Start verbindet sich das Gerät **still** mit dem gespeicherten Netz (kein Blockieren, kein Portal).

### 4. Temperatur einstellen

- **Am Gerät:** BOOT-Taste (−) oder KEY-Taste (+), halten für schnellen Lauf
- **Im Browser:** `http://<IP-Adresse>` → Temperatur per Webseite einstellen

### 5. OTA-Firmware-Update

Neue Firmware einspielen unter `http://<IP-Adresse>/update`

---

## Projektstruktur

```
TempSensorEmulator/
├── src/
│   ├── main.cpp              # Haupt-Loop, Zustands-Automat, Deep-Sleep
│   ├── display.h/.cpp        # Alle vier Screens + Icons
│   ├── ntc.h                 # NTC-Kennlinie, Formel
│   ├── mcp4018.h/.cpp        # I²C-Treiber MCP4018
│   ├── battery.h/.cpp        # ADC, Ladestand, Lade-Erkennung
│   ├── wifi_mgr.h/.cpp       # WiFiManager, Webserver, OTA
│   └── prefs.h/.cpp          # NVS-Persistenz
├── data/
│   └── logo.png              # piep design Logo (LittleFS, Splash + Main)
├── mockups/
│   ├── screen_splash.png     # Splash-Screen Mockup
│   ├── screen_main.png       # Haupt-Screen Mockup
│   ├── screen_wifi_setup.png # WiFi-Setup-Screen Mockup
│   └── screen_shutdown.png   # Shutdown-Screen Mockup
├── datasheets/
│   ├── piep-design1.png      # Logo-Quelle (Original)
│   ├── DIGIPOT-10K-MCP4018.pdf
│   └── SOLDERED_MCP4018_DATASHEET.pdf
├── tools/
│   ├── png_to_littlefs.py    # Logo für LittleFS aufbereiten
│   └── generate_mockups.py   # Screen-Mockups erzeugen (benötigt Pillow)
├── platformio.ini
├── README.md                 # Kurzreferenz
├── PROJECT_OVERVIEW.md       # dieses Dokument
├── FDS.md                    # Functional Design Specification
└── FEATURES.md               # Feature-Liste (Planungsphase)
```

---

## Bibliotheken (PlatformIO)

| Bibliothek | PlatformIO-ID | Version | Zweck |
|---|---|---|---|
| LovyanGFX | `lovyan03/LovyanGFX` | ^1.1.16 | Display-Treiber (ST7789, 8080-Bus) |
| WiFiManager | `tzapu/WiFiManager` | ^2.0.17 | WLAN-Konfiguration, Captive Portal |
| ElegantOTA | `ayushsharma82/ElegantOTA` | ^3.1.0 | OTA-Firmware-Update über Browser |
| ArduinoJson | `bblanchon/ArduinoJson` | ^7.0.0 | JSON für `/status`-Endpunkt |

> **Hinweis:** PNG-Dateien aus LittleFS werden per `malloc`/`drawPng()` geladen (kompatibel mit LovyanGFX ≥ 1.2.x).  
> Das T-Display-S3 hat 16 MB Flash — `board_upload.flash_size = 16MB` muss in `platformio.ini` gesetzt sein, da PlatformIO standardmäßig 8 MB annimmt.

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
| [README.md](README.md) | Kurzreferenz: Verdrahtung, Flashen, Bedienung |
| [FDS.md](FDS.md) | Functional Design Specification — vollständige technische Spezifikation |
| [FEATURES.md](FEATURES.md) | Feature-Liste der Planungsphase |
| [mockups/](mockups/) | Display-Screen-Mockups (PNG, 960×510) |
| [datasheets/](datasheets/) | MCP4018-Datenblätter, Logo-Quelldatei |

---

## Versionsverlauf

| Version | Datum | Änderungen |
|---|---|---|
| 1.0.0 | 2026-05-06 | Initiale Implementierung — Grundfunktion (NTC-Emulation, Display, Tasten) |
| 1.1.0 | 2026-05-06 | Batterie, Deep Sleep, WiFiManager Captive Portal, Web-Interface, OTA |
| 1.2.0 | 2026-05-06 | WiFi optional (on-demand via BOOT 3,5 s); KEY 3,5 s = Sleep; LittleFS-Fix |
| 1.3.0 | 2026-05-06 | Neues Logo (piep-logo-projects1); überarbeitetes Display-Design (gelbe Farbgebung, Knopf-Beschriftung); Release-Trigger-Tasten (kein Auto-Repeat); Shutdown-Countdown 10 s |

---

*piep design · wolfgang@v-online.me*

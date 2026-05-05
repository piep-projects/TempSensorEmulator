# Wolf CHA-07 Außenfühler-Emulator

Simuliert den NTC-Außentemperaturfühler einer Wolf CHA-07/10/16/20 Monoblock-Wärmepumpe mittels digitalem Potentiometer. Ermöglicht Funktions- und Heizkurvtests unabhängig von der realen Außentemperatur.

→ Vollständige Dokumentation: [PROJECT_OVERVIEW.md](PROJECT_OVERVIEW.md) · [FDS.md](FDS.md)

---

## Hardware

| Komponente | Modell |
|---|---|
| Mikrocontroller | LilyGo T-Display-S3 (ESP32-S3, 1,9" Display) |
| Digitalpotentiometer | Soldered DIGIPOT **50 kΩ** (MCP4018T-503) |
| Akku | LiPo 3,7 V / 700 mAh |
| Verbindung | I²C über QWIIC-Buchse |

## Verdrahtung

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

## Sensor-Kennlinie

Wolf-Außenfühler Art.-Nr. 2748916 · NTC 5 kΩ bei 25 °C · B = 3977 K *(Schätzwert)*

| Temperatur | Widerstand | MCP4018-Schritt |
|---:|---:|---:|
| −15 °C | 39 499 Ω | 101 / 127 |
| −10 °C | 29 476 Ω | 75 / 127 |
| −5 °C | 22 237 Ω | 57 / 127 |
| 0 °C | 16 950 Ω | 43 / 127 |
| +5 °C | 13 047 Ω | 33 / 127 |
| +10 °C | 10 136 Ω | 26 / 127 |
| +20 °C | 6 277 Ω | 16 / 127 |
| +30 °C | 4 013 Ω | 10 / 127 |

## Bedienung

| Taste | Kurz | Halten |
|---|---|---|
| **BOOT** (GPIO 0) | −0,5 °C | schnell runter |
| **KEY** (GPIO 14) | +0,5 °C | schnell hoch |
| **Beide 2 s** | → Deep Sleep | — |

## Flashen

```bash
git clone https://github.com/piep-projects/TempSensorEmulator
cd TempSensorEmulator

# Logo für LittleFS aufbereiten (einmalig, benötigt Pillow)
pip install Pillow
python3 tools/png_to_littlefs.py

pio run -t uploadfs   # LittleFS-Image
pio run -t upload     # Firmware
pio device monitor    # Serieller Monitor (115200 Baud)
```

## WLAN einrichten (WiFiManager Captive Portal)

Beim ersten Start ohne gespeicherte WLAN-Daten:

1. Gerät öffnet Hotspot **`CHA-Emulator`** (Passwort: `wolf1234`)
2. Handy mit diesem Netz verbinden → Browser öffnet sich **automatisch**
3. Heimnetz aus der Liste wählen, Passwort eingeben, speichern
4. Gerät startet neu und verbindet sich automatisch

Danach: Temperatur per Browser unter `http://<IP>` einstellen.

## OTA-Update

Neue Firmware einspielen unter `http://<IP>/update`

## Kompatibilität

| Wärmepumpe | ab Baujahr | Fühler-Art.-Nr. |
|---|---|---|
| Wolf CHA-07 | 10/2018 | 2748916 |
| Wolf CHA-10 | 10/2018 | 2748916 |
| Wolf CHA-16 | 05/2023 | 2748916 |
| Wolf CHA-20 | 05/2023 | 2748916 |

## Projektstruktur

```
TempSensorEmulator/
├── src/
│   ├── main.cpp          # Haupt-Loop, Deep-Sleep, Zustands-Automat
│   ├── display.h/.cpp    # Alle vier Screens + Icons
│   ├── ntc.h             # NTC-Kennlinie, Formel
│   ├── mcp4018.h/.cpp    # I²C-Treiber MCP4018
│   ├── battery.h/.cpp    # ADC, Ladestand, Lade-Erkennung
│   ├── wifi_mgr.h/.cpp   # WiFiManager, Webserver, OTA
│   └── prefs.h/.cpp      # NVS-Persistenz
├── data/
│   └── logo.png          # piep design Logo (LittleFS, Splash-Screen)
├── mockups/              # Display-Mockups (PNG)
├── datasheets/           # MCP4018-Datenblätter, Logo-Quelle
├── tools/
│   ├── png_to_littlefs.py    # Logo-Konvertierung
│   └── generate_mockups.py   # Screen-Mockups erzeugen
└── platformio.ini
```

*piep design · wolfgang@v-online.me*

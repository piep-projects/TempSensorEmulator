# Wolf CHA-07 Außenfühler-Emulator

Simuliert den NTC-Außentemperaturfühler einer Wolf CHA-07/10 Monoblock-Wärmepumpe mittels digitalem Potentiometer. Ermöglicht das Testen der Heizkurve und des Reglerverhalten ohne reale Außentemperatur.

## Hardware

| Komponente | Modell |
|---|---|
| Mikrocontroller | LilyGo T-Display-S3 (ESP32-S3, 1,9" Display) |
| Digitalpotentiometer | Soldered DIGIPOT 50 kΩ (MCP4018T-503) |
| Anschluss | I²C über QWIIC-Buchse |

## Sensor-Kennlinie

Der Wolf-Außenfühler (Art.-Nr. 2748916) ist ein **NTC 5 kΩ** bei 25 °C.

| Temperatur | Widerstand | MCP4018-Schritt |
|---:|---:|---:|
| −15 °C | 39 499 Ω | 101 / 127 |
| −10 °C | 29 476 Ω | 75 / 127 |
| −5 °C | 22 237 Ω | 57 / 127 |
| 0 °C | 16 950 Ω | 43 / 127 |
| +5 °C | 13 047 Ω | 33 / 127 |
| +10 °C | 10 136 Ω | 26 / 127 |
| +15 °C | 7 943 Ω | 20 / 127 |
| +20 °C | 6 277 Ω | 16 / 127 |
| +25 °C | 5 000 Ω | 13 / 127 |
| +30 °C | 4 013 Ω | 10 / 127 |

> **Hinweis:** Der B-Wert (3977 K) ist ein Schätzwert — Wolf veröffentlicht keine Kennlinie.  
> Verifizierung: Sensor bei bekannter Temperatur messen, `NTC_B` in `src/main.cpp` ggf. anpassen.

## Verdrahtung

```
T-Display-S3          MCP4018 (DIGIPOT 50K)
─────────────         ─────────────────────
3,3 V      ───────── VCC
GND        ───────── GND
GPIO 18    ───────── SDA   (QWIIC)
GPIO 17    ───────── SCL   (QWIIC)

                      PW ──── Heizung Klemme 1 (Fühler +)
                      PB ──── Heizung Klemme 2 (Fühler −)
                      PA ──── offen
```

> Die Wolf CHA-07 verwendet eine 2-Draht-NTC-Messung (kein Bezugspotential).  
> Polarität der Klemmen spielt keine Rolle.

## Bedienung

| Taste | Kurz | Gedrückt halten |
|---|---|---|
| **BOOT** (links, GPIO 0) | −0,5 °C | schnell runter |
| **KEY** (rechts, GPIO 14) | +0,5 °C | schnell hoch |

Simulationsbereich: **−15 °C bis +30 °C**

Das Display zeigt:
- aktuelle Simulationstemperatur (groß)
- berechneter Widerstand in Ohm
- MCP4018-Schrittnummer
- I²C-Verbindungsstatus

## Software

**Framework:** Arduino (PlatformIO)  
**Display-Bibliothek:** [LovyanGFX](https://github.com/lovyan03/LovyanGFX)

### Bauen & Flashen

```bash
# Abhängigkeiten installieren und flashen
pio run -t upload

# Seriellen Monitor öffnen (115200 Baud)
pio device monitor
```

Der serielle Monitor gibt beim Start die vollständige Kennlinien-Tabelle aus — nützlich zum Abgleich mit einem Multimeter.

### Konfiguration (src/main.cpp)

```cpp
static constexpr float NTC_R25 = 5000.0f;   // Nennwiderstand bei 25 °C
static constexpr float NTC_B   = 3977.0f;   // B-Konstante — anpassen falls nötig
static constexpr float T_MIN   = -15.0f;    // untere Grenze
static constexpr float T_MAX   =  30.0f;    // obere Grenze
static constexpr float T_STEP  =   0.5f;    // Schrittweite pro Tastendruck
```

## Kompatibilität

Getestet mit / ausgelegt für:

- Wolf CHA-07 Monoblock-Wärmepumpe (ab Baujahr 10/2018)
- Wolf CHA-10 Monoblock-Wärmepumpe (ab Baujahr 10/2018)
- Wolf CHA-16/20 (ab Baujahr 05/2023, gleicher Fühler Art. 2748916)

## Dateien

```
TempSensorEmulator/
├── src/
│   └── main.cpp              # Firmware
├── datasheets/
│   ├── DIGIPOT-10K-MCP4018.pdf       # Modul-Datenblatt (10K-Variante)
│   └── SOLDERED_MCP4018_DATASHEET.pdf # MCP4018-IC Datenblatt
└── platformio.ini            # Build-Konfiguration
```

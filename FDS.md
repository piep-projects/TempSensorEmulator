# Functional Design Specification
## Wolf CHA-07 Außenfühler-Emulator

| Feld | Wert |
|---|---|
| Dokument | FDS-TempSensorEmulator |
| Version | 1.0 |
| Status | In Bearbeitung |
| Autor | piep design |
| Datum | 2026-05-06 |
| Hardware | LilyGo T-Display-S3 · MCP4018T-503 · LiPo 700 mAh |
| Firmware | v1.1.0 (geplant) |

---

## Inhaltsverzeichnis

1. [Zweck und Geltungsbereich](#1-zweck-und-geltungsbereich)
2. [Abkürzungen und Definitionen](#2-abkürzungen-und-definitionen)
3. [Systemübersicht](#3-systemübersicht)
4. [Hardware-Schnittstellen](#4-hardware-schnittstellen)
5. [Funktionale Anforderungen](#5-funktionale-anforderungen)
6. [Nicht-funktionale Anforderungen](#6-nicht-funktionale-anforderungen)
7. [Software-Architektur](#7-software-architektur)
8. [Zustands-Automat](#8-zustands-automat)
9. [Display-Spezifikation](#9-display-spezifikation)
10. [Web-Interface-Spezifikation](#10-web-interface-spezifikation)
11. [Kommunikations-Schnittstellen](#11-kommunikations-schnittstellen)
12. [Fehlverhalten und Ausnahmen](#12-fehlverhalten-und-ausnahmen)
13. [NTC-Kennlinien-Daten](#13-ntc-kennlinien-daten)
14. [Offene Punkte](#14-offene-punkte)

---

## 1. Zweck und Geltungsbereich

### 1.1 Zweck

Dieses Dokument beschreibt die vollständige funktionale Spezifikation des Wolf CHA-07 Außenfühler-Emulators. Es dient als verbindliche Grundlage für die Firmware-Implementierung.

### 1.2 Geltungsbereich

Das Dokument gilt für Firmware-Version 1.1.0 und die zugehörige Hardware-Revision 1.0.

### 1.3 Zweck des Geräts

Der Außentemperaturfühler der Wolf CHA-07/10/16/20 Monoblock-Wärmepumpe ist ein passiver NTC-Widerstand (5 kΩ bei 25 °C, Art.-Nr. 2748916). Das Gerät ersetzt diesen Sensor durch ein per Benutzer einstellbares digitales Potentiometer und ermöglicht damit Funktionstests des Wärmepumpen-Reglers bei beliebiger simulierter Außentemperatur.

---

## 2. Abkürzungen und Definitionen

| Begriff | Bedeutung |
|---|---|
| ADC | Analog-Digital-Wandler |
| AP | Access Point (WLAN-Hotspot) |
| FDS | Functional Design Specification |
| GPIO | General Purpose Input/Output |
| I²C | Inter-Integrated Circuit (serieller Bus) |
| LiPo | Lithium-Polymer-Akku |
| NTC | Negative Temperature Coefficient (Heißleiter) |
| NVS | Non-Volatile Storage (Flash-Speicher ESP32) |
| OTA | Over-The-Air (drahtloses Firmware-Update) |
| STA | Station Mode (WLAN-Client) |
| QWIIC | SparkFun I²C-Steckersystem (JST 1 mm, 4-polig) |

---

## 3. Systemübersicht

### 3.1 Blockdiagramm

```
┌────────────────────────────────────────────────────────┐
│                  LilyGo T-Display-S3                   │
│                                                        │
│  ┌──────────┐  I²C   ┌───────────────┐                │
│  │ ESP32-S3 │────────│  MCP4018T-503  │──PW──┐         │
│  │          │        │  Digipot 50kΩ  │      │ 2-Draht │
│  │          │        └───────────────┘──PB──┼────────── Wolf CHA-07
│  │          │                               │  Fühler  │
│  │          │  8080   ┌──────────────┐      │           │
│  │          │─────────│  ST7789 TFT  │      │           │
│  │          │         │  1,9"  320×170│      │           │
│  │          │         └──────────────┘      │           │
│  │          │  ADC    ┌──────────────┐      │           │
│  │          │─────────│  LiPo 700mAh │      │           │
│  │          │         │  + Lader     │      │           │
│  │          │         └──────────────┘      │           │
│  │          │  WiFi   ┌──────────────┐      │           │
│  │          │─────────│  802.11 b/g/n│      │           │
│  └──────────┘         └──────────────┘      │           │
│       │                                     │           │
│  GPIO 0 (BOOT)  GPIO 14 (KEY)              │           │
└────────────────────────────────────────────┼───────────┘
                                             │
                               Heizungsregler-Eingang
```

### 3.2 Betriebsmodi

| Modus | Beschreibung |
|---|---|
| Normal | Display an, Tasten aktiv, WiFi verbunden |
| AP-Setup | Kein WLAN konfiguriert; eigener Hotspot aktiv |
| Deep Sleep | Display aus, CPU im Tiefschlaf; Akku geschont |
| Laden | USB angeschlossen; Akku wird geladen; Betrieb normal |

---

## 4. Hardware-Schnittstellen

### 4.1 T-Display-S3 — GPIO-Belegung

| GPIO | Funktion | Richtung | Anmerkung |
|---:|---|---|---|
| 0 | Taste BOOT (−) | Eingang | Active LOW, interner Pull-up |
| 4 | Batterie-ADC | Eingang | Spannungsteiler ÷2 on-board |
| 5 | LCD Reset | Ausgang | Active LOW |
| 6 | LCD CS | Ausgang | |
| 7 | LCD DC | Ausgang | |
| 8 | LCD WR | Ausgang | |
| 9 | LCD RD | Ausgang | |
| 14 | Taste KEY (+) | Eingang | Active LOW, interner Pull-up |
| 15 | LCD Power Enable | Ausgang | Active HIGH |
| 17 | I²C SCL | Ausgang | QWIIC-Buchse |
| 18 | I²C SDA | Ein/Aus | QWIIC-Buchse |
| 38 | LCD Backlight | Ausgang | PWM, aktiv HIGH |
| 39–48 | LCD Datenbus D0–D7 | Ausgang | 8-Bit parallel 8080 |

### 4.2 MCP4018T-503 — Digitalpotentiometer

| Parameter | Wert |
|---|---|
| Typ | Single-End-Rheostat |
| Nennwiderstand R_AB | 50 000 Ω |
| Wiper-Widerstand R_W | 75 Ω (typ.) |
| Auflösung | 128 Schritte (7-Bit) |
| I²C-Adresse | 0x2F (fest) |
| Betriebsspannung | 1,8 – 5,5 V (3,3 V) |
| I²C-Takt | 100 kHz |

**Widerstandsformel:**

```
R_WB(step) = step × (R_AB / 128) + R_W
           = step × 390,625 Ω + 75 Ω
```

**I²C-Schreibzugriff:**

```
START · 0x2F·WRITE · [step & 0x7F] · STOP
```

### 4.3 Batterie

| Parameter | Wert |
|---|---|
| Zellchemie | LiPo 1S |
| Nennspannung | 3,7 V |
| Kapazität | 700 mAh |
| Ladeschluss | 4,20 V |
| Entladeschluss | 3,20 V |
| Messung | GPIO 4 · ADC1 · Teiler ÷2 |
| Lade-Erkennung | USB-Spannung > Batterie-Spannung (Hysterese 0,1 V) |

**Spannungs-zu-Ladezustand-Kennlinie:**

| Spannung | SoC |
|---:|---:|
| 4,20 V | 100 % |
| 4,00 V | 85 % |
| 3,80 V | 60 % |
| 3,60 V | 30 % |
| 3,40 V | 10 % |
| 3,20 V | 0 % |

Interpolation linear zwischen Stützpunkten. Messung alle 10 s, gleitender Mittelwert über 4 Werte.

---

## 5. Funktionale Anforderungen

### FR-01 — Startsequenz

| ID | Anforderung |
|---|---|
| FR-01.1 | Nach dem Einschalten oder Aufwachen aus Deep Sleep zeigt das Gerät den Splash-Screen. |
| FR-01.2 | Der Splash-Screen enthält das piep design Logo (aus LittleFS) zentriert auf schwarzem Hintergrund. |
| FR-01.3 | Die Firmware-Version (`vMAJOR.MINOR.PATCH`) wird unten rechts im Splash-Screen angezeigt. |
| FR-01.4 | Der Splash-Screen bleibt 2,0 Sekunden sichtbar. |
| FR-01.5 | Nach Ablauf der 2 Sekunden wechselt das Gerät selbstständig in den Hauptscreen. |
| FR-01.6 | Beim ersten Start ohne NVS-Daten wird als Starttemperatur +10,0 °C verwendet. |
| FR-01.7 | War ein Wert in NVS gespeichert, wird dieser unmittelbar nach dem Start an den MCP4018 übertragen — noch während des Splash-Screens. |

### FR-02 — Hauptscreen

| ID | Anforderung |
|---|---|
| FR-02.1 | Der Hauptscreen zeigt die aktuelle Simulationstemperatur in großer Schrift zentriert. |
| FR-02.2 | Unterhalb der Temperatur werden der berechnete NTC-Widerstand (Ω) und der MCP4018-Schritt (0–127) angezeigt. |
| FR-02.3 | In der oberen Statusleiste werden WiFi-Status und Batterie-Icon angezeigt. |
| FR-02.4 | In der unteren Leiste werden Hinweise auf die Tastenbelegung angezeigt. |
| FR-02.5 | Der Screen wird nach jeder Temperaturänderung vollständig neu gezeichnet. |
| FR-02.6 | Der I²C-Verbindungsstatus (OK / FEHLER) ist in der Statusleiste sichtbar. |

### FR-03 — Temperaturregelung über Tasten

| ID | Anforderung |
|---|---|
| FR-03.1 | Kurzer Druck (< 600 ms) auf BOOT reduziert die Temperatur um 0,5 °C. |
| FR-03.2 | Kurzer Druck auf KEY erhöht die Temperatur um 0,5 °C. |
| FR-03.3 | Halten der Taste > 600 ms aktiviert Auto-Repeat mit 130 ms Intervall. |
| FR-03.4 | Die Temperatur ist auf den Bereich −15,0 °C bis +30,0 °C begrenzt. |
| FR-03.5 | Jede Temperaturänderung wird sofort an den MCP4018 übertragen. |
| FR-03.6 | Jede Temperaturänderung wird in NVS gespeichert (max. alle 2 s, um Flash-Zyklen zu schonen). |

### FR-04 — NTC-Emulation (MCP4018)

| ID | Anforderung |
|---|---|
| FR-04.1 | Der Ziel-Widerstand wird nach der Steinhart-Hart-vereinfachten B-Formel berechnet: `R = R25 × exp(B × (1/T − 1/T25))` |
| FR-04.2 | Parameter: R25 = 5000 Ω, B = 3977 K, T25 = 298,15 K. |
| FR-04.3 | Der berechnete Widerstand wird in einen MCP4018-Schritt umgerechnet: `step = round((R − R_W) × 128 / R_AB)`, begrenzt auf 0–127. |
| FR-04.4 | Nach jedem Schreibzugriff wird der I²C-Rückgabewert geprüft; bei Fehler wird der I²C-Status im Display rot angezeigt. |
| FR-04.5 | Beim Start wird ein I²C-Scan durchgeführt; ist der MCP4018 nicht erreichbar, bleibt der Fehler dauerhaft angezeigt. |

### FR-05 — Persistenz (NVS)

| ID | Anforderung |
|---|---|
| FR-05.1 | Die zuletzt eingestellte Temperatur wird im NVS-Namespace `emulator` unter dem Schlüssel `temp` gespeichert. |
| FR-05.2 | WLAN-SSID und Passwort werden unter `wifi_ssid` und `wifi_pass` gespeichert. |
| FR-05.3 | Fehlt ein NVS-Schlüssel, wird der definierte Standardwert verwendet (Temp: +10,0 °C; WiFi: leer). |

### FR-06 — Deep Sleep / Energiesparmodus

| ID | Anforderung |
|---|---|
| FR-06.1 | Werden beide Tasten (BOOT + KEY) gleichzeitig länger als 2,0 Sekunden gehalten, wechselt das Gerät in den Deep Sleep. |
| FR-06.2 | Vor dem Einschlafen wird die Temperatur in NVS gespeichert. |
| FR-06.3 | Das Display wird vor dem Einschlafen ausgeschaltet (Backlight aus, Power-Enable LOW). |
| FR-06.4 | Das Gerät wacht durch Druck auf eine beliebige Taste auf (GPIO 0 oder GPIO 14 als Wakeup-Quelle). |
| FR-06.5 | Nach dem Aufwachen durchläuft das Gerät die vollständige Startsequenz (FR-01). |
| FR-06.6 | Der MCP4018 verliert im Deep Sleep seinen Zustand nicht (I²C-Pull-ups halten den Bus; Widerstand bleibt). |
| FR-06.7 | Fällt der Akku unter 5 % SoC, wechselt das Gerät automatisch in den Deep Sleep (mit Hinweis auf dem Display). |

### FR-07 — Batterieanzeige

| ID | Anforderung |
|---|---|
| FR-07.1 | Der Ladezustand wird als Prozentangabe und 5-stufiges Icon in der Statusleiste angezeigt. |
| FR-07.2 | Icon-Stufen: 0–20 % (leer), 21–40 %, 41–60 %, 61–80 %, 81–100 % (voll). |
| FR-07.3 | Wird das Gerät über USB geladen, erscheint ein Blitz-Symbol neben dem Batterie-Icon. |
| FR-07.4 | Bei < 15 % SoC blinkt das Icon im Sekundentakt (rot). |
| FR-07.5 | Bei < 5 % SoC erscheint eine Warnmeldung und das Gerät geht nach 5 Sekunden in Deep Sleep. |

### FR-08 — WiFi-Verbindung

| ID | Anforderung |
|---|---|
| FR-08.1 | Beim Start prüft das Gerät, ob WLAN-Daten in NVS vorhanden sind. |
| FR-08.2 | Sind Daten vorhanden, wird der Verbindungsaufbau im STA-Modus versucht (Timeout: 10 s). |
| FR-08.3 | Schlägt die Verbindung fehl oder fehlen die Daten, wechselt das Gerät in den AP-Modus (SSID: `CHA-Emulator`, Passwort: `wolf1234`, IP: `192.168.4.1`). |
| FR-08.4 | Der WiFi-Status (IP-Adresse oder `AP`) ist in der Statusleiste des Hauptscreens sichtbar. |
| FR-08.5 | WLAN-Daten können über das Web-Interface (FR-09) neu konfiguriert werden. |
| FR-08.6 | Im Deep Sleep wird das WiFi-Modul abgeschaltet. |

### FR-09 — Web-Interface

| ID | Anforderung |
|---|---|
| FR-09.1 | Der interne HTTP-Server läuft auf Port 80. |
| FR-09.2 | `GET /` liefert eine HTML-Seite mit aktueller Temperatur, Widerstand, Batterie-SoC, Firmware-Version und IP. |
| FR-09.3 | Die HTML-Seite enthält Schaltflächen `[−]` und `[+]` zum Ändern der Temperatur in 0,5-°C-Schritten. |
| FR-09.4 | Die Seite enthält ein Formular zur Eingabe von WLAN-SSID und Passwort. |
| FR-09.5 | `GET /set?t=<wert>` setzt die Temperatur (Bereich −15 bis +30, Auflösung 0,5 °C); antwortet mit Redirect auf `/`. |
| FR-09.6 | `GET /status` antwortet mit JSON (siehe §10.2). |
| FR-09.7 | `POST /wifi` nimmt SSID + Passwort entgegen, speichert in NVS und startet neu. |
| FR-09.8 | `/update` stellt die ElegantOTA-Seite bereit. |
| FR-09.9 | Die HTML-Seite ist mobilgerecht (Schriftgröße, Abstände für Touchbedienung). |

### FR-10 — OTA-Firmware-Update

| ID | Anforderung |
|---|---|
| FR-10.1 | OTA-Updates werden über ElegantOTA unter `http://<IP>/update` bereitgestellt. |
| FR-10.2 | Während des Updates zeigt das Display einen Ladebalken. |
| FR-10.3 | Nach erfolgreichem Update startet das Gerät automatisch neu. |
| FR-10.4 | Bei fehlerhaftem Update bleibt die alte Firmware aktiv (ESP32-Rollback-Mechanismus). |

---

## 6. Nicht-funktionale Anforderungen

### 6.1 Reaktionszeit

| Anforderung | Wert |
|---|---|
| Temperaturänderung → MCP4018-Update | < 50 ms |
| Tastendruck → Display-Update | < 100 ms |
| Web-Request `/set` → Antwort | < 500 ms |
| `/status` JSON-Antwort | < 100 ms |

### 6.2 Energieverbrauch (Schätzwerte)

| Betriebszustand | Strom | Laufzeit bei 700 mAh |
|---|---:|---:|
| Normal (Display an, WiFi) | ~110 mA | ~6,5 h |
| Normal (Display an, kein WiFi) | ~80 mA | ~8,5 h |
| Deep Sleep | ~15 µA | praktisch unbegrenzt |

### 6.3 Umgebungsbedingungen

Das Gerät ist für Innenraumeinsatz (Keller, Technikraum) ausgelegt.  
Betriebstemperatur: 0 °C – 50 °C · Luftfeuchtigkeit: nicht kondensierend.

### 6.4 Zuverlässigkeit

| Anforderung | Wert |
|---|---|
| NTC-Genauigkeit (nach B-Wert-Kalibrierung) | ±1 °C |
| MCP4018-Schritt-Auflösung | 390 Ω/Schritt → ±0,3 °C bei 0 °C |
| I²C-Fehler | 3 Wiederholungsversuche, dann Fehler-Flag |

---

## 7. Software-Architektur

### 7.1 Modulübersicht

```
src/
├── main.cpp          Haupt-Loop · Zustands-Automat · Deep-Sleep-Logik
├── ntc.h             ntcOhm(tempC) · ohmToStep(R) — pure functions, kein State
├── mcp4018.h/.cpp    I²C-Init · mcp4018Set(step) · mcp4018Probe()
├── battery.h/.cpp    batteryReadVolt() · batteryPercent() · batteryCharging()
├── display.h/.cpp    drawSplash() · drawMain(state) · drawShutdown()
├── wifi_mgr.h/.cpp   wifiConnect() · wifiStartAP() · webServerSetup() · otaSetup()
└── prefs.h/.cpp      prefsLoad() · prefsSave() · prefsLoadWifi() · prefsSaveWifi()

data/
└── logo.png          Splash-Logo (LittleFS, 260×158 px, RGB)

tools/
└── png_to_littlefs.py  Logo-Konvertierung (einmalig, benötigt Pillow)
```

### 7.2 Abhängigkeiten (PlatformIO)

```ini
lib_deps =
    lovyan03/LovyanGFX @ ^1.1.16
    ayushsharma82/ElegantOTA @ ^3.1.0
    arduino-libraries/ArduinoJson @ ^7.0.0
```

LittleFS und NVS sind Teil des ESP32-Arduino-Frameworks (keine externe Abhängigkeit).

---

## 8. Zustands-Automat

```
        Einschalten / Aufwachen
               │
               ▼
         ┌──────────┐
         │  SPLASH  │ 2 s
         └────┬─────┘
              │ automatisch
              ▼
         ┌──────────┐   WiFi-Verbindung ──► ┌──────────────┐
         │   MAIN   │◄────────────────────── │  WIFI_SETUP  │
         │  (STA)   │   Fehler / kein WLAN   │   (AP-Modus) │
         └────┬─────┘                        └──────────────┘
              │ beide Tasten 2 s
              │ oder SoC < 5 %
              ▼
         ┌──────────┐
         │  SLEEP   │ Deep Sleep (µA)
         └────┬─────┘
              │ Taste drücken
              └──────► SPLASH
```

Zustandsvariable `AppState` in `main.cpp`:  
`SPLASH → MAIN → SLEEP` (und `MAIN ↔ WIFI_SETUP`)

---

## 9. Display-Spezifikation

Display: ST7789V · 170 × 320 px · 8-Bit parallel · Rotation 1 (Querformat: 320 × 170)  
Bibliothek: LovyanGFX

### 9.1 Splash-Screen

```
┌────────────────────────────────────────────────────────────────┐  y=0
│                                                                 │
│                   ┌───────────────────┐                        │
│                   │  piep design Logo  │   260 × 158 px        │
│                   │  (logo.png)        │   zentriert            │
│                   └───────────────────┘                        │
│                                                      v1.0.0    │  y=155
└────────────────────────────────────────────────────────────────┘  y=170
```

| Element | Font | Farbe | Position |
|---|---|---|---|
| Logo | Bild | — | zentriert |
| Version | Font2 | Dunkelgrau | rechts unten (x=260, y=155) |

### 9.2 Hauptscreen

```
┌────────────────────────────────────────────────────────────────┐  y=0
│ Wolf CHA-07               I2C OK   ))) 192.168.1.42   🔋 78%  │  y=4
├────────────────────────────────────────────────────────────────┤  y=20
│                                                                 │
│                         -5.5 °C                                │  y=35
│                                                                 │
├────────────────────────────────────────────────────────────────┤  y=120
│    R = 22 237 Ω                        Schritt 57 / 127        │  y=134
├────────────────────────────────────────────────────────────────┤  y=150
│  BOOT ▼  (−)                                    (+)  ▲ KEY    │  y=155
└────────────────────────────────────────────────────────────────┘  y=170
```

| Element | Font | Größe | Farbe |
|---|---|---|---|
| Titel „Wolf CHA-07" | Font2 | 1 | Dunkelgrau |
| I²C-Status | Font2 | 1 | Grün / Rot |
| WiFi-Symbol + IP | Font2 | 1 | Weiß / Rot |
| Batterie-Icon + % | Font2 | 1 | Weiß / Rot (< 15 %) |
| Temperaturwert | Font7 | 1 | Weiß |
| °C | Font4 | 1 | Cyan |
| R = … Ω, Schritt … | Font2 | 1 | Gelb |
| Tasten-Hint | Font2 | 1 | Dunkelgrau |

### 9.3 Shutdown-Screen (vor Deep Sleep)

```
┌────────────────────────────────────────────────────────────────┐
│                                                                 │
│                    Gerät wird ausgeschaltet                     │
│                    Letzte Temp: -5.5 °C                        │
│                    Bitte Fühler wieder anschliessen             │
│                                                                 │
└────────────────────────────────────────────────────────────────┘
```

Anzeige für 2 Sekunden, dann Deep Sleep.

---

## 10. Web-Interface-Spezifikation

### 10.1 HTML-Hauptseite `GET /`

Responsive HTML5-Seite. Automatischer Refresh alle 10 Sekunden (`<meta http-equiv="refresh">`).

Inhalte:
- piep design Logo (eingebettet als Base64 oder verlinkt)
- Titel „Wolf CHA-07 Außenfühler-Emulator"
- Aktuelle Temperatur (groß)
- Schaltflächen `[−]` und `[+]` (POST auf `/set`)
- Tabellarische Statuswerte: Widerstand, MCP4018-Schritt, Batterie, Firmware-Version, IP
- Formular zur WLAN-Konfiguration (SSID + Passwort + Schaltfläche „Speichern & Neustart")

### 10.2 JSON-Status `GET /status`

```json
{
  "temp_c":      -5.5,
  "resistance":  22237,
  "step":        57,
  "battery_pct": 78,
  "charging":    false,
  "wifi_ssid":   "MeinNetz",
  "ip":          "192.168.1.42",
  "fw_version":  "v1.1.0",
  "uptime_s":    3600
}
```

### 10.3 Temperatur setzen `GET /set?t=<wert>`

- Parameter `t`: Dezimalzahl, z. B. `-5.5`
- Auflösung: 0,5 °C (Wert wird auf nächsten 0,5-°C-Schritt gerundet)
- Bereich: −15,0 bis +30,0
- Antwort: HTTP 302 Redirect auf `/`
- Fehler (außerhalb Bereich): HTTP 400 mit Fehlermeldung

### 10.4 WLAN-Konfiguration `POST /wifi`

- Body: `ssid=<name>&pass=<passwort>`
- Aktion: In NVS speichern, HTTP 200 mit Hinweis auf Neustart, dann `ESP.restart()`

---

## 11. Kommunikations-Schnittstellen

### 11.1 I²C — MCP4018

| Parameter | Wert |
|---|---|
| Pins | SDA = GPIO 18, SCL = GPIO 17 |
| Frequenz | 100 kHz |
| Adresse | 0x2F (7-Bit, fest) |
| Protokoll | Single-Byte-Write: `[START][0x5E][step][STOP]` |
| Fehlerbehandlung | 3 Retries, danach Fehler-Flag |

### 11.2 ADC — Batterie

| Parameter | Wert |
|---|---|
| Pin | GPIO 4 (ADC1_CH3) |
| Auflösung | 12 Bit (0–4095) |
| Spannungsteiler | ÷2 (on-board) |
| Formel | `U_bat = ADC_raw × 3,3 V / 4095 × 2` |
| Mittelung | 4 Messungen, gleitender Mittelwert |
| Intervall | alle 10 s |

### 11.3 WiFi — HTTP

| Parameter | Wert |
|---|---|
| Standard | 802.11 b/g/n, 2,4 GHz |
| Port | 80 |
| Protokoll | HTTP/1.1 |
| Bibliothek | ESP32 WebServer (Arduino-Framework) |

---

## 12. Fehlverhalten und Ausnahmen

| Fehler | Erkennung | Reaktion |
|---|---|---|
| MCP4018 nicht erreichbar | I²C EndTransmission ≠ 0 | Rote Statusanzeige; 3 Retries; weiter laufen |
| LittleFS mount fehlt | `LittleFS.begin()` false | Splash ohne Logo; Text-Fallback |
| WiFi-Verbindung fehlgeschlagen | 10-s-Timeout | AP-Modus starten |
| Temperatur außerhalb Bereich (Web) | Validierung in `/set` | HTTP 400 |
| Akku kritisch (< 5 %) | SoC-Berechnung | Shutdown-Screen, dann Deep Sleep |
| NVS leer | `nvs_get` not found | Standardwerte verwenden |

---

## 13. NTC-Kennlinien-Daten

**Sensor:** Wolf CHA-07 Außenfühler, Art.-Nr. 2748916  
**Typ:** NTC 5 kΩ bei 25 °C  
**B-Wert:** 3977 K *(Schätzwert — Herstellerangabe nicht veröffentlicht; mit Messung verifizieren)*

| Temperatur | Widerstand | MCP4018-Schritt | Schrittweite* |
|---:|---:|---:|---:|
| −15 °C | 39 499 Ω | 101 | — |
| −10 °C | 29 476 Ω | 75 | ≈ 0,2 °C/Schritt |
| −5 °C | 22 237 Ω | 57 | ≈ 0,3 °C/Schritt |
| 0 °C | 16 950 Ω | 43 | ≈ 0,4 °C/Schritt |
| +5 °C | 13 047 Ω | 33 | ≈ 0,5 °C/Schritt |
| +10 °C | 10 136 Ω | 26 | ≈ 0,7 °C/Schritt |
| +15 °C | 7 943 Ω | 20 | ≈ 0,8 °C/Schritt |
| +20 °C | 6 277 Ω | 16 | ≈ 1,3 °C/Schritt |
| +25 °C | 5 000 Ω | 13 | ≈ 1,5 °C/Schritt |
| +30 °C | 4 013 Ω | 10 | ≈ 1,7 °C/Schritt |

*\* Temperaturauflösung pro MCP4018-Schritt — im relevanten Winterbereich (−10 bis +5 °C) besser als 0,5 °C.*

**B-Wert verifizieren:**  
Sensor bei bekannter Temperatur (z. B. Eiswasser = 0 °C) mit Multimeter messen.  
Gemessenen Wert R und T = 273,15 K in `B = ln(R/R25) / (1/T − 1/T25)` einsetzen.  
Ergebnis in `ntc.h` als `NTC_B` eintragen.

---

## 14. Offene Punkte

| Nr. | Punkt | Priorität | Status |
|---|---|---|---|
| OP-01 | Wolf NTC B-Wert messtechnisch verifizieren | Hoch | Offen |
| OP-02 | Laden-Erkennung: GPIO-Pin des Lader-IC prüfen (falls vorhanden) | Mittel | Offen |
| OP-03 | Display-Ausrichtung festlegen (USB-C nach oben oder unten) | Niedrig | Offen |
| OP-04 | Gehäuse / Halterung | Niedrig | Nicht spezifiziert |
| OP-05 | OTA-Passwortschutz (aktuell ohne Passwort) | Niedrig | Bewusst offen |

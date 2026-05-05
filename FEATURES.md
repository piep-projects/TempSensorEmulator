# Feature-Spezifikation — Wolf CHA-07 Außenfühler-Emulator

> **Hinweis:** Dieses Dokument ist die Planungsgrundlage. Die verbindliche Spezifikation steht in [FDS.md](FDS.md).

Version: 1.1  
Hardware: LilyGo T-Display-S3 · MCP4018T-503 (50 kΩ) · LiPo 700 mAh

---

## 1. Startsequenz

**Splash-Screen** (2 Sekunden):
- piep design Logo zentriert auf dem Display
- Firmware-Version unten rechts (z. B. `v1.0.0`)
- Keine Benutzeraktion erforderlich; danach automatisch → Hauptscreen

---

## 2. Hauptscreen

Layout (Querformat 320 × 170 px):

```
┌──────────────────────────────────────────────────────────────┐
│ Wolf CHA-07                         ))) 192.168.1.x  🔋 78% │  ← Statusleiste
├──────────────────────────────────────────────────────────────┤
│                                                               │
│                      -5.5 °C                                  │  ← groß, zentriert
│                                                               │
│    R = 22 237 Ω              Schritt 57 / 127                 │  ← Info
├──────────────────────────────────────────────────────────────┤
│  BOOT ▼ (–)                                   (＋) ▲ KEY    │  ← Tasten-Hint
└──────────────────────────────────────────────────────────────┘
```

| Element | Beschreibung |
|---|---|
| Temperatur | Groß, weiß; ½-°C-Schritte |
| Widerstand | Berechneter NTC-Wert in Ω |
| Schritt | MCP4018-Schrittnummer (0–127) |
| WiFi-Symbol | Signal-Balken + IP-Adresse; rot = getrennt |
| Batterie | Icon + Prozent; Blitz-Symbol wenn laden |

---

## 3. Temperaturregelung

| Aktion | Funktion |
|---|---|
| Kurzer Druck BOOT | −0,5 °C |
| Kurzer Druck KEY | +0,5 °C |
| Halten BOOT | Schnell runter (auto-repeat 130 ms) |
| Halten KEY | Schnell hoch (auto-repeat 130 ms) |
| Bereich | −15 °C bis +30 °C |

Eingestellte Temperatur wird sofort an MCP4018 übertragen.  
Wert wird im Flash (NVS) gespeichert und nach Neustart wiederhergestellt.

---

## 4. Ein / Aus (Deep Sleep)

| Aktion | Funktion |
|---|---|
| Beide Tasten gleichzeitig 2 s halten | Display aus, ESP32 → Deep Sleep |
| Beliebige Taste drücken | Aufwachen, Splash → Hauptscreen |

Im Deep Sleep: MCP4018 hält letzten Wert (I²C-Pegel fallen ab → Widerstand undefiniert).  
**Empfehlung:** Heizung vor dem Ausschalten auf echten Sensor umschalten.

---

## 5. Batterie

| Funktion | Details |
|---|---|
| Spannungsmessung | GPIO 4 (ADC, Teiler ÷2 on-board) |
| Ladestand-Anzeige | 0–100 %, 5-stufiges Icon |
| Laden-Erkennung | Spannung > 4,0 V und steigend → Blitz-Symbol |
| Kapazität | 700 mAh LiPo |
| Warnschwelle | < 15 % → Icon rot blinkend |
| Kritisch | < 5 % → automatisch Deep Sleep |

Spannungs-zu-Prozent-Kurve (LiPo typisch):

| Spannung | Ladezustand |
|---:|---:|
| 4,20 V | 100 % |
| 4,00 V | 85 % |
| 3,80 V | 60 % |
| 3,60 V | 30 % |
| 3,40 V | 10 % |
| 3,20 V | 0 % (Schutzabschaltung) |

---

## 6. WiFi

> **Entschieden:** WiFi-Konfiguration über **WiFiManager Captive Portal** (tzapu/WiFiManager).  
> Browser öffnet sich automatisch beim Verbinden mit dem AP — wie Hotel-WLAN.

### Verbindungsaufbau
1. Gespeicherte Zugangsdaten (NVS) vorhanden → verbinden, max. 10 s
2. Kein Netz konfiguriert oder Verbindung fehlgeschlagen → **WiFiManager Captive Portal**
   - SSID: `CHA-Emulator`
   - Passwort: `wolf1234`
   - Browser öffnet sich automatisch → Netzauswahl + Passwort → Speichern → Neustart

### Web-Interface (Port 80)

Aufrufbar über IP-Adresse im Browser (Handy oder PC).

**Hauptseite `/`**

```
┌─────────────────────────────────────────────┐
│        piep design                           │
│   Wolf CHA-07 Außenfühler-Emulator          │
│                                             │
│   Temperatur          [ − ]  -5.5°C  [ + ] │
│   Widerstand          22 237 Ω              │
│   Batterie            78 %  (laden)         │
│   Firmware            v1.0.0                │
│   IP                  192.168.1.42          │
└─────────────────────────────────────────────┘
```

- Seite aktualisiert sich alle 5 Sekunden (auto-refresh)
- `[−]` / `[+]` senden HTTP-GET `/set?t=-6.0`
- Temperatur in 0,5-°C-Schritten einstellbar

**Endpunkte:**

| Endpunkt | Methode | Beschreibung |
|---|---|---|
| `/` | GET | Hauptseite (HTML) |
| `/set?t=<wert>` | GET | Temperatur setzen (−15 bis +30) |
| `/status` | GET | JSON: temp, resistance, step, battery_pct, charging, ip |
| `/update` | GET/POST | OTA-Firmware-Update (ElegantOTA) |

### OTA
- Bibliothek: **ElegantOTA**
- Erreichbar unter `http://<IP>/update`
- Kein Passwort (Gerät ist im lokalen Netz)

---

## 7. Software-Architektur

```
src/
  main.cpp          # Setup, Loop, Zustands-Automat, Deep-Sleep
  display.h/.cpp    # Splash, Main, WiFi-Setup, Shutdown + Icons
  ntc.h             # NTC-Kennlinie, ohmToStep()
  mcp4018.h/.cpp    # I²C-Treiber MCP4018
  battery.h/.cpp    # ADC-Messung, Ladestand, Laden-Erkennung
  wifi_mgr.h/.cpp   # WiFiManager, Web-Server, OTA
  prefs.h/.cpp      # NVS: Temperatur + WiFi-Credentials speichern
data/
  logo.png          # piep design Logo weiß (für LittleFS)
mockups/
  screen_splash.png
  screen_main.png
  screen_wifi_setup.png
  screen_shutdown.png
```

---

## 8. Firmware-Versionsschema

`MAJOR.MINOR.PATCH` — z. B. `v1.0.0`  
Definiert als `#define FW_VERSION "v1.0.0"` in `main.cpp`.

---

## 9. Offene Punkte / Entscheidungen

| Punkt | Status |
|---|---|
| Wolf NTC B-Wert verifizieren | offen — Messung ausstehend |
| WiFi-Ersteinrichtung | ✅ WiFiManager Captive Portal |
| Display-Screens | ✅ 4 Screens: Splash, Main, WiFi-Setup, Shutdown |
| Logo auf Display | ✅ weiß auf schwarz; klein in Statusleiste (Main) |
| Temperaturanzeige | ✅ große Mono-Schrift, °C in Cyan |
| Display-Ausrichtung (USB oben/unten) | offen |
| Gehäuse / Halterung | nicht spezifiziert |

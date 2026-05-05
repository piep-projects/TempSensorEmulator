#include "wifi_mgr.h"
#include "config.h"
#include "display.h"
#include "ntc.h"
#include "battery.h"
#include <WiFiManager.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>

// ── Geteilter Zustand (gesetzt von main.cpp via Setter) ───────
static float*   g_pTemp = nullptr;   // Zeiger auf g_tempC in main
static bool     g_connected = false;
static String   g_ip = "---";

static WebServer  server(WEB_PORT);
static WiFiManager wm;

// ── HTML-Template ─────────────────────────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="10">
<title>CHA-07 Emulator</title>
<style>
  body{background:#111;color:#eee;font-family:sans-serif;text-align:center;padding:16px;max-width:420px;margin:auto}
  h1{font-size:1em;color:#888;margin:0 0 4px}
  h2{font-size:2.8em;font-weight:bold;margin:16px 0 4px}
  .unit{color:#0dd;font-size:.7em}
  .row{display:flex;justify-content:center;gap:12px;margin:12px 0}
  a.btn{background:#2a2a2a;color:#fff;border:1px solid #444;border-radius:10px;
        font-size:2em;padding:12px 28px;text-decoration:none;display:inline-block}
  a.btn:hover{background:#3a3a3a}
  table{margin:16px auto;border-collapse:collapse;width:100%%}
  td{padding:6px 12px;text-align:left;border-bottom:1px solid #222}
  td:first-child{color:#888;width:45%%}
  .warn{color:#f80}
  hr{border-color:#333;margin:20px 0}
  a.rst{color:#f44;font-size:.85em;text-decoration:none;border:1px solid #622;
        border-radius:6px;padding:6px 14px}
</style>
</head>
<body>
<h1>Wolf CHA-07 Aussenfuhler-Emulator</h1>
<h2>%s <span class="unit">°C</span></h2>
<div class="row">
  <a class="btn" href="/set?t=%s">−</a>
  <a class="btn" href="/set?t=%s">+</a>
</div>
<table>
<tr><td>Widerstand</td><td>%.0f &Omega;</td></tr>
<tr><td>Schritt</td><td>%d / 127</td></tr>
<tr><td>Batterie</td><td>%d %% %s</td></tr>
<tr><td>Firmware</td><td>%s</td></tr>
<tr><td>IP</td><td>%s</td></tr>
</table>
<hr>
<a class="rst" href="/wifi-reset">WLAN zurucksetzen &amp; Neustart</a>
</body>
</html>
)rawhtml";

// ── Setter von main.cpp ───────────────────────────────────────
void wifiSetTempPtr(float* ptr) { g_pTemp = ptr; }

// ── Handler ───────────────────────────────────────────────────
static void handleRoot() {
    if (!g_pTemp) { server.send(503, "text/plain", "not ready"); return; }
    float t   = *g_pTemp;
    float ohm = ntcOhm(t);
    int   stp = ntcToStep(t);
    int   bat = batteryPercent();
    bool  chg = batteryCharging();

    // Temp ±0,5 gerundet
    float tMinus = max(t - TEMP_STEP, (float)TEMP_MIN);
    float tPlus  = min(t + TEMP_STEP, (float)TEMP_MAX);

    char tStr[8], tmStr[8], tpStr[8];
    snprintf(tStr,  sizeof(tStr),  "%.1f", t);
    snprintf(tmStr, sizeof(tmStr), "%.1f", tMinus);
    snprintf(tpStr, sizeof(tpStr), "%.1f", tPlus);

    char page[sizeof(HTML_PAGE) + 128];
    snprintf(page, sizeof(page), HTML_PAGE,
             tStr, tmStr, tpStr,
             ohm, stp,
             bat, chg ? "&#x26A1;" : "",
             FW_VERSION,
             g_ip.c_str());

    server.send(200, "text/html; charset=UTF-8", page);
}

static void handleSet() {
    if (!g_pTemp || !server.hasArg("t")) {
        server.send(400, "text/plain", "missing t");
        return;
    }
    float t = server.arg("t").toFloat();
    // Auf 0,5-°C-Raster runden und begrenzen
    t = roundf(t / TEMP_STEP) * TEMP_STEP;
    t = constrain(t, TEMP_MIN, TEMP_MAX);
    *g_pTemp = t;
    server.sendHeader("Location", "/");
    server.send(302);
}

static void handleStatus() {
    if (!g_pTemp) { server.send(503, "text/plain", "not ready"); return; }
    float t = *g_pTemp;
    JsonDocument doc;
    doc["temp_c"]      = t;
    doc["resistance"]  = (int)ntcOhm(t);
    doc["step"]        = ntcToStep(t);
    doc["battery_pct"] = batteryPercent();
    doc["charging"]    = batteryCharging();
    doc["ip"]          = g_ip;
    doc["fw_version"]  = FW_VERSION;
    doc["uptime_s"]    = millis() / 1000;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

static void handleWifiReset() {
    server.send(200, "text/html; charset=UTF-8",
        "<html><body style='background:#111;color:#eee;font-family:sans-serif;text-align:center;padding:40px'>"
        "<h2>WLAN wird zurueckgesetzt...</h2><p>Gerat startet neu.</p></body></html>");
    delay(1000);
    wm.resetSettings();
    ESP.restart();
}

// ── Öffentliche API ───────────────────────────────────────────
void wifiBegin() {
    wm.setConfigPortalTimeout(300);   // 5 min Timeout für Captive Portal
    wm.setConnectTimeout(WIFI_TIMEOUT_S);
    wm.setConnectRetries(3);

    // Beim Start des AP-Modus WiFi-Setup-Screen anzeigen
    wm.setAPCallback([](WiFiManager*) {
        drawWifiSetup();
        g_connected = false;
        g_ip = "AP 192.168.4.1";
    });

    if (wm.autoConnect(WIFI_AP_SSID, WIFI_AP_PASS)) {
        g_connected = true;
        g_ip = WiFi.localIP().toString();
        Serial.printf("WiFi verbunden: %s\n", g_ip.c_str());
    } else {
        g_connected = false;
        g_ip = "---";
        Serial.println("WiFi nicht verbunden — ohne Netz weiter.");
    }

    // Webserver starten (auch ohne WiFi, für AP-Modus)
    server.on("/",           HTTP_GET,  handleRoot);
    server.on("/set",        HTTP_GET,  handleSet);
    server.on("/status",     HTTP_GET,  handleStatus);
    server.on("/wifi-reset", HTTP_GET,  handleWifiReset);
    ElegantOTA.begin(&server);
    server.begin();
    Serial.printf("Webserver gestartet auf Port %d\n", WEB_PORT);
}

void wifiLoop() {
    server.handleClient();
    ElegantOTA.loop();
}

String wifiIP() { return g_ip; }

bool wifiConnected() { return g_connected; }

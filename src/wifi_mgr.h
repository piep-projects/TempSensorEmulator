#pragma once
#include <Arduino.h>

// Zeiger auf g_tempC setzen (vor wifiBegin aufrufen)
void wifiSetTempPtr(float* ptr);

// Verbindet mit gespeichertem Netz (kurzer Timeout, kein Portal).
void wifiBegin();

// Startet WiFiManager Captive Portal (blockierend, ~5 min).
// Ruft drawWifiSetup() auf. Startet Webserver falls danach verbunden.
void wifiStartPortal();

// HTTP-Server in loop() bedienen
void wifiLoop();

// Aktuell zugewiesene IP (oder "AP 192.168.4.1" oder "---")
String wifiIP();

bool wifiConnected();

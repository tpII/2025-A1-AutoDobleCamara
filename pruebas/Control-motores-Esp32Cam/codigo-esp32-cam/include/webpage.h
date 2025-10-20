#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>

// Configuración WiFi
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Servidor web
WebServer server(80);
IPAddress robotIP(192,168,4,2); // IP del otro ESP32 que controla los motores
uint16_t robotPort = 12345;     // puerto del otro ESP32

// Configuración de la cámara según tu tabla
camera_config_t config;

// HTML de emergencia ultra-simplificado (solo para casos extremos)
const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head><title>ESP32 Robot - Emergency</title></head>
<body style="background:#000;color:#fff;text-align:center;font-family:Arial;">
    <h1>🚨 ESP32 Robot - Emergency Mode</h1>
    <p>Main interface failed to load</p>
    <a href="/cmd?cmd=STOP" style="color:#f00;font-size:24px;">EMERGENCY STOP</a><br><br>
    <a href="/stream" style="color:#0f0;">Video Stream</a><br>
    <a href="/cmd?cmd=PING" style="color:#00f;">Test Connection</a>
</body>
</html>
)rawliteral";
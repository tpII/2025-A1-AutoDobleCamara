#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "config.h"
#include "wifi_config.h"
#include "camera_config.h"

WebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32-CAM Stream</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #1a1a1a;
            color: #fff;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
        }
        h1 {
            margin-bottom: 20px;
            color: #4CAF50;
        }
        #stream-container {
            max-width: 90vw;
            max-height: 80vh;
            border: 3px solid #4CAF50;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.3);
        }
        img {
            display: block;
            width: 100%;
            height: auto;
        }
        .info {
            margin-top: 20px;
            padding: 10px;
            background-color: #2a2a2a;
            border-radius: 5px;
            text-align: center;
        }
        .status {
            display: inline-block;
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 8px;
            background-color: #4CAF50;
        }
        .offline {
            background-color: #f44336;
        }
    </style>
</head>
<body>
    <h1>ESP32-CAM Live Stream</h1>
    <div id="stream-container">
        <img id="stream" src="/stream" alt="Video Stream">
    </div>
    <div class="info">
        <span id="status" class="status"></span>
        <span id="status-text">Connected</span>
    </div>
    <script>
        const streamImg = document.getElementById('stream');
        const status = document.getElementById('status');
        const statusText = document.getElementById('status-text');
        streamImg.onerror = function() {
            status.classList.add('offline');
            statusText.textContent = 'Stream Disconnected';
        };
        streamImg.onload = function() {
            status.classList.remove('offline');
            statusText.textContent = 'Connected';
        };
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleStream() {
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    String head = "--frame\r\n";
    head += "Content-Type: image/jpeg\r\n";
    head += "Content-Length: " + String(fb->len) + "\r\n\r\n";
    
    server.sendContent(head);
    client.write(fb->buf, fb->len);
    server.sendContent("\r\n");
    
    esp_camera_fb_return(fb);
    
    if (!client.connected()) {
      break;
    }
  }
}

void connectToWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttemptTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    Serial.println("Failed to connect to WiFi");
    Serial.println("Please check your credentials in wifi_config.h");
    return;
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP32-CAM Starting...");
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    return;
  }
  
  // Connect to WiFi
  connectToWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    // Setup web server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/stream", HTTP_GET, handleStream);
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("Open your browser and navigate to:");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
}
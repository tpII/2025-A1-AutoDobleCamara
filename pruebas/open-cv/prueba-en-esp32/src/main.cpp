#include <Arduino.h>
#include "config.h"
#include "CameraVision.h"
#include "Network.h"

CameraVision cameraVision;
NetworkManager networkManager;

void OnDataRecv(const uint8_t *mac, int len) {}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== Iniciando sistema ===");
    
    networkManager.setup(OnDataRecv);
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi conectado - IP: ");
        Serial.println(WiFi.localIP());
    }
    
    delay(2000);
    
    if (cameraVision.setup()) {
        Serial.println("Camara OK");
        g_cameraVision = &cameraVision;
        
        ColorRange rango;
        rango.r_min = 0; rango.r_max = 90;
        rango.g_min = 120; rango.g_max = 255;
        rango.b_min = 0; rango.b_max = 90;
        cameraVision.setColorRange(rango);
    }
    
    Serial.println("=== Sistema listo ===\n");
}

void loop() {
    networkManager.handleClient();
    cameraVision.run();
    delay(50);
}

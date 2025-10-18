#include "camera.h"
#include <esp_camera.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_timer.h>
#include <Arduino.h>
#include <Wire.h>
#include "config.h"

static WebServer* server = nullptr;

// Función para escanear dispositivos I2C
void scanI2C() {
    Serial.println("\n🔍 Escaneando bus I2C...");
    
    // Configuración I2C más robusta
    Wire.begin(CAMERA_PIN_SIOD, CAMERA_PIN_SIOC); // SDA, SCL
    Wire.setClock(50000); // 50kHz - Velocidad muy baja para mayor estabilidad
    Wire.setTimeOut(1000); // Timeout de 1 segundo
    
    // Configurar pull-ups explícitos para I2C
    pinMode(CAMERA_PIN_SIOD, INPUT_PULLUP);
    pinMode(CAMERA_PIN_SIOC, INPUT_PULLUP);
    delay(100); // Tiempo extra para estabilizar pull-ups
    
    int devices = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("✅ Dispositivo I2C encontrado en 0x%02X", addr);
            if (addr == 0x30) Serial.print(" (OV2640 esperado)");
            if (addr == 0x21) Serial.print(" (OV7670)");
            if (addr == 0x3C) Serial.print(" (OV5640)");
            Serial.println();
            devices++;
        } else if (error == 4) {
            Serial.printf("⚠️ Error desconocido en dirección 0x%02X\n", addr);
        }
    }
    
    if (devices == 0) {
        Serial.println("❌ No se encontraron dispositivos I2C");
        Serial.println("   Verificar conexiones SDA/SCL y alimentación");
        Serial.println("   También verificar resistencias pull-up en SDA/SCL");
    } else {
        Serial.printf("📊 Total dispositivos I2C: %d\n", devices);
    }
    Serial.println();
}

bool cameraInit()
{
    Serial.println("📸 Iniciando diagnóstico de cámara...");
    
    // Diagnóstico de alimentación
    Serial.printf("⚡ Voltaje de alimentación: %.2fV\n", analogRead(A0) * 3.3 / 4096.0);
    Serial.printf("🧠 RAM libre antes de cámara: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("💾 PSRAM disponible: %s\n", psramFound() ? "SÍ" : "NO");
    if (psramFound()) {
        Serial.printf("💾 PSRAM libre: %d bytes\n", ESP.getFreePsram());
    }
    
    // Secuencia de reset extendida del sensor
    if (CAMERA_PIN_PWDN >= 0) {
        Serial.printf("🔧 Reset completo del sensor OV2640 (GPIO %d)...\n", CAMERA_PIN_PWDN);
        pinMode(CAMERA_PIN_PWDN, OUTPUT);
        
        // Secuencia de reset extendida
        digitalWrite(CAMERA_PIN_PWDN, HIGH); // Poner en power-down
        delay(200);  // Mantener en power-down por 200ms
        digitalWrite(CAMERA_PIN_PWDN, LOW);  // Activar cámara
        delay(300);  // Esperar estabilización del sensor
        
        Serial.println("✅ Reset del sensor completado");
    }
    
    // Diagnóstico I2C después del reset
    scanI2C();
    
    // Verificar alimentación estable
    Serial.println("⏱️ Esperando estabilización completa del sistema...");
    delay(500);
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;
    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;
    config.pin_sccb_sda = CAMERA_PIN_SIOD;
    config.pin_sccb_scl = CAMERA_PIN_SIOC;
    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;
    config.pixel_format = PIXFORMAT_JPEG;

    // init with high resolution but reduce if OOM
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    // Configuración conservadora de memoria para evitar crashes TCP/IP
    config.frame_size = FRAMESIZE_QQVGA;    // 160x120 - Mínimo absoluto
    config.jpeg_quality = 20;               // Máxima compresión
    config.fb_count = 1;                    // Un solo buffer
    
    // Detectar automáticamente si hay PSRAM disponible
    if (psramFound()) {
        config.fb_location = CAMERA_FB_IN_PSRAM;
        Serial.println("🎯 Usando PSRAM para frame buffers");
    } else {
        config.fb_location = CAMERA_FB_IN_DRAM;
        Serial.println("🎯 Usando DRAM para frame buffers (sin PSRAM)");
    }
    
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // Modo más conservador
    config.xclk_freq_hz = 0;                // Sin clock externo

    Serial.println("Intentando inicializar camara con configuracion I2C optimizada...");
    esp_err_t err = esp_camera_init(&config);
    
    // Si falla la primera vez, intentar con configuración alternativa
    if (err != ESP_OK) {
        Serial.println("Primer intento fallo - probando configuracion alternativa...");
        
        // Reinicializar I2C con configuración aún más conservadora
        Wire.end();
        delay(100);
        Wire.begin(CAMERA_PIN_SIOD, CAMERA_PIN_SIOC);
        Wire.setClock(20000); // 20kHz - Muy lento pero más estable
        delay(200);
        
        // Segundo intento con configuración mínima
        config.frame_size = FRAMESIZE_96X96;  // 96x96 - Absolutamente mínimo
        config.jpeg_quality = 30;             // Compresión muy alta
        config.fb_count = 1;                  // Un solo buffer
        config.fb_location = CAMERA_FB_IN_DRAM; // Solo memoria interna
        
        Serial.println("Segundo intento con configuracion minima...");
        err = esp_camera_init(&config);
    }
    
    // Si aún falla, último intento con reset adicional
    if (err != ESP_OK && CAMERA_PIN_PWDN >= 0) {
        Serial.println("Ultimo intento con reset adicional del sensor...");
        
        // Reset adicional más agresivo
        digitalWrite(CAMERA_PIN_PWDN, HIGH);
        delay(500);  // 500ms en power-down
        digitalWrite(CAMERA_PIN_PWDN, LOW);
        delay(1000); // 1 segundo de estabilización
        
        err = esp_camera_init(&config);
    }
    if (err != ESP_OK) {
        Serial.printf("❌ Camera init failed with error 0x%x\n", err);
        
        // Diagnóstico específico según el error
        switch(err) {
            case ESP_ERR_INVALID_ARG:
                Serial.println("💡 Error: Configuración inválida");
                break;
            case ESP_ERR_NOT_FOUND:
                Serial.println("💡 Error: Cámara no encontrada - verificar conexiones I2C");
                break;
            case ESP_ERR_NO_MEM:
                Serial.println("💡 Error: Sin memoria suficiente para frame buffers");
                Serial.println("   Solución: Reducir frame_size o usar PSRAM");
                break;
            case ESP_FAIL:
                Serial.println("💡 Error: Fallo general");
                Serial.println("   Posibles causas:");
                Serial.println("   - Memoria insuficiente para buffers");
                Serial.println("   - Configuración de pines incorrecta");
                Serial.println("   - Problema de alimentación");
                break;
            default:
                Serial.printf("💡 Error desconocido: 0x%x\n", err);
        }
        
        // Volver a escanear I2C después del fallo
        Serial.println("\n🔍 Re-escaneando I2C después del fallo...");
        scanI2C();
        
        return false;
    } else if (err == ESP_OK) {
        Serial.println("¡Cámara inicializada correctamente!");
        
        // Determinar qué configuración se usó finalmente
        const char* frameSize = "DESCONOCIDO";
        switch(config.frame_size) {
            case FRAMESIZE_96X96: frameSize = "96x96"; break;
            case FRAMESIZE_QQVGA: frameSize = "QQVGA (160x120)"; break;
            case FRAMESIZE_QVGA: frameSize = "QVGA (320x240)"; break;
            case FRAMESIZE_CIF: frameSize = "CIF (400x296)"; break;
            case FRAMESIZE_VGA: frameSize = "VGA (640x480)"; break;
        }
        
        Serial.printf("Configuración: %s, Calidad: %d, Buffers: %d\n", 
                  frameSize, config.jpeg_quality, config.fb_count);
        Serial.printf("📊 Memoria libre después de cámara: %d bytes\n", ESP.getFreeHeap());
    }
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
      s->set_brightness(s, 0);    
      s->set_contrast(s, 1);      
      s->set_saturation(s, 1);    
      s->set_special_effect(s, 0); 
      s->set_whitebal(s, 1);       
      s->set_awb_gain(s, 1);       
      s->set_wb_mode(s, 0);        
      s->set_exposure_ctrl(s, 1);  
      s->set_aec2(s, 1);           
      s->set_gain_ctrl(s, 1);      
      s->set_agc_gain(s, 0);       
      s->set_bpc(s, 1);            
      s->set_wpc(s, 1);            
      s->set_raw_gma(s, 1);        
      s->set_lenc(s, 1);           
      s->set_hmirror(s, 0);        
      s->set_vflip(s, 0);          
      s->set_dcw(s, 1);            
      s->set_colorbar(s, 0);       
      Serial.println("Configuración avanzada del sensor aplicada");
    }

    // Test de captura
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("❌ Error capturando imagen de prueba");
    } else {
      Serial.printf("✅ Imagen de prueba: %d bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
    
    return true;
}

// handler stream
static void handleStream()
{
    WiFiClient client = server->client();
    String response = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ";
    camera_fb_t * fb = nullptr;
    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            server->send(500, "text/plain", "Camera capture failed");
            return;
        }
        client.print("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: ");
        client.print(fb->len);
        client.print("\r\n\r\n");
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        // break condition if client disconnects
        if (!client.connected()) break;
        // yield to avoid watchdog
        delay(10);
    }
}

void cameraServerBegin(uint16_t port)
{
    if (server) return;
    server = new WebServer(port);
    server->on("/", [](){ server->send(200, "text/html", "<html><body><a href=\"/stream\">/stream</a></body></html>"); });
    server->on("/stream", HTTP_GET, [](){
        server->sendHeader("Connection", "close");
        server->sendHeader("Max-Age", "0");
        server->sendHeader("Expires", "0");
        server->sendHeader("Cache-Control", "no-cache, private");
        server->sendHeader("Pragma", "no-cache");
        server->setContentLength(CONTENT_LENGTH_UNKNOWN);
        server->send(200, "multipart/x-mixed-replace; boundary=frame", "");
        handleStream();
    });
    server->begin();
    Serial.printf("Camera server started on port %u\n", port);
}

void cameraLoop()
{
    if (server) server->handleClient();
}

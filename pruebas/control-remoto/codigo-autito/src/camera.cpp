#include "camera.h"
#include <esp_camera.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_timer.h>
#include <Arduino.h>
#include "config.h"

static WebServer* server = nullptr;

bool cameraInit()
{
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

    config.frame_size = FRAMESIZE_QVGA;     // 320x240 - Mejor calidad
    config.jpeg_quality = 8;                // Calidad excelente (menor compresión)
    config.fb_count = 2;                    // Doble buffer para fluidez
    config.fb_location = CAMERA_FB_IN_DRAM; // Forzar DRAM para velocidad
    config.grab_mode = CAMERA_GRAB_LATEST;  // Siempre el frame más reciente
    config.xclk_freq_hz = 0;                // Sin clock externo

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    } else if (err == ESP_OK) {
        Serial.println("¡Cámara inicializada correctamente!");
        Serial.printf("Configuración: QVGA (320x240), Calidad: %d, Buffers: %d\n", 
                  config.jpeg_quality, config.fb_count);
    }
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV2640_PID) {
        Serial.println("Camera: OV2640 detected");
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

#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "color_detector.h"

const float OBJETO_ANCHO_REAL_CM = 5.0;
const float FOCAL_LENGTH = 200.0;

#define CAMERA_FRAME_SIZE FRAMESIZE_SVGA
#define JPEG_QUALITY 12
#define XCLK_FREQ_HZ 20000000

const char* ssid = "Personal-140-2.4GHz";
const char* password = "00417225972";

WebServer server(80);

camera_config_t config;

void testCameraConnections() {
  Serial.println("\n🔍 === DIAGNÓSTICO DE CONEXIONES ===");
  
  // Verificar pines básicos
  Serial.println("📋 Configuración de pines:");
  Serial.printf("- SIOD (SDA): GPIO %d\n", 21);
  Serial.printf("- SIOC (SCL): GPIO %d\n", 22);
  Serial.printf("- XCLK: GPIO %d\n", -1);
  Serial.printf("- PCLK: GPIO %d\n", 26);
  Serial.printf("- VSYNC: GPIO %d\n", 25);
  Serial.printf("- HREF: GPIO %d\n", 23);
  Serial.printf("- D0-D7: %d,%d,%d,%d,%d,%d,%d,%d\n", 5,18,19,27,35,34,39,36);
  Serial.printf("- PWDN: %s\n", "No conectado");
  Serial.printf("- RESET: %s\n", "No conectado");
}

void setupCamera() {
  testCameraConnections();
  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 27;
  config.pin_d4 = 35;
  config.pin_d5 = 34;
  config.pin_d6 = 39;
  config.pin_d7 = 36;
  config.pin_xclk = -1;      
  config.pin_pclk = 26;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 21;
  config.pin_sccb_scl = 22;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = CAMERA_FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 1;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  
  config.xclk_freq_hz = XCLK_FREQ_HZ;                    

  esp_err_t err = esp_camera_init(&config);
  
  if (err == ESP_OK) {
    Serial.println("✅ Cámara inicializada correctamente!");
    
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
      framesize_t fs = s->status.framesize;
      const char* size_name = "Unknown";
      int width = 0, height = 0;
      switch(fs) {
        case FRAMESIZE_96X96:    size_name = "96x96";     width = 96;   height = 96;   break;
        case FRAMESIZE_QQVGA:    size_name = "QQVGA";     width = 160;  height = 120;  break;
        case FRAMESIZE_QCIF:     size_name = "QCIF";      width = 176;  height = 144;  break;
        case FRAMESIZE_HQVGA:    size_name = "HQVGA";     width = 240;  height = 176;  break;
        case FRAMESIZE_240X240:  size_name = "240x240";   width = 240;  height = 240;  break;
        case FRAMESIZE_QVGA:     size_name = "QVGA";      width = 320;  height = 240;  break;
        case FRAMESIZE_CIF:      size_name = "CIF";       width = 400;  height = 296;  break;
        case FRAMESIZE_HVGA:     size_name = "HVGA";      width = 480;  height = 320;  break;
        case FRAMESIZE_VGA:      size_name = "VGA";       width = 640;  height = 480;  break;
        case FRAMESIZE_SVGA:     size_name = "SVGA";      width = 800;  height = 600;  break;
        case FRAMESIZE_XGA:      size_name = "XGA";       width = 1024; height = 768;  break;
        case FRAMESIZE_HD:       size_name = "HD";        width = 1280; height = 720;  break;
        case FRAMESIZE_SXGA:     size_name = "SXGA";      width = 1280; height = 1024; break;
        case FRAMESIZE_UXGA:     size_name = "UXGA";      width = 1600; height = 1200; break;
      }
      
      Serial.printf("📐 Resolución: %s (%dx%d)\n", size_name, width, height);
      Serial.printf("📊 Calidad JPEG: %d (menor = mejor)\n", config.jpeg_quality);
      Serial.printf("🗃️  Buffers: %d\n", config.fb_count);
    }
    
    if (s) {
      s->set_brightness(s, 0);
      s->set_contrast(s, 0);
      s->set_saturation(s, 0);
      s->set_special_effect(s, 0);
      s->set_whitebal(s, 1);
      s->set_awb_gain(s, 1);
      s->set_wb_mode(s, 0);
      s->set_exposure_ctrl(s, 1);
      s->set_aec2(s, 0);
      s->set_gain_ctrl(s, 1);
      s->set_agc_gain(s, 0);
      s->set_bpc(s, 0);
      s->set_wpc(s, 0);
      s->set_raw_gma(s, 1);
      s->set_lenc(s, 1);
      s->set_hmirror(s, 0);
      s->set_vflip(s, 0);
      s->set_dcw(s, 1);
      s->set_colorbar(s, 0);
      
      s->set_framesize(s, CAMERA_FRAME_SIZE);
      s->set_quality(s, JPEG_QUALITY);
      
      Serial.println("Configuración avanzada del sensor aplicada");
    }
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Error capturando imagen de prueba");
    } else {
      Serial.printf("Imagen de prueba: %d bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
  } else {
    Serial.printf("ERROR CRÍTICO: No se puede inicializar cámara: 0x%x\n", err);
    Serial.println("Incluso con configuración mínima falla");
  }
}

struct Rect {
    int x, y, width, height;
};

void draw_rectangle_manual(uint8_t* buf, int w, int h, Rect rect, uint16_t color) {
    uint16_t* p_buf = (uint16_t*)buf;

    int x1 = std::max(0, rect.x);
    int y1 = std::max(0, rect.y);
    int x2 = std::min(w - 1, rect.x + rect.width);
    int y2 = std::min(h - 1, rect.y + rect.height);

    for (int x = x1; x <= x2; x++) {
        p_buf[y1 * w + x] = color;
        p_buf[y2 * w + x] = color;
    }

    for (int y = y1; y <= y2; y++) {
        p_buf[y * w + x1] = color;
        p_buf[y * w + x2] = color;
    }
}

void handleDetect() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) {
    server.send(500, "application/json", "{\"error\":\"Sensor not available\"}");
    return;
  }
  
  size_t freeBefore = esp_get_free_heap_size();
  Serial.printf("🔍 Iniciando detección - Heap: %u bytes\n", freeBefore);
  
  s->set_pixformat(s, PIXFORMAT_RGB565);
  delay(100);
  
  camera_fb_t *fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("❌ Error: No se pudo capturar frame para detección");
    s->set_pixformat(s, PIXFORMAT_JPEG);
    server.send(500, "application/json", "{\"error\":\"Camera capture failed\"}");
    return;
  }
  
  Serial.printf("📸 Frame RGB565 capturado: %dx%d, %u bytes\n", fb->width, fb->height, fb->len);
  
  uint16_t* rgb565_frame = (uint16_t*)fb->buf;
  DetectionResult result = detect_colored_object(rgb565_frame, fb->width, fb->height, Colors::RED);
  
  String json = "{";
  json += "\"found\":" + String(result.found ? "true" : "false") + ",";
  
  if (result.found) {
    float distance = calculate_distance(result.width, OBJETO_ANCHO_REAL_CM, FOCAL_LENGTH);
    
    json += "\"x\":" + String(result.x_center) + ",";
    json += "\"y\":" + String(result.y_center) + ",";
    json += "\"width\":" + String(result.width) + ",";
    json += "\"height\":" + String(result.height) + ",";
    json += "\"pixels\":" + String(result.pixel_count) + ",";
    json += "\"distance_cm\":" + String(distance, 2);
    
    Serial.printf("✅ Objeto detectado - Distancia: %.2f cm\n", distance);
  }
  
  json += "}";
  
  esp_camera_fb_return(fb);
  
  s->set_pixformat(s, PIXFORMAT_JPEG);
  
  size_t freeAfter = esp_get_free_heap_size();
  Serial.printf("   Heap después: %u bytes\n\n", freeAfter);
  
  server.send(200, "application/json", json);
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 CAM</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #000;
            color: #0f0;
            font-family: 'Courier New', monospace;
            overflow: hidden;
        }
        .info {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            background: rgba(0, 0, 0, 0.8);
            padding: 10px 20px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 20px;
            z-index: 10;
            border-bottom: 2px solid #0f0;
        }
        .distance {
            font-size: 32px;
            font-weight: bold;
            color: #0f0;
        }
        .status {
            font-size: 16px;
            color: #888;
        }
        .found { color: #0f0; }
        .not-found { color: #f00; }
        #stream {
            position: fixed;
            top: 62px;
            left: 0;
            width: 100vw;
            height: calc(100vh - 62px);
            object-fit: contain;
            background: #000;
        }
    </style>
</head>
<body>
    <div class="info">
        <div>
            <span class="distance" id="distance">--- cm</span>
            <span class="status" id="status">●</span>
        </div>
        <div style="font-size: 14px; color: #666;">
            <span id="pos">x:- y:-</span> | 
            <span id="size">-×-</span>
        </div>
    </div>
    
    <img id="stream" />

    <script>
        const stream = document.getElementById('stream');
        const distanceEl = document.getElementById('distance');
        const statusEl = document.getElementById('status');
        const posEl = document.getElementById('pos');
        const sizeEl = document.getElementById('size');
        
        // Iniciar stream
        stream.src = '/stream?t=' + Date.now();
        
        // Actualizar detección cada 500ms
        setInterval(() => {
            fetch('/detect')
                .then(r => r.json())
                .then(d => {
                    if (d.found) {
                        distanceEl.textContent = d.distance_cm.toFixed(1) + ' cm';
                        statusEl.textContent = '●';
                        statusEl.className = 'status found';
                        posEl.textContent = `x:${d.x} y:${d.y}`;
                        sizeEl.textContent = `${d.width}×${d.height}`;
                    } else {
                        distanceEl.textContent = '--- cm';
                        statusEl.textContent = '●';
                        statusEl.className = 'status not-found';
                        posEl.textContent = 'x:- y:-';
                        sizeEl.textContent = '-×-';
                    }
                })
                .catch(e => console.log(e));
        }, 500);
        
        // Reiniciar stream si hay error
        stream.onerror = () => {
            setTimeout(() => stream.src = '/stream?t=' + Date.now(), 1000);
        };
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleStream() {
  WiFiClient client = server.client();
  
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  Serial.println("\n🎥 Cliente conectado al stream");
  Serial.printf("   Heap disponible: %u bytes\n", esp_get_free_heap_size());
  
  int frameCount = 0;
  int failCount = 0;
  size_t minHeapDuringStream = esp_get_free_heap_size();
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    
    if (!fb) {
      failCount++;
      if (failCount % 10 == 1) {
        Serial.printf("❌ Camera capture failed (frame #%d)\n", frameCount);
        Serial.printf("   Heap: %u bytes\n", esp_get_free_heap_size());
      }
      
      // Reintentar después de muchos fallos
      if (failCount > 50) {
        Serial.println("🔄 Demasiados fallos - reinicializando cámara...");
        esp_camera_deinit();
        delay(200);
        setupCamera();
        failCount = 0;
      }
      
      delay(20);
      continue;
    }
    
    failCount = 0;
    
    String header = "--frame\r\n";
    header += "Content-Type: image/jpeg\r\n";
    header += "Content-Length: " + String(fb->len) + "\r\n\r\n";
    
    server.sendContent(header);
    server.sendContent_P((char*)fb->buf, fb->len);
    server.sendContent("\r\n");
    
    size_t currentHeap = esp_get_free_heap_size();
    if (currentHeap < minHeapDuringStream) {
      minHeapDuringStream = currentHeap;
    }
    
    if (frameCount % 100 == 0) {
      Serial.printf("📊 Frame #%d - JPEG: %u bytes - Heap: %u bytes (min: %u)\n", 
                    frameCount, fb->len, currentHeap, minHeapDuringStream);
      
      if (minHeapDuringStream < 30000) {
        Serial.println("⚠️  Advertencia: Heap bajo!");
      }
    }
    
    esp_camera_fb_return(fb);
    frameCount++;
    delay(100);
  }
  
  Serial.printf("\n🔌 Cliente desconectado - %d frames\n", frameCount);
  Serial.printf("   Heap mínimo: %u bytes (%.1f KB)\n", 
                minHeapDuringStream, minHeapDuringStream/1024.0);
  Serial.printf("   Heap actual: %u bytes\n\n", esp_get_free_heap_size());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=================================");
  Serial.println("ESP32 Camera - Detección de Objetos");
  Serial.println("=================================\n");

  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ ¡Conectado a WiFi exitosamente!");
    Serial.println("\n--- Información de Conexión ---");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("Intensidad de señal (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.println("-------------------------------\n");
  } else {
    Serial.println("❌ ERROR: No se pudo conectar a WiFi");
    Serial.println("Verifica:");
    Serial.println("  1. Que el SSID y password sean correctos");
    Serial.println("  2. Que el router esté encendido y en rango");
    Serial.println("  3. Que el WiFi sea de 2.4GHz (ESP32 no soporta 5GHz)");
    Serial.println("\nReiniciando en 5 segundos...");
    delay(5000);
    ESP.restart();
  }

  setupCamera();

  server.on("/", handleRoot);
  server.on("/detect", handleDetect);
  server.on("/stream", handleStream);
  server.begin();
  
  Serial.println("\n🚀 Servidor web iniciado en puerto 80");
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  Accede desde tu navegador a:          ║");
  Serial.print("║  http://");
  Serial.print(WiFi.localIP());
  Serial.println("                 ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("\nEndpoints disponibles:");
  Serial.print("  • Interfaz web: http://");
  Serial.println(WiFi.localIP());
  Serial.print("  • Detección JSON: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/detect");
  Serial.print("  • Stream video: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
  Serial.println();
}

void loop() {
  static unsigned long lastMemCheck = 0;
  if (millis() - lastMemCheck > 5000) {
    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();
    size_t maxAllocHeap = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    
    Serial.println("\n╔══════════ ESTADO DE MEMORIA ══════════╗");
    Serial.printf("║ Heap Libre:        %6u bytes (%.1f KB) ║\n", freeHeap, freeHeap/1024.0);
    Serial.printf("║ Heap Mínimo:       %6u bytes (%.1f KB) ║\n", minFreeHeap, minFreeHeap/1024.0);
    Serial.printf("║ Bloque más grande: %6u bytes (%.1f KB) ║\n", maxAllocHeap, maxAllocHeap/1024.0);
    
    if (freeHeap < 20000) {
      Serial.println("║ ⚠️  ADVERTENCIA: Memoria baja!          ║");
    }
    if (freeHeap < 10000) {
      Serial.println("║ 🚨 CRÍTICO: Memoria muy baja!          ║");
    }
    
    Serial.println("╚════════════════════════════════════════╝\n");
    
    lastMemCheck = millis();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  WiFi desconectado. Reconectando...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Reconectado a WiFi");
      Serial.print("Nueva IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n❌ Fallo al reconectar. Reiniciando...");
      delay(3000);
      ESP.restart();
    }
  }
  
  server.handleClient();
  delay(10);
}
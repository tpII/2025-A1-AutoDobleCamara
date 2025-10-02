#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// Configuración WiFi
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Servidor web
WebServer server(80);

// Configuración de la cámara según tu tabla
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
  // Primer intento con configuración original
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
  config.pin_sccb_sda = 21; // SDA
  config.pin_sccb_scl = 22; // SCL
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.pixel_format = PIXFORMAT_JPEG;
  
  Serial.println("\n📸 Iniciando configuración de cámara...");
  Serial.printf("PSRAM encontrada: %s\n", psramFound() ? "SI" : "NO");
  Serial.println("🔧 Configuración: OPTIMIZADA para alta calidad");
  
  // Configuración mejorada - priorizar fluidez máxima
  config.frame_size = FRAMESIZE_QVGA;     // 320x240 - Mejor calidad
  config.jpeg_quality = 8;                // Calidad excelente (menor compresión)
  config.fb_count = 2;                    // Doble buffer para fluidez
  config.fb_location = CAMERA_FB_IN_DRAM; // Forzar DRAM para velocidad
  config.grab_mode = CAMERA_GRAB_LATEST;  // Siempre el frame más reciente
  config.xclk_freq_hz = 0;                // Sin clock externo

  esp_err_t err = esp_camera_init(&config);
  
  if (err == ESP_OK) {
    Serial.println("✅ ¡Cámara inicializada correctamente!");
    Serial.printf("📸 Configuración: QVGA (320x240), Calidad: %d, Buffers: %d\n", 
                  config.jpeg_quality, config.fb_count);
    
    // Configuración avanzada del sensor para máxima calidad
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
      s->set_brightness(s, 0);     // 0 = neutro
      s->set_contrast(s, 1);       // +1 para mejor definición
      s->set_saturation(s, 1);     // +1 para colores más vivos
      s->set_special_effect(s, 0); // Sin efectos
      s->set_whitebal(s, 1);       // Balance blancos automático
      s->set_awb_gain(s, 1);       // Ganancia AWB activa
      s->set_wb_mode(s, 0);        // Modo automático
      s->set_exposure_ctrl(s, 1);  // Exposición automática
      s->set_aec2(s, 1);           // Algoritmo AEC mejorado
      s->set_gain_ctrl(s, 1);      // Ganancia automática
      s->set_agc_gain(s, 0);       // Ganancia base
      s->set_bpc(s, 1);            // Corrección píxeles defectuosos
      s->set_wpc(s, 1);            // Corrección píxeles blancos
      s->set_raw_gma(s, 1);        // Gamma activado
      s->set_lenc(s, 1);           // Corrección de lente
      s->set_hmirror(s, 0);        // Sin espejo horizontal
      s->set_vflip(s, 0);          // Sin volteo vertical
      s->set_dcw(s, 1);            // Windowing activado
      s->set_colorbar(s, 0);       // Sin barras de test
      Serial.println("⚙️ Configuración avanzada del sensor aplicada");
    }
    
    // Test de captura
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("❌ Error capturando imagen de prueba");
    } else {
      Serial.printf("✅ Imagen de prueba: %d bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
  } else {
    Serial.printf("❌ ERROR CRÍTICO: No se puede inicializar cámara: 0x%x\n", err);
    Serial.println("🔴 Incluso con configuración mínima falla");
    Serial.println("🔴 Verificar conexiones físicas de nuevo");
  }
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Stream</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            background: #000;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            font-family: Arial, sans-serif;
        }
        .stream-container {
            border: 2px solid #333;
            border-radius: 8px;
            overflow: hidden;
            background: #111;
        }
        #stream {
            display: block;
            max-width: 100vw;
            max-height: 100vh;
            image-rendering: -webkit-optimize-contrast;
            image-rendering: pixelated;
        }
        .loading {
            color: #fff;
            text-align: center;
            padding: 50px;
            font-size: 18px;
        }
    </style>
</head>
<body>
    <div class="stream-container">
        <div class="loading" id="loading">Conectando...</div>
        <img id="stream" style="display:none;" />
    </div>

    <script>
        let streamImg = document.getElementById('stream');
        let loading = document.getElementById('loading');
        
        // Auto-iniciar stream inmediatamente
        streamImg.src = '/stream?t=' + new Date().getTime();
        
        streamImg.onload = function() {
            loading.style.display = 'none';
            streamImg.style.display = 'block';
        };
        
        streamImg.onerror = function() {
            loading.textContent = 'Error - Reintentando...';
            setTimeout(() => {
                streamImg.src = '/stream?t=' + new Date().getTime();
            }, 1000);
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
  
  Serial.println("Cliente conectado al stream");
  int frameCount = 0;
  int failCount = 0;
  
  // Buffer para mantener el último frame válido
  static uint8_t* lastFrame = nullptr;
  static size_t lastFrameSize = 0;
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    
    if (!fb) {
      failCount++;
      if (failCount % 10 == 1) {  // Solo log cada 10 fallos
        Serial.printf("❌ Camera capture failed (frame #%d) - manteniendo stream\n", frameCount);
      }
      
      // Si tenemos un frame previo, lo enviamos para mantener el stream activo
      if (lastFrame && lastFrameSize > 0) {
        String header = "--frame\r\n";
        header += "Content-Type: image/jpeg\r\n";
        header += "Content-Length: " + String(lastFrameSize) + "\r\n\r\n";
        
        server.sendContent(header);
        server.sendContent_P((char*)lastFrame, lastFrameSize);
        server.sendContent("\r\n");
        
        frameCount++;
        delay(50);  // Delay más largo para frames repetidos
        continue;
      }
      
      // Si llevamos muchos fallos, intentar reinicializar
      if (failCount > 50) {
        Serial.println("🔄 Demasiados fallos - reinicializando cámara...");
        esp_camera_deinit();
        delay(200);
        setupCamera();
        failCount = 0;
      }
      
      delay(20);  // Delay corto para reintento rápido
      continue;
    }
    
    // Frame capturado exitosamente
    failCount = 0;  // Reset contador de fallos
    
    // Guardar copia del frame para emergencias futuras
    if (fb->len > 0 && fb->len < 50000) {  // Sanity check del tamaño
      if (lastFrame) free(lastFrame);
      lastFrame = (uint8_t*)malloc(fb->len);
      if (lastFrame) {
        memcpy(lastFrame, fb->buf, fb->len);
        lastFrameSize = fb->len;
      }
    }
    
    if (frameCount % 100 == 0) { // Log cada 100 frames
      Serial.printf("📸 Frame #%d capturado: %d bytes\n", frameCount, fb->len);
    }
    
    String header = "--frame\r\n";
    header += "Content-Type: image/jpeg\r\n";
    header += "Content-Length: " + String(fb->len) + "\r\n\r\n";
    
    server.sendContent(header);
    server.sendContent_P((char*)fb->buf, fb->len);
    server.sendContent("\r\n");
    
    esp_camera_fb_return(fb);
    frameCount++;
    delay(33); // ~30 FPS para mejor calidad con QVGA
  }
  
  Serial.printf("Cliente desconectado después de %d frames\n", frameCount);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Camera Stream iniciando...");

  // Configurar WiFi AP
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP iniciado");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Inicializar cámara
  setupCamera();

  // Configurar servidor web
  server.on("/", handleRoot);              // Página principal
  server.on("/stream", handleStream);      // Stream de video
  server.begin();
  Serial.println("Servidor web iniciado en puerto 80");
  Serial.println("Conecta tu celular al WiFi 'ESP32_AP' y ve a:");
  Serial.println("- Página principal: http://192.168.4.1");
  Serial.println("- Stream directo: http://192.168.4.1/stream");
}

void loop() {
  server.handleClient();
  delay(10);
}
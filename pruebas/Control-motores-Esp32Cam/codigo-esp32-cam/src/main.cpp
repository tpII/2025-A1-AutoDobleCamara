#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <SPIFFS.h>  // Usar SPIFFS en lugar de headers
#include <webpage.h>

// Servidor TCP para recibir conexiones del ESP32-auto
WiFiServer tcpServer(12345);
WiFiClient tcpClientFromAuto;

void setupCamera() {

  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  // Pines para módulo AI-Thinker (ESP32-CAM)
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;       // XCLK en GPIO0
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26; // SDA (SIOC/SIOD)
  config.pin_sccb_scl = 27; // SCL
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.pixel_format = PIXFORMAT_JPEG;
  
  Serial.println("\n Iniciando configuración de cámara...");
  Serial.printf("PSRAM encontrada: %s\n", psramFound() ? "SI" : "NO");
  
  // Ajustes basados en PSRAM
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;      // mayor resolución cuando hay PSRAM
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 2;
    config.jpeg_quality = 10;                // buena calidad, no excesiva
  } else {
    config.frame_size = FRAMESIZE_QVGA;      // resolución segura sin PSRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.fb_count = 1;
    config.jpeg_quality = 12;                // calidad moderada para reducir memoria
  }

  config.grab_mode = CAMERA_GRAB_LATEST;
  config.xclk_freq_hz = 20000000; // 20 MHz, valor típico para ESP32-CAM

  esp_err_t err = esp_camera_init(&config);
  
  if (err == ESP_OK) {
    Serial.println(" ¡Cámara inicializada correctamente!");
    Serial.printf(" Config: %d (jpeg q=%d, fb_count=%d, fb_loc=%s)\n", 
                  config.frame_size, config.jpeg_quality, config.fb_count,
                  (config.fb_location==CAMERA_FB_IN_PSRAM) ? "PSRAM" : "DRAM");
    
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
      Serial.println(" Configuración avanzada del sensor aplicada");
    }
    
    // Test de captura
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println(" Error capturando imagen de prueba");
    } else {
      Serial.printf(" Imagen de prueba: %d bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
  } else {
    Serial.printf(" ERROR CRÍTICO: No se puede inicializar cámara: 0x%x\n", err);
    Serial.println(" Incluso con configuración mínima falla");
    Serial.println(" Verificar conexiones físicas de nuevo");
  }
}

// Función para envío de comandos por TCP al ESP32-auto
static bool sendCommandToRobot(const String &cmd_in, IPAddress overrideIP = IPAddress(0,0,0,0), uint16_t overridePort = 0) {
    String cmd = cmd_in;
    if (!cmd.endsWith("\n")) cmd += "\n";
    
    // Si hay cliente TCP conectado, enviar por ahí
    if (tcpClientFromAuto && tcpClientFromAuto.connected()) {
        tcpClientFromAuto.print(cmd);
        Serial.printf("Comando enviado al auto via TCP: %s", cmd.c_str());
        return true;
    }
    
    // Si no hay cliente TCP, intentar conexión directa (fallback)
    IPAddress dest = (overrideIP != IPAddress(0,0,0,0)) ? overrideIP : robotIP;
    uint16_t port = (overridePort != 0) ? overridePort : robotPort;

    WiFiClient client;
    Serial.printf("Enviando comando TCP a %s:%u : %s", dest.toString().c_str(), port, cmd.c_str());
    
    if (!client.connect(dest, port)) {
        Serial.printf("TCP connect failed to %s:%u\n", dest.toString().c_str(), port);
        return false;
    }
    
    client.print(cmd);
    client.flush();
    
    // Leer respuesta si está disponible
    unsigned long timeout = millis() + 1000;
    while (client.connected() && millis() < timeout) {
        if (client.available()) {
            String response = client.readStringUntil('\n');
            response.trim();
            if (response.length() > 0) {
                Serial.println("Respuesta del robot: " + response);
                break;
            }
        }
        delay(10);
    }
    
    client.stop();
    Serial.println("TCP comando enviado exitosamente");
    return true;
}

void handleCmd() {
    Serial.printf("[HTTP] 🔧 /cmd request from %s\n", server.client().remoteIP().toString().c_str());
    Serial.printf("[HTTP] 📋 Full URI: %s\n", server.uri().c_str());
    Serial.printf("[HTTP] 📋 Arguments count: %d\n", server.args());
    
    // Imprimir todos los argumentos para debug
    for (int i = 0; i < server.args(); i++) {
        Serial.printf("[HTTP] 📋 Arg[%d]: %s = %s\n", i, server.argName(i).c_str(), server.arg(i).c_str());
    }
    
    if (!server.hasArg("cmd")) {
        Serial.println("[HTTP] ❌ ERROR: missing cmd parameter");
        server.send(400, "text/plain", "ERR missing cmd");
        return;
    }
    
    String cmd = server.arg("cmd");
    String ip_str = server.arg("ip");
    String port_str = server.arg("port");
    
    Serial.printf("[HTTP] 🎯 Parsed - Command: '%s', IP: '%s', Port: '%s'\n", 
                  cmd.c_str(), 
                  ip_str.length() ? ip_str.c_str() : "(default)", 
                  port_str.length() ? port_str.c_str() : "(default)");
    
    IPAddress ip_override(0,0,0,0);
    uint16_t port_override = 0;
    
    if (ip_str.length()) {
        if (!ip_override.fromString(ip_str)) {
            Serial.printf("[HTTP] ❌ ERROR: invalid IP format: %s\n", ip_str.c_str());
            server.send(400, "text/plain", "ERR invalid ip");
            return;
        }
    }
    
    if (port_str.length()) {
        port_override = (uint16_t)port_str.toInt();
        if (port_override == 0) {
            Serial.printf("[HTTP] ❌ ERROR: invalid port: %s\n", port_str.c_str());
            server.send(400, "text/plain", "ERR invalid port");
            return;
        }
    }
    
    Serial.printf("[HTTP] 🚀 About to send command: '%s'\n", cmd.c_str());
    bool ok = sendCommandToRobot(cmd, ip_override, port_override);
    
    if (ok) {
        Serial.println("[HTTP] ✅ Command sent successfully");
        server.send(200, "text/plain", "OK - Comando enviado al robot");
    } else {
        Serial.println("[HTTP] ❌ ERROR: Failed to send command");
        server.send(500, "text/plain", "ERR send failed");
    }
}

void handleRoot() {
  if (SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
      Serial.println("[HTTP] ✅ Served index.html from SPIFFS");
      return;
    }
  }
  // Fallback al HTML embebido
  Serial.println("[HTTP] 🚨 Serving fallback HTML (SPIFFS not available)");
  server.send(200, "text/html", index_html);
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
        Serial.printf(" Camera capture failed (frame #%d) - manteniendo stream\n", frameCount);
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
        Serial.println(" Demasiados fallos - reinicializando cámara...");
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
    
    // if (frameCount % 100 == 0) { // Log cada 100 frames
    //   Serial.printf("📸 Frame #%d capturado: %d bytes\n", frameCount, fb->len);
    // }
    
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
  Serial.println("🚀 ESP32-CAM Control Web Server iniciando...");

  // Inicializar SPIFFS para archivos web separados
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ ERROR: Failed to mount SPIFFS - usando HTML embebido");
  } else {
    Serial.println("✅ SPIFFS montado correctamente");
    
    // Listar archivos disponibles
    File root = SPIFFS.open("/");
    if (root && root.isDirectory()) {
      File file = root.openNextFile();
      Serial.println("📁 Archivos en SPIFFS:");
      while (file) {
        Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
        file = root.openNextFile();
      }
    }
  }

  // Configurar WiFi AP
  WiFi.softAP(ssid, password);
  
  // Configurar IP del punto de acceso
  IPAddress local_IP(192, 168, 4, 1);  // IP estándar del AP
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  Serial.println("WiFi AP iniciado");
  Serial.print("IP del AP: ");
  Serial.println(WiFi.softAPIP());
  Serial.printf("Robot IP objetivo: %s:%u\n", robotIP.toString().c_str(), robotPort);

  // Inicializar servidor TCP para recibir conexiones del auto
  tcpServer.begin();
  Serial.println("Servidor TCP iniciado en puerto 12345");
  Serial.println("Esperando conexiones del ESP32-auto...");

  // Inicializar cámara
  setupCamera();

  // Configurar servidor web con archivos separados
  server.on("/cmd", HTTP_GET, handleCmd);
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.on("/style.css", handleCSS);           // CSS desde SPIFFS
  server.on("/robot-control.js", handleJS);    // JavaScript desde SPIFFS
  
  server.begin();
  
  Serial.println("🌐 Servidor web iniciado con archivos separados");
  Serial.println("📂 Archivos disponibles:");
  Serial.printf("- 🏠 HTML: http://%s/\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("- 🎨 CSS: http://%s/style.css\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("- ⚡ JS: http://%s/robot-control.js\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("- 📹 Stream: http://%s/stream\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("- 🔧 API: http://%s/cmd?cmd=PING\n", WiFi.softAPIP().toString().c_str());
}

void loop() {
  // Watchdog manual para detectar cuelgues
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 30000) { // cada 30 segundos
    lastHeartbeat = millis();
    Serial.printf("[HEARTBEAT] Free heap: %d bytes, WiFi clients: %d\n", 
                  ESP.getFreeHeap(), WiFi.softAPgetStationNum());
  }
  
  server.handleClient();
  
  // Manejar conexiones TCP entrantes del ESP32-auto
  if (!tcpClientFromAuto || !tcpClientFromAuto.connected()) {
    tcpClientFromAuto = tcpServer.available();
    if (tcpClientFromAuto) {
      Serial.println("ESP32-auto conectado desde: " + tcpClientFromAuto.remoteIP().toString());
      tcpClientFromAuto.println("ESP32-CAM ready");
    }
  }
  
  // Leer datos del auto si está conectado (aunque no esperamos comandos del auto)
  if (tcpClientFromAuto && tcpClientFromAuto.connected() && tcpClientFromAuto.available()) {
    String message = tcpClientFromAuto.readStringUntil('\n');
    message.trim();
    if (message.length() > 0) {
      Serial.println("Mensaje del auto: " + message);
    }
  }
  
  delay(10);
}
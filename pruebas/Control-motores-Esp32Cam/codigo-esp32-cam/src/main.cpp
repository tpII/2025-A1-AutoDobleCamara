#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// Configuración WiFi
const char* ssid = "ESP32_AP";
const char* password = "12345678";

// Servidor web
WebServer server(80);
IPAddress robotIP(192,168,4,2); // IP por defecto del otro ESP32 (ajustable desde la UI)
uint16_t robotPort = 12345;     // puerto por defecto (configurable)

// Configuración de la cámara según tu tabla
camera_config_t config;

void testCameraConnections() {
  Serial.println("\n🔍 === DIAGNÓSTICO DE CONEXIONES ===");
  
  // Verificar pines básicos (ESP32-CAM AI-Thinker)
  Serial.println("📋 Configuración de pines (AI-Thinker ESP32-CAM):");
  Serial.printf("- SIOD (SDA): GPIO %d\n", 26);
  Serial.printf("- SIOC (SCL): GPIO %d\n", 27);
  Serial.printf("- XCLK: GPIO %d\n", 0);
  Serial.printf("- PCLK: GPIO %d\n", 22);
  Serial.printf("- VSYNC: GPIO %d\n", 25);
  Serial.printf("- HREF: GPIO %d\n", 23);
  Serial.printf("- D0-D7: %d,%d,%d,%d,%d,%d,%d,%d\n", 5,18,19,21,36,39,34,35);
  Serial.printf("- PWDN: GPIO %d\n", 32);
  Serial.printf("- RESET: %s\n", "-1 (no conectado)");
}

void setupCamera() {
  // Primer intento con configuración original
  testCameraConnections();
  
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
  
  Serial.println("\n📸 Iniciando configuración de cámara...");
  Serial.printf("PSRAM encontrada: %s\n", psramFound() ? "SI" : "NO");
  Serial.println("🔧 Configuración: OPTIMIZADA para ESP32-CAM (AI-Thinker)");
  
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
    Serial.println("✅ ¡Cámara inicializada correctamente!");
    Serial.printf("📸 Config: %d (jpeg q=%d, fb_count=%d, fb_loc=%s)\n", 
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

// Nuevo: envío por TCP (no necesita variable global)
static bool sendCommandToRobot(const String &cmd_in, IPAddress overrideIP = IPAddress(0,0,0,0), uint16_t overridePort = 0) {
    String cmd = cmd_in;
    if (!cmd.endsWith("\n")) cmd += "\n";
    IPAddress dest = (overrideIP != IPAddress(0,0,0,0)) ? overrideIP : robotIP;
    uint16_t port = (overridePort != 0) ? overridePort : robotPort;

    WiFiClient client;
    // Timeout razonable para conectar (ej. 2000ms)
    if (!client.connect(dest, port)) {
        Serial.printf("TCP connect failed to %s:%u\n", dest.toString().c_str(), port);
        return false;
    }
    // Enviar datos completos
    size_t total = 0;
    const char *buf = cmd.c_str();
    size_t len = cmd.length();
    while (total < len) {
        int w = client.write((const uint8_t*)(buf + total), len - total);
        if (w <= 0) {
            Serial.printf("TCP write failed after %u bytes to %s:%u\n", total, dest.toString().c_str(), port);
            client.stop();
            return false;
        }
        total += (size_t)w;
    }
    client.flush();
    client.stop();
    Serial.printf("CMD TCP -> %s:%u : %s", dest.toString().c_str(), port, cmd.c_str());
    return true;
}

// Nuevo: handler HTTP para /cmd
void handleCmd() {
    // Parámetros: cmd (string), ip (opcional), port (opcional)
    if (!server.hasArg("cmd")) {
        server.send(400, "text/plain", "ERR missing cmd");
        return;
    }
    String cmd = server.arg("cmd");
    String ip_str = server.arg("ip");
    String port_str = server.arg("port");
    IPAddress ip_override(0,0,0,0);
    uint16_t port_override = 0;
    if (ip_str.length()) {
        if (!ip_override.fromString(ip_str)) {
            server.send(400, "text/plain", "ERR invalid ip");
            return;
        }
    }
    if (port_str.length()) {
        port_override = (uint16_t)port_str.toInt();
        if (port_override == 0) {
            server.send(400, "text/plain", "ERR invalid port");
            return;
        }
    }
    bool ok = sendCommandToRobot(cmd, ip_override, port_override);
    if (ok) server.send(200, "text/plain", "OK");
    else server.send(500, "text/plain", "ERR send failed");
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
        body { margin:0; padding:0; background:#000; color:#fff; font-family:Arial, sans-serif; }
        .page { display:flex; gap:12px; align-items:flex-start; padding:12px; }
        .stream-container { border:2px solid #333; border-radius:8px; overflow:hidden; background:#111; }
        #stream { display:block; max-width:60vw; max-height:80vh; image-rendering:-webkit-optimize-contrast; image-rendering:pixelated; }
        .controls { width:260px; background:#111; padding:10px; border-radius:8px; border:2px solid #333; }
        .btn { width:100%; padding:12px; margin:6px 0; font-size:18px; border-radius:6px; cursor:pointer; }
        .grid { display:grid; grid-template-columns:1fr 1fr; gap:6px; }
        input, label { width:100%; box-sizing:border-box; margin:6px 0; padding:8px; background:#000; color:#fff; border:1px solid #444; border-radius:4px; }
        .small { font-size:13px; color:#bbb; margin-bottom:6px; }
    </style>
</head>
<body>
  <div class="page">
    <div class="stream-container">
      <img id="stream" />
    </div>

    <div class="controls">
      <div class="small">Robot target (IP:port)</div>
      <input id="robot_ip" placeholder="192.168.4.2" />
      <input id="robot_port" placeholder="12345" />

      <div class="small">Control</div>
      <div class="grid">
        <button class="btn" id="btn_fwd" onclick="sendCmd('ALL F 200')">Adelante</button>
        <button class="btn" id="btn_back" onclick="sendCmd('ALL R 200')">Atras</button>
        <button class="btn" id="btn_left" onclick="sendCmd('MOTOR 1 F 180')">Izquierda</button>
        <button class="btn" id="btn_right" onclick="sendCmd('MOTOR 2 F 180')">Derecha</button>
      </div>
      <button class="btn" style="background:#b22222;color:#fff;" onclick="sendCmd('STOP')">STOP</button>

      <div class="small">Estado</div>
      <pre id="status" style="height:120px; overflow:auto; background:#000; border:1px solid #222; padding:6px;"></pre>
    </div>
  </div>

<script>
const img = document.getElementById('stream');
const status = document.getElementById('status');

function appendStatus(s){ status.textContent = new Date().toLocaleTimeString() + ' - ' + s + '\\n' + status.textContent; }

function getTarget(){
  const ip = document.getElementById('robot_ip').value.trim();
  const port = document.getElementById('robot_port').value.trim();
  return { ip: ip, port: port };
}

function sendCmd(cmd){
  const tgt = getTarget();
  let url = '/cmd?cmd=' + encodeURIComponent(cmd);
  if (tgt.ip) url += '&ip=' + encodeURIComponent(tgt.ip);
  if (tgt.port) url += '&port=' + encodeURIComponent(tgt.port);
  fetch(url).then(r => r.text()).then(t => {
    appendStatus('Sent: ' + cmd + ' -> ' + t);
  }).catch(e => {
    appendStatus('ERROR sending: ' + e);
  });
}

// Auto start stream
img.src = '/stream?t=' + Date.now();
img.onload = function(){ appendStatus('Stream OK'); };
img.onerror = function(){ appendStatus('Stream error, retrying...'); setTimeout(()=>img.src='/stream?t='+Date.now(),1000); };
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
  server.on("/cmd", HTTP_GET, handleCmd); // acepta argumentos: cmd, ip (opcional), port (opcional)
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
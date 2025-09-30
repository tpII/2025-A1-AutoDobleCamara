#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// Credenciales del Access Point
const char* ssid = "CamaraAP";
const char* password = "12345678";

// IP estática del AP
IPAddress local_ip(192,168,5,1);
IPAddress gateway(192,168,5,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// Configuración de pines para ESP32-CAM (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Prototipos
void handle_OnConnect();
void handle_NotFound();
void handle_capture();
void handle_stream();
bool initCamera();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando ESP32-CAM Access Point...");
  
  // Inicializar cámara
  if (!initCamera()) {
    Serial.println("ERROR: Failed to initialize camera");
    return;
  }
  
  // Desconectar de cualquier red WiFi existente
  WiFi.disconnect(true);
  delay(1000);
  
  // Configurar modo AP
  WiFi.mode(WIFI_AP);
  delay(1000);
  
  // Configurar IP estática para la interfaz softAP
  if(!WiFi.softAPConfig(local_ip, gateway, subnet)) {
    Serial.println("ERROR: softAPConfig failed");
    return;
  }
  
  // Iniciar Access Point con configuraciones específicas
  bool apStarted = WiFi.softAP(ssid, password, 1, 0, 4);
  
  if(apStarted) {
    Serial.println("=== ACCESS POINT INICIADO EXITOSAMENTE ===");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("IP del AP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.println("==========================================");
  } else {
    Serial.println("ERROR: Failed to start Access Point");
    return;
  }
  
  // Esperar un poco para que el AP se estabilice
  delay(2000);
  
  // Configurar rutas HTTP
  server.on("/", handle_OnConnect);
  server.on("/capture", handle_capture);
  server.on("/stream", handle_stream);
  server.onNotFound(handle_NotFound);
  
  // Iniciar servidor web
  server.begin();
  Serial.println("Servidor HTTP iniciado en puerto 80");
  Serial.println("Conecta tu dispositivo a la red 'CamaraAP' y ve a http://192.168.5.1");
  Serial.println("Stream disponible en: http://192.168.5.1/stream");
}

void loop() {
  server.handleClient();
  
  // Mostrar información cada 10 segundos
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    Serial.print("Clientes conectados: ");
    Serial.println(WiFi.softAPgetStationNum());
    lastPrint = millis();
  }
}

// Inicializar cámara
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Configuración de resolución
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Inicializar cámara
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  
  // Ajustar configuración del sensor
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 a 2
  s->set_contrast(s, 0);       // -2 a 2
  s->set_saturation(s, 0);     // -2 a 2
  s->set_special_effect(s, 0); // 0 a 6 (0=Normal, 1=Negative, 2=Grayscale, 3=Red Tint, 4=Green Tint, 5=Blue Tint, 6=Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 a 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 a 2
  s->set_aec_value(s, 300);    // 0 a 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 a 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 a 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  
  Serial.println("Cámara inicializada correctamente");
  return true;
}

// Handler para la página principal
void handle_OnConnect() {
  String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>ESP32-CAM Feed</title></head><body>";
  html += "<h1>ESP32-CAM Access Point</h1>";
  html += "<p>Conexión exitosa al Access Point.</p>";
  html += "<p>IP del servidor: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>Clientes conectados: " + String(WiFi.softAPgetStationNum()) + "</p>";
  html += "<h2>Feed de Video en Vivo</h2>";
  html += "<img src='/stream' style='width:100%; max-width:800px; height:auto;'>";
  html += "<br><br>";
  html += "<a href='/capture' target='_blank'><button style='padding:10px 20px; font-size:16px;'>Capturar Foto</button></a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
  Serial.println("Cliente accedió a la página principal");
}

// Handler para capturar una foto
void handle_capture() {
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  Serial.println("Foto capturada y enviada");
}

// Handler para stream de video
void handle_stream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);
  
  Serial.println("Iniciando stream de video...");
  
  while(client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if(!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    if(fb->len > 0) {
      String header = "--frame\r\n";
      header += "Content-Type: image/jpeg\r\n";
      header += "Content-Length: " + String(fb->len) + "\r\n\r\n";
      
      client.print(header);
      client.write(fb->buf, fb->len);
      client.print("\r\n");
    }
    
    esp_camera_fb_return(fb);
    
    if(!client.connected()) {
      break;
    }
  }
  
  Serial.println("Stream de video terminado");
}

// Handler para rutas no encontradas
void handle_NotFound() {
  String message = "Página no encontrada\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(404, "text/plain", message);
  Serial.println("404 - Página no encontrada: " + server.uri());
}
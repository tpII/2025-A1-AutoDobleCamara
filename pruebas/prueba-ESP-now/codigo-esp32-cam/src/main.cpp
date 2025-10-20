#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// Camera pins for ESP32-CAM
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


// MAC Address del ESP32 del auto (reemplazar con la direccion correcta)
// Valor observado en el dispositivo objetivo: E0:5A:1B:D2:40:74
uint8_t carAddress[] = {0xE0, 0x5A, 0x1B, 0xD2, 0x40, 0x74};
esp_now_peer_info_t peerInfo = {}; // Zero-initialize peerInfo
AsyncWebServer server(80);

// Diagnostics
unsigned long lastDiagMillis = 0;

// WiFi event handler to log AP/STA connect/disconnect and AP state
void onWiFiEvent(WiFiEvent_t event) {
    Serial.print("WiFi event: ");
    Serial.println((int)event);
    switch (event) {
        case SYSTEM_EVENT_AP_START:
            Serial.println("AP started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("AP stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.print("Station connected. Count: ");
            Serial.println(WiFi.softAPgetStationNum());
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.print("Station disconnected. Count: ");
            Serial.println(WiFi.softAPgetStationNum());
            break;
        default:
            break;
    }
}

// Callback cuando se envía un mensaje por ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("Enviado con éxito");
    } else {
        Serial.println("Error en el envío");
    }
}

// Inicializar la cámara
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // Configuración para streaming
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Error al inicializar la cámara: 0x%x", err);
        return false;
    }
    return true;
}

// Función para enviar comandos al auto
void sendCommand(const char* command) {
    esp_err_t result = esp_now_send(carAddress, (uint8_t*)command, strlen(command));
    if (result != ESP_OK) {
        Serial.println("Error enviando comando");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Inicializar SPIFFS
    if(!SPIFFS.begin(true)){
        Serial.println("Error al montar SPIFFS");
        return;
    }

    // Configurar WiFi en modo AP
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ESP32-CAM-Control", "123456789");

    // Registrar handler para eventos WiFi (con fines de diagnóstico)
    WiFi.onEvent(onWiFiEvent);

    // Imprimir datos iniciales de diagnóstico
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("SoftAP MAC: ");
    Serial.println(WiFi.softAPmacAddress());
    Serial.print("Local (STA) IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());

    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    // Registrar callback
    esp_now_register_send_cb(OnDataSent);

    // Registrar peer
    memcpy(peerInfo.peer_addr, carAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error agregando peer");
        return;
    }

    // Inicializar cámara
    if (!initCamera()) {
        Serial.println("Error al inicializar la cámara");
        return;
    }

    // Rutas del servidor web
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
    });

    server.on("/robot-control.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/robot-control.js", "application/javascript");
    });

    server.on("/control", HTTP_POST, [](AsyncWebServerRequest *request){
        String command;
        if(request->hasParam("cmd", true)){
            command = request->getParam("cmd", true)->value();
            sendCommand(command.c_str());
            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Comando no especificado");
        }
    });

    server.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request){
        camera_fb_t * fb = esp_camera_fb_get();
        if(!fb) {
            request->send(500, "text/plain", "Error de cámara");
            return;
        }
        
        AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg",
                                                                 fb->buf, fb->len);
        response->addHeader("Content-Disposition", "inline; filename=capture.jpg");
        request->send(response);
        
        esp_camera_fb_return(fb);
    });

    server.begin();
}

void loop() {
    // Diagnostics: print status every 5 seconds
    if (millis() - lastDiagMillis > 5000) {
        lastDiagMillis = millis();
        Serial.print("Uptime ms: "); Serial.println(millis());
        Serial.print("Free heap: "); Serial.println(ESP.getFreeHeap());
        Serial.print("SoftAP IP: "); Serial.println(WiFi.softAPIP());
        Serial.print("SoftAP MAC: "); Serial.println(WiFi.softAPmacAddress());
        Serial.print("Local (STA) IP: "); Serial.println(WiFi.localIP());
        Serial.print("Stations connected: "); Serial.println(WiFi.softAPgetStationNum());
    }

    // El manejo de la cámara y los comandos se hace a través de las callbacks
    delay(10);
}

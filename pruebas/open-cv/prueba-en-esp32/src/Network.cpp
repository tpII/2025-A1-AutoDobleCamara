#include "Network.h"
#include "config.h"
#include "CameraVision.h"
#include "esp_camera.h"
#include "img_converters.h"  // Para frame2jpg
#include <ESPmDNS.h>          // Para mDNS en modo cliente

// Puntero externo al sistema de visión (se inicializa en main.cpp)
CameraVision* g_cameraVision = nullptr;

/**
 * @brief Inicializa el sistema de red completo
 * @param onDataRecv Callback para procesar datos recibidos por ESP-NOW
 */
void NetworkManager::setup(void (*onDataRecv)(const uint8_t *, int)) {
    // Configurar WiFi según el modo seleccionado
#ifdef ACCESS_POINT_MODE
    setupWiFiAP();
    IPAddress localIP = WiFi.softAPIP();
#endif

#ifdef CONNECT_TO_NETWORK
    setupWiFiClient();
    IPAddress localIP = WiFi.localIP();
#endif

#ifndef MODO_PRUEBA_SIN_COMPANERO
    // Configurar ESP-NOW solo si no estamos en modo prueba
    setupESPNOW(onDataRecv);
#else
    DEBUG_PRINTLN("[NETWORK] Modo prueba activado - ESP-NOW deshabilitado");
#endif

    // Inicializar servidor web
    server = new WiFiServer(WEB_SERVER_PORT);
    server->begin();
    
    DEBUG_PRINTLN("[NETWORK] Servidor web iniciado en puerto " + String(WEB_SERVER_PORT));
    DEBUG_PRINTLN("[NETWORK] Stream disponible en: http://" + localIP.toString() + STREAM_PATH);
    
#ifdef MODO_PRUEBA_SIN_COMPANERO
    DEBUG_PRINTLN("[NETWORK] Control de prueba en: http://" + localIP.toString() + TEST_CONTROL_PATH);
#endif
}

/**
 * @brief Configura el WiFi en modo Access Point
 */
void NetworkManager::setupWiFiAP() {
#ifdef ACCESS_POINT_MODE
    DEBUG_PRINTLN("[NETWORK] Configurando WiFi AP...");
    
    WiFi.mode(WIFI_AP);
    
    bool result = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);
    
    if (result) {
        IPAddress IP = WiFi.softAPIP();
        DEBUG_PRINTLN("[NETWORK] AP configurado exitosamente");
        DEBUG_PRINTLN("[NETWORK] SSID: " + String(WIFI_AP_SSID));
        DEBUG_PRINTLN("[NETWORK] IP: " + IP.toString());
        DEBUG_PRINTLN("[NETWORK] Canal: " + String(WIFI_AP_CHANNEL));
    } else {
        DEBUG_PRINTLN("[NETWORK] Error al configurar AP");
    }
#endif
}

/**
 * @brief Configura el WiFi en modo Cliente (conecta a red existente)
 */
void NetworkManager::setupWiFiClient() {
#ifdef CONNECT_TO_NETWORK
    DEBUG_PRINTLN("[NETWORK] Configurando WiFi Client...");
    DEBUG_PRINTLN("[NETWORK] Conectando a: " + String(WIFI_STA_SSID));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        DEBUG_PRINT(".");
    }
    DEBUG_PRINTLN("");
    
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("[NETWORK] WiFi conectado exitosamente");
        DEBUG_PRINTLN("[NETWORK] IP: " + WiFi.localIP().toString());
        DEBUG_PRINTLN("[NETWORK] Gateway: " + WiFi.gatewayIP().toString());
        DEBUG_PRINTLN("[NETWORK] DNS: " + WiFi.dnsIP().toString());
        DEBUG_PRINTLN("[NETWORK] RSSI: " + String(WiFi.RSSI()) + " dBm");
        
        // Configurar mDNS para acceso fácil por nombre
        if (MDNS.begin("autito-robot")) {
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
            DEBUG_PRINTLN("[NETWORK] mDNS iniciado: http://autito-robot.local");
        } else {
            DEBUG_PRINTLN("[NETWORK] Error al iniciar mDNS");
        }
    } else {
        DEBUG_PRINTLN("[NETWORK] Error al conectar a WiFi");
        DEBUG_PRINTLN("[NETWORK] Reiniciando en modo AP de respaldo...");
        
        // Fallback a modo AP si no se puede conectar
        WiFi.mode(WIFI_AP);
        WiFi.softAP("AUTITO_FALLBACK", "12345678");
        DEBUG_PRINTLN("[NETWORK] AP de respaldo iniciado");
        DEBUG_PRINTLN("[NETWORK] IP: " + WiFi.softAPIP().toString());
    }
#endif
}

#ifndef MODO_PRUEBA_SIN_COMPANERO
/**
 * @brief Configura ESP-NOW para recibir comandos de la cámara fija
 * @param onDataRecv Callback para procesar datos recibidos
 */
void NetworkManager::setupESPNOW(void (*onDataRecv)(const uint8_t *, int)) {
    DEBUG_PRINTLN("[NETWORK] Inicializando ESP-NOW...");
    
    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        DEBUG_PRINTLN("[NETWORK] Error al inicializar ESP-NOW");
        return;
    }
    
    DEBUG_PRINTLN("[NETWORK] ESP-NOW inicializado correctamente");
    
    // Registrar callback para recepción de datos
    esp_now_register_recv_cb(onDataRecv);
    
    // Imprimir dirección MAC para que el compañero la registre
    uint8_t mac[6];
    WiFi.macAddress(mac);
    DEBUG_PRINTF("[NETWORK] MAC del Autito: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
#endif

/**
 * @brief Procesa las conexiones de clientes web
 */
void NetworkManager::handleClient() {
    WiFiClient client = server->available();
    
    if (!client) {
        return;
    }
    
    DEBUG_PRINTLN("[NETWORK] Nuevo cliente conectado");
    
    // Esperar a que haya datos disponibles
    unsigned long timeout = millis() + 5000;
    while (!client.available() && millis() < timeout) {
        delay(1);
    }
    
    if (!client.available()) {
        client.stop();
        return;
    }
    
    // Leer la primera línea del request HTTP
    String request = client.readStringUntil('\r');
    client.flush();
    
    DEBUG_PRINTLN("[NETWORK] Request: " + request);
    
    // Determinar qué servir según el path
    if (request.indexOf(STREAM_PATH) != -1) {
        serveVideoStream(client);
    }
#ifdef MODO_PRUEBA_SIN_COMPANERO
    else if (request.indexOf(TEST_CONTROL_PATH) != -1) {
        serveTestControl(client, request);
    }
#endif
    else {
        serveIndexPage(client);
    }
    
    client.stop();
    DEBUG_PRINTLN("[NETWORK] Cliente desconectado");
}

/**
 * @brief Sirve la página principal con enlaces
 */
void NetworkManager::serveIndexPage(WiFiClient& client) {
    // Enviar headers HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    
    // Página HTML
    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<title>Autito Robot - Control</title>");
    client.println("<style>");
    client.println("body { font-family: Arial; text-align: center; background: #2c3e50; color: white; }");
    client.println("h1 { color: #3498db; }");
    client.println("a { display: inline-block; margin: 20px; padding: 15px 30px; ");
    client.println("    background: #3498db; color: white; text-decoration: none; ");
    client.println("    border-radius: 5px; font-size: 18px; }");
    client.println("a:hover { background: #2980b9; }");
    client.println("img { max-width: 90%; border: 3px solid #3498db; margin-top: 20px; }");
    client.println("</style></head><body>");
    client.println("<h1>Autito Robot - Sistema de Vision</h1>");
    client.println("<p>Firmware ESP32 con Deteccion de Color y Sistema de Veto Distribuido</p>");
    
    // Enlace al stream
    client.println("<a href='" + String(STREAM_PATH) + "' target='_blank'>Ver Stream de Video</a>");
    
#ifdef MODO_PRUEBA_SIN_COMPANERO
    client.println("<h2>Modo Prueba Activado</h2>");
    client.println("<p>Controla el autito sin la camara fija:</p>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=1&riesgo=0'>Avanzar</a>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=2&riesgo=0'>Atras</a><br>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=3&riesgo=0'>Izquierda</a>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=4&riesgo=0'>Derecha</a><br>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=0&riesgo=0'>Detener</a>");
    client.println("<br><br>");
    client.println("<a href='" + String(TEST_CONTROL_PATH) + "?cmd=1&riesgo=1' style='background:#e74c3c;'>Simular Veto Global</a>");
#endif
    
    // Mostrar el stream en línea
    client.println("<h2>Vista de la Cámara del Autito:</h2>");
    client.println("<img src='" + String(STREAM_PATH) + "' />");
    
    client.println("</body></html>");
}

/**
 * @brief Sirve el stream MJPEG de video
 * Captura directamente de la cámara y envía frames JPEG
 */
void NetworkManager::serveVideoStream(WiFiClient& client) {
    DEBUG_PRINTLN("[NETWORK] Iniciando stream de video");
    
    // Verificar si la cámara está disponible
    // Si no está inicializada, intentar inicializarla ahora
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (!test_fb) {
        DEBUG_PRINTLN("[NETWORK] Cámara no disponible, intentando reinicializar...");
        
        // Enviar respuesta de error temporal
        client.println("HTTP/1.1 503 Service Unavailable");
        client.println("Content-Type: text/html");
        client.println("Refresh: 5");  // Auto-refresh cada 5 segundos
        client.println("Connection: close");
        client.println();
        client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
        client.println("<title>Camara no disponible</title>");
        client.println("<style>body{font-family:Arial;text-align:center;background:#2c3e50;color:white;padding:50px;}");
        client.println("h1{color:#e74c3c;}</style></head><body>");
        client.println("<h1>Camara no disponible</h1>");
        client.println("<p>La camara no pudo inicializarse al arranque.</p>");
        client.println("<p>Esta pagina se recargara automaticamente cada 5 segundos.</p>");
        client.println("<p>Si el problema persiste, presiona el boton RESET del ESP32.</p>");
        client.println("</body></html>");
        return;
    }
    
    // La cámara está funcionando, liberar el frame de prueba
    esp_camera_fb_return(test_fb);
    
    // Headers para MJPEG stream
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Connection: close");
    client.println();
    
    // Stream continuo de frames
    while (client.connected()) {
        // Capturar frame directamente de la cámara
        camera_fb_t* fb = esp_camera_fb_get();
        
        if (!fb) {
            DEBUG_PRINTLN("[NETWORK] Error capturando frame");
            delay(100);
            continue;
        }
        
        // Si el formato no es JPEG, convertir a JPEG
        uint8_t* jpg_buf = nullptr;
        size_t jpg_buf_len = 0;
        bool need_free = false;
        
        if (fb->format == PIXFORMAT_JPEG) {
            // Ya está en JPEG, usar directamente
            jpg_buf = fb->buf;
            jpg_buf_len = fb->len;
        } else {
            // Convertir RGB565 a JPEG
            need_free = frame2jpg(fb, 80, &jpg_buf, &jpg_buf_len);
            if (!need_free || jpg_buf_len == 0) {
                DEBUG_PRINTLN("[NETWORK] Error convirtiendo a JPEG");
                esp_camera_fb_return(fb);
                delay(100);
                continue;
            }
        }
        
        // Enviar frame JPEG al cliente
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(jpg_buf_len));
        client.println();
        client.write(jpg_buf, jpg_buf_len);
        client.println();
        
        // Liberar memoria
        if (need_free && jpg_buf != nullptr) {
            free(jpg_buf);
        }
        esp_camera_fb_return(fb);
        
        // Pequeño delay para controlar FPS
        delay(50);  // ~20 FPS
    }
    
    DEBUG_PRINTLN("[NETWORK] Stream finalizado");
}

#ifdef MODO_PRUEBA_SIN_COMPANERO
/**
 * @brief Endpoint de prueba para simular comandos ESP-NOW
 * Permite controlar el autito mediante HTTP GET sin necesidad de la cámara fija
 */
void NetworkManager::serveTestControl(WiFiClient& client, String& request) {
    // Parsear parámetros GET
    int cmd = CMD_DETENER;
    bool riesgo = false;
    
    // Extraer parámetro 'cmd'
    int cmdIndex = request.indexOf("cmd=");
    if (cmdIndex != -1) {
        String cmdStr = request.substring(cmdIndex + 4);
        int endIndex = cmdStr.indexOf('&');
        if (endIndex == -1) endIndex = cmdStr.indexOf(' ');
        if (endIndex != -1) cmdStr = cmdStr.substring(0, endIndex);
        cmd = cmdStr.toInt();
    }
    
    // Extraer parámetro 'riesgo'
    int riesgoIndex = request.indexOf("riesgo=");
    if (riesgoIndex != -1) {
        String riesgoStr = request.substring(riesgoIndex + 7);
        int endIndex = riesgoStr.indexOf('&');
        if (endIndex == -1) endIndex = riesgoStr.indexOf(' ');
        if (endIndex != -1) riesgoStr = riesgoStr.substring(0, endIndex);
        riesgo = (riesgoStr.toInt() != 0);
    }
    
    // Actualizar variables globales (definidas en main.cpp)
    extern volatile int g_comandoUsuario;
    extern volatile bool g_hayRiesgoGlobal;
    
    g_comandoUsuario = cmd;
    g_hayRiesgoGlobal = riesgo;
    
    DEBUG_PRINTF("[NETWORK] Test Control - CMD: %d, Riesgo: %d\n", cmd, riesgo);
    
    // Responder con JSON
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println("Connection: close");
    client.println();
    
    client.print("{\"status\":\"ok\",\"comando\":");
    client.print(cmd);
    client.print(",\"riesgoGlobal\":");
    client.print(riesgo ? "true" : "false");
    client.println("}");
}
#endif

/**
 * @brief Retorna el puntero al servidor WiFi
 */
WiFiServer* NetworkManager::getServer() {
    return server;
}

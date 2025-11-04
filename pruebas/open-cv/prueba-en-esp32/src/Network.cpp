#include "Network.h"
#include "config.h"
#include "CameraVision.h"
#include "esp_camera.h"
#include "img_converters.h"
#include <ESPmDNS.h>
#include "esp_jpg_decode.h"

CameraVision* g_cameraVision = nullptr;

// Variables globales para procesamiento de imagen
static uint8_t* g_processed_buffer = nullptr;
static int g_img_width = 0;
static int g_img_height = 0;
static const uint8_t* g_jpeg_data = nullptr;
static size_t g_jpeg_len = 0;
static size_t g_jpeg_pos = 0;

// Reader callback
static uint32_t jpg_read(void * arg, size_t index, uint8_t *buf, size_t len) {
    if (g_jpeg_pos >= g_jpeg_len) return 0;
    if (g_jpeg_pos + len > g_jpeg_len) {
        len = g_jpeg_len - g_jpeg_pos;
    }
    memcpy(buf, g_jpeg_data + g_jpeg_pos, len);
    g_jpeg_pos += len;
    return len;
}

// Writer callback
static bool jpg_write(void * arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data) {
    uint16_t* bmp = (uint16_t*)data;
    
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t col = 0; col < w; col++) {
            uint16_t pixel = bmp[row * w + col];
            
            // Convertir RGB565 a RGB888
            uint8_t r = ((pixel >> 11) & 0x1F) * 255 / 31;
            uint8_t g = ((pixel >> 5) & 0x3F) * 255 / 63;
            uint8_t b = (pixel & 0x1F) * 255 / 31;
            
            // Calcular brillo total
            int brightness = r + g + b;
            
            // Solo procesar píxeles con algo de brillo (no completamente negros)
            bool es_verde = false;
            bool es_azul = false;
            
            if (brightness > 80) {  // Umbral de brillo mínimo
                // Verde: G dominante (G > R y G > B)
                es_verde = (g > r * 1.2 && g > b * 1.2 && g > 40);
                
                // Azul: B dominante (B > R y B > G)
                es_azul = (b > r * 1.2 && b > g * 1.1 && b > 40);
            }
            
            int out_x = x + col;
            int out_y = y + row;
            
            if (out_x < g_img_width && out_y < g_img_height && g_processed_buffer) {
                g_processed_buffer[out_y * g_img_width + out_x] = (es_verde || es_azul) ? 255 : 0;
            }
        }
    }
    
    return true;
}

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
    
    // Desconectar cualquier conexión previa
    WiFi.disconnect(true);
    delay(100);
    
    // Configurar modo WiFi
    WiFi.mode(WIFI_STA);
    delay(100);
    
    // Iniciar conexión
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);
    
    // Esperar conexión con feedback detallado
    unsigned long startTime = millis();
    int dotCount = 0;
    
    DEBUG_PRINT("[NETWORK] Conectando");
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        DEBUG_PRINT(".");
        dotCount++;
        
        // Mostrar estado cada 5 segundos
        if (dotCount % 10 == 0) {
            DEBUG_PRINTLN("");
            DEBUG_PRINTF("[NETWORK] Tiempo transcurrido: %lu ms, Estado: %d\n", 
                        millis() - startTime, WiFi.status());
            DEBUG_PRINT("[NETWORK] Reintentando");
        }
    }
    DEBUG_PRINTLN("");
    
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("[NETWORK] ✓ WiFi conectado exitosamente");
        DEBUG_PRINTLN("[NETWORK] IP: " + WiFi.localIP().toString());
        DEBUG_PRINTLN("[NETWORK] Gateway: " + WiFi.gatewayIP().toString());
        DEBUG_PRINTLN("[NETWORK] DNS: " + WiFi.dnsIP().toString());
        DEBUG_PRINTLN("[NETWORK] RSSI: " + String(WiFi.RSSI()) + " dBm");
        DEBUG_PRINTLN("[NETWORK] Canal: " + String(WiFi.channel()));
        
        // Configurar mDNS para acceso fácil por nombre
        DEBUG_PRINTLN("[NETWORK] Configurando mDNS...");
        if (MDNS.begin("autito-robot")) {
            MDNS.addService("http", "tcp", WEB_SERVER_PORT);
            DEBUG_PRINTLN("[NETWORK] ✓ mDNS iniciado: http://autito-robot.local");
        } else {
            DEBUG_PRINTLN("[NETWORK] ⚠️ Error al iniciar mDNS");
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
    else if (request.indexOf("/stream_processed") != -1) {
        serveProcessedStream(client);
    }
    else if (request.indexOf("/shutdown") != -1) {
        serveShutdown(client);
    }
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
    client.println(".shutdown-btn { background: #e74c3c !important; margin-top: 30px; }");
    client.println(".shutdown-btn:hover { background: #c0392b !important; }");
    client.println("img { max-width: 90%; border: 3px solid #3498db; margin-top: 20px; }");
    client.println("</style></head><body>");
    client.println("<h1>🎥 Sistema de Detección de Objetos</h1>");
    client.println("<p>ESP32-CAM con Detección de Color en Tiempo Real</p>");
    
    // Enlace al stream
    client.println("<a href='" + String(STREAM_PATH) + "' target='_blank'>Ver Stream Original</a>");
    client.println("<a href='/stream_processed' target='_blank'>Ver Stream Procesado</a>");
    
    // Información del sistema
    client.println("<div style='margin: 30px auto; max-width: 600px; text-align: left; background: #34495e; padding: 20px; border-radius: 10px;'>");
    client.println("<h3 style='color: #3498db; margin-top: 0;'>📊 Información del Sistema</h3>");
    client.println("<p><strong>Estado:</strong> Operativo</p>");
    
    // Obtener información de detección si la cámara está disponible
    if (g_cameraVision != nullptr) {
        DetectionResult det = g_cameraVision->getUltimaDeteccion();
        float distancia = g_cameraVision->getDistanciaMinima();
        bool riesgo = g_cameraVision->getRiesgoLocal();
        ColorRange rango = g_cameraVision->getColorRange();
        
        if (det.encontrado) {
            client.println("<p><strong>Detección:</strong> ✅ Objeto encontrado</p>");
            client.printf("<p><strong>Posición:</strong> (%d, %d) px</p>", det.centroide_x, det.centroide_y);
            client.printf("<p><strong>Tamaño:</strong> %dx%d px (Área: %d px²)</p>", det.ancho, det.alto, det.area);
            client.printf("<p><strong>Distancia:</strong> %.1f cm</p>", distancia);
            client.printf("<p><strong>Alerta:</strong> %s</p>", riesgo ? "⚠️ Objeto muy cerca" : "✅ Distancia segura");
        } else {
            client.println("<p><strong>Detección:</strong> ❌ No se detectó ningún objeto</p>");
        }
        
        client.println("<hr style='border-color: #2c3e50;'>");
        client.println("<p style='color: #95a5a6; font-size: 14px;'><strong>Rango de Color Configurado:</strong></p>");
        client.printf("<p style='color: #95a5a6; font-size: 14px;'>R: %d-%d | G: %d-%d | B: %d-%d</p>", 
                     rango.r_min, rango.r_max, rango.g_min, rango.g_max, rango.b_min, rango.b_max);
    } else {
        client.println("<p><strong>Detección:</strong> ⚠️ Cámara no inicializada</p>");
    }
    
    client.println("</div>");
    
    // Mostrar ambos streams
    client.println("<h2>Vista Original:</h2>");
    client.println("<img src='" + String(STREAM_PATH) + "' style='width:45%; display:inline-block;' />");
    client.println("<h2>Vista Procesada (Verde/Azul):</h2>");
    client.println("<img src='/stream_processed' style='width:45%; display:inline-block;' />");
    
    // Botón de apagado del sistema
    client.println("<br><br>");
    client.println("<a href='/shutdown' class='shutdown-btn' onclick='return confirm(\"¿Seguro que deseas apagar el sistema? Se liberará toda la memoria y el ESP32 se reiniciará.\");'>⚠ Apagar Sistema</a>");
    
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
    
    esp_camera_fb_return(test_fb);
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Connection: close");
    client.println();
    
    // Stream continuo de frames
    while (client.connected()) {
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
        
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(jpg_buf_len));
        client.println();
        client.write(jpg_buf, jpg_buf_len);
        client.println();
        
        if (need_free && jpg_buf != nullptr) {
            free(jpg_buf);
        }
        esp_camera_fb_return(fb);
        
        //  delay para controlar FPS
        delay(50);  
    }
    
    DEBUG_PRINTLN("[NETWORK] Stream finalizado");
}

void NetworkManager::serveProcessedStream(WiFiClient& client) {
    DEBUG_PRINTLN("[NETWORK] Iniciando stream procesado (OpenCV)");
    
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (!test_fb) {
        // ... (tu código de error 503 está bien) ...
        client.println("HTTP/1.1 503 Service Unavailable");
        client.println("Connection: close");
        client.println();
        return;
    }

     if (test_fb->format != PIXFORMAT_RGB565) {
        DEBUG_PRINTLN("[NETWORK] ¡Error! El stream procesado REQUIERE formato PIXFORMAT_RGB565");
        esp_camera_fb_return(test_fb);
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Connection: close");
        client.println();
        return;
    }
    esp_camera_fb_return(test_fb);
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Connection: close");
    client.println();
    
    while (client.connected()) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            delay(100);
            continue;
        }
        
        if (fb->format == PIXFORMAT_RGB565) {
            if (g_cameraVision) {
                g_cameraVision->processFrameForUI(fb);
            }
        }

        uint8_t* jpg_buf = nullptr;
        size_t jpg_len = 0;
        
        bool ok = frame2jpg(fb, 12, &jpg_buf, &jpg_len);
        
        esp_camera_fb_return(fb); // Devolver el buffer original

        if (!ok || !jpg_buf) {
            DEBUG_PRINTLN("[NETWORK] Error convirtiendo a JPEG");
            continue;
        }
        
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(jpg_len));
        client.println();
        client.write((char*)jpg_buf, jpg_len);
        client.println();
        
        free(jpg_buf); 
        
        delay(50); 
    }
    
    DEBUG_PRINTLN("[NETWORK] Stream procesado finalizado");
}

void NetworkManager::processBinaryImage(camera_fb_t* fb, uint8_t* output) {
    if (fb->format != PIXFORMAT_JPEG) {
        memset(output, 0, fb->width * fb->height);
        return;
    }
    
    // Configurar variables globales
    g_processed_buffer = output;
    g_img_width = fb->width;
    g_img_height = fb->height;
    g_jpeg_data = fb->buf;
    g_jpeg_len = fb->len;
    g_jpeg_pos = 0;
    memset(output, 0, fb->width * fb->height);
    
    // Decodificar JPEG
    esp_err_t err = esp_jpg_decode(fb->len, JPG_SCALE_NONE, jpg_read, jpg_write, nullptr);
    
    if (err != ESP_OK) {
        DEBUG_PRINTF("[NETWORK] Error decodificando JPEG: %d\n", err);
    }
    
    g_processed_buffer = nullptr;
}

/**
 * @brief Endpoint para apagar el sistema y liberar toda la memoria
 * Desinicializa la cámara, libera buffers, desconecta WiFi y reinicia el ESP32
 */
void NetworkManager::serveShutdown(WiFiClient& client) {
    DEBUG_PRINTLN("[NETWORK] Iniciando apagado del sistema...");
    
    // Enviar respuesta HTML al cliente
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    
    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<title>Apagando Sistema</title>");
    client.println("<style>");
    client.println("body { font-family: Arial; text-align: center; background: #2c3e50; color: white; padding: 50px; }");
    client.println("h1 { color: #e74c3c; }");
    client.println(".spinner { border: 8px solid #f3f3f3; border-top: 8px solid #e74c3c; ");
    client.println("           border-radius: 50%; width: 60px; height: 60px; ");
    client.println("           animation: spin 1s linear infinite; margin: 20px auto; }");
    client.println("@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }");
    client.println("</style></head><body>");
    client.println("<h1>⚠ Apagando Sistema</h1>");
    client.println("<div class='spinner'></div>");
    client.println("<p>Liberando memoria y recursos...</p>");
    client.println("<p>El ESP32 se reiniciará en unos segundos.</p>");
    client.println("<p><strong>Puedes cerrar esta ventana.</strong></p>");
    client.println("</body></html>");
    
    client.flush();
    delay(500);  // Dar tiempo para que se envíe la respuesta
    
    // Liberar recursos de cámara
    DEBUG_PRINTLN("[NETWORK] Desinicializando cámara...");
    esp_camera_deinit();
    delay(100);
    
    // Desconectar WiFi
    DEBUG_PRINTLN("[NETWORK] Desconectando WiFi...");
    WiFi.disconnect(true);
    delay(100);
    
    // Liberar ESP-NOW
    DEBUG_PRINTLN("[NETWORK] Liberando ESP-NOW...");
    esp_now_deinit();
    delay(100);
    
    // Log final
    DEBUG_PRINTLN("[NETWORK] Memoria liberada. Reiniciando ESP32...");
    delay(500);
    
    // Reiniciar el ESP32
    esp_restart();
}


/**
 * @brief Retorna el puntero al servidor WiFi
 */
WiFiServer* NetworkManager::getServer() {
    return server;
}

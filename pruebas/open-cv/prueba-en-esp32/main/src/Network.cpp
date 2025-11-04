#include "Network.h"
#include "CameraVision.h"
#include "config.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "esp_mac.h"  // Para MACSTR y MAC2STR
#include <string.h>

static const char *TAG = "NETWORK";

CameraVision* g_cameraVision = nullptr;

// Event handler para WiFi
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Desconectado del AP, reintentando...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "✓ WiFi conectado - IP: " IPSTR, IP2STR(&event->ip_info.ip));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "Cliente conectado al AP, MAC:" MACSTR, MAC2STR(event->mac));
    }
}

bool NetworkManager::initWiFi() {
    // Inicializar NVS si no está inicializado
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicializar TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef ACCESS_POINT_MODE
    ESP_LOGI(TAG, "Configurando WiFi en modo AP...");
    
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.ap.ssid, WIFI_AP_SSID);
    strcpy((char*)wifi_config.ap.password, WIFI_AP_PASSWORD);
    wifi_config.ap.ssid_len = strlen(WIFI_AP_SSID);
    wifi_config.ap.channel = WIFI_AP_CHANNEL;
    wifi_config.ap.max_connection = WIFI_AP_MAX_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    
    if (strlen(WIFI_AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(ap_netif, &ip_info);
    
    ESP_LOGI(TAG, "✓ AP configurado - SSID: %s", WIFI_AP_SSID);
    ESP_LOGI(TAG, "  IP: " IPSTR, IP2STR(&ip_info.ip));
    
#elif defined(CONNECT_TO_NETWORK)
    ESP_LOGI(TAG, "Configurando WiFi en modo STA...");
    ESP_LOGI(TAG, "Conectando a: %s", WIFI_STA_SSID);
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_STA_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_STA_PASSWORD);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Esperar conexión
    ESP_LOGI(TAG, "Esperando conexión WiFi...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Dar tiempo para conectar
    
    // mDNS deshabilitado por ahora (requiere componente adicional en ESP-IDF 6.0)
    // Para habilitarlo: idf.py add-dependency "espressif/mdns"
    /*
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("autito-robot"));
    ESP_LOGI(TAG, "✓ mDNS iniciado: http://autito-robot.local");
    */
#endif

    return true;
}

bool NetworkManager::setup() {
    ESP_LOGI(TAG, "=== Inicializando sistema de red ===");
    
    if (!initWiFi()) {
        ESP_LOGE(TAG, "Error al inicializar WiFi");
        return false;
    }
    
    if (!initHTTPServer()) {
        ESP_LOGE(TAG, "Error al inicializar servidor HTTP");
        return false;
    }
    
    char ip[16];
    getIP(ip, sizeof(ip));
    ESP_LOGI(TAG, "✓ Servidor web iniciado en http://%s:%d", ip, WEB_SERVER_PORT);
    ESP_LOGI(TAG, "  Stream: http://%s%s", ip, STREAM_PATH);
    
    return true;
}

void NetworkManager::getIP(char* ip_str, size_t len) {
#ifdef ACCESS_POINT_MODE
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
#else
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
#endif
    
    if (netif) {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(netif, &ip_info);
        snprintf(ip_str, len, IPSTR, IP2STR(&ip_info.ip));
    } else {
        snprintf(ip_str, len, "0.0.0.0");
    }
}

// Handler para página principal
esp_err_t NetworkManager::index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    
    // HTML de la página principal
    const char* html_start = 
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<title>Autito Robot - Control</title>"
        "<style>"
        "body { font-family: Arial; text-align: center; background: #2c3e50; color: white; padding: 20px; }"
        "h1 { color: #3498db; }"
        "a { display: inline-block; margin: 15px; padding: 15px 30px; "
        "    background: #3498db; color: white; text-decoration: none; "
        "    border-radius: 5px; font-size: 18px; }"
        "a:hover { background: #2980b9; }"
        ".info { max-width: 600px; margin: 20px auto; text-align: left; "
        "        background: #34495e; padding: 20px; border-radius: 10px; }"
        ".shutdown-btn { background: #e74c3c !important; }"
        ".shutdown-btn:hover { background: #c0392b !important; }"
        "img { max-width: 90%; border: 3px solid #3498db; margin: 20px; }"
        "</style></head><body>"
        "<h1>🎥 Sistema de Detección de Objetos</h1>"
        "<p>ESP32-S3 con OpenCV + Detección en Tiempo Real</p>";
    
    httpd_resp_send_chunk(req, html_start, strlen(html_start));
    
    // Enlaces
    const char* links = 
        "<div>"
        "<a href='/stream' target='_blank'>Ver Stream de Cámara</a>"
        "</div>";
    httpd_resp_send_chunk(req, links, strlen(links));
    
    // Información del sistema
    httpd_resp_sendstr_chunk(req, "<div class='info'>");
    httpd_resp_sendstr_chunk(req, "<h3 style='color: #3498db; margin-top: 0;'>📊 Información del Sistema</h3>");
    
    if (g_cameraVision != nullptr && g_cameraVision->isCameraInitialized()) {
        DetectionResult det = g_cameraVision->getUltimaDeteccion();
        float distancia = g_cameraVision->getDistanciaMinima();
        bool riesgo = g_cameraVision->getRiesgoLocal();
        
        if (det.encontrado) {
            char buf[256];
            snprintf(buf, sizeof(buf), 
                "<p><strong>Detección:</strong> ✅ Objeto encontrado</p>"
                "<p><strong>Posición:</strong> (%d, %d) px</p>"
                "<p><strong>Tamaño:</strong> %dx%d px (Área: %d px²)</p>"
                "<p><strong>Distancia:</strong> %.1f cm</p>"
                "<p><strong>Alerta:</strong> %s</p>",
                det.centroide_x, det.centroide_y,
                det.ancho, det.alto, det.area,
                distancia,
                riesgo ? "⚠️ Objeto muy cerca" : "✅ Distancia segura");
            httpd_resp_sendstr_chunk(req, buf);
        } else {
            httpd_resp_sendstr_chunk(req, "<p><strong>Detección:</strong> ❌ No se detectó objeto</p>");
        }
    } else {
        httpd_resp_sendstr_chunk(req, "<p><strong>Estado:</strong> ⚠️ Cámara no inicializada</p>");
    }
    
    httpd_resp_sendstr_chunk(req, "</div>");
    
    // Stream embebido
    httpd_resp_sendstr_chunk(req, "<h2>Vista en Vivo:</h2>");
    httpd_resp_sendstr_chunk(req, "<img src='/stream' />");
    
    // Botón de shutdown
    httpd_resp_sendstr_chunk(req, 
        "<br><br><a href='/shutdown' class='shutdown-btn' "
        "onclick='return confirm(\"¿Seguro que deseas reiniciar el ESP32?\");'>"
        "⚠ Reiniciar Sistema</a>");
    
    httpd_resp_sendstr_chunk(req, "</body></html>");
    httpd_resp_sendstr_chunk(req, NULL);  // Finalizar chunked response
    
    return ESP_OK;
}

// Handler para stream MJPEG
esp_err_t NetworkManager::stream_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (!test_fb) {
        ESP_LOGE(TAG, "Cámara no disponible");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    esp_camera_fb_return(test_fb);
    
    // Headers para MJPEG streaming
    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    ESP_LOGI(TAG, "Iniciando stream de video");
    
    while (true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Error capturando frame");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        // Convertir a JPEG si es necesario
        uint8_t* jpg_buf = nullptr;
        size_t jpg_len = 0;
        bool need_free = false;
        
        if (fb->format == PIXFORMAT_JPEG) {
            jpg_buf = fb->buf;
            jpg_len = fb->len;
        } else {
            need_free = frame2jpg(fb, 80, &jpg_buf, &jpg_len);
            if (!need_free || jpg_len == 0) {
                esp_camera_fb_return(fb);
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
        }
        
        // Enviar frame
        char part_buf[128];
        snprintf(part_buf, sizeof(part_buf),
                "--frame\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %zu\r\n\r\n",
                jpg_len);
        
        res = httpd_resp_send_chunk(req, part_buf, strlen(part_buf));
        
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)jpg_buf, jpg_len);
        }
        
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n", 2);
        }
        
        if (need_free && jpg_buf != nullptr) {
            free(jpg_buf);
        }
        esp_camera_fb_return(fb);
        
        if (res != ESP_OK) {
            ESP_LOGW(TAG, "Cliente desconectado del stream");
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // ~20 FPS
    }
    
    return res;
}

// Handler para shutdown/reinicio
esp_err_t NetworkManager::shutdown_handler(httpd_req_t *req) {
    ESP_LOGW(TAG, "Reinicio solicitado vía web");
    
    const char* html = 
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'><title>Reiniciando</title>"
        "<style>"
        "body { font-family: Arial; text-align: center; background: #2c3e50; color: white; padding: 50px; }"
        "h1 { color: #e74c3c; }"
        ".spinner { border: 8px solid #f3f3f3; border-top: 8px solid #e74c3c; "
        "           border-radius: 50%; width: 60px; height: 60px; "
        "           animation: spin 1s linear infinite; margin: 20px auto; }"
        "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }"
        "</style></head><body>"
        "<h1>⚠ Reiniciando Sistema</h1>"
        "<div class='spinner'></div>"
        "<p>El ESP32 se reiniciará en 3 segundos...</p>"
        "</body></html>";
    
    httpd_resp_send(req, html, strlen(html));
    
    // Programar reinicio
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    
    return ESP_OK;
}

bool NetworkManager::initHTTPServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.ctrl_port = 32768;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;
    
    ESP_LOGI(TAG, "Iniciando servidor HTTP en puerto %d", config.server_port);
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Error al iniciar servidor HTTP");
        return false;
    }
    
    // Registrar handlers
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &index_uri);
    
    httpd_uri_t stream_uri = {
        .uri = STREAM_PATH,
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &stream_uri);
    
    httpd_uri_t shutdown_uri = {
        .uri = "/shutdown",
        .method = HTTP_GET,
        .handler = shutdown_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(server, &shutdown_uri);
    
    return true;
}

void NetworkManager::stop() {
    if (server) {
        httpd_stop(server);
        server = nullptr;
    }
    esp_wifi_stop();
    ESP_LOGI(TAG, "Servidor detenido");
}

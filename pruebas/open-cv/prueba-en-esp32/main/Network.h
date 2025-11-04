#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "lwip/sockets.h"

/**
 * @file Network.h
 * @brief Gestor de red para WiFi y streaming de video con ESP-IDF
 * 
 * Esta clase maneja:
 * - Configuración del WiFi (AP o STA)
 * - Servidor HTTP para streaming de video MJPEG
 * - Página web con información del sistema
 */

// Forward declaration
class CameraVision;

class NetworkManager {
public:
    /**
     * @brief Inicializa el sistema de red
     */
    bool setup();

    /**
     * @brief Detiene el servidor y libera recursos
     */
    void stop();

    /**
     * @brief Obtiene la IP actual
     */
    void getIP(char* ip_str, size_t len);

private:
    httpd_handle_t server = nullptr;
    
    /**
     * @brief Inicializa WiFi en modo AP o STA según config.h
     */
    bool initWiFi();

    /**
     * @brief Inicializa el servidor HTTP
     */
    bool initHTTPServer();

    /**
     * @brief Handlers HTTP estáticos (requeridos por ESP-IDF)
     */
    static esp_err_t index_handler(httpd_req_t *req);
    static esp_err_t stream_handler(httpd_req_t *req);
    static esp_err_t shutdown_handler(httpd_req_t *req);
};

// Puntero global al sistema de visión
extern CameraVision* g_cameraVision;

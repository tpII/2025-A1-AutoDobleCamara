#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

// Forward declaration
class CameraVision;

class NetworkManager {
public:
    bool setup();
    void stop();
    void getIP(char* ip_str, size_t len);

private:
    httpd_handle_t server = nullptr;
    
    bool initWiFi();
    bool initHTTPServer();

    static esp_err_t index_handler(httpd_req_t *req);
    static esp_err_t stream_handler(httpd_req_t *req);
    static esp_err_t shutdown_handler(httpd_req_t *req);
};

// Puntero global al sistema de visión
extern CameraVision* g_cameraVision;

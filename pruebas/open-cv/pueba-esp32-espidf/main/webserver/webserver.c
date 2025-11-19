#include "webserver.h"
#include "../camera_driver/camera_driver.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_camera.h"
#include <string.h>

static const char *TAG = "Webserver";
static httpd_handle_t server = NULL;

// HTML page with embedded JavaScript for video streaming
static const char STREAM_HTML[] =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "    <title>ESP32 Camera Stream</title>\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
    "    <style>\n"
    "        body {\n"
    "            font-family: Arial, Helvetica, sans-serif;\n"
    "            background-color: #181818;\n"
    "            color: #fff;\n"
    "            text-align: center;\n"
    "            margin: 0;\n"
    "            padding: 20px;\n"
    "        }\n"
    "        h1 {\n"
    "            color: #4CAF50;\n"
    "            margin-bottom: 20px;\n"
    "        }\n"
    "        img {\n"
    "            max-width: 100%;\n"
    "            height: auto;\n"
    "            border: 3px solid #4CAF50;\n"
    "            border-radius: 8px;\n"
    "            box-shadow: 0 4px 8px rgba(0,0,0,0.3);\n"
    "        }\n"
    "        .container {\n"
    "            max-width: 1200px;\n"
    "            margin: 0 auto;\n"
    "        }\n"
    "        .info {\n"
    "            margin-top: 20px;\n"
    "            padding: 10px;\n"
    "            background-color: #282828;\n"
    "            border-radius: 5px;\n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <div class=\"container\">\n"
    "        <h1>ESP32-S3 Camera Live Stream</h1>\n"
    "        <img id=\"stream\" src=\"/stream\" alt=\"Camera Stream\">\n"
    "        <div class=\"info\">\n"
    "            <p>Streaming video from ESP32-S3 with OV2640 camera</p>\n"
    "        </div>\n"
    "    </div>\n"
    "</body>\n"
    "</html>\n";

// Content type for MJPEG stream
#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

/**
 * Handler for the root page
 */
static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, STREAM_HTML, HTTPD_RESP_USE_STRLEN);
}

/**
 * Handler for MJPEG stream
 */
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char part_buf[64];

    ESP_LOGI(TAG, "Stream requested");

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set response type");
        return res;
    }

    while (true)
    {
        fb = camera_capture();
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        if (fb->format != PIXFORMAT_JPEG)
        {
            ESP_LOGE(TAG, "Non-JPEG format not supported");
            camera_fb_return(fb);
            res = ESP_FAIL;
            break;
        }

        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;

        // Send boundary
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        // Send content type and length
        if (res == ESP_OK)
        {
            size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, part_buf, hlen);
        }

        // Send image data
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }

        camera_fb_return(fb);
        fb = NULL;
        _jpg_buf = NULL;

        if (res != ESP_OK)
        {
            ESP_LOGW(TAG, "Stream interrupted");
            break;
        }
    }

    if (fb)
    {
        camera_fb_return(fb);
    }

    ESP_LOGI(TAG, "Stream ended");
    return res;
}

/**
 * Handler for capture single frame
 */
static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

    fb = camera_capture();
    if (!fb)
    {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

    res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    camera_fb_return(fb);

    return res;
}

esp_err_t webserver_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.ctrl_port = 32768;
    config.max_uri_handlers = 8;
    config.max_resp_headers = 8;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    // URI handler for root page
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    // URI handler for stream
    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    // URI handler for single capture
    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL};

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &stream_uri);
        httpd_register_uri_handler(server, &capture_uri);
        ESP_LOGI(TAG, "Web server started successfully");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting web server!");
    return ESP_FAIL;
}

esp_err_t webserver_stop(void)
{
    if (server)
    {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
        return ESP_OK;
    }
    return ESP_FAIL;
}

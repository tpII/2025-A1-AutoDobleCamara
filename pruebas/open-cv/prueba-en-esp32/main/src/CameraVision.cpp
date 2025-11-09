#include "CameraVision.h"
#include "config.h"
#include <algorithm>
#include "img_converters.h"
#include "esp_camera.h"
#include "sensor.h"

static const char *TAG = "VISION";

#define COLOR_H_MIN 8
#define COLOR_H_MAX 25
#define COLOR_S_MIN 50
#define COLOR_S_MAX 255
#define COLOR_V_MIN 120
#define COLOR_V_MAX 255

#define RECT_COLOR 0xF800

CameraVision::CameraVision() {
    cameraInitialized = false;
    distanciaMinimaLocal = -1.0f;
    riesgoLocal = false;
    ultimaDeteccion = {false, 0, 0, 0, 0, 0};
    dataMutex = NULL;
}

bool CameraVision::setup() {
    cameraInitialized = false;
    distanciaMinimaLocal = -1.0f;
    riesgoLocal = false;
    ultimaDeteccion = {false, 0, 0, 0, 0, 0};

    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        ESP_LOGE(TAG, "Error al crear mutex");
        return false;
    }

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    
    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SDA;
    config.pin_sccb_scl = CAM_PIN_SCL;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    
    config.xclk_freq_hz = 20000000;
    
    config.pixel_format = PIXFORMAT_RGB565; 
    config.frame_size = FRAMESIZE_QVGA;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM; 
    config.grab_mode = CAMERA_GRAB_LATEST;   
    config.jpeg_quality = 10;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar cámara: 0x%x", err);
        return false;
    }

    // Ajustes manuales de los parametros dela camarita
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_exposure_ctrl(s, 1);
        s->set_gain_ctrl(s, 1);
        s->set_awb_gain(s, 1);
        ESP_LOGI(TAG, "Configuracion del sensor manual aplicada");
    }

    cameraInitialized = true;
    ESP_LOGI(TAG, "Cámara inicializada correctamente (Modo OpenCV/PSRAM)");
    
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (test_fb) {
        ESP_LOGI(TAG, "Test exitoso: %d bytes, %dx%d, formato=%d", 
                    test_fb->len, test_fb->width, test_fb->height, test_fb->format);
        esp_camera_fb_return(test_fb);
    } else {
        ESP_LOGW(TAG, "Advertencia: No se pudo capturar frame de prueba");
    }
    
    return true;
}

void CameraVision::draw_rectangle_rgb565(camera_fb_t* fb, const cv::Rect& rect, uint16_t color) {
    if (!fb || fb->format != PIXFORMAT_RGB565) return;

    uint16_t* p_buf = (uint16_t*)fb->buf;
    int w = fb->width;
    int h = fb->height;

    int x1 = std::max(0, rect.x);
    int y1 = std::max(0, rect.y);
    int x2 = std::min(w - 1, rect.x + rect.width);
    int y2 = std::min(h - 1, rect.y + rect.height);

    for (int x = x1; x <= x2; x++) { p_buf[y1 * w + x] = color; p_buf[y2 * w + x] = color; }
    for (int y = y1; y <= y2; y++) { p_buf[y * w + x1] = color; p_buf[y * w + x2] = color; }
}

void CameraVision::run() {
    if (!cameraInitialized) {
        return;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Error al capturar frame");
        return;
    }

    if (fb->format != PIXFORMAT_RGB565) {
        ESP_LOGW(TAG, "Formato incorrecto, esperaba RGB565");
        esp_camera_fb_return(fb);
        return;
    }

    cv::Mat mat_rgb565(fb->height, fb->width, CV_8UC2, fb->buf);
    cv::Mat mat_bgr, mat_hsv, mask_green;
    
    cv::cvtColor(mat_rgb565, mat_bgr, cv::COLOR_BGR5652BGR);
    cv::cvtColor(mat_bgr, mat_hsv, cv::COLOR_BGR2HSV);

    cv::Mat mask_color;
    cv::inRange(mat_hsv, cv::Scalar(COLOR_H_MIN, COLOR_S_MIN, COLOR_V_MIN),
                cv::Scalar(COLOR_H_MAX, COLOR_S_MAX, COLOR_V_MAX), mask_color);

    int pixeles_color = cv::countNonZero(mask_color);
    int total_pixeles = mat_bgr.cols * mat_bgr.rows;
    float porcentaje_color = (pixeles_color * 100.0f) / total_pixeles;
    
    if (pixeles_color > 0) {
        ESP_LOGI(TAG, "🔍 DEBUG: Píxeles detectados=%d/%d (%.1f%%) | HSV: H[%d-%d] S[%d-%d] V[%d-%d]",
                 pixeles_color, total_pixeles, porcentaje_color,
                 COLOR_H_MIN, COLOR_H_MAX, COLOR_S_MIN, COLOR_S_MAX, COLOR_V_MIN, COLOR_V_MAX);
    }

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(mask_color, mask_color, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(mask_color, mask_color, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask_color, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    float dist_minima = 999.0f;
    bool objeto_encontrado = false;
    int objeto_ancho_px = 0;
    int objeto_alto_px = 0;
    int objeto_area = 0;

    if (contours.size() > 0) {
        ESP_LOGI(TAG, "📊 Contornos encontrados: %d", contours.size());
    }

    for (size_t i = 0; i < contours.size(); i++) {
        int area = cv::contourArea(contours[i]);
        
        if (area < 1500) {
            continue;
        }
        
        if (area > (mat_bgr.cols * mat_bgr.rows * 0.5)) {
            ESP_LOGW(TAG, "⚠️ Contorno #%d IGNORADO (demasiado grande): área=%dpx² (>50%% imagen)", i, area);
            continue;
        }
        
        cv::Rect rect = cv::boundingRect(contours[i]);
        
        if (rect.width >= mat_bgr.cols * 0.8 || rect.height >= mat_bgr.rows * 0.8) {
            ESP_LOGW(TAG, "⚠️ Contorno #%d IGNORADO (dimensiones): %dx%d px (>80%% imagen)", i, rect.width, rect.height);
            continue;
        }

        draw_rectangle_rgb565(fb, rect, RECT_COLOR); // Dibuja en el buffer
        
        float distancia = calcularDistancia(static_cast<float>(rect.width));
        
        if (distancia > 0 && distancia < dist_minima) {
            dist_minima = distancia;
            objeto_ancho_px = rect.width;
            objeto_alto_px = rect.height;
            objeto_area = area;
            objeto_encontrado = true;
        }
    }

    esp_camera_fb_return(fb);

    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        if (objeto_encontrado) {
            distanciaMinimaLocal = dist_minima;
            riesgoLocal = (dist_minima < UMBRAL_RIESGO_LOCAL_CM);
            ultimaDeteccion.encontrado = true;
            ultimaDeteccion.ancho = objeto_ancho_px;
            ultimaDeteccion.alto = objeto_alto_px;
            ultimaDeteccion.area = objeto_area;

            ESP_LOGI(TAG, "🟢 Objeto NARANJA detectado: ancho=%dpx alto=%dpx área=%dpx² distancia=%.1fcm %s",
                     objeto_ancho_px, objeto_alto_px, objeto_area, dist_minima,
                     riesgoLocal ? "⚠️ CERCA" : "✓");
        } else {
            distanciaMinimaLocal = -1.0f;
            riesgoLocal = false;
            ultimaDeteccion.encontrado = false;
        }
        xSemaphoreGive(dataMutex);
    }
}

float CameraVision::calcularDistancia(float anchoEnPixeles) {
    if (anchoEnPixeles <= 0) {
        return -1.0f;
    }
    float distancia = (ANCHO_OBSTACULO_CM * FOCAL_AUTITO_PX) / anchoEnPixeles;
    return distancia;
}

bool CameraVision::getRiesgoLocal() {
    bool riesgo = false;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        riesgo = riesgoLocal;
        xSemaphoreGive(dataMutex);
    }
    return riesgo;
}

float CameraVision::getDistanciaMinima() {
    float distancia = -1.0f;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        distancia = distanciaMinimaLocal;
        xSemaphoreGive(dataMutex);
    }
    return distancia;
}

DetectionResult CameraVision::getUltimaDeteccion() {
    DetectionResult deteccion = {false, 0, 0, 0, 0, 0};
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        deteccion = ultimaDeteccion;
        xSemaphoreGive(dataMutex);
    }
    return deteccion;
}

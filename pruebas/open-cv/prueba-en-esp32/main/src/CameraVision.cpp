#include "CameraVision.h"
#include "config.h"
#include <algorithm>

static const char *TAG = "VISION";

#define VERDE_H_MIN 35
#define VERDE_H_MAX 85
#define VERDE_S_MIN 100
#define VERDE_S_MAX 255
#define VERDE_V_MIN 100
#define VERDE_V_MAX 255

#define AZUL_H_MIN 95
#define AZUL_H_MAX 130
#define AZUL_S_MIN 100
#define AZUL_S_MAX 255
#define AZUL_V_MIN 100
#define AZUL_V_MAX 255

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
    config.frame_size = FRAMESIZE_HVGA;     
    config.fb_count = 2;                    
    config.fb_location = CAMERA_FB_IN_PSRAM; 
    config.grab_mode = CAMERA_GRAB_LATEST;  
    config.jpeg_quality = 63;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error al inicializar cámara: 0x%x", err);
        return false;
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

    // Asegurarse de que las coordenadas no se salgan de la imagen
    int x1 = std::max(0, rect.x);
    int y1 = std::max(0, rect.y);
    int x2 = std::min(w - 1, rect.x + rect.width);
    int y2 = std::min(h - 1, rect.y + rect.height);

    // Dibujar líneas horizontales (superior e inferior)
    for (int x = x1; x <= x2; x++) {
        p_buf[y1 * w + x] = color;
        p_buf[y2 * w + x] = color;
    }
    // Dibujar líneas verticales (izquierda y derecha)
    for (int y = y1; y <= y2; y++) {
        p_buf[y * w + x1] = color;
        p_buf[y * w + x2] = color;
    }
}

void CameraVision::rgb565ToRgb(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b) {
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    
    *r = (r5 * 255) / 31;
    *g = (g6 * 255) / 63;
    *b = (b5 * 255) / 31;
}

bool CameraVision::estaEnRangoColor(uint8_t r, uint8_t g, uint8_t b, const ColorRange& range) {
    return (r >= range.r_min && r <= range.r_max &&
            g >= range.g_min && g <= range.g_max &&
            b >= range.b_min && b <= range.b_max);
}

void CameraVision::detectarObjetoRGB565(camera_fb_t* fb, DetectionResult& resultado) {
    resultado.encontrado = false;
    
    if (fb == NULL || fb->format != PIXFORMAT_RGB565) {
        return;
    }

    int width = fb->width;
    int height = fb->height;
    uint16_t* pixels = (uint16_t*)fb->buf;
    
    int suma_x = 0;
    int suma_y = 0;
    int pixeles_detectados = 0;
    int min_x = width;
    int max_x = 0;
    int min_y = height;
    int max_y = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            uint16_t pixel = pixels[idx];
            
            uint8_t r, g, b;
            rgb565ToRgb(pixel, &r, &g, &b);
            
            if (estaEnRangoColor(r, g, b, colorObstaculo)) {
                suma_x += x;
                suma_y += y;
                pixeles_detectados++;
                
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }

    const int umbral_min_pixeles = 20;
    
    if (pixeles_detectados >= umbral_min_pixeles) {
        resultado.encontrado = true;
        resultado.centroide_x = suma_x / pixeles_detectados;
        resultado.centroide_y = suma_y / pixeles_detectados;
        resultado.ancho = max_x - min_x;
        resultado.alto = max_y - min_y;
        resultado.area = pixeles_detectados;
    }
}

void CameraVision::detectarObjeto(camera_fb_t* fb, DetectionResult& resultado) {
    detectarObjetoRGB565(fb, resultado);
}

void CameraVision::run() {
    if (!cameraInitialized) {
        return;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        static int errorCount = 0;
        errorCount++;
        if (errorCount % 10 == 1) {
            ESP_LOGE(TAG, "Error al capturar frame");
        }
        
        if (errorCount > 100) {
            ESP_LOGE(TAG, "Muchos errores - reintentando inicialización...");
            esp_camera_deinit();
            vTaskDelay(pdMS_TO_TICKS(200));
            setup();
            errorCount = 0;
        }
        return;
    }

    DetectionResult deteccion;
    detectarObjeto(fb, deteccion);

    float distancia = -1.0f;
    if (deteccion.encontrado && deteccion.ancho > 0) {
        distancia = calcularDistancia(static_cast<float>(deteccion.ancho));
    }

    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        ultimaDeteccion = deteccion;
        distanciaMinimaLocal = distancia;
        riesgoLocal = (distancia > 0 && distancia < UMBRAL_RIESGO_LOCAL_CM);
        xSemaphoreGive(dataMutex);
    }

    static uint32_t lastDebug = 0;
    if (deteccion.encontrado && ((xTaskGetTickCount() * portTICK_PERIOD_MS) - lastDebug > 1000)) {
        ESP_LOGI(TAG, "Objeto detectado: ancho=%dpx dist=%.1fcm", 
                    deteccion.ancho, distancia);
        lastDebug = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }

    esp_camera_fb_return(fb);
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

void CameraVision::setColorRange(const ColorRange& range) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        colorObstaculo = range;
        xSemaphoreGive(dataMutex);
    }
    ESP_LOGI(TAG, "Rango de color actualizado");
}

ColorRange CameraVision::getColorRange() {
    ColorRange range;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        range = colorObstaculo;
        xSemaphoreGive(dataMutex);
    }
    return range;
}

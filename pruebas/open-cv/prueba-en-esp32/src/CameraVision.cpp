#include "CameraVision.h"
#include "config.h"

#ifdef DETECTION_JPEG
#include <TJpg_Decoder.h>
#endif

#ifdef DETECTION_JPEG
int CameraVision::jpegDecodeCount = 0;
int CameraVision::jpegSumX = 0;
int CameraVision::jpegSumY = 0;
int CameraVision::jpegMinX = 99999;
int CameraVision::jpegMaxX = 0;
int CameraVision::jpegMinY = 99999;
int CameraVision::jpegMaxY = 0;
ColorRange CameraVision::jpegColorRange = {0, 90, 120, 255, 0, 90};
#endif

bool CameraVision::setup() {
    cameraInitialized = false;
    distanciaMinimaLocal = -1.0f;
    riesgoLocal = false;
    ultimaDeteccion = {false, 0, 0, 0, 0, 0};

    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        DEBUG_PRINTLN("[VISION] Error al crear mutex");
        return false;
    }

    colorObstaculo.r_min = 0;
    colorObstaculo.r_max = 90;
    colorObstaculo.g_min = 120;
    colorObstaculo.g_max = 255;
    colorObstaculo.b_min = 0;
    colorObstaculo.b_max = 90;

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
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

#ifdef DETECTION_JPEG
    DEBUG_PRINTLN("[VISION] Modo: JPEG con decodificacion linea por linea");
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QQVGA;
    config.jpeg_quality = 15;
#else
    DEBUG_PRINTLN("[VISION] Modo: RGB565 directo");
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_96X96;
    config.jpeg_quality = 15;
#endif

    DEBUG_PRINTLN("[VISION] Configuracion de camara:");
    DEBUG_PRINTF("  XCLK: Deshabilitado (freq=0)\n");
    DEBUG_PRINTF("  Formato: %s\n", 
#ifdef DETECTION_JPEG
                 "JPEG"
#else
                 "RGB565"
#endif
    );
    DEBUG_PRINTF("  Resolucion: %s\n",
#ifdef DETECTION_JPEG
                 "QQVGA (160x120)"
#else
                 "96x96"
#endif
    );

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        DEBUG_PRINTF("[VISION] Error al inicializar: 0x%x\n", err);
        
#ifndef DETECTION_JPEG
        DEBUG_PRINTLN("[VISION] Reintentando con QQVGA...");
        config.frame_size = FRAMESIZE_QQVGA;
        err = esp_camera_init(&config);
        
        if (err != ESP_OK) {
            DEBUG_PRINTF("[VISION] Error con QQVGA: 0x%x\n", err);
            return false;
        }
#else
        return false;
#endif
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_brightness(s, 0);
        s->set_contrast(s, 1);
        s->set_saturation(s, 2);
        s->set_special_effect(s, 0);
        s->set_whitebal(s, 1);
        s->set_awb_gain(s, 1);
        s->set_wb_mode(s, 0);
        s->set_exposure_ctrl(s, 1);
        s->set_aec2(s, 1);
        s->set_ae_level(s, 0);
        s->set_aec_value(s, 300);
        s->set_gain_ctrl(s, 1);
        s->set_agc_gain(s, 0);
        s->set_gainceiling(s, (gainceiling_t)0);
        s->set_bpc(s, 1);
        s->set_wpc(s, 1);
        s->set_raw_gma(s, 1);
        s->set_lenc(s, 1);
        s->set_hmirror(s, 0);
        s->set_vflip(s, 0);
        s->set_dcw(s, 1);
        s->set_colorbar(s, 0);
        
        DEBUG_PRINTF("[VISION] Sensor ID: 0x%02X\n", s->id.PID);
    }

#ifdef DETECTION_JPEG
    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(jpegOutputCallback);
    DEBUG_PRINTLN("[VISION] TJpgDec inicializado");
#endif

    cameraInitialized = true;
    DEBUG_PRINTLN("[VISION] Camara inicializada correctamente");
    
    camera_fb_t* test_fb = esp_camera_fb_get();
    if (test_fb) {
        DEBUG_PRINTF("[VISION] Test exitoso: %d bytes, %dx%d, formato=%d\n", 
                    test_fb->len, test_fb->width, test_fb->height, test_fb->format);
        esp_camera_fb_return(test_fb);
    } else {
        DEBUG_PRINTLN("[VISION] Advertencia: No se pudo capturar frame de prueba");
    }
    
    return true;
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

#ifdef DETECTION_JPEG

bool CameraVision::jpegOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    for (uint16_t i = 0; i < w * h; i++) {
        uint16_t pixel = bitmap[i];
        uint8_t r, g, b;
        
        uint8_t r5 = (pixel >> 11) & 0x1F;
        uint8_t g6 = (pixel >> 5) & 0x3F;
        uint8_t b5 = pixel & 0x1F;
        
        r = (r5 * 255) / 31;
        g = (g6 * 255) / 63;
        b = (b5 * 255) / 31;
        
        if (r >= jpegColorRange.r_min && r <= jpegColorRange.r_max &&
            g >= jpegColorRange.g_min && g <= jpegColorRange.g_max &&
            b >= jpegColorRange.b_min && b <= jpegColorRange.b_max) {
            
            int px = x + (i % w);
            int py = y + (i / w);
            
            jpegDecodeCount++;
            jpegSumX += px;
            jpegSumY += py;
            
            if (px < jpegMinX) jpegMinX = px;
            if (px > jpegMaxX) jpegMaxX = px;
            if (py < jpegMinY) jpegMinY = py;
            if (py > jpegMaxY) jpegMaxY = py;
        }
    }
    
    return true;
}

void CameraVision::detectarObjetoJPEG(camera_fb_t* fb, DetectionResult& resultado) {
    resultado.encontrado = false;
    
    if (fb == NULL || fb->format != PIXFORMAT_JPEG) {
        return;
    }
    
    jpegDecodeCount = 0;
    jpegSumX = 0;
    jpegSumY = 0;
    jpegMinX = 99999;
    jpegMaxX = 0;
    jpegMinY = 99999;
    jpegMaxY = 0;
    
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        jpegColorRange = colorObstaculo;
        xSemaphoreGive(dataMutex);
    }
    
    TJpgDec.drawJpg(0, 0, (const uint8_t*)fb->buf, fb->len);
    
    const int umbral_min_pixeles = 20;
    
    if (jpegDecodeCount >= umbral_min_pixeles) {
        resultado.encontrado = true;
        resultado.centroide_x = jpegSumX / jpegDecodeCount;
        resultado.centroide_y = jpegSumY / jpegDecodeCount;
        resultado.ancho = jpegMaxX - jpegMinX;
        resultado.alto = jpegMaxY - jpegMinY;
        resultado.area = jpegDecodeCount;
    }
}

void CameraVision::detectarObjeto(camera_fb_t* fb, DetectionResult& resultado) {
    detectarObjetoJPEG(fb, resultado);
}

#else

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

#endif

void CameraVision::run() {
    if (!cameraInitialized) {
        return;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        static int errorCount = 0;
        errorCount++;
        if (errorCount % 10 == 1) {
            DEBUG_PRINTLN("[VISION] Error al capturar frame");
        }
        
        if (errorCount > 100) {
            DEBUG_PRINTLN("[VISION] Muchos errores - reintentando inicializacion...");
            esp_camera_deinit();
            delay(200);
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

    #ifdef DEBUG_SERIAL
    static unsigned long lastDebug = 0;
    if (deteccion.encontrado && (millis() - lastDebug > 1000)) {
        DEBUG_PRINTF("[VISION] Objeto detectado: ancho=%dpx dist=%.1fcm\n", 
                    deteccion.ancho, distancia);
        lastDebug = millis();
    }
    #endif

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
    DEBUG_PRINTLN("[VISION] Rango de color actualizado");
}

ColorRange CameraVision::getColorRange() {
    ColorRange range;
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        range = colorObstaculo;
        xSemaphoreGive(dataMutex);
    }
    return range;
}
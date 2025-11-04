#pragma once

#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Undef FreeRTOS EPS macro que conflictúa con OpenCV
#ifdef EPS
#undef EPS
#endif

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

struct ColorRange {
    uint8_t r_min, r_max;
    uint8_t g_min, g_max;
    uint8_t b_min, b_max;
};

struct DetectionResult {
    bool encontrado;
    int centroide_x;
    int centroide_y;
    int ancho;
    int alto;
    int area;
};

class CameraVision {
public:
    bool setup();
    void run();
    
    bool getRiesgoLocal();
    float getDistanciaMinima();
    DetectionResult getUltimaDeteccion();
    
    void setColorRange(const ColorRange& range);
    ColorRange getColorRange();
    
    bool isCameraInitialized() { return cameraInitialized; }

    // Función para dibujar rectángulo en RGB565
    void draw_rectangle_rgb565(camera_fb_t* fb, const cv::Rect& rect, uint16_t color);

private:
    bool cameraInitialized;
    float distanciaMinimaLocal;
    bool riesgoLocal;
    DetectionResult ultimaDeteccion;
    ColorRange colorObstaculo;
    
    SemaphoreHandle_t dataMutex;
    
    void rgb565ToRgb(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b);
    bool estaEnRangoColor(uint8_t r, uint8_t g, uint8_t b, const ColorRange& range);
    
    void detectarObjeto(camera_fb_t* fb, DetectionResult& resultado);
    float calcularDistancia(float anchoEnPixeles);
    
    void detectarObjetoRGB565(camera_fb_t* fb, DetectionResult& resultado);
};

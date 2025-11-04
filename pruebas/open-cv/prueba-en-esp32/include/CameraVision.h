#pragma once

#include <Arduino.h>
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "config.h"
#include <opencv2/opencv.hpp>

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
    
#ifdef DETECTION_JPEG
    static int jpegDecodeCount;
    static int jpegSumX;
    static int jpegSumY;
    static int jpegMinX;
    static int jpegMaxX;
    static int jpegMinY;
    static int jpegMaxY;
    static ColorRange jpegColorRange;
    
    static bool jpegOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
    void detectarObjetoJPEG(camera_fb_t* fb, DetectionResult& resultado);
#else
    void detectarObjetoRGB565(camera_fb_t* fb, DetectionResult& resultado);
#endif
};
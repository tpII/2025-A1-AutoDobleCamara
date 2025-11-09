#pragma once

#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#ifdef EPS
#undef EPS
#endif

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

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
    CameraVision();
    bool initCamera();
    void deinitCamera();
    camera_fb_t* getFrame();
    void returnFrame();
    
    // Métodos para la lógica de la cámara
    bool setup();
    void run();

    // Métodos para obtener datos
    bool getRiesgoLocal();
    float getDistanciaMinima();
    DetectionResult getUltimaDeteccion();
    
    bool isCameraInitialized() { return cameraInitialized; }

private:
    bool cameraInitialized;
    float distanciaMinimaLocal;
    bool riesgoLocal;
    DetectionResult ultimaDeteccion;

    SemaphoreHandle_t dataMutex;

    // Métodos privados de procesamiento
    bool procesarFrame();
    float calcularDistancia(float anchoEnPixeles);    
    void draw_rectangle_rgb565(camera_fb_t* fb, const cv::Rect& rect, uint16_t color);
};

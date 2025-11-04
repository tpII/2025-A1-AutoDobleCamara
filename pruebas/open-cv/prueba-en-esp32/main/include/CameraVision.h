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
    bool setup();
    void run();
    
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
    float calcularDistancia(float anchoEnPixeles);    
};

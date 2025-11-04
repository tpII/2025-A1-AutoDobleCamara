// ESP-IDF headers
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

// Proyecto headers
#include "config.h"
#include "CameraVision.h"

static const char *TAG = "MAIN";

// Objetos globales
CameraVision cameraVision;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "\n=== Iniciando sistema ===");
    
    // 1. Inicializar NVS (necesario para WiFi y configuración)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS inicializado");
    
    // 2. Esperar un poco para estabilización
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 3. Inicializar cámara con OpenCV
    if (cameraVision.setup()) {
        ESP_LOGI(TAG, "Cámara OK");
        
        // Configurar rango de color para detección
        ColorRange rango;
        rango.r_min = 0; rango.r_max = 90;
        rango.g_min = 120; rango.g_max = 255;
        rango.b_min = 0; rango.b_max = 90;
        cameraVision.setColorRange(rango);
        ESP_LOGI(TAG, "Rango de color configurado (verde)");
    } else {
        ESP_LOGE(TAG, "Error al inicializar cámara");
    }
    
    ESP_LOGI(TAG, "=== Sistema listo ===\n");
    
    // Loop principal - procesar frames de cámara
    while(1) {
        cameraVision.run();
        
        // Obtener información de detección
        DetectionResult det = cameraVision.getUltimaDeteccion();
        if (det.encontrado) {
            float dist = cameraVision.getDistanciaMinima();
            bool riesgo = cameraVision.getRiesgoLocal();
            
            static uint32_t lastPrint = 0;
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (now - lastPrint > 2000) {  // Imprimir cada 2 segundos
                ESP_LOGI(TAG, "🎯 Objeto: Centro(%d,%d) Tamaño(%dx%d) Dist=%.1fcm Riesgo=%s",
                         det.centroide_x, det.centroide_y,
                         det.ancho, det.alto,
                         dist,
                         riesgo ? "⚠️ SÍ" : "✓ NO");
                lastPrint = now;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  // 20 FPS aprox
    }
}

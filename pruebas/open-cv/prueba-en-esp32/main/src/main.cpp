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
#include "Network.h"

static const char *TAG = "MAIN";

// Objetos globales
CameraVision cameraVision;
NetworkManager networkManager;

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "\n=== Iniciando sistema ===");
    
    // 1. Esperar un poco para estabilización
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 2. Inicializar cámara con OpenCV
    if (cameraVision.setup()) {
        ESP_LOGI(TAG, "✓ Cámara OK");
        
        // Hacer disponible globalmente para Network
        g_cameraVision = &cameraVision;
        
        // Configurar rango de color para detección (verde)
        ColorRange rango;
        rango.r_min = 0; rango.r_max = 90;
        rango.g_min = 120; rango.g_max = 255;
        rango.b_min = 0; rango.b_max = 90;
        cameraVision.setColorRange(rango);
        ESP_LOGI(TAG, "✓ Rango de color configurado (verde)");
    } else {
        ESP_LOGE(TAG, "✗ Error al inicializar cámara");
    }
    
    // 3. Inicializar red y servidor web
    if (networkManager.setup()) {
        ESP_LOGI(TAG, "✓ Red y servidor web OK");
        
        char ip[16];
        networkManager.getIP(ip, sizeof(ip));
        ESP_LOGI(TAG, "📡 Accede a: http://%s", ip);
    } else {
        ESP_LOGW(TAG, "⚠️  Red no disponible (continuando solo con cámara)");
    }
    
    ESP_LOGI(TAG, "=== Sistema listo ===\n");
    
    // Loop principal - procesar frames de cámara
    uint32_t frame_count = 0;
    while(1) {
        cameraVision.run();
        frame_count++;
        
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
        
        vTaskDelay(pdMS_TO_TICKS(50));  // ~20 FPS
    }
}

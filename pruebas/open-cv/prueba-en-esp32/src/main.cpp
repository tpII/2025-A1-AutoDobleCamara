/**
 * @file main.cpp
 * @brief Firmware principal del Autito con Sistema de Veto Distribuido
 * 
 * Este firmware implementa:
 * - Detección de obstáculos verdes con OpenCV
 * - Control de motores para navegación
 * - Recepción de comandos vía ESP-NOW desde cámara fija
 * - Lógica de fusión de decisiones (riesgo local + riesgo global)
 * - Stream de video MJPEG con anotaciones de debug
 * - Modo de prueba para desarrollo sin la cámara fija
 * 
 * Arquitectura: Sistema de Veto Distribuido
 * - El autito actúa como "Decisor Final"
 * - Solo avanza si: comandoUsuario==AVANZAR && !riesgoLocal && !riesgoGlobal
 */

#include <Arduino.h>
#include "config.h"
#include "MotorControl.h"
#include "CameraVision.h"
#include "Network.h"

// Variables de estado actualizadas por ESP-NOW (o por /test_control en modo prueba)
volatile int g_comandoUsuario = CMD_DETENER;
volatile bool g_hayRiesgoGlobal = false;

// Instancias de los módulos principales
MotorController motorController;
CameraVision cameraVision;
NetworkManager networkManager;

// Tarea FreeRTOS para el sistema de visión
TaskHandle_t visionTaskHandle = NULL;


#ifndef MODO_PRUEBA_SIN_COMPANERO
/**
 * @brief Callback ejecutado cuando se reciben datos por ESP-NOW
 * @param mac Dirección MAC del emisor
 * @param incomingData Datos recibidos
 * @param len Longitud de los datos
 * 
 * Este callback actualiza las variables globales g_comandoUsuario y g_hayRiesgoGlobal
 * con los datos enviados por la cámara fija.
 */
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    DEBUG_PRINTF("[ESP-NOW] Datos recibidos - %d bytes desde: %02X:%02X:%02X:%02X:%02X:%02X\n",
                len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    if (len == sizeof(struct_ComandoRemoto)) {
        struct_ComandoRemoto comandoRecibido;
        memcpy(&comandoRecibido, incomingData, sizeof(struct_ComandoRemoto));
        
        // Actualizar variables globales (volatile para thread-safety básico)
        g_comandoUsuario = comandoRecibido.comando;
        g_hayRiesgoGlobal = comandoRecibido.hayRiesgoGlobal;
        
        DEBUG_PRINTF("[ESP-NOW] Comando: %d, Riesgo Global: %s\n",
                    g_comandoUsuario, g_hayRiesgoGlobal ? "SI" : "NO");
    } else {
        DEBUG_PRINTLN("[ESP-NOW] Error: Tamaño de datos incorrecto");
    }
}
#else
// En modo prueba, el callback no se usa (los datos se reciben por HTTP)
void OnDataRecv(const uint8_t *mac, int len) {
    // No hace nada en modo prueba
}
#endif

/**
 * @brief Tarea que ejecuta continuamente el sistema de visión
 * @param pvParameters Parámetros de la tarea (no utilizado)
 * 
 * Esta tarea corre en un core separado para maximizar el rendimiento.
 * Captura frames, detecta obstáculos y actualiza el estado de riesgo local.
 */
void visionTask(void *pvParameters) {
    DEBUG_PRINTLN("[VISION TASK] Iniciada");
    
    while (true) {
        // Ejecutar procesamiento de visión
        cameraVision.run();
        
        // Pequeño delay para no saturar el CPU
        vTaskDelay(pdMS_TO_TICKS(50));  // ~20 FPS
    }
}

void ejecutarLogicaDeFusion() {
    // Leer estados actuales
    int comandoUsuario = g_comandoUsuario;
    bool riesgoGlobal = g_hayRiesgoGlobal;
    bool riesgoLocal = cameraVision.getRiesgoLocal();
    
    // Determinar comando final
    int comandoFinal = comandoUsuario;
    
    // Aplicar vetos solo si el comando es AVANZAR
    if (comandoUsuario == CMD_AVANZAR) {
        if (riesgoLocal || riesgoGlobal) {
            comandoFinal = CMD_DETENER;
            
            #ifdef DEBUG_SERIAL
            if (riesgoLocal && riesgoGlobal) {
                DEBUG_PRINTLN("[FUSION] VETO DOBLE - Deteniendo (riesgo local + global)");
            } else if (riesgoLocal) {
                DEBUG_PRINTLN("[FUSION] VETO LOCAL - Deteniendo (obstaculo cerca)");
            } else {
                DEBUG_PRINTLN("[FUSION] VETO GLOBAL - Deteniendo (alerta de vigilante)");
            }
            #endif
        }
    }
    
    // Log de estado (cada 2 segundos)
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 2000) {
        float distancia = cameraVision.getDistanciaMinima();
        DEBUG_PRINTF("[FUSION] CMD=%d, Local=%s (%.1fcm), Global=%s -> Final=%d\n",
                    comandoUsuario,
                    riesgoLocal ? "RIESGO" : "OK",
                    distancia,
                    riesgoGlobal ? "RIESGO" : "OK",
                    comandoFinal);
        lastLog = millis();
    }
    
    // Ejecutar comando final en los motores
    motorController.controlarComando(comandoFinal);
}

/**
 * @brief Inicialización del sistema
 */
void setup() {
    // Inicializar comunicación serial
    #ifdef DEBUG_SERIAL
    Serial.begin(DEBUG_BAUD_RATE);
    while (!Serial && millis() < 3000) {
        delay(10);  // Esperar hasta 3 segundos por Serial
    }
    #endif
    
    DEBUG_PRINTLN("\n");
    DEBUG_PRINTLN("╔════════════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   AUTITO ROBOT - Sistema de Veto Distribuido  ║");
    DEBUG_PRINTLN("║   Firmware ESP32 con OpenCV + ESP-NOW         ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════════════╝");
    DEBUG_PRINTLN("");
    
    // Mostrar configuración actual
    #ifdef MODO_PRUEBA_SIN_COMPANERO
    DEBUG_PRINTLN("[CONFIG] MODO: Prueba sin companero (ESP-NOW deshabilitado)");
    #else
    DEBUG_PRINTLN("[CONFIG] MODO: Produccion (ESP-NOW habilitado)");
    #endif
    
    DEBUG_PRINTF("[CONFIG] Resolucion: QVGA (320x240)\n");
    DEBUG_PRINTF("[CONFIG] Umbral riesgo local: %.1f cm\n", UMBRAL_RIESGO_LOCAL_CM);
    DEBUG_PRINTF("[CONFIG] Ancho obstaculo: %.1f cm\n", ANCHO_OBSTACULO_CM);
    DEBUG_PRINTF("[CONFIG] Focal: %.1f px\n", FOCAL_AUTITO_PX);
    DEBUG_PRINTLN("");
    
    // 1. Inicializar controlador de motores
    DEBUG_PRINTLN("[SETUP] Inicializando motores...");
    motorController.setup();
    
    // 2. Inicializar red PRIMERO (WiFi AP + ESP-NOW + Servidor Web)
    DEBUG_PRINTLN("[SETUP] Inicializando red...");
    networkManager.setup(OnDataRecv);
    DEBUG_PRINTLN("[SETUP] Red inicializada - AP disponible");
    
    // 3. Inicializar sistema de visión
    DEBUG_PRINTLN("[SETUP] Inicializando sistema de vision...");
    bool cameraOk = cameraVision.setup();
    
    if (!cameraOk) {
        DEBUG_PRINTLN("[SETUP] Error al inicializar la camara");
        DEBUG_PRINTLN("[SETUP] El sistema continuara sin vision");
        DEBUG_PRINTLN("[SETUP] Intenta acceder al stream para reinicializar");
        g_cameraVision = nullptr;
    } else {
        DEBUG_PRINTLN("[SETUP] Camara inicializada correctamente");
        g_cameraVision = &cameraVision;
    }
    
    // 4. Crear tarea de visión en el segundo core (solo si la cámara está OK)
    if (cameraOk) {
        DEBUG_PRINTLN("[SETUP] Creando tarea de vision...");
        xTaskCreatePinnedToCore(
            visionTask,
            "VisionTask",
            VISION_TASK_STACK_SIZE,
            NULL,
            VISION_TASK_PRIORITY,
            &visionTaskHandle,
            VISION_TASK_CORE
        );
        
        if (visionTaskHandle == NULL) {
            DEBUG_PRINTLN("[SETUP] Error al crear tarea de vision");
        } else {
            DEBUG_PRINTLN("[SETUP] Tarea de vision creada exitosamente");
        }
    } else {
        DEBUG_PRINTLN("[SETUP] Tarea de vision no creada (camara no disponible)");
    }
    
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("╔════════════════════════════════════════════════╗");
    DEBUG_PRINTLN("║           SISTEMA INICIALIZADO                 ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════════════╝");
    DEBUG_PRINTLN("");
    
    // Mostrar información de memoria
    DEBUG_PRINTF("[INFO] Heap libre: %u bytes (%.1f KB)\n", 
                esp_get_free_heap_size(), 
                esp_get_free_heap_size() / 1024.0);
    DEBUG_PRINTF("[INFO] Heap minimo historico: %u bytes\n", 
                esp_get_minimum_free_heap_size());
    DEBUG_PRINTLN("");
    
    // Estado inicial: detenido
    motorController.detener();
}

/**
 * @brief Loop principal del sistema
 * 
 * Ejecuta:
 * 1. Procesamiento de clientes web (stream + endpoints)
 * 2. Lógica de fusión de decisiones
 * 3. Monitoreo de memoria (opcional)
 */
void loop() {
    // 1. Manejar clientes web (stream de video, control de prueba, etc.)
    networkManager.handleClient();
    
    // 2. Ejecutar lógica de fusión y control de motores
    ejecutarLogicaDeFusion();
    
    // 3. Monitoreo de memoria (cada 10 segundos)
    #ifdef DEBUG_SERIAL
    static unsigned long lastMemoryCheck = 0;
    if (millis() - lastMemoryCheck > 10000) {
        uint32_t freeHeap = esp_get_free_heap_size();
        uint32_t minHeap = esp_get_minimum_free_heap_size();
        
        DEBUG_PRINTLN("─────────────────────────────────────────");
        DEBUG_PRINTF("[MEMORIA] Heap libre: %u bytes (%.1f KB)\n", freeHeap, freeHeap / 1024.0);
        DEBUG_PRINTF("[MEMORIA] Heap minimo: %u bytes (%.1f KB)\n", minHeap, minHeap / 1024.0);
        
        if (freeHeap < 30000) {
            DEBUG_PRINTLN("[MEMORIA] Advertencia: Heap bajo");
        }
        
        DEBUG_PRINTLN("─────────────────────────────────────────");
        lastMemoryCheck = millis();
    }
    #endif
    
    // Pequeño delay para no saturar el loop
    delay(10);
}
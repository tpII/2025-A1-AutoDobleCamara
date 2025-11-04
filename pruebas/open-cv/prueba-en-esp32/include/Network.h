#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "esp_now.h"
#include "esp_camera.h"
#include "config.h"

/**
 * @file Network.h
 * @brief Gestor de red para WiFi, ESP-NOW y streaming de video
 * 
 * Esta clase maneja:
 * - Configuración del Access Point WiFi
 * - Comunicación ESP-NOW para recibir comandos de la cámara fija
 * - Servidor web para streaming de video MJPEG
 * - Endpoint de prueba para desarrollo sin la cámara fija
 */

/**
 * @brief Estructura de datos para comunicación ESP-NOW
 * IMPORTANTE: Esta estructura debe ser idéntica en la cámara fija
 */
typedef struct struct_ComandoRemoto {
    int comando;          ///< Código del comando (CMD_DETENER, CMD_AVANZAR, etc.)
    bool hayRiesgoGlobal; ///< Veto de la cámara fija (true = detener)
} struct_ComandoRemoto;

class NetworkManager {
public:
    /**
     * @brief Inicializa el sistema de red
     * @param onDataRecv Callback para procesar datos recibidos por ESP-NOW
     *                   Firma: void callback(const uint8_t *mac, int len)
     */
    void setup(void (*onDataRecv)(const uint8_t *, int));

    /**
     * @brief Procesa las conexiones de clientes web
     * Debe llamarse en el loop principal
     */
    void handleClient();

    /**
     * @brief Retorna el servidor WiFi
     */
    WiFiServer* getServer();

private:
    WiFiServer* server;    ///< Servidor web para streaming

    /**
     * @brief Configura el WiFi en modo Access Point
     */
    void setupWiFiAP();

    /**
     * @brief Configura el WiFi en modo Cliente (conecta a red existente)
     */
    void setupWiFiClient();

    /**
     * @brief Configura ESP-NOW para recibir comandos
     * @param onDataRecv Callback para datos recibidos
     */
    void setupESPNOW(void (*onDataRecv)(const uint8_t *, int));

    /**
     * @brief Sirve la página principal con enlaces al stream
     * @param client Cliente web conectado
     */
    void serveIndexPage(WiFiClient& client);

    /**
     * @brief Sirve el stream MJPEG de video
     * @param client Cliente web conectado
     */
    void serveVideoStream(WiFiClient& client);

    /**
     * @brief Sirve el stream procesado (binario: verde/azul=blanco)
     * @param client Cliente web conectado
     */
    void serveProcessedStream(WiFiClient& client);
    
    /**
     * @brief Procesa frame a imagen binaria
     * @param fb Frame buffer de entrada
     * @param output Buffer de salida (grayscale)
     */
    void processBinaryImage(camera_fb_t* fb, uint8_t* output);

    /**
     * @brief Endpoint para apagar el sistema y liberar memoria
     * Desinicializa cámara, WiFi y reinicia el ESP32
     * @param client Cliente web conectado
     */
    void serveShutdown(WiFiClient& client);
};

// Puntero global al sistema de visión (se configurará en main.cpp)
extern class CameraVision* g_cameraVision;

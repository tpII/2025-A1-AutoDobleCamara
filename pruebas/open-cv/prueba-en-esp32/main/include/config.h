#pragma once

#include "esp_log.h"

// --- MODO DE OPERACION ---
#define MODO_PRUEBA_SIN_COMPANERO

// --- MODO DE RED ---
// Descomentar UNA de las siguientes opciones:
// #define ACCESS_POINT_MODE       // Modo AP: Crea red propia
#define CONNECT_TO_NETWORK   // Modo STA: Conecta a red existente

// --- PINES DE LA CÁMARA (ESP32-S3) ---
#define CAM_PIN_PWDN    -1 // No se usa
#define CAM_PIN_RESET   -1 // No se usa
#define CAM_PIN_XCLK    -1
#define CAM_PIN_SDA    4  // SDA
#define CAM_PIN_SCL    5  // SCL

// Pines de datos (Bus de 8 bits)
#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      11
#define CAM_PIN_D2      10
#define CAM_PIN_D1      9
#define CAM_PIN_D0      8

// Pines de sincronización
#define CAM_PIN_VSYNC   7
#define CAM_PIN_HREF    13
#define CAM_PIN_PCLK    14

// --- PINES DE CONTROL DE MOTORES (L298N) ---
#define MOTOR_A_IN1    19
#define MOTOR_A_IN2    20
#define MOTOR_A_ENA    21
#define MOTOR_B_IN3    47
#define MOTOR_B_IN4    48
#define MOTOR_B_ENB    45

#define MOTOR_PWM_CHANNEL_A 0
#define MOTOR_PWM_CHANNEL_B 1
#define MOTOR_PWM_FREQ      1000
#define MOTOR_PWM_RESOLUTION 8

#define MOTOR_VELOCIDAD_AVANZAR     200
#define MOTOR_VELOCIDAD_RETROCEDER  180
#define MOTOR_VELOCIDAD_GIRAR       150

// --- PARÁMETROS DE VISIÓN ---
#define RESOLUCION_FRAME        FRAMESIZE_SVGA
#define FORMATO_PIXEL           PIXFORMAT_RGB565
#define CALIDAD_JPEG            11

#define FOCAL_AUTITO_PX         312.5
#define ANCHO_OBSTACULO_CM      5.0
#define UMBRAL_RIESGO_LOCAL_CM  25.0
#define AREA_MINIMA_CONTORNO    200

#define OBSTACULO_H_MIN 35
#define OBSTACULO_H_MAX 85
#define OBSTACULO_S_MIN 100
#define OBSTACULO_S_MAX 255
#define OBSTACULO_V_MIN 100
#define OBSTACULO_V_MAX 255

#define VISION_TASK_STACK_SIZE  8192
#define VISION_TASK_PRIORITY    1
#define VISION_TASK_CORE        0

// --- PARÁMETROS DE RED ---
#ifdef ACCESS_POINT_MODE
    #define WIFI_AP_SSID      "AUTITO_ROBOT_AP"
    #define WIFI_AP_PASSWORD  "12345678"
    #define WIFI_AP_CHANNEL   1
    #define WIFI_AP_MAX_CONN  4
#endif

#ifdef CONNECT_TO_NETWORK
    #define WIFI_STA_SSID     "Personal-140-2.4GHz"
    #define WIFI_STA_PASSWORD "00417225972"
    #define WIFI_CONNECT_TIMEOUT 20000  // 20 segundos
#endif

#define WEB_SERVER_PORT   80
#define STREAM_PATH       "/stream"
#define TEST_CONTROL_PATH "/test_control"

#define ESPNOW_CHANNEL    1

// --- ESTADOS DE COMANDO ---
#define CMD_DETENER     0
#define CMD_AVANZAR     1
#define CMD_ATRAS       2
#define CMD_IZQUIERDA   3
#define CMD_DERECHA     4

// --- CONFIGURACIÓN DE DEBUG (usando ESP_LOG) ---
#define DEBUG_TAG "AUTITO"
#define DEBUG_PRINT(x)    ESP_LOGI(DEBUG_TAG, "%s", x)
#define DEBUG_PRINTLN(x)  ESP_LOGI(DEBUG_TAG, "%s", x)
#define DEBUG_PRINTF(...) ESP_LOGI(DEBUG_TAG, __VA_ARGS__)

// --- MATRIZ DE CALIBRACIÓN ---
#define CAM_MTX_00 312.5f
#define CAM_MTX_01 0.0f
#define CAM_MTX_02 160.0f
#define CAM_MTX_10 0.0f
#define CAM_MTX_11 312.5f
#define CAM_MTX_12 120.0f
#define CAM_MTX_20 0.0f
#define CAM_MTX_21 0.0f
#define CAM_MTX_22 1.0f

#define CAM_DIST_K1  0.0f
#define CAM_DIST_K2  0.0f
#define CAM_DIST_P1  0.0f
#define CAM_DIST_P2  0.0f
#define CAM_DIST_K3  0.0f

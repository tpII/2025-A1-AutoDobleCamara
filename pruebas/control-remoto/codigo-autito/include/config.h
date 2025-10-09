#pragma once
#include <stdint.h>

/*
 * Configuración del dispositivo (editar antes de compilar)
 *
 * - WIFI_SSID / WIFI_PASS: credenciales de la red Wi‑Fi.
 * - SERVER_IP / SERVER_PORT: destino TCP al que se conectará comm.cpp.
 * - SERIAL_BAUD: velocidad del puerto serie (coincidir con Serial.begin()).
 * - ENABLE_DIAGNOSTIC: 1 para ejecutar test al arranque, 0 para omitir.
 *
 * Nota: no dupliques pines ni parámetros PWM aquí si ya están definidos en motor.h.
 */

// Red / servidor
#ifndef WIFI_SSID
#define WIFI_SSID      "access-point"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS      "12345678"
#endif

#ifndef SERVER_IP
#define SERVER_IP      "10.42.0.1"  // IP o hostname del servidor TCP
#endif

#ifndef SERVER_PORT
#define SERVER_PORT    12345
#endif

// IP estática del ESP32 (opcional - deja en blanco para DHCP)
#ifndef ESP_STATIC_IP
#define ESP_STATIC_IP  "10.42.0.100"  // IP fija del ESP32
#endif

#ifndef ESP_GATEWAY
#define ESP_GATEWAY    "10.42.0.1"    // Gateway 
#endif

#ifndef ESP_SUBNET
#define ESP_SUBNET     "255.255.255.0"
#endif

// Serial / diagnóstico
#ifndef SERIAL_BAUD
#define SERIAL_BAUD    115200
#endif

// Camera defaults (OV2640 wiring - adaptado a tu conexión)
#ifndef CAMERA_PIN_PWDN
#define CAMERA_PIN_PWDN     32
#endif
#ifndef CAMERA_PIN_RESET
#define CAMERA_PIN_RESET    -1
#endif
#ifndef CAMERA_PIN_XCLK
#define CAMERA_PIN_XCLK     -1
#endif
#ifndef CAMERA_PIN_SIOD
#define CAMERA_PIN_SIOD     21 // SDA
#endif
#ifndef CAMERA_PIN_SIOC
#define CAMERA_PIN_SIOC     22 // SCL
#endif
#ifndef CAMERA_PIN_D7
#define CAMERA_PIN_D7       36
#endif
#ifndef CAMERA_PIN_D6
#define CAMERA_PIN_D6       39
#endif
#ifndef CAMERA_PIN_D5
#define CAMERA_PIN_D5       34
#endif
#ifndef CAMERA_PIN_D4
#define CAMERA_PIN_D4       35
#endif
#ifndef CAMERA_PIN_D3
#define CAMERA_PIN_D3       27
#endif
#ifndef CAMERA_PIN_D2
#define CAMERA_PIN_D2       19
#endif
#ifndef CAMERA_PIN_D1
#define CAMERA_PIN_D1       18
#endif
#ifndef CAMERA_PIN_D0
#define CAMERA_PIN_D0       5
#endif
#ifndef CAMERA_PIN_VSYNC
#define CAMERA_PIN_VSYNC    25
#endif
#ifndef CAMERA_PIN_HREF
#define CAMERA_PIN_HREF     23
#endif
#ifndef CAMERA_PIN_PCLK
#define CAMERA_PIN_PCLK     26
#endif

// Timeouts / reintentos (en ms)
#ifndef CONNECT_RETRY_MS
#define CONNECT_RETRY_MS 3000u
#endif

// Límites útiles (puedes ajustarlos si los migras desde otros ficheros)
#ifndef NETBUF_MAXLEN
#define NETBUF_MAXLEN 200
#endif

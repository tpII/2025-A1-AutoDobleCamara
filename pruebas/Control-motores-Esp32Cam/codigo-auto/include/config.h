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
#define WIFI_SSID      "ESP32_AP"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS      "12345678"
#endif

#ifndef SERVER_IP
#define SERVER_IP      "192.168.4.1"  // IP del ESP32-CAM (servidor)
#endif

#ifndef SERVER_PORT
#define SERVER_PORT    12345  // Puerto donde el ESP32-CAM escucha
#endif

// Serial / diagnóstico
#ifndef SERIAL_BAUD
#define SERIAL_BAUD    9600
#endif

// Timeouts / reintentos (en ms)
#ifndef CONNECT_RETRY_MS
#define CONNECT_RETRY_MS 3000u
#endif

// Límites útiles (puedes ajustarlos si los migras desde otros ficheros)
#ifndef NETBUF_MAXLEN
#define NETBUF_MAXLEN 200
#endif

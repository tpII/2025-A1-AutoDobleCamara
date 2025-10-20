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

// Serial / diagnóstico
#ifndef SERIAL_BAUD
#define SERIAL_BAUD    115200  // Aumentado para mejor depuración
#endif

// Límites útiles
#ifndef NETBUF_MAXLEN
#define NETBUF_MAXLEN 200
#endif

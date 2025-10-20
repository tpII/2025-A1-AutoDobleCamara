#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <config.h>

// Inicializa la comunicación ESP-NOW
void commNetBegin();

// Procesa la comunicación ESP-NOW (debe llamarse en el loop)
void commNetLoop();

// Obtiene la dirección MAC del ESP32
void printMacAddress();
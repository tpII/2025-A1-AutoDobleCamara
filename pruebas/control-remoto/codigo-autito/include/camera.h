#pragma once
#include <Arduino.h>

// Inicializa la cámara (devuelve true si OK)
bool cameraInit();

// Inicia el servidor HTTP para streaming (puerto por defecto 8080)
void cameraServerBegin(uint16_t port = 8080);

// Llamar periódicamente desde loop() (no bloqueante)
void cameraLoop();

// URL del stream: http://<IP>:<port>/stream

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <config.h>

void commNetBegin(const char* ssid, const char* pass, const char* serverIp, uint16_t port);
void commNetLoop();
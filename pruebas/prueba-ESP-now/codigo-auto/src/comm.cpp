#include <comm.h>
#include "config.h"

// Declaración de la función definida en main.cpp para procesar comandos
extern void processLine(String line);

// Buffer para recibir datos
static String netBuf;

// Callback cuando recibimos datos por ESP-NOW
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    char command[NETBUF_MAXLEN + 1];
    if (data_len > NETBUF_MAXLEN) {
        data_len = NETBUF_MAXLEN;
    }
    memcpy(command, data, data_len);
    command[data_len] = '\0';
    
    // Convertir a String y procesar
    String cmdString = String(command);
    Serial.printf("Comando recibido: %s\n", cmdString.c_str());
    processLine(cmdString);
}

void commNetBegin() {
    // Configurar ESP32 en modo station
    WiFi.mode(WIFI_STA);

    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    // Registrar callback para recepción de datos
    esp_now_register_recv_cb(OnDataRecv);

    // Mostrar la dirección MAC del dispositivo
    printMacAddress();
}

void printMacAddress() {
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.print("Dirección MAC del ESP32: ");
    for(int i = 0; i < 6; i++) {
        Serial.printf("%02X", macAddr[i]);
        if(i < 5) Serial.print(":");
    }
    Serial.println();
}

void commNetLoop() {
    // ESP-NOW maneja la comunicación a través de interrupciones
    // No necesitamos hacer nada aquí
    delay(1);
}
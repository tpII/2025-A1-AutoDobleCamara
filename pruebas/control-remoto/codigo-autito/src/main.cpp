#include <Arduino.h>
#include <config.h>
#include <motor.h>
#include <comm.h>
#include <vector>
#include <camera.h>
#include <esp_camera.h>

// Protocolo y parser (idéntico al que ya tenías; usa API pública del módulo motor)
void processLine(String line) {
    line.trim();
    if (line.length() == 0) return;

    if (line.equalsIgnoreCase("STOP")) {
        stopAll();
        Serial.println("ACK STOP");
        return;
    }
    if (line.equalsIgnoreCase("PING")) {
        Serial.println("PONG");
        return;
    }

    // Tokenizar
    std::vector<String> parts;
    int idx = 0;
    while (true) {
        int sp = line.indexOf(' ', idx);
        if (sp == -1) {
            parts.push_back(line.substring(idx));
            break;
        }
        parts.push_back(line.substring(idx, sp));
        idx = sp + 1;
    }

    if (parts.size() == 3 && parts[0].equalsIgnoreCase("ALL")) {
        bool fwd = (parts[1].equalsIgnoreCase("F"));
        int spd = parts[2].toInt();
        spd = constrain(spd, 0, 255);
        setMotorTarget(1, spd, fwd);
        setMotorTarget(2, spd, fwd);
        Serial.printf("ACK ALL %s %d\n", fwd ? "F" : "R", spd);
        return;
    }
    if (parts.size() == 4 && parts[0].equalsIgnoreCase("MOTOR")) {
        int m = parts[1].toInt();
        bool fwd = (parts[2].equalsIgnoreCase("F"));
        int spd = parts[3].toInt();
        spd = constrain(spd, 0, 255);
        if (m == 1 || m == 2) {
            setMotorTarget(m, spd, fwd);
            Serial.printf("ACK MOTOR %d %s %d\n", m, fwd ? "F" : "R", spd);
        } else {
            Serial.println("ERR MOTOR numero invalido");
        }
        return;
    }

    Serial.println("ERR comando desconocido");
}

camera_config_t config;

// Buffer para entrada por Serial
static String inputBuffer = "";
static bool cameraServerStarted = false;

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 2000) ; // tiempo para abrir monitor serie
    Serial.println("Iniciando: software-auto");

    // ⚠️ IMPORTANTE: Inicializar cámara ANTES que motores 
    // para evitar conflictos en GPIO 0
    if (cameraInit()) {
        Serial.println("✅ Cámara inicializada - esperando WiFi para arrancar servidor");
    } else {
        Serial.println("❌ Error: cameraInit() failed - camera disabled");
    }

    // Pequeña pausa antes de inicializar otros periféricos
    delay(100);
    
    motorInit();
    commNetBegin(WIFI_SSID, WIFI_PASS, SERVER_IP, SERVER_PORT);
}


void loop() {
    // --- manejo red ---
    commNetLoop();

    // Iniciar servidor de cámara solo DESPUÉS de que WiFi esté conectado
    if (WiFi.status() == WL_CONNECTED && !cameraServerStarted && esp_camera_sensor_get() != nullptr) {
        Serial.println("🌐 WiFi conectado - iniciando servidor de cámara en puerto 8080");
        cameraServerBegin(8080);
        cameraServerStarted = true;
        Serial.printf("📺 Stream disponible en: http://%s:8080/stream\n", WiFi.localIP().toString().c_str());
    }

    // manejo cámara (no bloqueante)
    if (cameraServerStarted) {
        cameraLoop();
    }

    // --- avance rampa motores (no bloqueante; el módulo gestiona temporización interna) ---
    updateMotor();

    // pequeña pausa cooperativa
    delay(5);
}
#include <Arduino.h>
#include <config.h>
#include <motor.h>
#include <comm.h>
#include <vector>

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

// Buffer para entrada por Serial
static String inputBuffer = "";

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 2000) ; // tiempo para abrir monitor serie
    Serial.println("Iniciando: software-auto");


    motorInit();
    commNetBegin(WIFI_SSID, WIFI_PASS, SERVER_IP, SERVER_PORT);
}

void loop() {
    // --- manejo Serial (eco / backspace) ---
    // while (Serial.available()) {
    //     char c = Serial.read();
    //     if (c == '\r') continue;
    //     if (c == '\n') {
    //         Serial.println();
    //         processLine(inputBuffer);
    //         inputBuffer = "";
    //         continue;
    //     }
    //     if (c == 8 || c == 127) { // BS / DEL
    //         if (inputBuffer.length() > 0) {
    //             inputBuffer.remove(inputBuffer.length() - 1, 1);
    //             Serial.print("\b \b");
    //         }
    //         continue;
    //     }
    //     if (c >= 32 && c <= 126) {
    //         inputBuffer += c;
    //         Serial.print(c);
    //         if (inputBuffer.length() > NETBUF_MAXLEN) inputBuffer = inputBuffer.substring(inputBuffer.length() - NETBUF_MAXLEN);
    //     }
    // }

    // --- manejo red ---
    commNetLoop();

    // --- avance rampa motores (no bloqueante; el módulo gestiona temporización interna) ---
    updateMotor();

    // pequeña pausa cooperativa
    delay(5);
}
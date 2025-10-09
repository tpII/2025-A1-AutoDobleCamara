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

void setupCamera() {
    
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 27;
  config.pin_d4 = 35;
  config.pin_d5 = 34;
  config.pin_d6 = 39;
  config.pin_d7 = 36;
  config.pin_xclk = -1;      
  config.pin_pclk = 26;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 21; // SDA
  config.pin_sccb_scl = 22; // SCL
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.pixel_format = PIXFORMAT_JPEG;
  
  Serial.println("\n📸 Iniciando configuración de cámara...");
  Serial.printf("PSRAM encontrada: %s\n", psramFound() ? "SI" : "NO");
  Serial.println("🔧 Configuración: OPTIMIZADA para alta calidad");
  
  // Configuración mejorada - priorizar fluidez máxima
  config.frame_size = FRAMESIZE_QVGA;     // 320x240 - Mejor calidad
  config.jpeg_quality = 8;                // Calidad excelente (menor compresión)
  config.fb_count = 2;                    // Doble buffer para fluidez
  config.fb_location = CAMERA_FB_IN_DRAM; // Forzar DRAM para velocidad
  config.grab_mode = CAMERA_GRAB_LATEST;  // Siempre el frame más reciente
  config.xclk_freq_hz = 0;                // Sin clock externo

  esp_err_t err = esp_camera_init(&config);
  
  if (err == ESP_OK) {
    Serial.println("✅ ¡Cámara inicializada correctamente!");
    Serial.printf("📸 Configuración: QVGA (320x240), Calidad: %d, Buffers: %d\n", 
                  config.jpeg_quality, config.fb_count);
    
    // Configuración avanzada del sensor para máxima calidad
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
      s->set_brightness(s, 0);     // 0 = neutro
      s->set_contrast(s, 1);       // +1 para mejor definición
      s->set_saturation(s, 1);     // +1 para colores más vivos
      s->set_special_effect(s, 0); // Sin efectos
      s->set_whitebal(s, 1);       // Balance blancos automático
      s->set_awb_gain(s, 1);       // Ganancia AWB activa
      s->set_wb_mode(s, 0);        // Modo automático
      s->set_exposure_ctrl(s, 1);  // Exposición automática
      s->set_aec2(s, 1);           // Algoritmo AEC mejorado
      s->set_gain_ctrl(s, 1);      // Ganancia automática
      s->set_agc_gain(s, 0);       // Ganancia base
      s->set_bpc(s, 1);            // Corrección píxeles defectuosos
      s->set_wpc(s, 1);            // Corrección píxeles blancos
      s->set_raw_gma(s, 1);        // Gamma activado
      s->set_lenc(s, 1);           // Corrección de lente
      s->set_hmirror(s, 0);        // Sin espejo horizontal
      s->set_vflip(s, 0);          // Sin volteo vertical
      s->set_dcw(s, 1);            // Windowing activado
      s->set_colorbar(s, 0);       // Sin barras de test
      Serial.println("⚙️ Configuración avanzada del sensor aplicada");
    }
    
    // Test de captura
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("❌ Error capturando imagen de prueba");
    } else {
      Serial.printf("✅ Imagen de prueba: %d bytes\n", fb->len);
      esp_camera_fb_return(fb);
    }
  } else {
    Serial.printf("❌ ERROR CRÍTICO: No se puede inicializar cámara: 0x%x\n", err);
    Serial.println("🔴 Incluso con configuración mínima falla");
    Serial.println("🔴 Verificar conexiones físicas de nuevo");
  }
}

// Buffer para entrada por Serial
static String inputBuffer = "";

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 2000) ; // tiempo para abrir monitor serie
    Serial.println("Iniciando: software-auto");

    motorInit();
    commNetBegin(WIFI_SSID, WIFI_PASS, SERVER_IP, SERVER_PORT);

    // Usar la configuración de cámara que proporcionaste
    setupCamera();

    // Si la cámara se inicializó correctamente, arrancar el servidor
    if (esp_camera_sensor_get() != nullptr) {
        cameraServerBegin(8080);
    } else {
        Serial.println("Warning: cameraInit() failed - camera disabled");
    }
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

    // manejo cámara (no bloqueante)
    cameraLoop();

    // --- avance rampa motores (no bloqueante; el módulo gestiona temporización interna) ---
    updateMotor();

    // pequeña pausa cooperativa
    delay(5);
}
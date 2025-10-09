#include <motor.h>




static uint16_t targetPWM1 = 0, targetPWM2 = 0;
static uint16_t currentPWM1 = 0, currentPWM2 = 0;
static uint16_t desiredTargetPWM1 = 0, desiredTargetPWM2 = 0; // objetivo real después de conmutar dirección
static bool dir1Forward = true, dir2Forward = true;
static bool pendingDirChange1 = false, pendingDirChange2 = false;
static bool pendingDirValue1 = true, pendingDirValue2 = true;
static uint32_t lastRampMillis = 0;


uint16_t linearizeSpeed(uint8_t in8);
void setMotorDirect(uint8_t motor, uint16_t pwm, bool forward);

// convierte 0..255 entrada a 0..MAX_PWM usando curva gamma
uint16_t linearizeSpeed(uint8_t in8) {
    if (in8 == 0) return 0;
    if (in8 >= 255) return (uint16_t)((1u << PWM_RESOLUTION) - 1u);
    float norm = (float)in8 / 255.0f;
    float out = powf(norm, GAMMA);
    uint16_t maxPWM = (uint16_t)((1u << PWM_RESOLUTION) - 1u);
    return (uint16_t)roundf(out * (float)maxPWM);
}

// escribe instantáneamente (usada por la rampa)
void setMotorDirect(uint8_t motor, uint16_t pwm, bool forward) {
    if (motor == 1) {
        // escribir dirección sólo si pwm == 0 o coincide con la dirección actual (para seguridad)
        if (pwm == 0 || forward == dir1Forward) {
            digitalWrite(MOTOR1_IN1_PIN, forward ? HIGH : LOW);
            digitalWrite(MOTOR1_IN2_PIN, forward ? LOW : HIGH);
        }
        ledcWrite(CHANNEL_M1, pwm);
    } else {
        if (pwm == 0 || forward == dir2Forward) {
            digitalWrite(MOTOR2_IN1_PIN, forward ? HIGH : LOW);
            digitalWrite(MOTOR2_IN2_PIN, forward ? LOW : HIGH);
        }
        ledcWrite(CHANNEL_M2, pwm);
    }
}

// establece objetivo (externamente usado por main)
void setMotorTarget(uint8_t motor, uint8_t speed8, bool forward) {
    uint16_t pwm = linearizeSpeed(speed8);
    if (motor == 1) {
        desiredTargetPWM1 = pwm;
        if (pwm == 0) {
            // petición de 0 => cancelar pendiente y aplicar 0
            pendingDirChange1 = false;
            targetPWM1 = 0;
        } else if (forward == dir1Forward) {
            // misma dirección: aplicar directamente
            pendingDirChange1 = false;
            targetPWM1 = pwm;
        } else {
            // cambio de dirección solicitado y velocidad > 0:
            // bajar a 0 primero y recordar nuevo valor y dirección
            pendingDirChange1 = true;
            pendingDirValue1 = forward;
            targetPWM1 = 0;
        }
    } else {
        desiredTargetPWM2 = pwm;
        if (pwm == 0) {
            pendingDirChange2 = false;
            targetPWM2 = 0;
        } else if (forward == dir2Forward) {
            pendingDirChange2 = false;
            targetPWM2 = pwm;
        } else {
            pendingDirChange2 = true;
            pendingDirValue2 = forward;
            targetPWM2 = 0;
        }
    }
}

// stopAll ahora pone objetivos a 0 (rampa hará el resto) y cancela pendientes
void stopAll() {
    targetPWM1 = 0;
    targetPWM2 = 0;
    desiredTargetPWM1 = desiredTargetPWM2 = 0;
    pendingDirChange1 = pendingDirChange2 = false;
}

// Inicializa pines y canales LEDC
void motorInit() {
    pinMode(MOTOR1_IN1_PIN, OUTPUT);
    pinMode(MOTOR1_IN2_PIN, OUTPUT);
    pinMode(MOTOR2_IN1_PIN, OUTPUT);
    pinMode(MOTOR2_IN2_PIN, OUTPUT);

   
    dir1Forward = true;
    dir2Forward = true;
    digitalWrite(MOTOR1_IN1_PIN, dir1Forward ? HIGH : LOW);
    digitalWrite(MOTOR1_IN2_PIN, dir1Forward ? LOW : HIGH);
    digitalWrite(MOTOR2_IN1_PIN, dir2Forward ? HIGH : LOW);
    digitalWrite(MOTOR2_IN2_PIN, dir2Forward ? LOW : HIGH);

    ledcSetup(CHANNEL_M1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR1_PWM_PIN, CHANNEL_M1);

    ledcSetup(CHANNEL_M2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR2_PWM_PIN, CHANNEL_M2);

    currentPWM1 = currentPWM2 = 0;
    targetPWM1 = targetPWM2 = 0;
    desiredTargetPWM1 = desiredTargetPWM2 = 0;
    lastRampMillis = millis();
    stopAll();
}

// Llamada periódica para mover current hacia target en pasos cada RAMP_INTERVAL_MS
void updateMotor() {
    uint32_t now = millis();
    if (now - lastRampMillis >= RAMP_INTERVAL_MS) {
        lastRampMillis = now;
        // motor 1
        if (currentPWM1 < targetPWM1) {
            uint16_t delta = targetPWM1 - currentPWM1;
            currentPWM1 += (uint16_t)min((uint16_t)RAMP_STEP, delta);
            if (currentPWM1 > targetPWM1) currentPWM1 = targetPWM1;
            setMotorDirect(1, currentPWM1, dir1Forward);
        } else if (currentPWM1 > targetPWM1) {
            uint16_t delta = currentPWM1 - targetPWM1;
            currentPWM1 -= (uint16_t)min((uint16_t)RAMP_STEP, delta);
            if (currentPWM1 < targetPWM1) currentPWM1 = targetPWM1;
            setMotorDirect(1, currentPWM1, dir1Forward);
        }

        // Si llegamos a 0 y hay cambio de dirección pendiente, aplicarlo ahora (PWM==0)
        if (currentPWM1 == 0 && pendingDirChange1) {
            dir1Forward = pendingDirValue1;
            setMotorDirect(1, 0, dir1Forward);
            pendingDirChange1 = false;
            targetPWM1 = desiredTargetPWM1;
        }

        // motor 2
        if (currentPWM2 < targetPWM2) {
            uint16_t delta = targetPWM2 - currentPWM2;
            currentPWM2 += (uint16_t)min((uint16_t)RAMP_STEP, delta);
            if (currentPWM2 > targetPWM2) currentPWM2 = targetPWM2;
            setMotorDirect(2, currentPWM2, dir2Forward);
        } else if (currentPWM2 > targetPWM2) {
            uint16_t delta = currentPWM2 - targetPWM2;
            currentPWM2 -= (uint16_t)min((uint16_t)RAMP_STEP, delta);
            if (currentPWM2 < targetPWM2) currentPWM2 = targetPWM2;
            setMotorDirect(2, currentPWM2, dir2Forward);
        }

        if (currentPWM2 == 0 && pendingDirChange2) {
            dir2Forward = pendingDirValue2;
            setMotorDirect(2, 0, dir2Forward);
            pendingDirChange2 = false;
            targetPWM2 = desiredTargetPWM2;
        }
    }
}





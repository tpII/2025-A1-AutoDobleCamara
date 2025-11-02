#include "MotorControl.h"
#include "config.h"
#include <Arduino.h>

/**
 * @brief Inicializa los pines GPIO y canales PWM para el control de motores
 */
void MotorController::setup() {
    // Configurar pines de dirección como salidas
    pinMode(MOTOR_A_IN1, OUTPUT);
    pinMode(MOTOR_A_IN2, OUTPUT);
    pinMode(MOTOR_B_IN3, OUTPUT);
    pinMode(MOTOR_B_IN4, OUTPUT);

    // Configurar canales PWM para control de velocidad
    ledcSetup(MOTOR_PWM_CHANNEL_A, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(MOTOR_PWM_CHANNEL_B, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    
    // Asociar canales PWM a pines
    ledcAttachPin(MOTOR_A_ENA, MOTOR_PWM_CHANNEL_A);
    ledcAttachPin(MOTOR_B_ENB, MOTOR_PWM_CHANNEL_B);

    // Inicializar motores detenidos
    detener();

    DEBUG_PRINTLN("[MOTOR] Controlador de motores inicializado");
}

/**
 * @brief Mueve ambos motores hacia adelante
 */
void MotorController::avanzar() {
    setMotorVelocidad(0, MOTOR_VELOCIDAD_AVANZAR);   // Motor A (Izquierdo)
    setMotorVelocidad(1, MOTOR_VELOCIDAD_AVANZAR);   // Motor B (Derecho)
    DEBUG_PRINTLN("[MOTOR] Avanzando");
}

/**
 * @brief Mueve ambos motores hacia atrás
 */
void MotorController::retroceder() {
    setMotorVelocidad(0, -MOTOR_VELOCIDAD_RETROCEDER);  // Motor A (Izquierdo)
    setMotorVelocidad(1, -MOTOR_VELOCIDAD_RETROCEDER);  // Motor B (Derecho)
    DEBUG_PRINTLN("[MOTOR] Retrocediendo");
}

/**
 * @brief Gira a la izquierda (motor derecho adelante, izquierdo atrás)
 */
void MotorController::girarIzquierda() {
    setMotorVelocidad(0, -MOTOR_VELOCIDAD_GIRAR);  // Motor A atrás
    setMotorVelocidad(1, MOTOR_VELOCIDAD_GIRAR);   // Motor B adelante
    DEBUG_PRINTLN("[MOTOR] Girando izquierda");
}

/**
 * @brief Gira a la derecha (motor izquierdo adelante, derecho atrás)
 */
void MotorController::girarDerecha() {
    setMotorVelocidad(0, MOTOR_VELOCIDAD_GIRAR);   // Motor A adelante
    setMotorVelocidad(1, -MOTOR_VELOCIDAD_GIRAR);  // Motor B atrás
    DEBUG_PRINTLN("[MOTOR] Girando derecha");
}

/**
 * @brief Detiene ambos motores
 */
void MotorController::detener() {
    setMotorVelocidad(0, 0);  // Motor A
    setMotorVelocidad(1, 0);  // Motor B
    DEBUG_PRINTLN("[MOTOR] Detenido");
}

/**
 * @brief Ejecuta un comando basado en el código recibido
 * @param comando Código del comando (definido en config.h)
 */
void MotorController::controlarComando(int comando) {
    // Variable estática para recordar el último comando ejecutado
    static int ultimoComando = -1;
    
    // Solo ejecutar y loguear si el comando cambió
    if (comando != ultimoComando) {
        switch(comando) {
            case CMD_AVANZAR:
                avanzar();
                break;
            case CMD_ATRAS:
                retroceder();
                break;
            case CMD_IZQUIERDA:
                girarIzquierda();
                break;
            case CMD_DERECHA:
                girarDerecha();
                break;
            case CMD_DETENER:
            default:
                detener();
                break;
        }
        ultimoComando = comando;
    }
}

/**
 * @brief Establece la velocidad y dirección de un motor individual
 * @param motor 0 = Motor A (Izquierdo), 1 = Motor B (Derecho)
 * @param velocidad Rango -255 a 255. Positivo = adelante, Negativo = atrás
 */
void MotorController::setMotorVelocidad(int motor, int velocidad) {
    // Limitar velocidad al rango válido
    if (velocidad > 255) velocidad = 255;
    if (velocidad < -255) velocidad = -255;

    int pwmValue = abs(velocidad);
    bool adelante = velocidad >= 0;

    if (motor == 0) {  // Motor A (Izquierdo)
        if (adelante) {
            digitalWrite(MOTOR_A_IN1, HIGH);
            digitalWrite(MOTOR_A_IN2, LOW);
        } else {
            digitalWrite(MOTOR_A_IN1, LOW);
            digitalWrite(MOTOR_A_IN2, HIGH);
        }
        ledcWrite(MOTOR_PWM_CHANNEL_A, pwmValue);
    } 
    else if (motor == 1) {  // Motor B (Derecho)
        if (adelante) {
            digitalWrite(MOTOR_B_IN3, HIGH);
            digitalWrite(MOTOR_B_IN4, LOW);
        } else {
            digitalWrite(MOTOR_B_IN3, LOW);
            digitalWrite(MOTOR_B_IN4, HIGH);
        }
        ledcWrite(MOTOR_PWM_CHANNEL_B, pwmValue);
    }
}

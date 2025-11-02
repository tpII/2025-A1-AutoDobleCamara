#pragma once

/**
 * @file MotorControl.h
 * @brief Controlador de motores para el autito usando driver L298N
 * 
 * Esta clase maneja el control de dos motores DC mediante un puente H L298N.
 * Permite controlar la dirección y velocidad de los motores para implementar
 * movimientos básicos: avanzar, retroceder, girar a la izquierda/derecha y detener.
 */

class MotorController {
public:
    /**
     * @brief Inicializa los pines y configura los canales PWM
     */
    void setup();

    /**
     * @brief Mueve el autito hacia adelante
     */
    void avanzar();

    /**
     * @brief Mueve el autito hacia atrás
     */
    void retroceder();

    /**
     * @brief Gira el autito a la izquierda (motor derecho adelante, izquierdo atrás)
     */
    void girarIzquierda();

    /**
     * @brief Gira el autito a la derecha (motor izquierdo adelante, derecho atrás)
     */
    void girarDerecha();

    /**
     * @brief Detiene ambos motores
     */
    void detener();

    /**
     * @brief Ejecuta un comando específico de movimiento
     * @param comando Código del comando (CMD_DETENER, CMD_AVANZAR, etc.)
     */
    void controlarComando(int comando);

private:
    /**
     * @brief Establece la velocidad y dirección de un motor
     * @param motor 0 = Motor A (Izquierdo), 1 = Motor B (Derecho)
     * @param velocidad Velocidad en el rango -255 a 255
     *                  Positivo = adelante, Negativo = atrás, 0 = detenido
     */
    void setMotorVelocidad(int motor, int velocidad);
};

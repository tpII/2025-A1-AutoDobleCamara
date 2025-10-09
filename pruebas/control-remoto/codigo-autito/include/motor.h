#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <math.h>

// Pines (centralizados para evitar duplicados)
// Asignación solicitada por el usuario
// Motor A
#define MOTOR1_PWM_PIN 15
#define MOTOR1_IN1_PIN 2
#define MOTOR1_IN2_PIN 0
// Motor B
#define MOTOR2_PWM_PIN 4
#define MOTOR2_IN1_PIN 16
#define MOTOR2_IN2_PIN 17

// PWM / LEDC
static const uint32_t PWM_FREQ = 20000u;
static const uint8_t PWM_RESOLUTION = 10; // bits (0..(1<<PWM_RESOLUTION)-1)
static const uint8_t CHANNEL_M1 = 0;
static const uint8_t CHANNEL_M2 = 1;

// Rampa / linearización
static const float GAMMA = 0.5f;
static const uint16_t RAMP_STEP = 16; // step en unidades PWM escaladas
static const uint32_t RAMP_INTERVAL_MS = 20u;


void motorInit();
void setMotorTarget(uint8_t motor, uint8_t speed8, bool forward);
void stopAll();
void updateMotor();
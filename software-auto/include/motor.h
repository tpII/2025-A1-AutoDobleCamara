#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <math.h>

// Pines (centralizados para evitar duplicados)
#define MOTOR1_PWM_PIN 23
#define MOTOR1_IN1_PIN 22
#define MOTOR1_IN2_PIN 18

#define MOTOR2_PWM_PIN 5
#define MOTOR2_IN1_PIN 21
#define MOTOR2_IN2_PIN 19

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
void setMotorTarget(u_int8_t motor, u_int16_t speed8, bool forward);
void stopAll();
void updateMotor();
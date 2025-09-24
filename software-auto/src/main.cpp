#include <Arduino.h>

// Definir pines para el control del motor
#define MOTOR_PWM_PIN 23 // Pin PWM para controlar velocidad
#define MOTOR_IN1_PIN 22 // Pin de dirección 1
#define MOTOR_IN2_PIN 1  // Pin de dirección 2

void setup()
{
    // Inicializar comunicación serie para debug
    Serial.begin(115200);
    Serial.println("Iniciando control de motor L298N");

    // Configurar pines como salida
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);

    // Inicializar motor parado
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    analogWrite(MOTOR_PWM_PIN, 0);

    delay(2000); // Esperar 2 segundos antes de empezar
}

void loop()
{
    // Motor hacia adelante - acelerar gradualmente
    Serial.println("Motor hacia adelante - acelerando...");
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);

    for (int velocidad = 0; velocidad <= 255; velocidad += 5)
    {
        analogWrite(MOTOR_PWM_PIN, velocidad);
        Serial.print("Velocidad: ");
        Serial.println(velocidad);
        delay(100);
    }

    delay(1000); // Mantener velocidad máxima por 1 segundo

    // Desacelerar gradualmente
    Serial.println("Desacelerando...");
    for (int velocidad = 255; velocidad >= 0; velocidad -= 5)
    {
        analogWrite(MOTOR_PWM_PIN, velocidad);
        Serial.print("Velocidad: ");
        Serial.println(velocidad);
        delay(100);
    }

    delay(1000); // Pausa

    // Motor hacia atrás - acelerar gradualmente
    Serial.println("Motor hacia atrás - acelerando...");
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);

    for (int velocidad = 0; velocidad <= 255; velocidad += 5)
    {
        analogWrite(MOTOR_PWM_PIN, velocidad);
        Serial.print("Velocidad reversa: ");
        Serial.println(velocidad);
        delay(100);
    }

    delay(1000); // Mantener velocidad máxima por 1 segundo

    // Desacelerar gradualmente
    Serial.println("Desacelerando reversa...");
    for (int velocidad = 255; velocidad >= 0; velocidad -= 5)
    {
        analogWrite(MOTOR_PWM_PIN, velocidad);
        Serial.print("Velocidad reversa: ");
        Serial.println(velocidad);
        delay(100);
    }

    // Motor parado
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    analogWrite(MOTOR_PWM_PIN, 0);

    delay(2000); // Pausa de 2 segundos antes de repetir el ciclo
    Serial.println("Ciclo completado - reiniciando...\n");
}
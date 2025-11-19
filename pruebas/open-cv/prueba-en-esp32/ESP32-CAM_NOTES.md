# Notas para ESP32-CAM

## Configuración Aplicada

El proyecto ha sido adaptado para funcionar con **ESP32-CAM** (módulo AI-Thinker con OV2640).

### Cambios Principales

1. **Target del chip**: ESP32 (en lugar de ESP32-S3)
2. **Pines de la cámara**: Configurados para el pinout estándar del ESP32-CAM
3. **PSRAM**: Modo QUAD a 40MHz (4MB disponibles)
4. **Flash**: 4MB a 40MHz

## Pinout ESP32-CAM

### Cámara OV2640

- **PWDN**: GPIO32
- **XCLK**: GPIO0
- **SIOD (SDA)**: GPIO26
- **SIOC (SCL)**: GPIO27
- **D7**: GPIO35
- **D6**: GPIO34
- **D5**: GPIO39
- **D4**: GPIO36
- **D3**: GPIO21
- **D2**: GPIO19
- **D1**: GPIO18
- **D0**: GPIO5
- **VSYNC**: GPIO25
- **HREF**: GPIO23
- **PCLK**: GPIO22

### Motores (L298N)

- **MOTOR_A_IN1**: GPIO2
- **MOTOR_A_IN2**: GPIO4 (LED flash incorporado)
- **MOTOR_A_ENA**: GPIO12
- **MOTOR_B_IN3**: GPIO13
- **MOTOR_B_IN4**: GPIO14
- **MOTOR_B_ENB**: GPIO15

## ⚠️ Consideraciones Importantes

### GPIOs Limitados en ESP32-CAM

Muchos GPIOs están ocupados por la cámara. Los disponibles para uso general son limitados:

- **GPIO 2**: Disponible (requiere pull-down durante el boot)
- **GPIO 4**: LED flash incorporado
- **GPIO 12**: Disponible (debe estar LOW durante el boot)
- **GPIO 13**: Disponible
- **GPIO 14**: Disponible
- **GPIO 15**: Debe estar HIGH durante el boot

### Pines NO Recomendados

- **GPIO 0**: Clock de la cámara
- **GPIO 1, 3**: TX/RX (programación)
- **GPIO 16**: PSRAM (no usar)
- **GPIO 35, 36, 39**: Solo input, usados por la cámara

### Alimentación

- El ESP32-CAM requiere **5V y al menos 2A** para funcionar correctamente con la cámara
- Problemas comunes: brownout si la alimentación es insuficiente
- No intentar alimentar por el pin de 3.3V

## Programación

### Método 1: Adaptador FTDI/USB-Serial

1. Conectar:
   - **U0R** → TX del adaptador
   - **U0T** → RX del adaptador
   - **GND** → GND
   - **5V** → 5V (asegurar 2A+)
   - **GPIO0** → GND (para entrar en modo flash)
2. Flashear con `./flash.sh`
3. Desconectar GPIO0 de GND
4. Resetear el módulo

### Método 2: ESP32-CAM-MB (Módulo programador)

Si tienes la placa programadora ESP32-CAM-MB:

1. Insertar el ESP32-CAM en el zócalo
2. Conectar USB
3. Ejecutar `./flash.sh`
4. El reset automático funciona

## Scripts de Build

Los scripts existentes deberían funcionar:

```bash
./clean.sh    # Limpiar build anterior
./build.sh    # Compilar proyecto
./flash.sh    # Flashear a la placa
./monitor.sh  # Ver output serial
```

## Resolución de Problemas

### "Brownout detector was triggered"

- Aumentar la corriente de alimentación
- Usar una fuente de 5V/2A dedicada
- Agregar capacitor de 100-470µF cerca del pin 5V

### "Camera probe failed"

- Verificar conexiones de la cámara
- La cámara viene integrada en el módulo ESP32-CAM
- Revisar que PWDN (GPIO32) esté funcionando correctamente

### No entra en modo flash

- Asegurar que GPIO0 está conectado a GND durante el reset
- Verificar conexiones TX/RX (están cruzadas: U0R→TX, U0T→RX)
- Probar reducir baudrate en `flash.sh` si hay errores

### Conflictos con motores

- Si usas GPIO4, el LED flash puede interferir (quitar LED o cambiar pin)
- GPIO12 debe estar LOW durante el boot (OK con motor driver)
- GPIO15 debe estar HIGH durante el boot (configurar después como output)

## Testing Inicial

Después de flashear, prueba primero sin motores:

1. Conectar solo ESP32-CAM
2. Monitorear serial: `./monitor.sh`
3. Verificar que la cámara se inicializa
4. Verificar conexión WiFi
5. Conectar motores después de confirmar que funciona

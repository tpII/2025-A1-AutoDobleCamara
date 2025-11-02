# 🏗️ Arquitectura del Sistema - Autito Robot

Documentación técnica de la arquitectura del firmware.

---

## 📊 Diagrama de Arquitectura General

```
┌─────────────────────────────────────────────────────────────────┐
│                        SISTEMA COMPLETO                          │
│                  (Sistema de Veto Distribuido)                   │
└─────────────────────────────────────────────────────────────────┘
                                 │
                ┌────────────────┴────────────────┐
                │                                 │
        ┌───────▼────────┐               ┌───────▼────────┐
        │  CÁMARA FIJA   │               │     AUTITO     │
        │  (Vigilante)   │               │ (Decisor Final)│
        └───────┬────────┘               └───────┬────────┘
                │                                 │
                │    ESP-NOW (comandoUsuario,     │
                │            hayRiesgoGlobal)     │
                └─────────────────────────────────┤
                                                  │
                                    ┌─────────────▼──────────────┐
                                    │   LÓGICA DE FUSIÓN         │
                                    │   Solo avanzar si:         │
                                    │   cmd==AVANZAR &&          │
                                    │   !riesgoLocal &&          │
                                    │   !riesgoGlobal            │
                                    └─────────────┬──────────────┘
                                                  │
                                    ┌─────────────▼──────────────┐
                                    │    CONTROL DE MOTORES      │
                                    └────────────────────────────┘
```

---

## 🔧 Arquitectura del Firmware del Autito

```
┌──────────────────────────────────────────────────────────────────┐
│                          MAIN.CPP                                 │
│                      (Loop Principal)                             │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  setup()                                                 │    │
│  │  • Inicializar Serial                                    │    │
│  │  • Inicializar MotorController                           │    │
│  │  • Inicializar CameraVision                              │    │
│  │  • Inicializar NetworkManager                            │    │
│  │  • Crear Tarea FreeRTOS de Visión                        │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  loop()                                                  │    │
│  │  1. NetworkManager.handleClient()  ← Atender web        │    │
│  │  2. ejecutarLogicaDeFusion()       ← Decidir acción     │    │
│  │  3. Monitoreo de memoria           ← Debug              │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  visionTask() [FreeRTOS Task en Core 0]                 │    │
│  │  while(true):                                            │    │
│  │    • CameraVision.run()            ← Capturar + Procesar│    │
│  │    • delay(50ms)                   ← ~20 FPS            │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  OnDataRecv() [Callback ESP-NOW]                        │    │
│  │  • Actualizar g_comandoUsuario                           │    │
│  │  • Actualizar g_hayRiesgoGlobal                          │    │
│  └─────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘
                                │
                ┌───────────────┼───────────────┐
                │               │               │
        ┌───────▼────────┐ ┌───▼──────┐ ┌─────▼────────┐
        │ MotorControl   │ │  Camera  │ │   Network    │
        │                │ │  Vision  │ │              │
        └────────────────┘ └──────────┘ └──────────────┘
```

---

## 📦 Módulos del Sistema

### 1. MotorControl (Motor Control)

```
┌─────────────────────────────────────────┐
│        MotorControl.h / .cpp            │
├─────────────────────────────────────────┤
│  Responsabilidades:                     │
│  • Controlar 2 motores DC via L298N     │
│  • Configurar PWM para velocidad        │
│  • Implementar movimientos básicos      │
│                                         │
│  Métodos Públicos:                      │
│  • setup()          - Inicializar       │
│  • avanzar()        - Mover adelante    │
│  • retroceder()     - Mover atrás       │
│  • girarIzquierda() - Giro izq          │
│  • girarDerecha()   - Giro der          │
│  • detener()        - Frenar            │
│  • controlarComando(int cmd)            │
│                                         │
│  Hardware:                              │
│  • GPIO → L298N IN1-IN4 (dirección)     │
│  • PWM → ENA, ENB (velocidad)           │
└─────────────────────────────────────────┘
```

### 2. CameraVision (Computer Vision)

```
┌──────────────────────────────────────────────────┐
│         CameraVision.h / .cpp                    │
├──────────────────────────────────────────────────┤
│  Responsabilidades:                              │
│  • Capturar frames de la cámara ESP32           │
│  • Detectar obstáculos verdes (HSV)             │
│  • Calcular distancia (tamaño aparente)         │
│  • Determinar riesgo local                      │
│  • Generar frame de debug para stream           │
│                                                  │
│  Pipeline de Procesamiento:                     │
│  1. Capturar frame RGB565                       │
│  2. Convertir RGB565 → BGR                      │
│  3. Convertir BGR → HSV                         │
│  4. Aplicar máscara de color verde              │
│  5. Morfología (eliminar ruido)                 │
│  6. Encontrar contornos                         │
│  7. Filtrar por área mínima                     │
│  8. Calcular distancia del más cercano          │
│  9. Actualizar riesgoLocal                      │
│  10. Dibujar anotaciones en debugFrame          │
│                                                  │
│  Métodos Públicos:                              │
│  • setup()              - Inicializar cámara    │
│  • run()                - Procesar 1 frame      │
│  • getRiesgoLocal()     - Estado de riesgo      │
│  • getDebugFrame()      - Frame anotado         │
│  • getDistanciaMinima() - Distancia en cm       │
│                                                  │
│  Thread Safety:                                 │
│  • Mutex (frameMutex) para acceso al frame      │
│  • Corre en tarea FreeRTOS separada             │
└──────────────────────────────────────────────────┘
```

### 3. NetworkManager (Networking)

```
┌──────────────────────────────────────────────────┐
│          Network.h / .cpp                        │
├──────────────────────────────────────────────────┤
│  Responsabilidades:                              │
│  • WiFi Access Point                             │
│  • ESP-NOW (recibir comandos)                    │
│  • Servidor Web HTTP                             │
│  • Stream MJPEG de video                         │
│  • Endpoint de prueba (modo debug)               │
│                                                  │
│  Endpoints HTTP:                                 │
│  GET  /                 - Página principal       │
│  GET  /stream_auto      - Stream MJPEG           │
│  GET  /test_control     - Control manual (test)  │
│                                                  │
│  ESP-NOW:                                        │
│  • Recibe: struct_ComandoRemoto                  │
│    - int comando       (0-4)                     │
│    - bool hayRiesgoGlobal                        │
│  • Callback: OnDataRecv() en main.cpp            │
│                                                  │
│  Métodos Públicos:                              │
│  • setup(callback)   - Inicializar todo         │
│  • handleClient()    - Atender requests web     │
│                                                  │
│  Modos:                                          │
│  #ifdef MODO_PRUEBA_SIN_COMPANERO                │
│    → Solo WiFi + Web (sin ESP-NOW)               │
│  #else                                           │
│    → WiFi + Web + ESP-NOW                        │
│  #endif                                          │
└──────────────────────────────────────────────────┘
```

---

## 🧵 Flujo de Datos

### Flujo de Comando (ESP-NOW → Motores)

```
Cámara Fija              ESP-NOW                Autito
    │                                              │
    │ 1. Usuario mueve joystick                   │
    ├──────────────────────────────────────────►  │
    │    {comando: AVANZAR,                       │
    │     hayRiesgoGlobal: false}                 │
    │                                             │
    │                                             │
    │                      2. OnDataRecv()        │
    │                         actualiza globals   │
    │                         ↓                   │
    │                      g_comandoUsuario = 1   │
    │                      g_hayRiesgoGlobal = 0  │
    │                                             │
    │                      3. loop() lee:         │
    │                         • g_comandoUsuario  │
    │                         • g_hayRiesgoGlobal │
    │                         • riesgoLocal       │
    │                         ↓                   │
    │                      4. Lógica de Fusión    │
    │                         if (cmd==AVANZAR && │
    │                             !local &&        │
    │                             !global)         │
    │                           → AVANZAR         │
    │                         else                │
    │                           → DETENER         │
    │                         ↓                   │
    │                      5. MotorController     │
    │                         .controlarComando() │
    │                         ↓                   │
    │                      [MOTORES SE MUEVEN]    │
```

### Flujo de Visión (Cámara → Detección → Riesgo)

```
 Camera Hardware        CameraVision              Main Loop
       │                     │                        │
       │ 1. Capturar frame   │                        │
       ├────────────────────►│                        │
       │    RGB565           │                        │
       │                     │                        │
       │                     │ 2. Procesar            │
       │                     │    • RGB565→BGR        │
       │                     │    • BGR→HSV           │
       │                     │    • Máscara verde     │
       │                     │    • Contornos         │
       │                     │    • Calcular dist.    │
       │                     │                        │
       │                     │ 3. Actualizar estado   │
       │                     │    (con mutex)         │
       │                     │    distanciaMinima     │
       │                     │    riesgoLocal         │
       │                     │    debugFrame          │
       │                     │                        │
       │                     │                        │
       │                     │ 4. getRiesgoLocal() ◄──┤
       │                     ├────────────────────────►
       │                     │    return riesgoLocal  │
       │                     │                        │
       │                     │                        │
       │                     │ 5. Usar en fusión      │
       │                     │                   (loop ejecuta)
```

### Flujo de Stream (Visión → Web)

```
CameraVision          NetworkManager          Cliente Web
     │                      │                      │
     │ 1. debugFrame        │                      │
     │    actualizado       │                      │
     │                      │                      │
     │                      │  2. handleClient()   │
     │                      │     detecta request  │
     │                      │     /stream_auto     │
     │                      │                      │
     │                      │  3. while(connected):│
     │                      │                      │
     │ 4. getDebugFrame() ◄─┤                      │
     ├──────────────────────►                      │
     │    Mat frame         │                      │
     │                      │                      │
     │                      │  5. imencode(JPEG)   │
     │                      │     ↓                │
     │                      │  6. Enviar MJPEG     │
     │                      ├─────────────────────►│
     │                      │   --frame            │
     │                      │   Content: image/jpeg│
     │                      │   [JPEG data]        │
     │                      │                      │
     │                      │  7. delay(50ms)      │
     │                      │     y repetir        │
```

---

## 🔄 Diagrama de Estados del Sistema

```
                    ┌─────────────┐
                    │   INICIO    │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │   SETUP()   │
                    │ • Motores   │
                    │ • Cámara    │
                    │ • Red       │
                    └──────┬──────┘
                           │
                           ▼
              ┌────────────────────────┐
              │   OPERACIÓN NORMAL     │◄───────┐
              │   (loop infinito)      │        │
              └────────┬───────────────┘        │
                       │                        │
         ┌─────────────┼─────────────┐          │
         │             │             │          │
         ▼             ▼             ▼          │
  ┌──────────┐  ┌──────────┐  ┌──────────┐     │
  │  Recibir │  │ Procesar │  │ Ejecutar │     │
  │ Comandos │  │  Visión  │  │  Fusión  │     │
  └──────────┘  └──────────┘  └────┬─────┘     │
                                    │           │
                                    ▼           │
                            ┌──────────────┐    │
                            │   MOTORES    │    │
                            └──────┬───────┘    │
                                   │            │
                                   └────────────┘
```

### Estados de la Lógica de Fusión

```
                  ┌─────────────────┐
                  │ Leer Comandos y │
                  │  Estado Riesgo  │
                  └────────┬────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │ cmd == AVANZAR? │
                  └────┬─────────┬──┘
                   NO  │         │  SÍ
                       │         │
        ┌──────────────┘         └──────────────┐
        │                                       │
        ▼                                       ▼
┌───────────────┐                   ┌───────────────────┐
│ EJECUTAR cmd  │                   │ riesgoLocal ||    │
│ (Atrás, Giro, │                   │ riesgoGlobal?     │
│  Detener)     │                   └─────┬──────┬──────┘
└───────────────┘                    SÍ   │      │  NO
                                           │      │
                               ┌───────────┘      └──────────┐
                               │                             │
                               ▼                             ▼
                      ┌─────────────────┐         ┌──────────────┐
                      │ VETO APLICADO   │         │   AVANZAR    │
                      │ → CMD_DETENER   │         │              │
                      └─────────────────┘         └──────────────┘
                               │                             │
                               └──────────┬──────────────────┘
                                          │
                                          ▼
                                ┌──────────────────┐
                                │ Enviar a Motores │
                                └──────────────────┘
```

---

## 🔒 Sincronización y Thread Safety

### Recursos Compartidos

```
┌──────────────────────────────────────────────────────┐
│  RECURSO             ESCRITURA          LECTURA      │
├──────────────────────────────────────────────────────┤
│  g_comandoUsuario    OnDataRecv()      loop()        │
│  g_hayRiesgoGlobal   OnDataRecv()      loop()        │
│                      (callback ISR)    (main thread) │
│                                                       │
│  Protección: volatile (acceso atómico básico)        │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│  RECURSO             ESCRITURA          LECTURA      │
├──────────────────────────────────────────────────────┤
│  debugFrame          visionTask()       handleClient()│
│  riesgoLocal         visionTask()       loop()        │
│  distanciaMinima     visionTask()       loop()        │
│                      (FreeRTOS Task)    (main thread) │
│                                                       │
│  Protección: frameMutex (SemaphoreHandle_t)          │
└──────────────────────────────────────────────────────┘
```

### Cores del ESP32

```
┌─────────────────────────────────────────────┐
│              CORE 0 (PRO_CPU)               │
├─────────────────────────────────────────────┤
│  • visionTask()                             │
│    - Captura frames                         │
│    - Procesamiento OpenCV                   │
│    - Actualiza estado de riesgo             │
│  • WiFi/TCP stack (Arduino framework)       │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│              CORE 1 (APP_CPU)               │
├─────────────────────────────────────────────┤
│  • loop()                                   │
│    - Lógica de fusión                       │
│    - Control de motores                     │
│    - Monitoreo de memoria                   │
│  • setup()                                  │
│  • OnDataRecv() callback                    │
└─────────────────────────────────────────────┘
```

---

## 📊 Uso de Memoria

### Estimación de Uso (ESP32 sin PSRAM)

```
┌────────────────────────────────────────────┐
│  COMPONENTE             RAM USADA          │
├────────────────────────────────────────────┤
│  Arduino Framework      ~40 KB             │
│  WiFi Stack             ~20 KB             │
│  Camera Driver          ~15 KB             │
│  Frame Buffer QVGA      ~150 KB            │
│  (320x240x2 bytes)                         │
│  OpenCV Mat temp        ~150 KB            │
│  Stack visionTask       ~8 KB              │
│  Stack loop             ~16 KB             │
│  Variables globales     ~2 KB              │
│  ESP-NOW                ~2 KB              │
│  ────────────────────────────────          │
│  TOTAL APROX            ~403 KB            │
│                                            │
│  Heap disponible        ~100-150 KB        │
│  (depende del modelo ESP32)                │
└────────────────────────────────────────────┘
```

⚠️ **Nota:** Por esto es crítico usar QVGA (320x240) y no resoluciones mayores sin PSRAM.

---

## 🔌 Tabla de Pines Completa

```
┌──────────────────────────────────────────────────────┐
│  GPIO   FUNCIÓN           MÓDULO      TIPO           │
├──────────────────────────────────────────────────────┤
│  5      Cámara D0         OV2640      Input          │
│  18     Cámara D1         OV2640      Input          │
│  19     Cámara D2         OV2640      Input          │
│  27     Cámara D3         OV2640      Input          │
│  35     Cámara D4         OV2640      Input (ADC1)   │
│  34     Cámara D5         OV2640      Input (ADC1)   │
│  39     Cámara D6         OV2640      Input (ADC1)   │
│  36     Cámara D7         OV2640      Input (ADC1)   │
│  26     Cámara PCLK       OV2640      Input          │
│  25     Cámara VSYNC      OV2640      Input          │
│  23     Cámara HREF       OV2640      Input          │
│  21     Cámara SDA        I2C         Bidireccional  │
│  22     Cámara SCL        I2C         Output         │
│                                                       │
│  12     Motor A IN1       L298N       Output         │
│  13     Motor A IN2       L298N       Output         │
│  14     Motor A ENA       L298N       PWM Output     │
│  15     Motor B IN3       L298N       Output         │
│  16     Motor B IN4       L298N       Output         │
│  17     Motor B ENB       L298N       PWM Output     │
│                                                       │
│  1      TX (Serial)       Debug       Output         │
│  3      RX (Serial)       Debug       Input          │
└──────────────────────────────────────────────────────┘

⚠️ Pines NO disponibles (usados por cámara):
   5, 18, 19, 21, 22, 23, 25, 26, 27, 34, 35, 36, 39

✅ Pines libres para motores:
   4, 12, 13, 14, 15, 16, 17, 32, 33, (2 con precaución)
```

---

**Fin de la documentación de arquitectura. 🏗️**

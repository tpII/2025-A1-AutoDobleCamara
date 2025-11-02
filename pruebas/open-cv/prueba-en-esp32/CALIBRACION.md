# 📏 Guía de Calibración del Autito Robot

Esta guía te ayudará a calibrar correctamente todos los parámetros del sistema para obtener mediciones precisas de distancia.

---

## 📋 Índice

1. [Calibración Rápida (Método Experimental)](#1-calibración-rápida-método-experimental)
2. [Calibración Profesional (Con OpenCV)](#2-calibración-profesional-con-opencv)
3. [Calibración de Rangos de Color](#3-calibración-de-rangos-de-color)
4. [Verificación Final](#4-verificación-final)

---

## 1. Calibración Rápida (Método Experimental)

### ⏱️ Tiempo: ~10 minutos

### 🎯 Objetivo

Obtener un valor aproximado de la distancia focal sin herramientas externas.

### 📦 Materiales Necesarios

- Tu autito con el firmware cargado
- El objeto verde de detección
- Una cinta métrica o regla
- Calculadora

### 🔧 Procedimiento

#### Paso 1: Medir el Ancho del Objeto

Mide el ancho real del objeto verde con una regla:

```
Ancho Real = _____ cm
```

Ejemplo: 5.0 cm

Actualiza en `config.h`:

```cpp
#define ANCHO_OBSTACULO_CM  5.0  // Tu medida aquí
```

#### Paso 2: Posicionar el Objeto

1. Coloca el objeto verde a una **distancia conocida** del autito
2. Distancia recomendada: **50 cm** (medida desde la lente)
3. Asegúrate de que esté **centrado** en el campo de visión

```
            50 cm
[AUTITO] ←────────→ [🟢 OBJETO]
```

#### Paso 3: Capturar el Ancho en Píxeles

1. Enciende el autito
2. Conéctate al WiFi: `AUTITO_ROBOT_AP`
3. Accede al stream: `http://192.168.4.1/stream_auto`
4. Observa el rectángulo verde alrededor del objeto
5. Anota el ancho mostrado en pantalla

**Monitor Serial mostrará:**

```
[VISION] Rectángulo: x=120, y=80, w=32, h=45
                                    ^^
                              Ancho en píxeles
```

```
Ancho en Píxeles = _____ px
```

Ejemplo: 32 px

#### Paso 4: Calcular la Distancia Focal

Usa esta fórmula:

```
Focal = (Ancho_Píxeles × Distancia_Real) / Ancho_Real
```

**Ejemplo:**

```
Focal = (32 px × 50 cm) / 5.0 cm
Focal = 1600 / 5.0
Focal = 320.0 píxeles
```

```
Tu Focal = _______ px
```

#### Paso 5: Actualizar config.h

Edita `include/config.h`:

```cpp
#define FOCAL_AUTITO_PX  320.0  // Tu valor calculado
```

#### Paso 6: Recompilar y Probar

```bash
pio run --target upload
```

Prueba a diferentes distancias para verificar:

| Distancia Real | Distancia Medida | Error  |
| -------------- | ---------------- | ------ |
| 30 cm          | **\_** cm        | **\_** |
| 50 cm          | **\_** cm        | **\_** |
| 70 cm          | **\_** cm        | **\_** |

✅ **Calibración exitosa si el error es <10%**

---

## 2. Calibración Profesional (Con OpenCV)

### ⏱️ Tiempo: ~30 minutos

### 🎯 Objetivo

Obtener la matriz intrínseca de la cámara y corregir distorsiones de lente.

### 📦 Materiales Necesarios

- Python 3.x instalado
- OpenCV Python: `pip install opencv-python`
- Numpy: `pip install numpy`
- Tablero de ajedrez impreso (9x6 cuadrados)
- 20-30 fotos del tablero desde diferentes ángulos

### 🔧 Procedimiento

#### Paso 1: Crear el Script de Calibración

Crea `scripts/calibrar_camara.py`:

```python
#!/usr/bin/env python3
"""
Script de Calibración de Cámara para Autito Robot
Genera la matriz intrínseca y coeficientes de distorsión.
"""

import cv2
import numpy as np
import glob
import json

# Configuración del tablero de ajedrez
PATRON = (9, 6)  # Esquinas internas (columnas, filas)
TAMANO_CUADRADO = 2.5  # Tamaño del cuadrado en cm

def calibrar_camara(imagenes_path):
    """
    Calibra la cámara usando imágenes de un tablero de ajedrez.
    """
    # Preparar puntos del tablero en el mundo real
    objp = np.zeros((PATRON[0] * PATRON[1], 3), np.float32)
    objp[:, :2] = np.mgrid[0:PATRON[0], 0:PATRON[1]].T.reshape(-1, 2)
    objp *= TAMANO_CUADRADO

    # Arrays para almacenar puntos
    objpoints = []  # Puntos 3D en el mundo real
    imgpoints = []  # Puntos 2D en la imagen

    # Leer imágenes
    images = glob.glob(imagenes_path)

    if len(images) == 0:
        print("❌ No se encontraron imágenes en:", imagenes_path)
        return None

    print(f"📸 Procesando {len(images)} imágenes...")

    for fname in images:
        img = cv2.imread(fname)
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        # Encontrar esquinas del tablero
        ret, corners = cv2.findChessboardCorners(gray, PATRON, None)

        if ret:
            objpoints.append(objp)

            # Refinar detección de esquinas
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
            corners2 = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
            imgpoints.append(corners2)

            print(f"  ✅ {fname}")
        else:
            print(f"  ❌ {fname} - No se detectó el patrón")

    if len(objpoints) < 10:
        print("⚠️ Advertencia: Pocas imágenes válidas. Recomendado: >20")

    # Calibrar
    print("\n🔬 Calibrando cámara...")
    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
        objpoints, imgpoints, gray.shape[::-1], None, None
    )

    if ret:
        print("✅ Calibración exitosa!")
        return {
            'ret': ret,
            'mtx': mtx,
            'dist': dist,
            'rvecs': rvecs,
            'tvecs': tvecs
        }
    else:
        print("❌ Error en la calibración")
        return None

def generar_config_h(result):
    """
    Genera el código para config.h
    """
    mtx = result['mtx']
    dist = result['dist'][0]

    print("\n" + "="*60)
    print("📋 COPIA ESTOS VALORES EN include/config.h")
    print("="*60)
    print("\n// --- MATRIZ DE CALIBRACIÓN ---")
    print(f"#define CAM_MTX_00 {mtx[0, 0]:.2f}f  // fx")
    print(f"#define CAM_MTX_01 {mtx[0, 1]:.2f}f")
    print(f"#define CAM_MTX_02 {mtx[0, 2]:.2f}f  // cx")
    print(f"#define CAM_MTX_10 {mtx[1, 0]:.2f}f")
    print(f"#define CAM_MTX_11 {mtx[1, 1]:.2f}f  // fy")
    print(f"#define CAM_MTX_12 {mtx[1, 2]:.2f}f  // cy")
    print(f"#define CAM_MTX_20 {mtx[2, 0]:.2f}f")
    print(f"#define CAM_MTX_21 {mtx[2, 1]:.2f}f")
    print(f"#define CAM_MTX_22 {mtx[2, 2]:.2f}f")
    print("\n// --- COEFICIENTES DE DISTORSIÓN ---")
    print(f"#define CAM_DIST_K1  {dist[0]:.6f}f")
    print(f"#define CAM_DIST_K2  {dist[1]:.6f}f")
    print(f"#define CAM_DIST_P1  {dist[2]:.6f}f")
    print(f"#define CAM_DIST_P2  {dist[3]:.6f}f")
    print(f"#define CAM_DIST_K3  {dist[4]:.6f}f")
    print("\n" + "="*60)

    print(f"\n🎯 Distancia Focal Promedio: {(mtx[0,0] + mtx[1,1])/2:.2f} px")
    print(f"   Actualiza también: #define FOCAL_AUTITO_PX {(mtx[0,0] + mtx[1,1])/2:.2f}")

if __name__ == "__main__":
    print("🤖 Calibración de Cámara - Autito Robot")
    print("="*60)

    # Cambiar este path según donde guardes tus imágenes
    imagenes_path = "calibracion/*.jpg"

    print(f"📂 Buscando imágenes en: {imagenes_path}")

    result = calibrar_camara(imagenes_path)

    if result:
        # Guardar resultados
        np.savez('calibracion_resultado.npz',
                 mtx=result['mtx'],
                 dist=result['dist'])
        print("\n💾 Resultados guardados en: calibracion_resultado.npz")

        # Generar código
        generar_config_h(result)
    else:
        print("\n❌ Calibración fallida")
```

#### Paso 2: Capturar Imágenes

1. Crea la carpeta: `mkdir scripts/calibracion`
2. Imprime un tablero de ajedrez (9x6 esquinas internas)
3. Captura 20-30 fotos del tablero desde diferentes ángulos:
   - Diferentes distancias (cerca y lejos)
   - Diferentes rotaciones
   - Cubre toda el área de la imagen
4. Guarda las fotos en `scripts/calibracion/`

#### Paso 3: Ejecutar la Calibración

```bash
cd scripts
python3 calibrar_camara.py
```

**Salida esperada:**

```
📸 Procesando 25 imágenes...
  ✅ calibracion/img_001.jpg
  ✅ calibracion/img_002.jpg
  ...
🔬 Calibrando cámara...
✅ Calibración exitosa!

============================================================
📋 COPIA ESTOS VALORES EN include/config.h
============================================================

#define CAM_MTX_00 312.50f  // fx
#define CAM_MTX_02 160.00f  // cx
#define CAM_MTX_11 312.50f  // fy
#define CAM_MTX_12 120.00f  // cy
...
```

#### Paso 4: Actualizar config.h

Copia los valores generados en `include/config.h` y recompila.

---

## 3. Calibración de Rangos de Color

### 🎯 Objetivo

Ajustar los umbrales HSV para detectar el objeto verde correctamente.

### 🔧 Procedimiento

#### Opción A: Método Visual

1. Accede al stream: `http://192.168.4.1/stream_auto`
2. Observa si el objeto verde se detecta (rectángulo rojo/verde)
3. Si NO se detecta o hay falsos positivos, ajusta los rangos

**Tabla de Referencia HSV:**

| Color    | H min | H max | S min | S max | V min | V max |
| -------- | ----- | ----- | ----- | ----- | ----- | ----- |
| Verde    | 35    | 85    | 100   | 255   | 100   | 255   |
| Amarillo | 20    | 35    | 100   | 255   | 100   | 255   |
| Azul     | 100   | 130   | 100   | 255   | 100   | 255   |
| Rojo     | 0     | 10    | 100   | 255   | 100   | 255   |

#### Opción B: Herramienta HSV Picker

Crea `scripts/color_picker.py`:

```python
#!/usr/bin/env python3
import cv2
import numpy as np

def nothing(x):
    pass

# Crear ventana
cv2.namedWindow('Color Picker')

# Crear trackbars
cv2.createTrackbar('H min', 'Color Picker', 35, 179, nothing)
cv2.createTrackbar('H max', 'Color Picker', 85, 179, nothing)
cv2.createTrackbar('S min', 'Color Picker', 100, 255, nothing)
cv2.createTrackbar('S max', 'Color Picker', 255, 255, nothing)
cv2.createTrackbar('V min', 'Color Picker', 100, 255, nothing)
cv2.createTrackbar('V max', 'Color Picker', 255, 255, nothing)

# Capturar desde webcam o imagen
cap = cv2.VideoCapture(0)  # Cambiar a la URL del stream si es necesario

while True:
    ret, frame = cap.read()
    if not ret:
        break

    # Obtener valores de trackbars
    h_min = cv2.getTrackbarPos('H min', 'Color Picker')
    h_max = cv2.getTrackbarPos('H max', 'Color Picker')
    s_min = cv2.getTrackbarPos('S min', 'Color Picker')
    s_max = cv2.getTrackbarPos('S max', 'Color Picker')
    v_min = cv2.getTrackbarPos('V min', 'Color Picker')
    v_max = cv2.getTrackbarPos('V max', 'Color Picker')

    # Convertir a HSV
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Crear máscara
    lower = np.array([h_min, s_min, v_min])
    upper = np.array([h_max, s_max, v_max])
    mask = cv2.inRange(hsv, lower, upper)

    # Aplicar máscara
    result = cv2.bitwise_and(frame, frame, mask=mask)

    # Mostrar
    cv2.imshow('Original', frame)
    cv2.imshow('Mask', mask)
    cv2.imshow('Result', result)

    # ESC para salir
    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()

print(f"\n📋 Valores encontrados:")
print(f"#define OBSTACULO_H_MIN {h_min}")
print(f"#define OBSTACULO_H_MAX {h_max}")
print(f"#define OBSTACULO_S_MIN {s_min}")
print(f"#define OBSTACULO_S_MAX {s_max}")
print(f"#define OBSTACULO_V_MIN {v_min}")
print(f"#define OBSTACULO_V_MAX {v_max}")
```

Ejecuta:

```bash
python3 scripts/color_picker.py
```

Ajusta los trackbars hasta que **solo el objeto verde** aparezca en blanco en la ventana "Mask".

---

## 4. Verificación Final

### ✅ Checklist de Calibración

- [ ] `ANCHO_OBSTACULO_CM` medido y actualizado
- [ ] `FOCAL_AUTITO_PX` calculado o calibrado
- [ ] Rangos de color HSV ajustados
- [ ] Matriz de calibración actualizada (opcional)
- [ ] Umbral de riesgo `UMBRAL_RIESGO_LOCAL_CM` configurado
- [ ] Firmware recompilado y subido

### 🧪 Prueba de Precisión

Realiza esta prueba para verificar la calibración:

| Distancia Real | Distancia Medida | Error % |
| -------------- | ---------------- | ------- |
| 20 cm          |                  |         |
| 30 cm          |                  |         |
| 40 cm          |                  |         |
| 50 cm          |                  |         |
| 60 cm          |                  |         |
| 70 cm          |                  |         |

**Cálculo del error:**

```
Error % = |Distancia_Real - Distancia_Medida| / Distancia_Real × 100
```

**Criterios de aceptación:**

- ✅ Error <5% : Excelente
- ⚠️ Error 5-10% : Aceptable
- ❌ Error >10% : Re-calibrar

---

## 📝 Notas Finales

- La calibración es un proceso iterativo. No te desanimes si no es perfecto al primer intento.
- La iluminación afecta mucho la detección de color. Calibra en condiciones similares al uso final.
- Guarda tus valores de calibración en un lugar seguro.
- Vuelve a calibrar si cambias la cámara o la lente.

**¡Buena calibración! 🎯**

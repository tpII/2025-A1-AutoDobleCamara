# Directorio de Calibración

Este directorio contiene las herramientas necesarias para calibrar la cámara del autito.

## 📁 Estructura

```
calibracion/
├── calibrar_camara.py      # Script de calibración completa con tablero de ajedrez
├── color_picker.py          # Herramienta para ajustar rangos de color HSV
├── calcular_focal.py        # Calculadora rápida de distancia focal
├── imagenes/                # Coloca aquí las fotos del tablero (para calibración)
├── *.npz                    # Resultados de calibración (generados)
└── *.txt                    # Resultados en texto (generados)
```

## 🚀 Uso Rápido

### 1. Calibración Rápida (Solo Focal)

```bash
python3 calcular_focal.py
```

Sigue las instrucciones en pantalla.

### 2. Ajustar Colores

```bash
# Con webcam
python3 color_picker.py

# Con el stream del autito
python3 color_picker.py --source http://192.168.4.1/stream_auto

# Con una imagen
python3 color_picker.py --source foto_objeto.jpg
```

### 3. Calibración Completa (Profesional)

```bash
# 1. Toma 20-30 fotos del tablero de ajedrez
# 2. Guárdalas en imagenes/
# 3. Ejecuta:
python3 calibrar_camara.py
```

## 📋 Requisitos

```bash
pip install opencv-python numpy
```

#!/usr/bin/env python3
"""
Genera un tablero de ajedrez para calibración de cámara
"""
import cv2
import numpy as np

# Configuración: 10x7 cuadrados (9x6 esquinas internas)
rows, cols = 7, 10
square_size = 100  # píxeles

# Crear imagen blanca
board = np.zeros((rows * square_size, cols * square_size), dtype=np.uint8)

# Dibujar cuadrados negros y blancos
for i in range(rows):
    for j in range(cols):
        if (i + j) % 2 == 0:
            cv2.rectangle(
                board,
                (j * square_size, i * square_size),
                ((j + 1) * square_size, (i + 1) * square_size),
                255,
                -1,
            )

# Guardar
cv2.imwrite("tablero_ajedrez.png", board)
print("✅ Tablero de ajedrez generado: tablero_ajedrez.png")
print(f"   Tamaño: {cols}x{rows} cuadrados ({cols-1}x{rows-1} esquinas internas)")
print(f"   Dimensiones: {board.shape[1]}x{board.shape[0]} píxeles")
print("\n📝 Instrucciones:")
print("   1. Imprimir este archivo en hoja A4")
print("   2. Pegar en cartón rígido para que quede plano")
print("   3. Medir el tamaño real de un cuadrado en cm")

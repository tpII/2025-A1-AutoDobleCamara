#!/usr/bin/env python3
"""
Script de Calibración de Cámara para Autito Robot
Genera la matriz intrínseca y coeficientes de distorsión.

Uso:
    1. Tomar 20-30 fotos del tablero de ajedrez desde diferentes ángulos
    2. Guardar las fotos en el directorio 'imagenes/'
    3. Ejecutar: python3 calibrar_camara.py
"""

import cv2
import numpy as np
import glob
import json
import os
import sys

# Configuración del tablero de ajedrez
PATRON = (9, 6)  # Esquinas internas (columnas, filas)
TAMANO_CUADRADO = 2.5  # Tamaño del cuadrado en cm

# Directorio con las imágenes
IMAGENES_DIR = "imagenes/"


def calibrar_camara(imagenes_path):
    """
    Calibra la cámara usando imágenes de un tablero de ajedrez.
    """
    print("=" * 70)
    print("🤖 CALIBRACIÓN DE CÁMARA - AUTITO ROBOT")
    print("=" * 70)

    # Preparar puntos del tablero en el mundo real
    objp = np.zeros((PATRON[0] * PATRON[1], 3), np.float32)
    objp[:, :2] = np.mgrid[0 : PATRON[0], 0 : PATRON[1]].T.reshape(-1, 2)
    objp *= TAMANO_CUADRADO

    # Arrays para almacenar puntos
    objpoints = []  # Puntos 3D en el mundo real
    imgpoints = []  # Puntos 2D en la imagen

    # Leer imágenes
    images = glob.glob(imagenes_path)

    if len(images) == 0:
        print(f"❌ No se encontraron imágenes en: {imagenes_path}")
        print(f"   Por favor, guarda las fotos del tablero en '{IMAGENES_DIR}'")
        return None

    print(f"\n📸 Procesando {len(images)} imágenes...")
    print("-" * 70)

    imagenes_validas = 0
    for idx, fname in enumerate(images, 1):
        img = cv2.imread(fname)
        if img is None:
            print(f"  ⚠️  [{idx:2d}] {os.path.basename(fname)} - No se pudo leer")
            continue

        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        # Encontrar esquinas del tablero
        ret, corners = cv2.findChessboardCorners(gray, PATRON, None)

        if ret:
            objpoints.append(objp)

            # Refinar detección de esquinas
            criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
            corners2 = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
            imgpoints.append(corners2)

            imagenes_validas += 1
            print(f"  ✅ [{idx:2d}] {os.path.basename(fname)}")
        else:
            print(
                f"  ❌ [{idx:2d}] {os.path.basename(fname)} - No se detectó el patrón"
            )

    print("-" * 70)
    print(f"📊 Resultado: {imagenes_validas}/{len(images)} imágenes válidas")

    if imagenes_validas < 10:
        print("⚠️  ADVERTENCIA: Pocas imágenes válidas. Recomendado: >20")
        print("   La calibración puede ser imprecisa.")

    if imagenes_validas == 0:
        print("❌ No se pudo calibrar. Ninguna imagen válida.")
        return None

    # Calibrar
    print("\n🔬 Calibrando cámara...")
    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(
        objpoints, imgpoints, gray.shape[::-1], None, None
    )

    if ret:
        print("✅ Calibración exitosa!")

        # Calcular error de reproyección
        mean_error = 0
        for i in range(len(objpoints)):
            imgpoints2, _ = cv2.projectPoints(
                objpoints[i], rvecs[i], tvecs[i], mtx, dist
            )
            error = cv2.norm(imgpoints[i], imgpoints2, cv2.NORM_L2) / len(imgpoints2)
            mean_error += error
        mean_error /= len(objpoints)

        print(f"📏 Error de reproyección: {mean_error:.4f} píxeles")

        if mean_error < 0.5:
            print("   ✅ Excelente precisión!")
        elif mean_error < 1.0:
            print("   ✅ Buena precisión")
        else:
            print("   ⚠️  Precisión aceptable (considera tomar más fotos)")

        return {
            "ret": ret,
            "mtx": mtx,
            "dist": dist,
            "rvecs": rvecs,
            "tvecs": tvecs,
            "error": mean_error,
        }
    else:
        print("❌ Error en la calibración")
        return None


def generar_config_h(result):
    """
    Genera el código para config.h
    """
    mtx = result["mtx"]
    dist = result["dist"][0]

    print("\n" + "=" * 70)
    print("📋 COPIA ESTOS VALORES EN include/config.h")
    print("=" * 70)
    print("\n// --- MATRIZ DE CALIBRACIÓN ---")
    print(f"#define CAM_MTX_00 {mtx[0, 0]:.2f}f  // fx - Distancia focal X")
    print(f"#define CAM_MTX_01 {mtx[0, 1]:.2f}f")
    print(f"#define CAM_MTX_02 {mtx[0, 2]:.2f}f  // cx - Centro óptico X")
    print(f"#define CAM_MTX_10 {mtx[1, 0]:.2f}f")
    print(f"#define CAM_MTX_11 {mtx[1, 1]:.2f}f  // fy - Distancia focal Y")
    print(f"#define CAM_MTX_12 {mtx[1, 2]:.2f}f  // cy - Centro óptico Y")
    print(f"#define CAM_MTX_20 {mtx[2, 0]:.2f}f")
    print(f"#define CAM_MTX_21 {mtx[2, 1]:.2f}f")
    print(f"#define CAM_MTX_22 {mtx[2, 2]:.2f}f")
    print("\n// --- COEFICIENTES DE DISTORSIÓN ---")
    print(f"#define CAM_DIST_K1  {dist[0]:.6f}f  // Distorsión radial k1")
    print(f"#define CAM_DIST_K2  {dist[1]:.6f}f  // Distorsión radial k2")
    print(f"#define CAM_DIST_P1  {dist[2]:.6f}f  // Distorsión tangencial p1")
    print(f"#define CAM_DIST_P2  {dist[3]:.6f}f  // Distorsión tangencial p2")
    print(f"#define CAM_DIST_K3  {dist[4]:.6f}f  // Distorsión radial k3")
    print("\n" + "=" * 70)

    focal_promedio = (mtx[0, 0] + mtx[1, 1]) / 2
    print(f"\n🎯 Distancia Focal Promedio: {focal_promedio:.2f} píxeles")
    print(f"   Actualiza también: #define FOCAL_AUTITO_PX {focal_promedio:.2f}f")
    print(f"\n📐 Centro Óptico: ({mtx[0, 2]:.1f}, {mtx[1, 2]:.1f})")

    # Guardar en archivo
    with open("calibracion_resultado.txt", "w") as f:
        f.write("RESULTADOS DE CALIBRACIÓN\n")
        f.write("=" * 70 + "\n\n")
        f.write(f"Distancia Focal (fx): {mtx[0, 0]:.2f} px\n")
        f.write(f"Distancia Focal (fy): {mtx[1, 1]:.2f} px\n")
        f.write(f"Centro Óptico (cx): {mtx[0, 2]:.2f} px\n")
        f.write(f"Centro Óptico (cy): {mtx[1, 2]:.2f} px\n\n")
        f.write("Matriz de Calibración:\n")
        f.write(str(mtx) + "\n\n")
        f.write("Coeficientes de Distorsión:\n")
        f.write(str(dist) + "\n\n")
        f.write(f"Error de reproyección: {result['error']:.4f} px\n")

    print(f"\n💾 Resultados guardados en: calibracion_resultado.txt")


def main():
    # Verificar que existe el directorio de imágenes
    if not os.path.exists(IMAGENES_DIR):
        print(f"❌ El directorio '{IMAGENES_DIR}' no existe.")
        print(f"   Creando directorio...")
        os.makedirs(IMAGENES_DIR)
        print(f"\n📝 PASOS PARA CALIBRAR:")
        print(f"   1. Imprime un tablero de ajedrez ({PATRON[0]}x{PATRON[1]} esquinas)")
        print(f"   2. Toma 20-30 fotos del tablero desde diferentes ángulos")
        print(f"   3. Guarda las fotos en '{IMAGENES_DIR}'")
        print(f"   4. Vuelve a ejecutar este script")
        return

    imagenes_path = os.path.join(IMAGENES_DIR, "*.jpg")

    # Intentar también con PNG
    if len(glob.glob(imagenes_path)) == 0:
        imagenes_path = os.path.join(IMAGENES_DIR, "*.png")

    result = calibrar_camara(imagenes_path)

    if result:
        # Guardar resultados en formato NumPy
        np.savez(
            "calibracion_resultado.npz",
            mtx=result["mtx"],
            dist=result["dist"],
            error=result["error"],
        )
        print("\n💾 Resultados guardados en: calibracion_resultado.npz")

        # Generar código para config.h
        generar_config_h(result)

        print("\n" + "=" * 70)
        print("✅ CALIBRACIÓN COMPLETADA")
        print("=" * 70)
    else:
        print("\n❌ CALIBRACIÓN FALLIDA")
        sys.exit(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Calibración interrumpida por el usuario")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)

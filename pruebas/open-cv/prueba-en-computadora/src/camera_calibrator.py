import cv2
import numpy as np
import os

# IDs de las cámaras
CAM_ID_1 = 0  # Cámara principal (Webcam fija)
CAM_ID_2 = 2  # Cámara secundaria (Webcam del robot)

# Propiedades del tablero de ajedrez
CHESSBOARD_DIMS = (7, 7)
# Lado de un cuadrado del tablero
SQUARE_SIZE_CM = 4.0

# 40 imagenes para mayor precision
NUM_IMAGES_TO_CAPTURE = 40

CALIBRATION_FILE_NAME = "calibration_data.npz"


criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

objp = np.zeros((CHESSBOARD_DIMS[0] * CHESSBOARD_DIMS[1], 3), np.float32)
objp[:, :2] = np.mgrid[0 : CHESSBOARD_DIMS[0], 0 : CHESSBOARD_DIMS[1]].T.reshape(-1, 2)
objp = objp * SQUARE_SIZE_CM

objpoints = []
imgpoints1 = []
imgpoints2 = []

cap1 = cv2.VideoCapture(CAM_ID_1)
cap2 = cv2.VideoCapture(CAM_ID_2)

if not cap1.isOpened() or not cap2.isOpened():
    print("Error: No se pudieron abrir una o ambas cámaras.")
    exit()

print("\n--- Herramienta de Calibración Estéreo ---")
print("Muestre el tablero de ajedrez a ambas cámaras desde diferentes ángulos.")
print("Presione la tecla c para capturar un par de imágenes.")
print(f"Se necesitan {NUM_IMAGES_TO_CAPTURE} pares de imágenes.")
print("Presione 'q' para salir.")
print("-" * 50)

captured_count = 0
frame_shape = None

while captured_count < NUM_IMAGES_TO_CAPTURE:
    ret1, frame1 = cap1.read()
    ret2, frame2 = cap2.read()

    if not ret1 or not ret2:
        print("Error al leer frame.")
        break

    if frame_shape is None:
        frame_shape = frame1.shape[:2][::-1]

    # Convertir a escala de grises
    gray1 = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
    gray2 = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)

    # Flags para mejorar la detección en tableros no perfectos
    find_flags = (
        cv2.CALIB_CB_ADAPTIVE_THRESH
        + cv2.CALIB_CB_NORMALIZE_IMAGE
        + cv2.CALIB_CB_FAST_CHECK
    )

    # Encontrar las esquinas del tablero de ajedrez
    ret_corners1, corners1 = cv2.findChessboardCorners(
        gray1, CHESSBOARD_DIMS, find_flags
    )
    ret_corners2, corners2 = cv2.findChessboardCorners(
        gray2, CHESSBOARD_DIMS, find_flags
    )

    # Si se encuentran en ambas imágenes, dibujarlos
    if ret_corners1:
        cv2.drawChessboardCorners(frame1, CHESSBOARD_DIMS, corners1, ret_corners1)
    if ret_corners2:
        cv2.drawChessboardCorners(frame2, CHESSBOARD_DIMS, corners2, ret_corners2)

    status_text = f"Capturados: {captured_count}/{NUM_IMAGES_TO_CAPTURE}"
    cv2.putText(
        frame1, status_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2
    )
    cv2.putText(
        frame2, status_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2
    )

    cv2.imshow("Camara 1", frame1)
    cv2.imshow("Camara 2", frame2)

    key = cv2.waitKey(1) & 0xFF

    if key == ord("q"):
        break
    elif key == ord("c") and ret_corners1 and ret_corners2:
        # Guardar puntos solo si el tablero se detecta en AMBAS imágenes
        objpoints.append(objp)

        # Refinar las coordenadas de las esquinas para mayor precisión
        corners1_subpix = cv2.cornerSubPix(
            gray1, corners1, (11, 11), (-1, -1), criteria
        )
        imgpoints1.append(corners1_subpix)

        corners2_subpix = cv2.cornerSubPix(
            gray2, corners2, (11, 11), (-1, -1), criteria
        )
        imgpoints2.append(corners2_subpix)

        captured_count += 1
        print(f"Par de imágenes {captured_count} capturado.")

if captured_count < NUM_IMAGES_TO_CAPTURE:
    print("\nCalibración cancelada. No se capturaron suficientes imágenes.")
else:
    print("\nImágenes capturadas. Calibrando cámaras individualmente...")

    # calibrar cada cámara por separado para obtener una estimación inicial
    ret1, mtx1, dist1, rvecs1, tvecs1 = cv2.calibrateCamera(
        objpoints, imgpoints1, frame_shape, None, None
    )
    ret2, mtx2, dist2, rvecs2, tvecs2 = cv2.calibrateCamera(
        objpoints, imgpoints2, frame_shape, None, None
    )

    print("Calibrando sistema estéreo...")

    # realizar la calibración estéreo usando los parámetros intrínsecos como estimación inicial
    stereo_flags = cv2.CALIB_FIX_INTRINSIC
    ret, mtx1, dist1, mtx2, dist2, R, T, E, F = cv2.stereoCalibrate(
        objpoints,
        imgpoints1,
        imgpoints2,
        mtx1,
        dist1,
        mtx2,
        dist2,
        frame_shape,
        criteria=criteria,
        flags=stereo_flags,
    )

    if ret:
        print("Calibración exitosa")
        np.savez(
            CALIBRATION_FILE_NAME,
            mtx1=mtx1,
            dist1=dist1,
            mtx2=mtx2,
            dist2=dist2,
            R=R,
            T=T,
            E=E,
            F=F,
        )
        print(f"Datos de calibración guardados en '{CALIBRATION_FILE_NAME}'")
    else:
        print("La calibración estéreo falló.")

cap1.release()
cap2.release()
cv2.destroyAllWindows()

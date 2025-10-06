import cv2
import numpy as np

CAM_ID_1 = 0  # Cámara principal (Webcam fija)
CAM_ID_2 = 2  # Cámara secundaria (la del robot)

# Rango de color del objeto a detectar en HSV: amarillo
COLOR_LOWER_BOUND = np.array([20, 100, 100])
COLOR_UPPER_BOUND = np.array([30, 255, 255])

CALIBRATION_FILE_NAME = "calibration_data.npz"


def find_object_center(frame, lower_bound, upper_bound):
    """Encuentra el centroide del objeto de color más grande en el frame."""
    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    mask = cv2.inRange(hsv, lower_bound, upper_bound)
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)

    contours, _ = cv2.findContours(
        mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )

    if len(contours) > 0:
        c = max(contours, key=cv2.contourArea)
        M = cv2.moments(c)
        if M["m00"] > 0:
            return (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
    return None


try:
    with np.load(CALIBRATION_FILE_NAME) as data:
        mtx1, dist1 = data["mtx1"], data["dist1"]
        mtx2, dist2 = data["mtx2"], data["dist2"]
        R, T = data["R"], data["T"]
except FileNotFoundError:
    print(f"Error: Archivo de calibración '{CALIBRATION_FILE_NAME}' no encontrado.")
    print("Por favor, ejecute primero el script 'camera_calibrator.py'.")
    exit()

# Crear matrices de proyección para la triangulación
# P1 = K1 * [I | 0]
P1 = mtx1 @ np.hstack((np.identity(3), np.zeros((3, 1))))
# P2 = K2 * [R | T]
P2 = mtx2 @ np.hstack((R, T))

# Inicializar cámaras
cap1 = cv2.VideoCapture(CAM_ID_1)
cap2 = cv2.VideoCapture(CAM_ID_2)

if not cap1.isOpened() or not cap2.isOpened():
    print("Error: No se pudieron abrir una o ambas cámaras.")
    exit()

while True:
    ret1, frame1 = cap1.read()
    ret2, frame2 = cap2.read()

    if not ret1 or not ret2:
        print("Error al leer frame.")
        break

    center1 = find_object_center(frame1, COLOR_LOWER_BOUND, COLOR_UPPER_BOUND)
    center2 = find_object_center(frame2, COLOR_LOWER_BOUND, COLOR_UPPER_BOUND)

    if center1 is not None and center2 is not None:
        # Dibujar los centros detectados
        cv2.circle(frame1, center1, 5, (0, 255, 255), -1)
        cv2.circle(frame2, center2, 5, (0, 255, 255), -1)

        points1 = np.array([center1], dtype=np.float32).reshape(1, 1, 2)
        points2 = np.array([center2], dtype=np.float32).reshape(1, 1, 2)

        # Triangular el punto 3D
        point4d_hom = cv2.triangulatePoints(P1, P2, points1.T, points2.T)

        # Convertir de coordenadas homogéneas a 3D
        if point4d_hom[3][0] != 0:
            point3d = (point4d_hom[:3] / point4d_hom[3]).flatten()

            # El punto 'point3d' está en el sistema de coordenadas de la CÁMARA 1.
            # Lo transformamos al sistema de coordenadas de la CÁMARA 2.
            # La fórmula de transformación es: P_c2 = R.T * (P_c1 - T)
            point3d_cam1_frame = point3d.reshape(3, 1)
            point3d_cam2_frame = R.T @ (point3d_cam1_frame - T)

            # La distancia al objeto desde la cámara 2 es la magnitud (norma) de este vector.
            distancia_a_cam2 = np.linalg.norm(point3d_cam2_frame)

            # Mostrar información
            info_cam1 = f"3D (Cam1 Frame): Z={point3d[2]:.1f} cm"
            info_cam2 = f"Distancia a esta camara: {distancia_a_cam2:.1f} cm"
            cv2.putText(
                frame1,
                info_cam1,
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (255, 255, 0),
                2,
            )
            cv2.putText(
                frame2,
                info_cam2,
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 255),
                2,
            )

    cv2.imshow("Camara 1 (Fija)", frame1)
    cv2.imshow("Camara 2 (Robot)", frame2)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

cap1.release()
cap2.release()
cv2.destroyAllWindows()

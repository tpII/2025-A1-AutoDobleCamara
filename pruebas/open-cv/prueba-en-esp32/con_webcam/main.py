import cv2
import numpy as np


def setup_detector():
    """
    Configura los rangos de color para la detección de objetos naranjas.
    """
    # Rango de color naranja en HSV
    lower_orange = np.array([5, 100, 100])
    upper_orange = np.array([25, 255, 255])
    return lower_orange, upper_orange


def detect_object(frame, lower_range, upper_range):
    """
    Detecta un objeto dentro de un rango de color en un frame.
    """
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Crear una máscara para el color naranja
    mask = cv2.inRange(hsv, lower_range, upper_range)

    # Opcional: Mejorar la máscara con operaciones morfológicas
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)

    # Encontrar contornos en la máscara
    contours, _ = cv2.findContours(
        mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )

    if contours:
        # Encontrar el contorno más grande
        largest_contour = max(contours, key=cv2.contourArea)
        area = cv2.contourArea(largest_contour)

        # Filtrar por área mínima para reducir el ruido
        if area > 500:
            # Obtener el cuadro delimitador
            x, y, w, h = cv2.boundingRect(largest_contour)
            return True, (x, y, w, h)

    return False, None


def main():
    # Inicializar la webcam
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: No se pudo abrir la cámara.")
        return

    # Configurar el detector de color
    lower_orange, upper_orange = setup_detector()

    print("Presiona 'q' para salir.")

    while True:
        # Capturar frame por frame
        ret, frame = cap.read()
        if not ret:
            print("Error: No se pudo capturar el frame.")
            break

        # Detectar el objeto
        found, rect = detect_object(frame, lower_orange, upper_orange)

        # Si se encuentra un objeto, dibujar un rectángulo verde
        if found:
            x, y, w, h = rect
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(
                frame,
                "Objeto Detectado",
                (x, y - 10),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )

        # Mostrar el frame resultante
        cv2.imshow("Deteccion de Objeto", frame)

        # Salir del bucle si se presiona la tecla 'q'
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    # Liberar la cámara y cerrar todas las ventanas
    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

import cv2
import numpy as np


def main():
    # Desde la webcam
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        return

    # Para un objeto ROJO
    red_lower = np.array([170, 120, 70])
    red_upper = np.array([180, 255, 255])
    while True:
        ret, frame = cap.read()
        if not ret:
            print("Failed to capture frame")
            break

        # Filtra la imagen para reducir ruido y detalles innecesarios.
        blurred_frame = cv2.GaussianBlur(frame, (11, 11), 0)

        # HSV = Hue, Saturation, Value (brightness).
        # Convierte la imagen desde formato BGR a HSV (son formatos distintos de imagen)
        hsv_frame = cv2.cvtColor(blurred_frame, cv2.COLOR_BGR2HSV)

        # Pixeles rojos -> blanco
        # Caulquier otro color, pasa a negro
        mask = cv2.inRange(hsv_frame, red_lower, red_upper)

        # Saca puntitos blancos (blobs)
        mask = cv2.erode(mask, None, iterations=2)
        mask = cv2.dilate(mask, None, iterations=2)

        # Identifica contornos de objetos blancos
        contours, _ = cv2.findContours(
            mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
        )

        if len(contours) > 0:
            c = max(contours, key=cv2.contourArea)
            (x, y, w, h) = cv2.boundingRect(c)

            # if w > 0:
            #     distance_cm = (KNOWN_OBJECT_WIDTH_CM * FOCAL_LENGTH_CONSTANT) / w

            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            # if distance_cm is not None:
            #     cv2.putText(frame, f"{distance_cm:.1f} cm", (x, y - 10),
            #                 cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        cv2.imshow("Mask", mask)
        cv2.imshow(
            "Result", frame
        )  # Se ve medio cursed, pero sirve para procesar imagenes

        if "c" in locals():
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

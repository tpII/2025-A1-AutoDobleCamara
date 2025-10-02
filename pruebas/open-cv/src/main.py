import cv2


def main():
    # Desde la webcam
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Error: Could not open camera.")
        return

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
        cv2.imshow(
            "Webcam", hsv_frame
        )  # Se ve medio cursed, pero sirve para procesar imagenes

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

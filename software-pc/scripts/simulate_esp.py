"""
Simula dos ESP32-CAM enviando imágenes JPEG al servidor Flask.

Genera dos imágenes con un círculo amarillo que se mueve para simular detección.

Uso:
  source venv/bin/activate
  python3 scripts/simulate_esp.py

Este script hace POST a http://localhost:5000/video/cam1 y /video/cam2
"""
import time
import io
import requests
import cv2
import numpy as np

URL_CAM1 = 'http://localhost:5000/video/cam1'
URL_CAM2 = 'http://localhost:5000/video/cam2'

WIDTH = 640
HEIGHT = 480

def make_frame(x, y, color=(0, 255, 255)):
    img = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8)
    cv2.circle(img, (int(x), int(y)), 30, color, -1)
    return img


def post_image(url, img):
    _, buf = cv2.imencode('.jpg', img)
    try:
        resp = requests.post(url, data=buf.tobytes(), timeout=1.0)
        return resp.status_code == 200
    except Exception as e:
        print('Error posting to', url, e)
        return False


if __name__ == '__main__':
    t = 0.0
    try:
        while True:
            # Movimiento simple: cam1 circle moves horizontally, cam2 moves vertically
            x1 = (WIDTH//2) + (WIDTH//4) * np.cos(t)
            y1 = HEIGHT//2
            x2 = WIDTH//2
            y2 = (HEIGHT//2) + (HEIGHT//4) * np.sin(t)

            f1 = make_frame(x1, y1)
            f2 = make_frame(x2, y2)

            ok1 = post_image(URL_CAM1, f1)
            ok2 = post_image(URL_CAM2, f2)

            if not ok1 or not ok2:
                print('One or both posts failed. Is the server running?')

            t += 0.1
            time.sleep(0.1)
    except KeyboardInterrupt:
        print('Simulation stopped by user')

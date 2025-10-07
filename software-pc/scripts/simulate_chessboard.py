"""
Simula dos ESP32-CAM enviando imágenes JPEG con un tablero de ajedrez.
Útil para probar la calibración automáticamente.

Uso:
  source venv/bin/activate
  python3 scripts/simulate_chessboard.py
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

# Chessboard parameters (match calibrator defaults)
CHESS_W = 7
CHESS_H = 7
SQUARE_PX = 40


def draw_chessboard(w, h, square_px, offset_x=0, offset_y=0, angle=0.0):
    cols = w
    rows = h
    board_w = cols * square_px
    board_h = rows * square_px
    img = np.zeros((HEIGHT, WIDTH, 3), dtype=np.uint8) + 255

    # build board at center and then rotate/translate
    board = np.zeros((board_h, board_w), dtype=np.uint8)
    for y in range(rows):
        for x in range(cols):
            if (x + y) % 2 == 0:
                cv2.rectangle(board, (x * square_px, y * square_px), ((x + 1) * square_px, (y + 1) * square_px), 255, -1)

    # convert to BGR
    board_bgr = cv2.cvtColor(board, cv2.COLOR_GRAY2BGR)

    # resize if too big
    scale = min(WIDTH / (board_w + 1), HEIGHT / (board_h + 1), 1.0)
    board_bgr = cv2.resize(board_bgr, (int(board_w * scale), int(board_h * scale)), interpolation=cv2.INTER_AREA)

    bh, bw = board_bgr.shape[:2]
    x0 = (WIDTH - bw) // 2 + int(offset_x)
    y0 = (HEIGHT - bh) // 2 + int(offset_y)

    # place board
    img[y0:y0+bh, x0:x0+bw] = board_bgr

    # rotate around center
    M = cv2.getRotationMatrix2D((WIDTH/2, HEIGHT/2), angle, 1.0)
    img = cv2.warpAffine(img, M, (WIDTH, HEIGHT), borderValue=(255,255,255))

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
            angle = 10.0 * np.sin(t)
            off = 30.0 * np.cos(t/2)

            f1 = draw_chessboard(CHESS_W, CHESS_H, SQUARE_PX, offset_x=off, offset_y=0, angle=angle)
            f2 = draw_chessboard(CHESS_W, CHESS_H, SQUARE_PX, offset_x=-off, offset_y=0, angle=-angle)

            ok1 = post_image(URL_CAM1, f1)
            ok2 = post_image(URL_CAM2, f2)

            if not ok1 or not ok2:
                print('One or both posts failed. Is the server running?')

            t += 0.2
            time.sleep(0.2)
    except KeyboardInterrupt:
        print('Simulation stopped by user')

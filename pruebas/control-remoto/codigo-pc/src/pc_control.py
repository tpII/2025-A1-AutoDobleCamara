#!/usr/bin/env python3
"""
Simple PC controller for the ESP32 robot.

Features:
- Starts a TCP server and waits for the ESP32 to connect (ESP32 must be configured to connect to this host:port).
- When the ESP connects, the script grabs the client's IP and opens the camera stream at http://<ESP_IP>:8080/stream
- Shows a window with the live stream and sends motor commands when you press arrow keys.

Controls:
- Up arrow: forward (ALL F <speed>)
- Down arrow: backward (ALL R <speed>)
- Left arrow: turn left (MOTOR 1 R <speed> ; MOTOR 2 F <speed>)
- Right arrow: turn right (MOTOR 1 F <speed> ; MOTOR 2 R <speed>)
- Space: STOP ("STOP")
- + / = : increase speed (by 10)
- - : decrease speed (by 10)
- q : quit

Dependencies: opencv-python, requests
pip install opencv-python requests

Run from project root:
  python3 software-auto/scripts/pc_control.py --port 12345

"""

import socket
import threading
import argparse
import time
import sys

try:
    import cv2
except Exception:
    cv2 = None

try:
    import requests
except Exception:
    requests = None

DEFAULT_PORT = 12345
DEFAULT_SPEED = 150


class TCPServer:
    def __init__(self, host="0.0.0.0", port=DEFAULT_PORT):
        self.host = host
        self.port = port
        self.sock = None
        self.client = None
        self.client_addr = None
        self.running = False

    def start(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        self.sock.listen(1)
        self.running = True
        print(
            f"TCP server listening on {self.host}:{self.port}... waiting for ESP32 to connect"
        )
        self.accept_thread = threading.Thread(target=self._accept_loop, daemon=True)
        self.accept_thread.start()

    def _accept_loop(self):
        while self.running:
            try:
                client, addr = self.sock.accept()
            except OSError:
                break
            if client:
                print(f"Client connected from {addr}")
                # close previous client if any
                if self.client:
                    try:
                        self.client.close()
                    except Exception:
                        pass
                self.client = client
                self.client_addr = addr
                # start receiver thread
                t = threading.Thread(
                    target=self._recv_loop, args=(client,), daemon=True
                )
                t.start()

    def _recv_loop(self, client):
        with client:
            while self.running:
                try:
                    data = client.recv(1024)
                except OSError:
                    break
                if not data:
                    break
                try:
                    print(f"ESP -> {data.decode().strip()}")
                except Exception:
                    print("ESP -> <binary>")
        print("Client disconnected")
        self.client = None
        self.client_addr = None

    def send(self, msg: str):
        if not self.client:
            print("No ESP connected")
            return False
        try:
            self.client.sendall(msg.encode())
            return True
        except Exception as e:
            print("Send failed:", e)
            return False

    def stop(self):
        self.running = False
        try:
            if self.client:
                self.client.close()
        except Exception:
            pass
        try:
            if self.sock:
                self.sock.close()
        except Exception:
            pass


def open_stream_and_show(esp_ip, stop_event):
    url = f"http://{esp_ip}:8080/stream"
    print(f"Opening stream {url}")
    if cv2 is None:
        print("OpenCV not installed. Install opencv-python to view stream.")
        return

    # First try VideoCapture
    cap = cv2.VideoCapture(url)
    if not cap.isOpened():
        print("cv2.VideoCapture couldn't open stream, trying requests fallback")
        cap.release()
        if requests is None:
            print(
                "requests not installed; cannot use fallback. Install requests and opencv-python."
            )
            return
        # fallback: read MJPEG multipart via requests
        try:
            resp = requests.get(url, stream=True, timeout=5)
            if resp.status_code != 200:
                print("Stream HTTP error", resp.status_code)
                return
            bytes_stream = b""
            for chunk in resp.iter_content(chunk_size=1024):
                if stop_event.is_set():
                    break
                bytes_stream += chunk
                a = bytes_stream.find(b"\xff\xd8")
                b = bytes_stream.find(b"\xff\xd9")
                if a != -1 and b != -1:
                    jpg = bytes_stream[a : b + 2]
                    bytes_stream = bytes_stream[b + 2 :]
                    import numpy as np

                    img = cv2.imdecode(
                        np.frombuffer(jpg, dtype="uint8"), cv2.IMREAD_COLOR
                    )
                    if img is not None:
                        cv2.imshow("ESP32 Stream", img)
                        if cv2.waitKey(1) & 0xFF == ord("q"):
                            stop_event.set()
                            break
            resp.close()
        except Exception as e:
            print("Stream fallback failed:", e)
        return

    # use VideoCapture loop
    while not stop_event.is_set():
        ret, frame = cap.read()
        if not ret:
            # small sleep to avoid busy loop
            time.sleep(0.05)
            continue
        cv2.imshow("ESP32 Stream", frame)
        # don't handle key here; main loop will call waitKey
        if cv2.waitKey(1) & 0xFF == ord("q"):
            stop_event.set()
            break

    cap.release()
    cv2.destroyAllWindows()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--speed", type=int, default=DEFAULT_SPEED)
    args = parser.parse_args()

    server = TCPServer(host=args.host, port=args.port)
    server.start()

    stop_event = threading.Event()
    stream_thread = None

    speed = max(0, min(255, args.speed))
    last_cmd = None

    print("Controls: arrows to move, space to stop, +/- to change speed, q to quit")

    try:
        # main input loop uses OpenCV waitKey to capture arrow keys if available
        if cv2 is None:
            print(
                "Warning: OpenCV not available; keyboard control will still work but no stream window."
            )
        while True:
            # if an ESP connected and we don't have stream thread, start it
            if server.client_addr and stream_thread is None:
                esp_ip = server.client_addr[0]
                stop_event.clear()
                stream_thread = threading.Thread(
                    target=open_stream_and_show, args=(esp_ip, stop_event), daemon=True
                )
                stream_thread.start()

            # capture key using OpenCV if available
            key = -1
            if cv2:
                key = cv2.waitKey(50)
            else:
                # fallback: non-blocking stdin? use time.sleep
                time.sleep(0.05)

            # map key codes: OpenCV uses 81/82/83/84 for arrows on many systems
            cmd_sent = False
            if key == ord("q"):
                break
            if key == ord(" "):
                server.send("STOP\n")
                last_cmd = "STOP"
                cmd_sent = True
            if key == ord("+") or key == ord("="):
                speed = min(255, speed + 10)
                print("Speed ->", speed)
            if key == ord("-"):
                speed = max(0, speed - 10)
                print("Speed ->", speed)

            # arrow keys
            if key == 82:  # up
                server.send(f"ALL F {speed}\n")
                last_cmd = f"ALL F {speed}"
                cmd_sent = True
            elif key == 84:  # down
                server.send(f"ALL R {speed}\n")
                last_cmd = f"ALL R {speed}"
                cmd_sent = True
            elif key == 81:  # left
                # spin left: left motor back, right motor forward
                server.send(f"MOTOR 1 R {speed}\n")
                server.send(f"MOTOR 2 F {speed}\n")
                last_cmd = f"LEFT {speed}"
                cmd_sent = True
            elif key == 83:  # right
                server.send(f"MOTOR 1 F {speed}\n")
                server.send(f"MOTOR 2 R {speed}\n")
                last_cmd = f"RIGHT {speed}"
                cmd_sent = True

            # if no key pressed and last command was movement, optionally send STOP
            # (commented out to keep continuous hold semantics)
            # if key == -1 and last_cmd and last_cmd != 'STOP':
            #     server.send('STOP\n')
            #     last_cmd = 'STOP'

            # small sleep to reduce CPU
            time.sleep(0.01)

    except KeyboardInterrupt:
        print("Interrupted")
    finally:
        stop_event.set()
        server.stop()
        print("Exiting")


if __name__ == "__main__":
    main()

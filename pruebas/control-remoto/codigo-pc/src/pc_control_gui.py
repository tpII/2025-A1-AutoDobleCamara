#!/usr/bin/env python3
"""Basic GUI to view ESP stream and send control commands.

This provides:
- A connection status indicator (shows client IP when connected).
- A canvas that displays the MJPEG stream (via OpenCV or requests fallback).
- Buttons for basic controls (Forward, Backward, Left, Right, Stop) that send commands
  to the connected ESP via the running TCPServer in `pc_control.py`.

Run alongside the TCP server: the script will create its own TCPServer instance.
"""

import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

try:
    from PIL import Image, ImageTk
except Exception:
    Image = None
    ImageTk = None

import io

try:
    import requests
except Exception:
    requests = None

try:
    import cv2
except Exception:
    cv2 = None

from pc_control import TCPServer


class PCControlGUI(tk.Tk):
    def __init__(self, host="0.0.0.0", port=12345):
        super().__init__()
        self.title("PC Control GUI")
        self.geometry("720x560")

        self.server = TCPServer(host=host, port=port)
        self.server.start()

        self.conn_label = ttk.Label(self, text="No ESP connected", foreground="red")
        self.conn_label.pack(pady=6)

        # Canvas for stream
        self.canvas = tk.Label(self)
        self.canvas.pack(expand=True)
        # Control buttons
        frm = ttk.Frame(self)
        frm.pack(pady=6)
        ttk.Button(
            frm, text="Forward", command=lambda: self.send_cmd("ALL F 150\n")
        ).grid(row=0, column=1)
        ttk.Button(
            frm,
            text="Left",
            command=lambda: self.send_cmd("MOTOR 1 R 150\n;MOTOR 2 F 150\n"),
        ).grid(row=1, column=0)
        ttk.Button(frm, text="Stop", command=lambda: self.send_cmd("STOP\n")).grid(
            row=1, column=1
        )
        ttk.Button(
            frm,
            text="Right",
            command=lambda: self.send_cmd("MOTOR 1 F 150\n;MOTOR 2 R 150\n"),
        ).grid(row=1, column=2)
        ttk.Button(
            frm, text="Backward", command=lambda: self.send_cmd("ALL R 150\n")
        ).grid(row=2, column=1)

        # retry button
        self.retry_btn = ttk.Button(
            frm, text="Reintentar stream", command=self.retry_stream
        )
        self.retry_btn.grid(row=2, column=2, padx=6)

        self._stop_event = threading.Event()
        self._stream_thread = None
        self._stream_running = False
        self.last_esp_ip = None

        # periodic UI update
        self.after(200, self._periodic)

    def send_cmd(self, cmd: str):
        # support multiple commands separated by ; for convenience
        for part in cmd.split(";"):
            part = part.strip()
            if part:
                ok = self.server.send(part)
                if not ok:
                    messagebox.showwarning(
                        "No connection", "No ESP connected to send command"
                    )

    def _periodic(self):
        # Update connection label
        if self.server.client_addr:
            self.conn_label.config(
                text=f"Connected: {self.server.client_addr[0]}", foreground="green"
            )
            esp_ip = self.server.client_addr[0]
            # if stream not running, start it
            if not self._stream_running:
                self.start_stream(esp_ip)
        else:
            self.conn_label.config(text="No ESP connected", foreground="red")
            # stop stream thread
            self._stop_event.set()

        self.after(200, self._periodic)

    def start_stream(self, esp_ip):
        # stop previous
        self.stop_stream()
        self.last_esp_ip = esp_ip
        self._stop_event.clear()
        self._stream_thread = threading.Thread(
            target=self._stream_loop, args=(esp_ip,), daemon=True
        )
        self._stream_thread.start()

    def stop_stream(self):
        try:
            if self._stream_thread and self._stream_thread.is_alive():
                self._stop_event.set()
                self._stream_thread.join(timeout=1)
        except Exception:
            pass
        self._stream_thread = None
        self._stream_running = False

    def retry_stream(self):
        if self.last_esp_ip:
            self.start_stream(self.last_esp_ip)
        else:
            messagebox.showinfo(
                "Reintentar", "No hay IP de ESP conocida. Esperando conexión TCP."
            )

    def _stream_loop(self, esp_ip):
        url = f"http://{esp_ip}:8080/stream"
        print("Starting stream thread for", url)
        # Try cv2.VideoCapture first
        try:
            cap = cv2.VideoCapture(url)
            if cap.isOpened():
                while not self._stop_event.is_set():
                    ret, frame = cap.read()
                    if not ret:
                        time.sleep(0.05)
                        continue
                    # convert BGR to RGB
                    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                    img = Image.fromarray(frame)
                    imgtk = ImageTk.PhotoImage(image=img)
                    # update image on main thread
                    self.canvas.imgtk = imgtk
                    self.canvas.config(image=imgtk)
                    time.sleep(0.02)
                cap.release()
                return
        except Exception as e:
            print("cv2 stream error:", e)

        # fallback to requests MJPEG read
        try:
            resp = requests.get(url, stream=True, timeout=5)
            if resp.status_code != 200:
                print("Stream HTTP", resp.status_code)
                return
            bytes_stream = b""
            for chunk in resp.iter_content(chunk_size=1024):
                if self._stop_event.is_set():
                    break
                bytes_stream += chunk
                a = bytes_stream.find(b"\xff\xd8")
                b = bytes_stream.find(b"\xff\xd9")
                if a != -1 and b != -1:
                    jpg = bytes_stream[a : b + 2]
                    bytes_stream = bytes_stream[b + 2 :]
                    try:
                        img = Image.open(io.BytesIO(jpg)).convert("RGB")
                        imgtk = ImageTk.PhotoImage(img)
                        self.canvas.imgtk = imgtk
                        self.canvas.config(image=imgtk)
                    except Exception as e:
                        print("image decode error", e)
        except Exception as e:
            print("requests stream error:", e)

    def on_close(self):
        self._stop_event.set()
        self.server.stop()
        self.destroy()


def main():
    app = PCControlGUI()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()


if __name__ == "__main__":
    main()

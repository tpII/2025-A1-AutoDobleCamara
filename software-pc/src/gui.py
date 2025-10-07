"""
Modulo de GUI con Tkinter. Muestra los streams, la distancia y el joystick virtual.
"""

import tkinter as tk
from tkinter import Label, Canvas
from PIL import Image, ImageTk
import numpy as np
import cv2
from . import state, processing, control


class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Control de Robot con Doble Cámara")

        # --- boton de calibracion ---
        self.calib_button = tk.Button(
            root, text="Calibración", command=self.open_calib_window
        )
        self.calib_button.pack(pady=5)

        self.video_frame = tk.Frame(root)
        self.video_frame.pack(side=tk.TOP, padx=10, pady=10)

        self.label_cam1 = Label(self.video_frame)
        self.label_cam1.pack(side=tk.LEFT, padx=5, pady=5)

        self.label_cam2 = Label(self.video_frame)
        self.label_cam2.pack(side=tk.LEFT, padx=5, pady=5)

        self.distance_label = Label(
            root, text="Distancia: -- cm", font=("Helvetica", 16)
        )
        self.distance_label.pack(pady=10)

        self.joystick_canvas = Canvas(root, width=200, height=200, bg="lightgray")
        self.joystick_canvas.pack(pady=10)
        self.joystick_circle = self.joystick_canvas.create_oval(
            80, 80, 120, 120, fill="green"
        )
        self.joystick_active = False
        self.security_text = self.joystick_canvas.create_text(
            100, 30, text="", fill="red", font=("Helvetica", 12)
        )

        self.joystick_canvas.bind("<ButtonPress-1>", self.on_joystick_press)
        self.joystick_canvas.bind("<B1-Motion>", self.on_joystick_move)
        self.joystick_canvas.bind("<ButtonRelease-1>", self.on_joystick_release)

        self.center_x = 100
        self.center_y = 100
        self.last_command = None

        self.update_video()

    def on_joystick_press(self, event):
        self.joystick_active = True
        self.handle_joystick(event.x, event.y)

    def on_joystick_move(self, event):
        if self.joystick_active:
            self.handle_joystick(event.x, event.y)

    def on_joystick_release(self, event):
        self.joystick_active = False
        self.joystick_canvas.coords(self.joystick_circle, 80, 80, 120, 120)
        self.joystick_canvas.itemconfig(self.security_text, text="")
        control.send_command("STOP")
        self.last_command = "STOP"

    def handle_joystick(self, x, y):
        dx = x - self.center_x
        dy = self.center_y - y
        new_left = x - 20
        new_top = y - 20
        new_right = x + 20
        new_bottom = y + 20
        self.joystick_canvas.coords(
            self.joystick_circle, new_left, new_top, new_right, new_bottom
        )

        angle = np.arctan2(dy, dx) * 180 / np.pi
        threshold = 15
        command = None
        if np.hypot(dx, dy) > threshold:
            if -45 <= angle <= 45:
                command = "RIGHT"
            elif 45 < angle < 135:
                command = "FORWARD"
            elif angle >= 135 or angle <= -135:
                command = "LEFT"
            elif -135 < angle < -45:
                command = "BACKWARD"

        # Bloqueo cuando hay menos de 20cm
        if (
            command == "FORWARD"
            and state.last_distance is not None
            and state.last_distance < 20.0
        ):
            command = None
            self.joystick_canvas.itemconfig(self.security_text, text="BLOQUEADO")
            self.joystick_canvas.itemconfig(self.joystick_circle, fill="red")
        else:
            self.joystick_canvas.itemconfig(self.security_text, text="")
            self.joystick_canvas.itemconfig(self.joystick_circle, fill="green")

        if command and command != self.last_command:
            sent = control.safe_send(command)
            if sent:
                self.last_command = command

    def update_video(self):
        with state.frame_lock:
            f1 = state.frame_cam1.copy() if state.frame_cam1 is not None else None
            f2 = state.frame_cam2.copy() if state.frame_cam2 is not None else None

        if f1 is not None and f2 is not None:
            c1 = processing.find_object_center(f1)
            c2 = processing.find_object_center(f2)
            if c1 is not None and c2 is not None:
                distance = processing.triangulate_and_distance(c1, c2)
                if distance is not None:
                    self.distance_label.config(text=f"Distancia: {distance:.2f} cm")
                else:
                    self.distance_label.config(text="Distancia: -- cm")
            else:
                self.distance_label.config(text="Distancia: -- cm")

            img1 = cv2.cvtColor(f1, cv2.COLOR_BGR2RGB)
            img2 = cv2.cvtColor(f2, cv2.COLOR_BGR2RGB)
            img1 = Image.fromarray(img1)
            img2 = Image.fromarray(img2)
            imgtk1 = ImageTk.PhotoImage(image=img1.resize((320, 240)))
            imgtk2 = ImageTk.PhotoImage(image=img2.resize((320, 240)))
            self.label_cam1.imgtk = imgtk1
            self.label_cam2.imgtk = imgtk2
            self.label_cam1.config(image=imgtk1)
            self.label_cam2.config(image=imgtk2)

        self.root.after(50, self.update_video)

    # ---------------- UI de calibracion ----------------
    def open_calib_window(self):
        from .calibrate import Calibrator

        self.calib_win = tk.Toplevel(self.root)
        self.calib_win.title("Calibración estéreo")

        # auto-captura si se detecta el chessboard
        self.auto_var = tk.BooleanVar(value=False)
        auto_chk = tk.Checkbutton(
            self.calib_win, text="Auto-capturar", variable=self.auto_var
        )
        auto_chk.pack()

        self.calib = Calibrator(auto_capture=self.auto_var.get())

        self.status_label = tk.Label(self.calib_win, text="Estado: inactivo")
        self.status_label.pack(pady=5)

        btn_frame = tk.Frame(self.calib_win)
        btn_frame.pack(pady=5)

        self.capture_btn = tk.Button(
            btn_frame, text="Capturar par", command=self.capture_pair
        )
        self.capture_btn.pack(side=tk.LEFT, padx=5)

        self.finish_btn = tk.Button(
            btn_frame, text="Finalizar y Guardar", command=self.finalize_calib
        )
        self.finish_btn.pack(side=tk.LEFT, padx=5)

        self.cancel_btn = tk.Button(
            btn_frame, text="Cerrar", command=self.close_calib_window
        )
        self.cancel_btn.pack(side=tk.LEFT, padx=5)

        # Calibrator thread
        self.calib.start()
        self.update_calib_status()

    def update_calib_status(self):
        if hasattr(self, "calib") and self.calib is not None:
            # sync auto-capture
            try:
                self.calib.auto_capture = self.auto_var.get()
            except Exception:
                pass
            self.status_label.config(
                text=f"Estado: {self.calib.status} | Capturados: {self.calib.captured}"
            )
            if self.calib.captured >= self.calib.num_pairs:
                self.status_label.config(
                    text=f"Completado: {self.calib.captured} pares"
                )
                self.capture_btn.config(state=tk.DISABLED)
        if hasattr(self, "calib_win") and self.calib_win.winfo_exists():
            self.calib_win.after(200, self.update_calib_status)

    def capture_pair(self):
        ok, msg = self.calib.capture_pair()
        print("Capture:", ok, msg)

    def finalize_calib(self):
        ok, msg = self.calib.finalize()
        print("Finalize:", ok, msg)
        if ok:
            self.calib.status = msg

    def close_calib_window(self):
        if hasattr(self, "calib") and self.calib is not None:
            self.calib.running = False
        if hasattr(self, "calib_win") and self.calib_win.winfo_exists():
            self.calib_win.destroy()

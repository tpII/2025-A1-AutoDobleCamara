#!/usr/bin/env python3
"""
Capturador de frames desde el ESP32 para calibración.
Abre una UI simple donde puedes ver el stream y guardar frames con un botón.

Uso:
    python3 capturar_frames.py --ip 192.168.X.X
"""

import cv2
import numpy as np
import argparse
import os
from datetime import datetime
import requests
from threading import Thread
import time


class FrameCapture:
    def __init__(self, esp_ip):
        self.esp_ip = esp_ip
        self.stream_url = f"http://{esp_ip}/stream"
        self.frame = None
        self.running = False
        self.frame_count = 0
        self.save_dir = "imagenes"

        # Crear directorio si no existe
        if not os.path.exists(self.save_dir):
            os.makedirs(self.save_dir)

    def conectar_stream(self):
        """Intenta conectar al stream del ESP32"""
        print(f"🔌 Conectando a: {self.stream_url}")

        try:
            response = requests.get(self.stream_url, stream=True, timeout=5)
            if response.status_code == 200:
                print("✅ Conectado al stream")
                return response
            else:
                print(f"❌ Error: HTTP {response.status_code}")
                return None
        except Exception as e:
            print(f"❌ Error al conectar: {e}")
            return None

    def leer_frame_mjpeg(self, response):
        """Lee un frame del stream MJPEG"""
        bytes_data = bytes()

        for chunk in response.iter_content(chunk_size=1024):
            bytes_data += chunk

            # Buscar el inicio de un frame JPEG
            a = bytes_data.find(b"\xff\xd8")  # Inicio JPEG
            b = bytes_data.find(b"\xff\xd9")  # Fin JPEG

            if a != -1 and b != -1:
                jpg = bytes_data[a : b + 2]
                bytes_data = bytes_data[b + 2 :]

                # Decodificar JPEG
                frame = cv2.imdecode(
                    np.frombuffer(jpg, dtype=np.uint8), cv2.IMREAD_COLOR
                )
                return frame

        return None

    def stream_thread(self):
        """Thread para leer el stream continuamente"""
        while self.running:
            response = self.conectar_stream()

            if response is None:
                print("⚠️  Reintentando en 3 segundos...")
                time.sleep(3)
                continue

            try:
                for _ in range(1000):  # Leer hasta 1000 frames antes de reconectar
                    if not self.running:
                        break

                    frame = self.leer_frame_mjpeg(response)
                    if frame is not None:
                        self.frame = frame
                    else:
                        break
            except Exception as e:
                print(f"⚠️  Error en stream: {e}")
                time.sleep(1)

    def guardar_frame(self):
        """Guarda el frame actual"""
        if self.frame is None:
            print("⚠️  No hay frame para guardar")
            return

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = os.path.join(self.save_dir, f"frame_{timestamp}.jpg")

        cv2.imwrite(filename, self.frame)
        self.frame_count += 1

        print(f"✅ Frame guardado: {filename} (Total: {self.frame_count})")

    def run(self):
        """Loop principal con UI"""
        print("\n" + "=" * 70)
        print("📸 CAPTURADOR DE FRAMES - ESP32")
        print("=" * 70)
        print(f"\n🌐 IP del ESP32: {self.esp_ip}")
        print(f"📁 Guardando en: {self.save_dir}/")
        print("\n📝 CONTROLES:")
        print("   • Presiona ESPACIO para guardar el frame actual")
        print("   • Presiona 'q' o ESC para salir")
        print("-" * 70 + "\n")

        # Iniciar thread de stream
        self.running = True
        thread = Thread(target=self.stream_thread, daemon=True)
        thread.start()

        # Esperar a que llegue el primer frame
        print("⏳ Esperando primer frame...")
        while self.frame is None and self.running:
            time.sleep(0.1)

        if self.frame is None:
            print("❌ No se pudo obtener frames del ESP32")
            self.running = False
            return

        print("✅ Stream activo\n")

        # Loop de UI
        window_name = f"ESP32 Stream - {self.esp_ip}"
        cv2.namedWindow(window_name)

        while self.running:
            if self.frame is not None:
                # Clonar frame para no modificar el original
                display_frame = self.frame.copy()

                # Agregar información en pantalla
                texto = (
                    f"Frames guardados: {self.frame_count} | ESPACIO=Guardar | Q=Salir"
                )
                cv2.putText(
                    display_frame,
                    texto,
                    (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 255, 0),
                    2,
                )

                # Mostrar
                cv2.imshow(window_name, display_frame)

            # Procesar teclas
            key = cv2.waitKey(1) & 0xFF

            if key == 27 or key == ord("q"):  # ESC o 'q'
                break
            elif key == 32:  # ESPACIO
                self.guardar_frame()

        # Limpiar
        self.running = False
        cv2.destroyAllWindows()

        print("\n" + "=" * 70)
        print(f"✅ Total de frames guardados: {self.frame_count}")
        print(f"📁 Ubicación: {os.path.abspath(self.save_dir)}/")
        print("=" * 70)


def main():
    parser = argparse.ArgumentParser(description="Capturador de frames desde ESP32-CAM")
    parser.add_argument("--ip", required=True, help="IP del ESP32 (ej: 192.168.1.100)")

    args = parser.parse_args()

    capture = FrameCapture(args.ip)
    capture.run()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrumpido por el usuario")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback

        traceback.print_exc()

#!/usr/bin/env python3
"""
Herramienta interactiva para ajustar rangos de color HSV
Útil para calibrar la detección de objetos de colores específicos.

Uso:
    python3 color_picker.py [--source RUTA]

Opciones:
    --source: Puede ser:
              - 0, 1, 2... (número de webcam)
              - URL del stream (ej: http://192.168.4.1/stream_auto)
              - Ruta a una imagen (ej: test.jpg)
"""

import cv2
import numpy as np
import argparse
import sys


class ColorPicker:
    def __init__(self, source=0):
        self.source = source
        self.window_name = "Color Picker - Ajuste de Rangos HSV"

        # Valores iniciales (verde)
        self.h_min = 35
        self.h_max = 85
        self.s_min = 100
        self.s_max = 255
        self.v_min = 100
        self.v_max = 255

        # Presets de colores
        self.presets = {
            "verde": (35, 85, 100, 255, 100, 255),
            "amarillo": (20, 35, 100, 255, 100, 255),
            "azul": (100, 130, 100, 255, 100, 255),
            "rojo": (0, 10, 100, 255, 100, 255),
            "rojo2": (170, 180, 100, 255, 100, 255),  # Rojo wrap-around
        }

    def nothing(self, x):
        """Callback vacío para los trackbars"""
        pass

    def crear_interfaz(self):
        """Crea la ventana y los trackbars"""
        cv2.namedWindow(self.window_name)

        # Crear trackbars para HSV
        cv2.createTrackbar("H min", self.window_name, self.h_min, 179, self.nothing)
        cv2.createTrackbar("H max", self.window_name, self.h_max, 179, self.nothing)
        cv2.createTrackbar("S min", self.window_name, self.s_min, 255, self.nothing)
        cv2.createTrackbar("S max", self.window_name, self.s_max, 255, self.nothing)
        cv2.createTrackbar("V min", self.window_name, self.v_min, 255, self.nothing)
        cv2.createTrackbar("V max", self.window_name, self.v_max, 255, self.nothing)

        print("\n" + "=" * 70)
        print("🎨 COLOR PICKER - CALIBRACIÓN DE RANGOS HSV")
        print("=" * 70)
        print("\n📝 CONTROLES:")
        print("   • Ajusta los sliders para ver el resultado en tiempo real")
        print("   • Presiona 'q' o ESC para salir")
        print("   • Presiona 'g' para preset VERDE")
        print("   • Presiona 'y' para preset AMARILLO")
        print("   • Presiona 'b' para preset AZUL")
        print("   • Presiona 'r' para preset ROJO")
        print("   • Presiona 's' para guardar los valores actuales")
        print("-" * 70)

    def aplicar_preset(self, nombre):
        """Aplica un preset de color"""
        if nombre in self.presets:
            h_min, h_max, s_min, s_max, v_min, v_max = self.presets[nombre]
            cv2.setTrackbarPos("H min", self.window_name, h_min)
            cv2.setTrackbarPos("H max", self.window_name, h_max)
            cv2.setTrackbarPos("S min", self.window_name, s_min)
            cv2.setTrackbarPos("S max", self.window_name, s_max)
            cv2.setTrackbarPos("V min", self.window_name, v_min)
            cv2.setTrackbarPos("V max", self.window_name, v_max)
            print(f"✅ Preset '{nombre.upper()}' aplicado")

    def guardar_valores(self):
        """Guarda los valores actuales en un archivo"""
        with open("color_ranges.txt", "w") as f:
            f.write("RANGOS DE COLOR HSV\n")
            f.write("=" * 70 + "\n\n")
            f.write(f"#define OBSTACULO_H_MIN {self.h_min}\n")
            f.write(f"#define OBSTACULO_H_MAX {self.h_max}\n")
            f.write(f"#define OBSTACULO_S_MIN {self.s_min}\n")
            f.write(f"#define OBSTACULO_S_MAX {self.s_max}\n")
            f.write(f"#define OBSTACULO_V_MIN {self.v_min}\n")
            f.write(f"#define OBSTACULO_V_MAX {self.v_max}\n")

        print("\n" + "=" * 70)
        print("💾 Valores guardados en: color_ranges.txt")
        print("=" * 70)
        print("\n📋 COPIA ESTOS VALORES EN include/config.h:")
        print(f"#define OBSTACULO_H_MIN {self.h_min}")
        print(f"#define OBSTACULO_H_MAX {self.h_max}")
        print(f"#define OBSTACULO_S_MIN {self.s_min}")
        print(f"#define OBSTACULO_S_MAX {self.s_max}")
        print(f"#define OBSTACULO_V_MIN {self.v_min}")
        print(f"#define OBSTACULO_V_MAX {self.v_max}")
        print("=" * 70 + "\n")

    def run(self):
        """Ejecuta el loop principal"""
        self.crear_interfaz()

        # Determinar tipo de fuente
        if isinstance(self.source, str) and self.source.startswith("http"):
            print(f"📡 Conectando a stream: {self.source}")
        elif isinstance(self.source, str):
            print(f"📁 Cargando imagen: {self.source}")
        else:
            print(f"📹 Abriendo cámara #{self.source}")

        cap = cv2.VideoCapture(self.source)

        if not cap.isOpened():
            print(f"❌ Error: No se pudo abrir la fuente: {self.source}")
            return

        # Para imágenes estáticas
        es_imagen = isinstance(self.source, str) and not self.source.startswith("http")
        frame_estatico = None

        if es_imagen:
            ret, frame_estatico = cap.read()
            if not ret:
                print("❌ Error: No se pudo leer la imagen")
                return
            print("✅ Imagen cargada correctamente")

        print("✅ Fuente abierta. Procesando...")

        while True:
            if es_imagen:
                frame = frame_estatico.copy()
                ret = True
            else:
                ret, frame = cap.read()

            if not ret:
                print("⚠️  No se pudo leer frame. Intentando reconectar...")
                cap.release()
                cap = cv2.VideoCapture(self.source)
                continue

            # Obtener valores de trackbars
            self.h_min = cv2.getTrackbarPos("H min", self.window_name)
            self.h_max = cv2.getTrackbarPos("H max", self.window_name)
            self.s_min = cv2.getTrackbarPos("S min", self.window_name)
            self.s_max = cv2.getTrackbarPos("S max", self.window_name)
            self.v_min = cv2.getTrackbarPos("V min", self.window_name)
            self.v_max = cv2.getTrackbarPos("V max", self.window_name)

            # Convertir a HSV
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

            # Crear máscara
            lower = np.array([self.h_min, self.s_min, self.v_min])
            upper = np.array([self.h_max, self.s_max, self.v_max])
            mask = cv2.inRange(hsv, lower, upper)

            # Aplicar máscara
            result = cv2.bitwise_and(frame, frame, mask=mask)

            # Encontrar contornos para estadísticas
            contours, _ = cv2.findContours(
                mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
            )

            # Dibujar contornos en el frame original
            frame_with_contours = frame.copy()
            cv2.drawContours(frame_with_contours, contours, -1, (0, 255, 0), 2)

            # Mostrar información
            texto = f"Contornos: {len(contours)}"
            cv2.putText(
                frame_with_contours,
                texto,
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
            )

            if contours:
                areas = [cv2.contourArea(c) for c in contours]
                max_area = max(areas)
                texto2 = f"Area max: {int(max_area)} px"
                cv2.putText(
                    frame_with_contours,
                    texto2,
                    (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (0, 255, 0),
                    2,
                )

            # Mostrar ventanas
            cv2.imshow("1. Original", frame_with_contours)
            cv2.imshow("2. Mascara", mask)
            cv2.imshow("3. Resultado", result)

            # Procesar teclas
            key = cv2.waitKey(1) & 0xFF

            if key == 27 or key == ord("q"):  # ESC o 'q'
                break
            elif key == ord("g"):  # Verde
                self.aplicar_preset("verde")
            elif key == ord("y"):  # Amarillo
                self.aplicar_preset("amarillo")
            elif key == ord("b"):  # Azul
                self.aplicar_preset("azul")
            elif key == ord("r"):  # Rojo
                self.aplicar_preset("rojo")
            elif key == ord("s"):  # Guardar
                self.guardar_valores()

        cap.release()
        cv2.destroyAllWindows()

        # Mostrar valores finales
        print("\n" + "=" * 70)
        print("📊 VALORES FINALES:")
        print("=" * 70)
        print(f"H: [{self.h_min}, {self.h_max}]")
        print(f"S: [{self.s_min}, {self.s_max}]")
        print(f"V: [{self.v_min}, {self.v_max}]")
        print("-" * 70)
        print("\n¿Guardar estos valores? (s/n): ", end="")

        try:
            respuesta = input().strip().lower()
            if respuesta == "s":
                self.guardar_valores()
        except:
            pass


def main():
    parser = argparse.ArgumentParser(description="Calibrador de rangos de color HSV")
    parser.add_argument(
        "--source",
        default="0",
        help="Fuente de video: 0 (webcam), URL, o ruta de imagen",
    )

    args = parser.parse_args()

    # Convertir source a int si es un número
    try:
        source = int(args.source)
    except ValueError:
        source = args.source

    picker = ColorPicker(source)
    picker.run()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrumpido por el usuario")
        sys.exit(0)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback

        traceback.print_exc()
        sys.exit(1)

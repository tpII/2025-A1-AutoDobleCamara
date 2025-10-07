import threading
import tkinter as tk
import sys
import os

ROOT = os.path.dirname(os.path.abspath(__file__))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

from src import server, processing, gui


def main():
    # Cargar calibración
    try:
        processing.load_calibration()
        print("Calibración cargada con éxito.")
    except Exception as e:
        print("Advertencia: no se pudo cargar calibración:", e)

    # Iniciar servidor Flask en hilo separado
    flask_thread = threading.Thread(target=server.start_server, daemon=True)
    flask_thread.start()

    # Inicializasr GUI
    root = tk.Tk()
    app = gui.App(root)
    root.mainloop()


if __name__ == "__main__":
    main()

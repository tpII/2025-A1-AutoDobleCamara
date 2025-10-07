"""
Estado de la aplicacion (locks, frames y matrices de proyeccion)
"""

import threading
import numpy as np

# Frames
frame_cam1 = None
frame_cam2 = None
frame_lock = threading.Lock()

# Ultima distancia (cm)
last_distance = None

# Matrices de proyeccion
P1 = None
P2 = None

# Para un objeto amarillo
LOWER_COLOR = np.array([20, 100, 100])
UPPER_COLOR = np.array([30, 255, 255])

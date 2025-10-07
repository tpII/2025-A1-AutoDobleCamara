"""
Procesamiento principal, carga la calibracion, ubica el centroide, triangula y obtiene distancias.
"""

import cv2
import numpy as np
from . import state

CALIBRATION_FILE_NAME = "calibration_data.npz"


def load_calibration(filename=CALIBRATION_FILE_NAME):
    """Load stereo calibration and build projection matrices P1, P2.

    Expected arrays in the npz: mtx1, dist1, mtx2, dist2, R, T
    """
    try:
        with np.load(filename) as data:
            mtx1, dist1 = data["mtx1"], data["dist1"]
            mtx2, dist2 = data["mtx2"], data["dist2"]
            R, T = data["R"], data["T"]
    except Exception as e:
        raise RuntimeError(f"Error loading calibration file {filename}: {e}")

    P1 = mtx1 @ np.hstack((np.identity(3), np.zeros((3, 1))))
    P2 = mtx2 @ np.hstack((R, T.reshape(3, 1)))

    state.P1 = P1
    state.P2 = P2
    return P1, P2


def find_object_center(frame, lower_bound=None, upper_bound=None):
    """Return (x, y) centroid of largest colored object or None."""
    if lower_bound is None:
        lower_bound = state.LOWER_COLOR
    if upper_bound is None:
        upper_bound = state.UPPER_COLOR

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    mask = cv2.inRange(hsv, lower_bound, upper_bound)
    mask = cv2.erode(mask, None, iterations=2)
    mask = cv2.dilate(mask, None, iterations=2)

    contours, _ = cv2.findContours(
        mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )
    if len(contours) == 0:
        return None
    c = max(contours, key=cv2.contourArea)
    M = cv2.moments(c)
    if M["m00"] == 0:
        return None
    return (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))


def triangulate_and_distance(center1, center2):
    """Triangulate 3D point from two image centers and return distance (cm) from camera2."""
    P1 = state.P1
    P2 = state.P2
    if P1 is None or P2 is None:
        return None
    pts1 = np.array([center1], dtype=np.float32).reshape(1, 1, 2)
    pts2 = np.array([center2], dtype=np.float32).reshape(1, 1, 2)

    point4d = cv2.triangulatePoints(P1, P2, pts1.T, pts2.T)
    if point4d.shape[0] < 4 or point4d[3][0] == 0:
        return None
    point3d = (point4d[:3] / point4d[3]).flatten()

    # We don't have R and T here; approximate distance as norm in cam1 frame
    distance = np.linalg.norm(point3d)
    state.last_distance = distance
    return distance

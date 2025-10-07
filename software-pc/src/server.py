"""
Flask server con los endpoints para recibir imagenes JPEG.
"""

from flask import Flask, request, Response
import numpy as np
import cv2
from . import state

app = Flask(__name__)


@app.route("/video/cam1", methods=["POST"])
def video_cam1():
    try:
        data = request.get_data()
        np_arr = np.frombuffer(data, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        with state.frame_lock:
            state.frame_cam1 = img
        return Response(status=200)
    except Exception as e:
        print("Error in /video/cam1:", e)
        return Response(status=500)


@app.route("/video/cam2", methods=["POST"])
def video_cam2():
    try:
        data = request.get_data()
        np_arr = np.frombuffer(data, np.uint8)
        img = cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        with state.frame_lock:
            state.frame_cam2 = img
        return Response(status=200)
    except Exception as e:
        print("Error in /video/cam2:", e)
        return Response(status=500)


def start_server(host="0.0.0.0", port=5000):
    app.run(host=host, port=port, threaded=True)

"""
Helper de calibracion
"""

import threading
import time
import numpy as np
import cv2
from . import state


class Calibrator(threading.Thread):
    def __init__(
        self,
        chessboard_dims=(7, 7),
        square_size_cm=4.0,
        num_pairs=40,
        auto_capture=False,
    ):
        super().__init__(daemon=True)
        self.chessboard_dims = chessboard_dims
        self.square_size = square_size_cm
        self.num_pairs = num_pairs
        self.auto_capture = auto_capture
        self.running = False
        self.captured = 0
        self.objpoints = []
        self.imgpoints1 = []
        self.imgpoints2 = []

        # prepare object points
        objp = np.zeros((chessboard_dims[0] * chessboard_dims[1], 3), np.float32)
        objp[:, :2] = np.mgrid[
            0 : chessboard_dims[0], 0 : chessboard_dims[1]
        ].T.reshape(-1, 2)
        objp = objp * self.square_size
        self._objp = objp

        self.criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
        self.frame_shape = None
        self.status = ""

    def run(self):
        self.running = True
        self.status = "running"
        while self.running and self.captured < self.num_pairs:
            with state.frame_lock:
                f1 = None if state.frame_cam1 is None else state.frame_cam1.copy()
                f2 = None if state.frame_cam2 is None else state.frame_cam2.copy()

            if f1 is None or f2 is None:
                self.status = "waiting frames"
                time.sleep(0.1)
                continue

            if self.frame_shape is None:
                h, w = f1.shape[:2]
                self.frame_shape = (w, h)

            gray1 = cv2.cvtColor(f1, cv2.COLOR_BGR2GRAY)
            gray2 = cv2.cvtColor(f2, cv2.COLOR_BGR2GRAY)

            flags = (
                cv2.CALIB_CB_ADAPTIVE_THRESH
                | cv2.CALIB_CB_NORMALIZE_IMAGE
                | cv2.CALIB_CB_FAST_CHECK
            )
            ret1, corners1 = cv2.findChessboardCorners(
                gray1, self.chessboard_dims, flags
            )
            ret2, corners2 = cv2.findChessboardCorners(
                gray2, self.chessboard_dims, flags
            )

            # Actualizacion para GUI
            if ret1 and ret2:
                self.status = f"pair available ({self.captured}/{self.num_pairs})"
                # auto-capture
                if self.auto_capture:
                    ok, msg = self.capture_pair()
                    if ok:
                        self.status = (
                            f"captured auto ({self.captured}/{self.num_pairs})"
                        )
                        # cooldown para no capturar fotos repetidas
                        time.sleep(0.3)
            else:
                self.status = f"looking for board ({self.captured}/{self.num_pairs})"

            # pausa
            time.sleep(0.05)

        self.status = "idle"

    def capture_pair(self):
        """Force capture of current pair if chessboard detected in both frames."""
        with state.frame_lock:
            f1 = None if state.frame_cam1 is None else state.frame_cam1.copy()
            f2 = None if state.frame_cam2 is None else state.frame_cam2.copy()

        if f1 is None or f2 is None:
            return False, "frames missing"

        gray1 = cv2.cvtColor(f1, cv2.COLOR_BGR2GRAY)
        gray2 = cv2.cvtColor(f2, cv2.COLOR_BGR2GRAY)

        flags = (
            cv2.CALIB_CB_ADAPTIVE_THRESH
            | cv2.CALIB_CB_NORMALIZE_IMAGE
            | cv2.CALIB_CB_FAST_CHECK
        )
        ret1, corners1 = cv2.findChessboardCorners(gray1, self.chessboard_dims, flags)
        ret2, corners2 = cv2.findChessboardCorners(gray2, self.chessboard_dims, flags)

        if not (ret1 and ret2):
            return False, "board not found in both frames"

        corners1_subpix = cv2.cornerSubPix(
            gray1, corners1, (11, 11), (-1, -1), self.criteria
        )
        corners2_subpix = cv2.cornerSubPix(
            gray2, corners2, (11, 11), (-1, -1), self.criteria
        )

        self.objpoints.append(self._objp)
        self.imgpoints1.append(corners1_subpix)
        self.imgpoints2.append(corners2_subpix)
        self.captured += 1
        return True, f"captured {self.captured}/{self.num_pairs}"

    def finalize(self, out_file="calibration_data.npz"):
        """Run stereo calibration with captured pairs and save result to file."""
        if self.captured < 2:
            return False, "not enough pairs"

        ret1, mtx1, dist1, rvecs1, tvecs1 = cv2.calibrateCamera(
            self.objpoints, self.imgpoints1, self.frame_shape, None, None
        )
        ret2, mtx2, dist2, rvecs2, tvecs2 = cv2.calibrateCamera(
            self.objpoints, self.imgpoints2, self.frame_shape, None, None
        )

        stereo_flags = cv2.CALIB_FIX_INTRINSIC
        ret, mtx1, dist1, mtx2, dist2, R, T, E, F = cv2.stereoCalibrate(
            self.objpoints,
            self.imgpoints1,
            self.imgpoints2,
            mtx1,
            dist1,
            mtx2,
            dist2,
            self.frame_shape,
            criteria=self.criteria,
            flags=stereo_flags,
        )

        if not ret:
            return False, "stereoCalibrate failed"

        np.savez(
            out_file, mtx1=mtx1, dist1=dist1, mtx2=mtx2, dist2=dist2, R=R, T=T, E=E, F=F
        )
        return True, f"saved {out_file}"

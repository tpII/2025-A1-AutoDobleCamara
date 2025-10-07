"""Run the Flask server only (useful for testing without GUI).

Usage: python3 scripts/run_server.py
"""
import sys
import os

# Ensure project root is on sys.path so we can import the local `src` package
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if ROOT not in sys.path:
    sys.path.insert(0, ROOT)

from src import server, processing

if __name__ == '__main__':
    try:
        processing.load_calibration()
        print('Calibration loaded (if present).')
    except Exception as e:
        print('Warning: calibration not loaded:', e)
    server.start_server(host='0.0.0.0', port=5000)

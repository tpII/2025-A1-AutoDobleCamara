"""
CLiente de control del robot, manda comandos TCP
"""

import socket
from . import state

# robot address
ROBOT_IP = "192.168.4.2"
ROBOT_PORT = 8000


def send_command(command, ip=ROBOT_IP, port=ROBOT_PORT):
    """Send a single command over TCP to the robot.

    This is a best-effort function; network errors are logged but not raised.
    """
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(1.0)
            s.connect((ip, port))
            s.sendall(command.encode())
    except Exception as e:
        print(f"Error sending command {command}: {e}")


def safe_send(command):
    """Send command but block FORWARD if too close to object.

    Returns True if sent, False if blocked.
    """
    if (
        command == "FORWARD"
        and state.last_distance is not None
        and state.last_distance < 20.0
    ):
        return False
    send_command(command)
    return True

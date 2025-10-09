source venv/bin/activate

nmcli dev wifi hotspot ifname wlo1 ssid access-point password 12345678
sudo ip addr add 10.42.0.1 dev wlo1

python3 src/pc_control_gui.py --host 0.0.0.0 --port 12345
import socket
import time

# Update with the IP address of your ESP32
PEER_IP = "192.168.89.31"  # Replace with actual IP address
PEER_PORT = 10001

commands = ["GPIO4=0", "GPIO4=1"]
i = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
while 1:
    try:
        command = commands[i % 2]
        sock.sendto(command.encode(), (PEER_IP, PEER_PORT))
        print("Sent command:", command)
        i = i + 1
        time.sleep(1)
    except KeyboardInterrupt:
        break
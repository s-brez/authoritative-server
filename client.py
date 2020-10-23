import socket


UDP_IP = "192.168.1.103"
UDP_PORT = 55555

client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

client_socket.sendto("Hello".encode(), (UDP_IP, UDP_PORT))

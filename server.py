import socket


UDP_IP = "192.168.1.103"
UDP_PORT = 55555

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP, UDP_PORT))

while True:
    data, address = server_socket.recvfrom(1024)
    print("Message: ", data)

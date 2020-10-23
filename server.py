import socket
import sys

from player import Player
from world import World


UDP_IP = "192.168.1.103"
UDP_PORT = 55556
BUFFER_SIZE = 1024
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP, UDP_PORT))


def load_state():
    return [[Player('0'), Player('1')], World()]


players, world = load_state()
slots = {p.ID: None for p in players}

while True:

    """ First byte of client message dictates what message type it is
        Second byte is free for use as a boolean response to join requests
        Third and fourth bytes indicate player ID, 00 - 99. New user IP and
        port is allocated to a slot, with their player ID sent back in bytes 3
        and 4. Futher packets from that player must come from their initial IP
        and port, or they will be rejected."""

    data, address = server_socket.recvfrom(BUFFER_SIZE)

    # !st byte is '1', player input
    if(data[:1] == b'1'):
        print("Player input received")

    # 1st byte is '2', player chat message
    elif (data[:1] == b'2'):
        print("Chat message received")

    # 1st byte is '3', join request
    elif (data[:1] == b'3'):
        p_id = data[1:3].decode()

        # Player ID will be known to server if the player exists
        if p_id in slots:
            token = address[0] + ":" + str(address[1])

            # Successful join, accept
            if slots[p_id] is None:
                print("Player", p_id, "joined from", token)
                slots[p_id] = token
                server_socket.sendto(("3" + "1" + p_id).encode(), (address[0], address[1]))

            # Double connect, reject
            elif slots[p_id] is not None and slots[p_id] != token:
                print("Double connection attempt from", token)
                server_socket.sendto(("3" + "0" + p_id).encode(), (address[0], address[1]))

        # Player ID not known, reject
        else:
            print("Connection attempt by unknown player from", token)
            server_socket.sendto(("3" + "0" + p_id).encode(), (address[0], address[1]))

    else:
        print("Unknown message from",
              address[0] + ":" + str(address[1]) + ":", data)

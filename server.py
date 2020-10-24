import socket
import sys

from messaging import serialize_state, deserialize_client_input
from player import Player
from world import World


UDP_IP = "192.168.1.103"
UDP_PORT = 55556
BUFFER_SIZE = 1024
server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP, UDP_PORT))


def load_stored_game_state():  # TODO: don't hard-code this later
    p0 = Player('0')
    p1 = Player('1')

    world = World()

    return {p0.ID: p0, p1.ID: p1}, world


players, world = load_stored_game_state()
slots = {p.ID: None for p in players.values()}
clients = slots.keys()

while True:

    # First byte of client message dictates what message type it is
    data, address = server_socket.recvfrom(BUFFER_SIZE)

    # Player input
    if(data[:1] == b'1'):
        p_id = data[1:3].decode()
        if p_id in slots:
            player_input = deserialize_client_input(data)

            # Update player state
                        

    # Player chat message
    elif (data[:1] == b'2'):
        print("Chat message received")

    # Player join request. Second byte is used as a boolean response
    # to join requests. Third and fourth bytes indicate player ID, 00 - 99.
    # New user IP and port is allocated to a slot, with their player ID sent
    # back in bytes 3 and 4. Futher packets from that player must come from
    # their initial IP and port, or they will be rejected.
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
                world.add_player(players[p_id])
                print(world.players)

            # Double connect, reject
            elif slots[p_id] is not None and slots[p_id] != token:
                print("Double connection attempt from", token)
                server_socket.sendto(("3" + "0" + p_id).encode(), (address[0], address[1]))

        # Player ID not known, reject
        else:
            print("Connection attempt by unknown player from", token)
            server_socket.sendto(("3" + "0" + p_id).encode(), (address[0], address[1]))

    # Player exit message
    elif (data[:1] == b'4'):
        p_id = data[2:4].decode()
        print(p_id)
        if p_id in slots:
            token = address[0] + ":" + str(address[1])
            print("Exit message from", token)

            # Clear the players address from player slots
            pass

    # Error case, message doesnt match expected format
    else:
        print("Unknown message from",
              address[0] + ":" + str(address[1]) + ":", data)

    # Update game state for authenticated clients
    for p_id in clients:
        if slots[p_id] is not None:
            server_socket.sendto(('1' + p_id).encode(), (address[0], address[1]))

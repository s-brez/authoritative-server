from pynput import keyboard
import socket
import time

from player import Player


player_ID = '0'

FPS = 60

UDP_IP = "192.168.1.103"
UDP_PORT = 55556
BUFFER_SIZE = 1024
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

client_input = {
    'has_focus': True,
    'keys': {},
}


def on_press(key):
    if client_input['has_focus']:
        try:
            client_input['keys'][key.char] = 1
        except AttributeError:
            client_input['keys'][key] = 1


def on_release(key):
    if client_input['has_focus']:
        try:
            print(key.char)
            client_input['keys'][key.char] = 0
        except AttributeError:
            print(key)
            client_input['keys'][key] = 0


listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

player_state = {}
extra_state = {}
player = Player('0')  # assume we are player 00 for now

last_frame_finish_time = 0
joined, attempts = False, 0


while True:

    # Limit FPS
    # now = time.time()
    # sleep_time = 1./FPS - (now - last_frame_finish_time)
    # if sleep_time > 0:
    #     time.sleep(sleep_time)

    # Attempt to join server, if not joined
    if not joined:
        print("Attempting to join server")
        client_socket.sendto(("3" + "0" + player_ID).encode(), (UDP_IP, UDP_PORT))

    # First byte of server message dictates what message type it is
    data, address = client_socket.recvfrom(BUFFER_SIZE)

    # !st byte is '1', state update
    if(data[:1] == b'1' and joined):
        print("State update received")

    # 1st byte is '2', incoming chat message
    elif (data[:1] == b'2' and joined):
        print("Chat message received")

    # 1st byte is '3', join request response
    elif (data[:1] == b'3'):

        if (data[:2] == b'30'):
            print("Server denied you access")

        elif (data[:2] == b'31'):
            print("Server granted you access")
            joined = True

    else:
        print("Packet data error from",
              address[0] + ":" + str(address[1]) + ":", data)
        client_socket.sendto("Error reading your last packet".encode(), (UDP_IP, UDP_PORT))

    # update player and world state
    

    # send new input to server
    # if joined:
    # player_input = {
    #     'up': client_input[]
    #     'down':
    #     'left':
    #     'right':
    #     'jump':
    # }

    # render

    # last_frame_finish_time = time.time()

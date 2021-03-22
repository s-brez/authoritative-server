from pynput import keyboard
import socket
import time

from messaging import CLIENT_INPUT, serialize_client_input, deserialize_state
from player import Player
from world import World


FPS = 60

UDP_IP = "192.168.1.101"
UDP_PORT = 55556
BUFFER_SIZE = 1024
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
client_input = CLIENT_INPUT


def on_press(key):
    if client_input['has_focus']:
        try:
            client_input['keys'][str(key.vk)] = 1
        except AttributeError:
            client_input['keys'][str(key)] = 1


def on_release(key):
    if client_input['has_focus']:
        try:
            # print(key.vk)
            client_input['keys'][str(key.vk)] = 0
        except AttributeError:
            # print(key)
            client_input['keys'][str(key)] = 0


listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

player = Player('0')  # assume we are player 00 for now
world = World()

last_frame_finish_time = 0
running, joined, attempts = True, False, 0

while running:

    if client_input['keys']['Key.esc']:
        break

    # Limit FPS
    now = time.time()
    sleep_time = 1./FPS - (now - last_frame_finish_time)
    if sleep_time > 0:
        time.sleep(sleep_time)

    # Attempt to join server if we have not already joined
    if not joined:
        print("Attempting to join server")
        time.sleep(attempts * 2)
        client_socket.sendto(("3" + "0" + player.ID).encode(), (UDP_IP, UDP_PORT))
        attempts += 1

    # First byte of server message dictates what message type it is
    data, address = client_socket.recvfrom(BUFFER_SIZE)

    # State update
    if(data[:1] == b'1' and joined):
        new_state = deserialize_state(data)
        player.update(new_state)
        world.update(new_state)

    # Incoming chat message
    elif (data[:1] == b'2' and joined):
        print("Chat message received")

    # Join request response
    elif (data[:1] == b'3' and not joined):

        if (data[:2] == b'30'):
            print("Access denied")

        elif (data[:2] == b'31'):
            print("Authorized")
            joined = True

        else:
            print("Packet data error from",
                  address[0] + ":" + str(address[1]) + ":", data)
            client_socket.sendto("Error reading your last packet".encode(), (UDP_IP, UDP_PORT))

    # ?
    elif (data[:1] == b'4' and joined):
        pass

    # Error case, message doesn't match expected format
    else:
        print("Packet data error from",
              address[0] + ":" + str(address[1]) + ":", data)

    # Send new input to server
    if joined:
        player_input = {

            # movement
            'up': client_input['keys']['87'],
            'left': client_input['keys']['65'],
            'down': client_input['keys']['83'],
            'right': client_input['keys']['68'],
            'jump': client_input['keys']['Key.space'],

            # use/interact
            'use': client_input['keys']['69'],

            # skills
            's1': client_input['keys']['49'],
            's2': client_input['keys']['50'],
            's3': client_input['keys']['51'],
            's4': client_input['keys']['52'],
            's5': client_input['keys']['53'],
            's6': client_input['keys']['54'],
            's7': client_input['keys']['55'],
            's8': client_input['keys']['56'],
            's9': client_input['keys']['57'],
            's10': client_input['keys']['48'],
            's11': client_input['keys']['189'],
            's12': client_input['keys']['187'],

            # targeting
            'target': client_input['current_target'],
        }

        # Send input to server: first byte '1', next two bytes player ID,
        # next 4 bytes target ID or 4 1's. Lastly suffix with packed_input.
        packed_input = serialize_client_input(player_input)
        target_ID = player_input['target'] if player_input['target'] is not None else '1111'
        payload = "1" + player.ID + target_ID + str(packed_input)
        client_socket.sendto(payload.encode(), (UDP_IP, UDP_PORT))

        local_actions = {

            # modifiers
            'ctrl': client_input['keys']['Key.ctrl_l'] or client_input['keys']['Key.ctrl_r'],
            'shift': client_input['keys']['Key.shift'] or client_input['keys']['Key.shift_r'],
            'alt': client_input['keys']['Key.alt_l'] or client_input['keys']['Key.alt_gr'],

            # actions
            'target_nearest_enemy': client_input['keys']['Key.tab'],
            'target_nearest_ally': client_input['keys']['Key.f2'],
            'target_self': client_input['keys']['Key.f1'],
        }

        # render/update UI

    last_frame_finish_time = time.time()

# Exited, send an exit message to server
print("Exiting, sending exit message to server")
client_socket.sendto(("4" + player.ID).encode(), (UDP_IP, UDP_PORT))

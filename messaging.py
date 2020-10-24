CLIENT_INPUT = {
    'has_focus': True,
    'current_target': None,
    'keys': {

        # server-side actions

        # movement (arrow keys)
        # 'Key.up': 0,
        # 'Key.down': 0,
        # 'Key.left': 0,
        # 'Key.right': 0,
        # 'Key.space': 0,

        # movement (wasd)
        '87': 0,    # w
        '65': 0,    # a
        '83': 0,    # s
        '68': 0,    # d
        'Key.space': 0,

        # use/interact
        '69': 0,   # e

        # skills 1-12
        '49': 0,    # 1
        '50': 0,    # 2
        '51': 0,    # 3
        '52': 0,    # 4
        '53': 0,    # 5
        '54': 0,    # 6
        '55': 0,    # 7
        '56': 0,    # 8
        '57': 0,    # 9
        '48': 0,    # 0
        '189': 0,   # -
        '187': 0,   # =

        # client-only actions

        # misc
        'Key.esc': 0,

        # modifiers
        'Key.ctrl_l': 0,
        'Key.ctrl_r': 0,
        'Key.shift': 0,
        'Key.shift_r': 0,
        'Key.alt_l': 0,
        'Key.alt_gr': 0,

        # targeting
        'Key.tab': 0,
        'Key.f2': 0,
        'Key.f1': 0,

    },
}

INPUT_ITEMS = [
        'up',
        'left',
        'down',
        'right',
        'jump',
        'use',
        's1',
        's2',
        's3',
        's4',
        's5',
        's6',
        's7',
        's8',
        's9',
        's10',
        's11',
        's12']


def serialize_client_input(input):
    return (
        (1 << 0 if input['up'] else 0) +
        (1 << 1 if input['left'] else 0) +
        (1 << 2 if input['down'] else 0) +
        (1 << 3 if input['right'] else 0) +
        (1 << 4 if input['jump'] else 0) +
        (1 << 5 if input['use'] else 0) +
        (1 << 6 if input['s1'] else 0) +
        (1 << 7 if input['s2'] else 0) +
        (1 << 8 if input['s3'] else 0) +
        (1 << 9 if input['s4'] else 0) +
        (1 << 10 if input['s5'] else 0) +
        (1 << 11 if input['s6'] else 0) +
        (1 << 12 if input['s7'] else 0) +
        (1 << 13 if input['s8'] else 0) +
        (1 << 14 if input['s9'] else 0) +
        (1 << 15 if input['s10'] else 0) +
        (1 << 16 if input['s11'] else 0) +
        (1 << 17 if input['s12'] else 0))


def deserialize_client_input(input):
    target = input[3:7].decode()
    player_input = int(input[7::].decode())

    actions = {
        'up': player_input & 1,
        'left': player_input & (1 << 1),
        'down': player_input & (1 << 2),
        'right': player_input & (1 << 3),
        'jump': player_input & (1 << 4),
        'use': player_input & (1 << 5),
        's1': player_input & (1 << 6),
        's2': player_input & (1 << 7),
        's3': player_input & (1 << 8),
        's4': player_input & (1 << 9),
        's5': player_input & (1 << 10),
        's6': player_input & (1 << 11),
        's7': player_input & (1 << 12),
        's8': player_input & (1 << 13),
        's9': player_input & (1 << 14),
        's10': player_input & (1 << 15),
        's11': player_input & (1 << 16),
        's12': player_input & (1 << 17),
    }

    return actions, target


def serialize_state(state):
    pass


def deserialize_state(state):
    pass

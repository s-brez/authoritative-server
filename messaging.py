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

        # skills 1-12
        '49': 0,
        '50': 0,
        '51': 0,
        '52': 0,
        '53': 0,
        '54': 0,
        '55': 0,
        '56': 0,
        '57': 0,
        '48': 0,
        '189': 0,
        '187': 0,

        # client-only actions
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


def serialize_client_input(player_input):

    return (
        (1 << 0 if player_input['up'] else 0) +
        (1 << 1 if player_input['left'] else 0) +
        (1 << 2 if player_input['down'] else 0) +
        (1 << 3 if player_input['right'] else 0) +
        (1 << 4 if player_input['jump'] else 0) +
        (1 << 5 if player_input['s1'] else 0) +
        (1 << 6 if player_input['s2'] else 0) +
        (1 << 7 if player_input['s3'] else 0) +
        (1 << 8 if player_input['s4'] else 0) +
        (1 << 9 if player_input['s5'] else 0) +
        (1 << 10 if player_input['s6'] else 0) +
        (1 << 11 if player_input['s7'] else 0) +
        (1 << 12 if player_input['s8'] else 0) +
        (1 << 13 if player_input['s9'] else 0) +
        (1 << 14 if player_input['s10'] else 0) +
        (1 << 15 if player_input['s11'] else 0) +
        (1 << 16 if player_input['s12'] else 0))


def deserialize_client_input(player_input):
    pass    


def serialize_state(state):
    pass


def deserialize_state(state):
    pass

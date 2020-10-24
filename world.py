class World():

    def __init__(self):
        self.rooms = self.load_rooms()
        self.players = {}

    def update(self, new_state):
        pass

    def add_player(self, new_player):
        self.players[new_player.ID] = new_player

    def remove_player(self, p_ID):
        self.players.pop(p_ID, None)

    def load_rooms(self):
        rooms = {
            '000': {                            # 000 is the overworld
                'dimensions': (100, 100),       # room dimensions
                'doors': {                      # portals to other rooms
                    '001': {
                        'position': (50, 50),
                        'requires': None,        # item required to enter door
                        'unlocked': True        # traversable or not
                    }
                }
            },

            '001': {
                'dimensions': (20, 20),
                'doors': {
                    '000': {
                        'position': (0, 0),
                        'requires': None,
                        'unlocked': True
                    }
                }
            }
        }

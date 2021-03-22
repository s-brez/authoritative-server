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
        """ Loads all rooms data and maps for persistent rooms. """

        return {
            100: {
                'tag': "life_overworld",
                'persistent': True,
                'size': (1000, 1000, 100),
                'doors': {
                    1: {  # door to player home
                        'position': (50, 50, 0),
                        'requires': None,
                        'unlocked': True,
                        'destination': {
                            'id': 101,
                            'position': 'south'
                        }
                    }
                }
            },

            101: {
                'tag': "player_home",
                'persistent': True,
                'dimensions': (20, 20, 10),
                'doors': {
                    1: {  # door to overworld
                        'position': 'south',
                        'requires': None,
                        'unlocked': True,
                        'destination': {
                            'id': 100,
                            'position': (10, 10)
                        }
                    }
                }
            },

            102: {
                'tag': "death_overworld",
                'persistent': True,
                'size': (200, 5000, 100),
                'doors': {
                    1: {  # door to final zone
                        'position': 'north',
                        'requires': None,
                        'unlocked': True,
                        'destination': {
                            'id': 103,
                            'position': 'south'
                        }
                    },

                    2: {  # door to life
                        'position': 'south',
                        'requires': None,
                        'unlocked': True,
                        'destination': {
                            'id': 101,
                            'position': 'player_last_position'
                        }
                    }
                }
            },

            103: {
                'tag': "death_overworld_final_zone",
                'persistent': True,
                'size': (500, 500, 100),
                'doors': {
                    1: {  # door to main zone
                        'position': 'south',
                        'requires': None,
                        'unlocked': True,
                        'destination': {
                            'id': 103,
                            'position': (250, 0)
                        }
                    }
                }
            },
        }


    def load_map(self, room_id: int):
        """ Loads a persistent map from file """

        if self.rooms[room_id]['persistent']:
                            

        else:
            return None


    
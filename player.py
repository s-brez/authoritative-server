from resources import PLAYER_STATS, PLAYER_CRAFT_SKILLS


class Player():

    def __init__(self, ID: str):
        self.ID = ID if len(ID) == 2 else '0' + ID

        # x, y and z coords are relative to players current room
        self.x, self.y, self.x = 0, 0, 0
        self.previous_room = None
        self.current_room = None

        self.craft_skills = PLAYER_CRAFT_SKILLS
        self.stats = PLAYER_STATS

    def update(self, new_state):
        """ Update non-positional state. """

        pass

    def move(self, key):
        """ Update postional state. """

        if key == '1':
            self.x += 1 * self.move_speed
        elif key == '2':
            self.y -= 1 * self.move_speed
        elif key == '4':
            self.y += 1 * self.move_speed
        elif key == '8':
            self.x -= 1 * self.move_speed
        elif key == '16':
            self.jump()

    def jump(self):
        pass

    def goto_room(self, room_ID: str, xyz: tuple):
        self.previous_room = self.current_room
        self.current_room = room_ID
        self.x, self.y, self.z = xyz[0], xyz[1], xyz[2]

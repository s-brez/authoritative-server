class Player():

    def __init__(self, ID: str):
        self.ID = ID if len(ID) == 2 else '0' + ID

        self.x, self.y, self.x = 0, 0, 0
        self.current_room = None

    def update(self, state):
        pass

    def goto_room(new_room: str, xyz: tuple):
        self.previous_room = self.current_room
        self.current_room = new_room

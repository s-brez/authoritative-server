class Player():

    def __init__(self, ID: str):
        self.ID = ID if len(ID) == 2 else '0' + ID

    def update_player(self, state, input):
        pass

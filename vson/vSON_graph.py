
class Node:

    def __init__(self, ID=0):
        self.ID = ID
        self.i_links = []
        self.o_links = []

class Link:

    def  __init__(self):
        self.begin = Node()
        self.end = Node()
        self.cost = 0.0

class TopologyGraph:

    def __init__(self):
        self.nodes =
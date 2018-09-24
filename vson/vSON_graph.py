import logging
import json
import sys, os
sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *

class Node:

    def __init__(self, ID=0, battery=None, sign="", reg=False):
        logging.debug('creating new node {}'.format(ID))
        self.ID = ID
        self.registered = reg
        self.battery = battery
        self.sign = sign
        self.i_links = []
        self.o_links = []
        self.interfaces = []



    def tojson(self):
        r = {
            'ID': self.ID,
            'description': self.descr,
            'energy': "45%",
            'o_links': len(self.o_links)
        }

        return r


class Interface:
    def __init__(self, index=0x00, quality=0x00):
        self.index = index
        self.quality = quality

    def tojson(self):
        r = {
            'index': self.index,
            'quality': self.quality
        }

        return r


class Link:
    def __init__(self, topo, beginID, endID, link_q, intf_idx):
        self.begin = beginID
        self.end = endID
        self.lq = link_q
        self.interface = intf_idx
        self.cost = 0.0
        #TODO:  check ID in topology, add new node if end nod eis nort present (unregistered)


    def tojson(self):
        r = {
            'begin': self.begin,
            'end': self.end,
            'link_qiality': self.lq,
            'interface': self.end,
            'cost': self.cost
        }

        return r

    #TODO Method to compute cost


class TopologyGraph:
    def __init__(self):
        self.ID = 0xF1257
        self.nodes = []

    def tojson(self):
        j = {
            'ID': self.ID,
            'size': len(self.nodes),
            'nodes': []
        }

        for n in self.nodes:
            j['nodes'].append(n.tojson())

        return j


    def push_node(self, ID, descr, signature, reg):
        try:
            for n in self.nodes:
                if n.ID == ID:
                    return ERROR.NODE_ALREADY_EXISTENT

            newnode = Node(ID, descr, signature, reg)
            self.nodes.append(newnode)
            logging.info("Append node {}".format(newnode.tojson()))
            return newnode.tojson()
        except:
            logging.error("Error appending node")
            return ERROR.INTERNAL_ERROR

    def get_node(self, ID):
        logging.info("Retrieving node {} informations".format(ID))
        for node in self.nodes:
            if node.ID == ID:
                return node.tojson()
            return ERROR.NODE_NOT_FOUND

    def get_topo_all(self):
        logging.info("Retrieving topology {} informations")
        return self.tojson()

    def delete_node(self, ID):
        for node in self.nodes:
            if node.ID == ID:
                self.nodes.remove(node)
                return
        return ERROR.NODE_NOT_FOUND



### Global variabble for the topology graph
topo = TopologyGraph()


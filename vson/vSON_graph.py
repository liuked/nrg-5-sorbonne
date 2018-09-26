import logging
import json
import sys, os
sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *
import random


class Node:

    def __init__(self, _ID=0, _sign="", _description=None, _registered=False, _interfaces = {}, _o_links = {}, _battery=None):
        logging.debug('creating new node {}'.format(_ID))
        try:
            assert isinstance(_ID, int)
            self.ID = _ID
            self.reg = _registered
            self.sign = _sign
            self.desc = None
            self.batt = _battery
            self.N = len(_interfaces)
            self.intfs = {}
            if self.N:
                for i in _interfaces:
                    assert isinstance(i, Interface)
                    self.intfs[i.index]=i
            self.M = len(_o_links)
            self.o_links = []
            if self.M:
                for l in _o_links:
                    assert isinstance(l, Link)
                    self.o_links.append(l)
            self.i_links = []

        except AssertionError:
            raise NSOException(STATUS.INVALID_NODE_ID, "ID must be an integer, found {}".format(type(_ID)))

    def __repr__(self):
        return json.dumps(self.tojson())

    def setBattery(self, val):
        self.batt = val


    def addInterface(self, intf):
        assert isinstance(intf, Interface)

        if intf.index not in self.intfs:
            self.intfs[intf.index] = intf
            logging.debug("Added interface {} present in node {:X}".format(intf, self.ID))
        elif self.intfs[intf.index].quality != intf.quality:
            self.intfs[intf.index].quality = intf.quality
            logging.debug("Updating quality factor for interface {} in node {:X}".format(intf, self.ID))
        else:
            logging.warning("No changes in interface {} in node {:X}".format(intf, self.ID))
            return STATUS.INTERFACE_ALREADY_PRESENT


    def isRegistered(self):
        return self.reg


    def add_outlink(self, ol):
        assert isinstance(ol, Link)
        l = next((x for x in self.o_links if x.isSame(ol)), None)
        if l is None:
            self.o_links.append(ol)
            logging.debug("Added NEW link with node {:X} to node {:X} on interface {:X}".format(ol.end, ol.begin, ol.intf_idx))
        elif l != ol:   # if is same but different values -> UPDATE!
            l = ol
            logging.debug("Updating link {:X}->{:X} (intf {:X})".format(l.begin, l.end, l.intf_idx))
        else:
            logging.warning("No changes made on {:X}->{:X} (intf {:X})".format(l.begin, l.end, l.intf_idx))

    def add_inlink(self, il):
        assert isinstance(il, Link)
        l = next((x for x in self.i_links if x.isSame(il)), None)
        if l is None:
            self.i_links.append(il)
            logging.debug("Registered NEW entering link from node {:X} in node {:X} (interface {:X})".format(il.begin, il.end, il.intf_idx))
        elif l != il:   # if is same but different values -> UPDATE!
            l = il
            logging.debug("Updating incoming link {:X}<-{:X} (intf {:X})".format(l.end, l.begin, l.intf_idx))
        else:
            logging.warning("No changes made on incoming {:X}<-{:X} (intf {:X})".format(l.end, l.begin, l.intf_idx))



    def tojson(self, type='full'):

        if type == 'full':
            r = {
                'ID': hex(self.ID),
                'registered': bool(self.reg),
                'signature': self.sign,
                'description': self.desc,
                'battery': self.batt,
                'n_intfs': self.N,
                'intfs': [],
                'n_nbrs': self.M,
                'o_links': [],
                'i_links': []
            }

            for i in self.intfs:
                r['intfs'].append(self.intfs[i].tojson(type))

            for i in self.o_links:
                r['o_links'].append(i.tojson(type))

            for i in self.i_links:
                r['i_links'].append(i.tojson(type))


        if type == 'netgraph':

            r = {
                'id': hex(self.ID),
                'label': self.desc,
                'properties': {
                    'battery': self.batt,
                    'interfaces': self.N,
                    'neigbours': self.M,
                    'nbrs_lst': []
                }

            }

            for i in self.o_links:
                r['properties']['nbrs_lst'].append(hex(i.end))



        return r





class Interface:
    def __init__(self, index=0x00, quality=0x00):
        self.index = index
        self.quality = quality

    def tojson(self, type='full'):

        if type == 'full' or type == 'netgraph':
            r = {
                'index': hex(self.index),
                'quality': self.quality
            }

        return r

    def __repr__(self):
      return json.dumps(self.tojson())


class Link:
    def __init__(self, beginID, endID, link_q, intf_idx):
        self.begin = beginID
        self.end = endID
        self.lq = link_q
        self.intf_idx = intf_idx



    def __repr__(self):
      return json.dumps(self.tojson())


    def get_cost(self, function=random.random):
        #TODO Method to compute cost
        return function()

    def isSame(self, link):
        assert isinstance(link, Link)
        return (link.begin == self.begin and link.end==self.end and link.intf_idx==self.intf_idx)


    def tojson(self, type='full'):

        if type == 'full':
            r = {
                'begin': hex(self.begin),
                'end': hex(self.end),
                'link_quality': self.lq,
                'interface': hex(self.intf_idx),
                'cost': self.get_cost()
            }

        if type == 'netgraph':
            r = {
                'source': hex(self.begin),
                'target': hex(self.end),
                'cost': self.get_cost(),

                'properties': {

                    'link_quality': self.lq,
                    'interface': hex(self.intf_idx),
                }
            }

        return r

    def get_reversed(self):
        return Link(self.begin, self.end, self.lq, self.intf_idx)




class TopologyGraph:

    """
    Nodes are stored in a dictionary with ID as a key
    """

    def __init__(self):
        self.ID = 0xF1257
        self.nodes = {}



    def __repr__(self):
      return json.dumps(self.tojson())

    def tojson(self, type='full'):

        if type == 'full':
            j = {
                'ID': hex(self.ID),
                'size': len(self.nodes),
                'nodes': [],
                'links': []
            }

        if type == 'netgraph':
            j = {
                'type': "NetworkGraph",
                'protocol': "NSO",
                'version': "1",
                'nodes': [],
                'links': [],
                'topology_id': hex(self.ID)
            }

        for n in self.nodes:
            j['nodes'].append(self.nodes[n].tojson(type))
            for l in self.nodes[n].o_links:
                j['links'].append(l.tojson(type))

        return j


    def isNodeRegistered(self, ID):
        if ID in self.nodes:
            return self.nodes[ID].isRegistered()
        return STATUS.NODE_NOT_REGISTERED

    def put_node_info(self, ID, **kwargs):
        print ID, "; ID in topology:"
        for id in self.nodes:
            print id, ' '
        if ID in self.nodes:
            for key in kwargs:
                if key == 'battery':
                    self.nodes[ID].setBattery(kwargs['battery'])
                if key == 'intfs':
                    self.nodes[ID].N = len(kwargs['intfs'])
                    for i in kwargs['intfs']:
                        self.nodes[ID].addInterface(i)
                if key == 'nbrs':
                    self.nodes[ID].M = len(kwargs['nbrs'])
                    for n in kwargs['nbrs']:
                        assert isinstance(n, Link)
                        try:
                            self.__enforce_link(n)
                        except NSOException as exc:
                            logging.error(exc.msg)
                            raise

            self.__update_netgraph()
            return STATUS.SUCCESS, self.nodes[ID].tojson()

        logging.critical("Trying to update node not registered {:X}".format(ID))
        return STATUS.NODE_NOT_FOUND, None



    def __update_netgraph(self):
        with open(NETGRAPH_PATH, 'w') as outfile:
            json.dump(self.tojson('netgraph'), outfile)


    def __enforce_link(self, link):
        if (link.begin in self.nodes) and (link.end in self.nodes):
            self.nodes[link.begin].add_outlink(link)
            self.nodes[link.end].add_inlink(link)
        else:
            raise NSOException(STATUS.INVALID_LINK, "Begin or End node not registered!")

    def push_node(self, ID, sign, reg, msg):
        try:
            if ID in self.nodes:
                return STATUS.NODE_ALREADY_EXISTENT
            else:
                newnode = Node(ID, _sign=sign, _registered=reg, _description=msg)
                self.nodes[ID] = newnode
                logging.info("Append node {}".format(newnode.tojson()))
                self.__update_netgraph()
                return newnode.tojson()
        except:
            logging.error("Error appending node")
            raise



    def get_node(self, ID):
        logging.info("Retrieving node {} informations".format(ID))
        if ID in self.nodes:
            return self.nodes[ID].tojson()
        return STATUS.NODE_NOT_FOUND

    def get_topo_all(self):
        logging.info("Retrieving topology {} informations")
        return json.dumps(self.tojson())

    def delete_node(self, ID):
        for ID in self.nodes:
            del self.nodes[ID]
            self.__update_netgraph()
            return STATUS.SUCCESS
        return STATUS.NODE_NOT_FOUND



#TODO: Add dijksta computation

### Global variabble for the topology graph
topo = TopologyGraph()


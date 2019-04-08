import logging
import json
import sys, os
sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *
import random
import threading
import time
import struct
from collections import defaultdict
from heapq import *

from common.interop import Dptr

class Node:

    def __init__(self, _ID=0, _sign="", _description=None, _registered=False, _interfaces = {}, _o_links = {}, _battery=None, _bs=False, _dptr=None):
        logging.debug('creating new node {}'.format(_ID))
        assert isinstance(_bs, bool)
        try:
            self.ID = _ID
            self.reg = _registered
            self.sign = _sign
            self.desc = None
            self.batt = _battery
            self.N = len(_interfaces)
            self.intfs = {}
            self.bs = _bs
            self.alive = True   # used for ageing
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
            self.dptr = _dptr
            self.next_in_stop = []
            self.next_out_stop = []

        except AssertionError:
            raise NSOException(STATUS.INVALID_NODE_ID, "ID must be an integer, found {}".format(type(_ID)))

    def __repr__(self):
        return json.dumps(self.tojson())

    def setBattery(self, val):
        self.batt = val


    def remove_link_fromID(self, beginID):
        self.i_links = [x for x in self.i_links if x.begin != beginID] #keep only incoming links not from deleted node



    def remove_link_toID(self, endID):
        self.o_links = [x for x in self.o_links if x.end != endID] #keep only outgoing links not to deleted node


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

    # def add_next_in_stop(self, dic):    #add node id in next stop list for input link
    #     self.next_in_stop.append(dic)
    #
    # def remove_next_in_stop(self, dic): #remove node id in next stop list
    #     self.next_in_stop.remove(dic)
    #
    # def add_next_out_stop(self, dic):    #add node id in next stop list for output link
    #     self.next_out_stop.append(dic)
    #
    # def remove_next_out_stop(self, dic): #remove node id in next stop list
    #     self.next_out_stop.remove(dic)


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
                'i_links': [],
                'next_in_stop': json.dumps(self.next_in_stop),
                'next_out_stop': json.dumps(self.next_out_stop),
                'base_station': bool(self.bs)
            }

            for i in self.intfs:
                r['intfs'].append(self.intfs[i].tojson(type))

            for i in self.o_links:
                r['o_links'].append(i.tojson(type))

            for i in self.i_links:
                r['i_links'].append(i.tojson(type))



        if type == 'netgraph':

            tmp_id = struct.pack("!Q", self.ID)
            tmp_id = struct.unpack("=Q", tmp_id)[0]

            r = {
                'id': hex(tmp_id),
                'label': self.desc,
                'properties': {
                    'battery': self.batt,
                    'interfaces': self.N,
                    'neigbours': self.M,
                    'nbrs_lst': [],
                    'next_in_stop':json.dumps(self.next_in_stop),
                    'next_out_stop': json.dumps(self.next_out_stop),
                    'base_station': bool(self.bs)

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
            bg_id = struct.pack("!Q", self.begin)
            bg_id = struct.unpack("=Q", bg_id)[0]
            end_id = struct.pack("!Q", self.end)
            end_id = struct.unpack("=Q", end_id)[0]
            r = {
                'source': hex(bg_id),
                'target': hex(end_id),
                'cost': self.get_cost(),

                'properties': {

                    'link_quality': self.lq,
                    'interface': hex(self.intf_idx)
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
        self.__changed = False
        self.metric = "shortest path"
        self.ntup = 1
        self.alpha = 5

        self.__njg = self.tojson('netgraph')
        self.__update_netgraph()

    def start_device_monitor(self):
        timerThread = threading.Thread(target=self.__device_monitor)
        timerThread.daemon = True
        timerThread.start()

    def start_dijkstra_monitor(self):
        timerThread = threading.Thread(target=self.__dijstra_monitor)
        timerThread.daemon = True
        timerThread.start()
    # Acces to graph operations

    def isNodeRegistered(self, ID):
        if ID in self.nodes:
            return self.nodes[ID].isRegistered()
        return STATUS.NODE_NOT_REGISTERED

    def put_node_info(self, ID, **kwargs):

        if ID in self.nodes:
            self.nodes[ID].alive = True
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

            self.__changed = True
            self.__update_netgraph()
            return STATUS.SUCCESS, self.nodes[ID].tojson()

        logging.critical("Trying to update node not registered {:X}".format(ID))
        return STATUS.NODE_NOT_FOUND, None

    def tojson(self, type='full'):
        if type == 'full':
            j = {
                'ID': hex(self.ID),
                'size': len(self.nodes),
                'nodes': [],
                'links': []
            }

        elif type == 'netgraph':
            j = {
                'type': "NetworkGraph",
                'protocol': "NSO",
                'version': "1",
                'metric': self.metric,
                'nodes': [],
                'links': [],
                'topology_id': hex(self.ID)
            }

        for n in self.nodes:
            j['nodes'].append(self.nodes[n].tojson(type))
            for l in self.nodes[n].o_links:
                j['links'].append(l.tojson(type))


        return j



    def get_node(self, ID):
        logging.info("Retrieving node {} informations".format(ID))
        if ID in self.nodes:
            return self.nodes[ID].tojson('netgraph')
        return STATUS.NODE_NOT_FOUND

    def get_topo_all(self):
        logging.info("Retrieving topology {} informations")
        if self.__changed:
            self.__changed = False
            self.__njg = self.tojson('netgraph')
        return self.__njg

    def push_node(self, ID, sign, reg, msg, bs):
        try:
            if ID in self.nodes:
                return STATUS.NODE_ALREADY_EXISTENT
            else:

                newnode = Node(ID, _sign=sign, _registered=reg, _description=msg, _bs=bs)
                self.nodes[ID] = newnode
                logging.info("Append node {}".format('BS' if bool(bs) else '', newnode.tojson()))
                self.__changed = True
                self.__update_netgraph()
                return newnode.tojson()
        except:
            logging.error("Error appending node")
            raise

    def delete_node(self, ID):

        if ID in self.nodes:
            logging.debug("Deleting node {:X}".format(ID))
            # delete all the outgoing links
            for l in self.nodes[ID].o_links:
                # delete incoming link in every neighbour
                self.nodes[l.end].remove_link_fromID(ID)

            # delete links in node connected by incoming links
            for l in self.nodes[ID].i_links:
                # delete outgoing link in every incoming neighbour
                self.nodes[l.begin].remove_link_toID(ID)

            deleted = json.dumps(self.nodes[ID].tojson())
            del self.nodes[ID]

            self.__changed = True
            self.__update_netgraph()
            return deleted, STATUS.SUCCESS

        return None, STATUS.NODE_NOT_FOUND

    # PRIVATE

    def __repr__(self):
        return json.dumps(self.tojson())

    def __enforce_link(self, link):
        if (link.begin in self.nodes) and (link.end in self.nodes):
            self.nodes[link.begin].add_outlink(link)
            self.nodes[link.end].add_inlink(link)
        else:
            raise NSOException(STATUS.INVALID_LINK, "Begin or End node not registered!")

    def __device_monitor(self):
        next_call = time.time()
        while True:

            logging.debug("checking nodes...")
            # setup next call
            next_call = next_call + (self.ntup * self.alpha)

            logging.debug("TopoMonitor: waiting for lock")
            lock.acquire()
            logging.debug("TopoMonitor: lock acquired")
            try:

                to_delete = []

                for n in self.nodes:
                    # if not self.nodes[n].bs:        #FIXME: so far we don't deregister the BS
                    if self.nodes[n].alive:
                        self.nodes[n].alive = False
                    else:
                        to_delete.append(self.nodes[n])

                for n in to_delete:
                    logging.warning("Node {:X} expired, deregistering....".format(n.ID))
                    # unregister from DPTR
                    d = n.dptr
                    if d:
                        response = d.deregister_node(n)
                        if response.status_code != 200:
                            raise NSOException(STATUS.DEREGISTRATION_ERROR,
                                               "failed to de-register node {:X} from dptr {}".format(n, d))

                    # TODO: deregister from vMME
                    # TODO:: remove routes

                    # delete node from topology
                    self.delete_node(n.ID)

            except Exception as x:
                logging.error(x)
                raise

            finally:
                lock.release()
                logging.debug('TopoMonitor: Released a lock')

            time.sleep(max(0, next_call - time.time()))

    def __update_netgraph(self):
        with open(NETGRAPH_PATH, 'w') as outfile:
            if self.__changed:
                self.__changed = False
                self.__njg = self.tojson('netgraph')
            json.dump(self.__njg, outfile)




#TODO: Add dijksta computation

    def __dijstra_monitor(self):
        next_call = time.time()
        while True:
            logging.debug("checking nodes...")
            # setup next call
            next_call = next_call + (self.ntup * self.alpha)
            logging.debug("DijMonitor: waiting for lock")
            lock.acquire()
            logging.debug("DijMonitor: lock acquired")

            try:
                bsid = []   #get bsid list
                clientid = [] #all clients' id
                bsid = [key for key in self.nodes if self.nodes[key].bs]
                clientid = [key for key in self.nodes if not self.nodes[key].bs]

                paths = []  #get all paths

                for base in bsid:
                    for client in clientid:
                        paths.append(self.dijkstra_out(base, client)[1])

                        self.update_out_quality(paths[-1])  #update quality every time we find a shortest path for every node in graph

                for path in paths:
                    for n in self.nodes:
                        for i in range(0, len(path)-1):
                            if path[i] == n :
                                dic = dict()
                                dic = {path[-1]: path[i+1]}
                                self.add_next_out_stop(n,dic)


                paths_in = []  #get all paths

                for base in bsid:
                    for client in clientid:
                        paths_in.append(self.dijkstra_in(base, client)[1])
                        self.update_in_quality(paths_in[-1])  #update quality every time we find a shortest path for every node in graph

                for path in paths_in:
                    for n in self.nodes:
                        for i in range(0, len(path)-1):
                            if path[i] == n :
                                dic = dict()
                                dic = {path[-1]: path[i+1]}
                                self.add_next_in_stop(n,dic)

            except Exception as x:
                logging.error(x)
                raise

            finally:
                lock.release()
                logging.debug('TopoMonitor: Released a lock')

            time.sleep(max(0, next_call - time.time()))



    def dijkstra_out(self, startid, endid):
        g = defaultdict(list)
        bsid = startid
        for key in self.nodes:
            for link in self.nodes[key].o_links:    #for out link
                g[link.begin].append((link.lq,link.end))

        logging.debug('g is : '.format(g))

        q, seen, dist = [(0,startid,())], set(), {startid: bsid}
        while q:
            (cost,n1,path) = heappop(q)
            if n1 in seen:
                continue
            seen.add(n1)
            path += (n1,)
            if n1 == endid:    #reach the end node
                return (cost, path)
            for c, n2 in g.get(n1):
                if n2 in seen:
                    continue
                if n2 not in dist or cost+c < dist[n2]:
                    dist[n2] = cost+c
                    heappush(q, (cost+c, n2, path))

        return float("inf")

    def dijkstra_in(self, startid, endid):
        g = defaultdict(list)
        bsid = startid
        for key in self.nodes:
            for link in self.nodes[key].i_links:    #for in link
                g[link.begin].append((link.lq,link.end))

        q, seen, dist = [(0,startid,())], set(), {startid: bsid}
        while q:
            (cost,n1,path) = heappop(q)
            if n1 in seen:
                continue
            seen.add(n1)
            path += (n1,)
            if n1 == endid:    #reach the end node
                return (cost, path)
            for c, n2 in g.get(n1):
                if n2 in seen:
                    continue
                if n2 not in dist or cost+c < dist[n2]:
                    dist[n2] = cost+c
                    heappush(q, (cost+c, n2, path))

        return float("inf")

    def update_out_quality(self, path):
        if self.__changed:
            p = list(path)
            for key in self.nodes:
                for i in range(0, len(self.nodes[key].o_links)):
                    if self.nodes[key].o_links[i].begin == p[0] and self.nodes[key].o_links[i].end == p[1] and len(p) > 2:
                        self.nodes[key].o_links[i].up_lq()
                        p.pop(0)    # remove the first node


    def update_in_quality(self, path):
        if self.__changed:
            p = list(path)
            for key in self.nodes:
                for i in range(0, len(self.nodes[key].i_links)):
                    if self.nodes[key].i_links[i].begin == p[0] and self.nodes[key].i_links[i].end == p[1] and len(p) > 2:
                        self.nodes[key].i_links[i].up_lq()
                        p.pop(0)    # remove the first node



    def add_next_in_stop(self, ID,  dic):
        if self.__changed:
            self.nodes[ID].next_in_stop.append(dic)

    def add_next_out_stop(self, ID,  dic):
        if self.__changed:
            self.nodes[ID].next_out_stop.append(dic)


    def remove_next_in_stop(self, ID,  dic):
        if self.__changed:
            self.nodes[ID].next_in_stop.remove(dic)

    def remove_next_out_stop(self, ID,  dic):
        if self.__changed:
            self.nodes[ID].next_out_stop.remove(dic)
### Global variabble for the topology graph
topo = TopologyGraph()

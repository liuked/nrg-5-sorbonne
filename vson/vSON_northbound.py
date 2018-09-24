import socket
import threading
import datetime
import sys, os
import json, struct
import logging
import requests
from optparse import OptionParser

from flask import Flask
from flask_restful import Api, Resource, reqparse

sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *
from vSON_graph import *
from vSON_def import *



### Global variabble for the topology graph
topo = TopologyGraph()

### set log destination
logging.basicConfig(filename="vSON_listener.log", level=logging.DEBUG)



class vson(object):

    def __init__(self, host, port):
        ### open the request listener n specified port
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))

        logging.info("Listener is started on port " + str(port))


    def listen(self):
        self.sock.listen(5)
        while True:
            client, address = self.sock.accept()
            logging.info("Receive connection from " + str(address))
            # client.settimeout(60)
            threading.Thread(target=self.__listen_to_client, args=(client, address)).start()
            logging.debug("Opening a threaded socket for client: " + str(address))


    def __generate_device_reg_reply(self, dst, src, answer):
        # 8B src, 8B dst, 2B proto, 2B len, 1B MSG_TYPE, 1B ANSWER
        msg = struct.pack("!QQHHBB", src, dst, PROTO.SON, NSO_HDR_LEN + 2, MSGTYPE.DEV_REG_REPLY, answer)
        return msg

    def __generate_unsupported_msgtype_err(self, src, dst):
        # 8B src, 8B dst, 2B proto, 2B len, 1B MSG_TYPE, 1B ANSWER
        msg = struct.pack("!QQHHB", src, dst, PROTO.SON, NSO_HDR_LEN + 1, MSGTYPE.UNSUPPORTED_MSGTYPE_ERROR)
        return msg

    def __process_topo_repo(self, src, dst, msg):
        logging.info("Processing TOPOLOGY REPORT: {}".format(msg))
        return None

    def __receive_nso_hdr(self, sock):
        nso_hdr = sock.recv(NSO_HDR_LEN)
        if nso_hdr:
            logging.debug("Unpacking header {}".format(" ".join("{:02x}".format(ord(c)) for c in nso_hdr)))
            try:
                src, dst, proto, length = struct.unpack("!QQHH", nso_hdr)
            except struct.error:
                raise SouthboundException(ERROR.SOUTHBOUND_NOT_NSO, 'received a non NSO packet')

            return (src, dst, proto, length, nso_hdr)



    def __process_SON(self, src, dst, sock, pl_size):
        payload = sock.recv(pl_size)
        logging.info("Processing SON message: {}".format(" ".join("{:02x}".format(ord(c)) for c in payload)))

        msg_typ, msg = struct.unpack("!B{}s".format(pl_size-1), payload)
        logging.info("Received message type [{:02x}]".format(msg_typ))

        if msg_typ == TOPO_REPO:
            reply = self.__process_topo_repo(src, dst, msg)
        else:
            reply = self.__generate_unsupported_msgtype_err(src, dst)

        return reply

    def __listen_to_client(self, client, address):
        size = 1024
        while True:
            try:
                src, dst, proto, length, hdr_str = self.__receive_nso_hdr(client)
                if src and dst and length and hdr_str:
                    logging.info("Received packet protocol [{:02x}], payload size = {}".format(proto, length-NSO_HDR_LEN))
                    # FIXME: extend message discriminator
                    if proto == PROTO.SON:
                        response = self.__process_SON(src, dst, client, length-NSO_HDR_LEN)
                    else:
                        in_msg = hdr_str + client.recv(size)
                        response = self.__generate_unsupported_msgtype_err(src, dst)

                    client.send(response)
                    logging.debug("Replying to " + str(address) + " with " + "{}".format(" ".join("{:02x}".format(ord(c)) for c in response)))

            except SouthboundException as x:
                if x.code == ERROR.SOUTHBOUND_NOT_NSO:
                    pass

            # raise Exception, ('Client Disconnected')
            except Exception:
                print "vSON_listener: error ", sys.exc_info()
                client.close()
                raise

    def __device_is_authenticated(self, uuid, credentials):  # FIXME : connect to vAAA_simulation service
        response = False

        while True:
            response = raw_input(
                "Incoming registration request from {}, cred: {}. Do you want to accept it? (yes/no) ").format(uuid,
                                                                                                               credentials)
            if (response == "yes") or (response == "no"):
                break
        if response == "yes":
            return True
        else:
            return False


class RESTfulAPI:

    def __init__(self):
        self.app = Flask(__name__)
        self.api = Api(self.app)

        self.api.add_resource(RESTNode, "/topology/nodes/<string:ID>")
        self.api.add_resource(RESTTopo, "/topology")

        self.app.run()
        logging.debug("Opening a thread for rest API")


class RESTTopo(Resource):

    def get(self):
        logging.info("Received GET request for topology")
        res = topo.get_topo_all()

        logging.debug("Building response: {}".format(res))
        if res == ERROR.TOPO_EMPTY:
            return "Topology not found", 404
        return res, 200



class RESTNode(Resource):

    def get(self, ID):
        logging.info("Received GET request for node: {}".format(ID))
        res = topo.get_node(ID)
        logging.debug("Building response: {}".format(res))
        if res == ERROR.NODE_NOT_FOUND:
            return "Node not found", 404
        return res, 200

    def post(self, ID):

        prs = reqparse.RequestParser()
        prs.add_argument("descr")
        prs.add_argument("signature")
        prs.add_argument("registered")
        a = prs.parse_args()

        logging.info("Received POST request, {}".format(a))
        res = topo.push_node(ID, a["descr"], a["signature"], a["registered"])

        logging.debug("Building response: {}".format(res))

        if res == ERROR.NODE_ALREADY_EXISTENT:
            return "Node with ID: {} already exist".format(ID), 400

        if res == ERROR.INTERNAL_ERROR:
            return "Server Error", 500

        return res, 200



    def put(self, ID):
        return topo.push_node(ID)

    def delete(self, ID):
        topo.delete_node(ID)




if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-p", "--port", dest="port_num", help="select on wich port to open the listener, default = 2904",
                      metavar="<port>")
    (options, args) = parser.parse_args()

    port_num = options.port_num

    if not port_num:
        port_num = 2904

    while True:
        try:
            port_num = int(port_num)
            break
        except ValueError:
            print "vSON_listener:main: Invalid port number. Abort..."
            exit(1)


    logging.debug("Opening a thread for the listener...")
    t = threading.Thread(target=vson('', port_num).listen)
    t.setDaemon(True)
    t.start()

    RESTfulAPI()






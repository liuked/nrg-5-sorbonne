import socket
import threading
import datetime
import sys, os
import json, struct
import logging
import requests

sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *

from optparse import OptionParser


class vson(object):

    def __init__(self, host, port):
        ### open the request listener n specified port
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((self.host, self.port))
        ### set log destination
        logging.basicConfig(filename="vSON_listener.log", level=logging.DEBUG)
        logging.info("Listener is started on port " + str(port))


    def listen(self):
        self.sock.listen(5)
        while True:
            client, address = self.sock.accept()
            logging.info("Receive connection from " + str(address))
            client.settimeout(60)
            print threading.Thread(target=self.__listen_to_client, args=(client, address)).start()
            logging.debug("Opening a threaded socket for client: " + str(address))


    def __generate_device_reg_reply(self, dst, src, answer):
        # 8B src, 8B dst, 2B proto, 2B len, 1B MSG_TYPE, 1B ANSWER
        msg = struct.pack("!QQHHBB", src, dst, PROTO.TSD.value, NSO_HDR_LEN + 2, MSGTYPE.DEV_REG_REPLY.value, answer)
        return msg


    def __receive_nso_hdr(self, sock):
        nso_hdr = sock.recv(NSO_HDR_LEN)
        if nso_hdr:
            logging.debug("Unpacking header {}".format(" ".join("{:02x}".format(ord(c)) for c in nso_hdr)))
            src, dst, proto, length = struct.unpack("!QQHH", nso_hdr)
            return (src, dst, proto, length, nso_hdr)
        return (None, None, nso_hdr)


    def __process_SON(self, src, dst, sock, pl_size):
        payload = sock.recv(pl_size)
        logging.info("Processing SON message: {}".format(" ".join("{:02x}".format(ord(c)) for c in payload)))

        msg_typ, msg = struct.unpack("!B{}s".format(pl_size-1), payload)
        logging.info("Received message type [{:02x}]:{}".format(msg_typ, MSGTYPE(msg_typ)))

        if msg_typ == TOPO_REPO:
            reply = self.__process_topo_repo(src, dst, msg)
        else:
            reply = self.__generate_unsupported_msgtype_err(src, dst, msg)

        return reply

    def __listen_to_client(self, client, address):
        size = 1024
        while True:
            try:
                src, dst, proto, length, hdr_str = self.__receive_nso_hdr(client)
                logging.info("Received packet protocol [{:02x}]:{}, payload size = {}".format(proto, PROTO(proto), length-NSO_HDR_LEN))
                # FIXME: extend message discriminator
                if PROTO(proto) == PROTO.SON:
                    response = self.__process_SON(src, dst, client, length-NSO_HDR_LEN)
                else:
                    in_msg = hdr_str + client.recv(size)
                    # response = self.__generate_unsupported_msgtype_err(in_msg)

                client.send(response)
                logging.debug("Replying to " + str(address) + " with " + "{}".format(" ".join("{:02x}".format(ord(c)) for c in response)))

            # raise Exception, ('Client Disconnected')
            except:
                print "vSON_listener: error ", sys.exc_info()
                logging.error("vSON_listener: error " + str(sys.exc_info()))
                client.close()
                exit(1)

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


if __name__ == "__main__":

    parser = OptionParser()
    parser.add_option("-p", "--port", dest="port_num", help="select on wich port to open the listener, default = 2311",
                      metavar="<port>")
    (options, args) = parser.parse_args()

    port_num = options.port_num

    if not port_num:
        port_num = 2311

    while True:
        try:
            port_num = int(port_num)
            break
        except ValueError:
            print "vSON_listener:main: Invalid port number. Abort..."
            exit(1)

    Listener('', port_num).listen()

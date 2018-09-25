import socket
import sys, os
import json
import struct
import requests
import random

sys.path.append(os.path.abspath(os.path.join("..")))
from common.Def import *


TSD_IP = '127.0.0.1'
TSD_PORT = 2311
SON_IP = '127.0.0.1'
SON_PORT = 2904



class Sender(object):

    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            # print "Connecting to ", TSD_IP, ":", TSD_PORT
            # self.sock.connect((TSD_IP, TSD_PORT))

            print "Connecting to ", SON_IP, ":", SON_PORT
            self.sock.connect((SON_IP, SON_PORT))

            print "Succeed! You can start typing messages... \n\n"
        except socket.error, exc:
            print "socket.error: %s" % exc
            exit(1)






    def console(self):
        buffsize = 1024
        while True:
            message = str(raw_input("Message? "))
            if message == "":
                pass
            if message == "d":
                message = self.__gen_reg_msg()
            if message == "b":
                message = self.__gen_bs_reg_msg()
            if message == "r":
                message = self.__gen_random_msg()
            if message == "s":
                message = self.__gen_son_msg()
            if message == "t":
                message = self.__gen_topo_repo()
            if message == "post":
                message = self.__gen_httpreq("post")
            if message == "getn":
                message = self.__gen_httpreq("getn")
            if message == "gett":
                message = self.__gen_httpreq("gett")
            if message == "c":
                self.sock.close()
                exit(0)

            if message:
                print "Sending ({}) {}".format(message, " ".join("{:02x}".format(ord(c)) for c in message))
                try:
                    self.sock.send(message)
                    data = self.sock.recv(buffsize)
                    print "Received ({}) {}".format(data, " ".join("{:02x}".format(ord(c)) for c in data))
                except socket.error, exc:
                    print "socket.error: %s" % exc
                    self.sock.close()
                    exit(1)



    def __gen_topo_repo(self):
        src = 0x0001000100010000 + random.randint(0, 9)
        dst = 0xF001000100010001

        N =2
        M=3

        msg = struct.pack("!QQHHBBBBBBBHQBBQBBQBB",
                          src,                                      ##
                          dst,                                      ##
                          PROTO.SON,                                ##
                          NSO_HDR_LEN + 1 + 1 + 1 + 2*N + 2 + 10*M,      ##  hdr+msgtyp+batt+N+intfs+M+nbrs
                          SONMSG.TOPO_REPO,    #### messsage type
                          59,
                          N,
                          0x01,
                          random.randint(0, 255),
                          0x02,
                          random.randint(0, 255),
                          M,
                          0x0001000100010000 + random.randint(0, 2),
                          random.randint(0, 255),
                          0x01,
                          0x0001000100010000 + random.randint(3, 6),
                          random.randint(0, 255),
                          0x01,
                          0x0001000100010000 + random.randint(7, 9),
                          random.randint(0, 255),
                          0x02)

        print "{:X}\n{:X}\n{:X}\n{}\n{:X}\n{}\n{}\n{:X}\n{}\n{:X}\n{}\n{}\n{:X}\n{}\n{:X}\n{:X}\n{}\n{:X}\n{:X}\n{}\n{:X}\n".format(
            src,\
            dst,\
            PROTO.SON,\
            NSO_HDR_LEN + 2 + 2*2 +10*M, \
            SONMSG.TOPO_REPO,\
            59,\
            N,\
            0x01,\
            random.randint(0, 256),\
            0x02,\
            random.randint(0, 256),\
            M,\
            0x0001000100010000 + random.randint(0, 2),\
            random.randint(0, 256),\
            0x01,\
            0x0001000100010000 + random.randint(3, 6),\
            random.randint(0, 256),\
            0x01,\
            0x0001000100010000 + random.randint(7, 9),\
            random.randint(0, 256),\
            0x02)

        return msg


    def __gen_httpreq(self, method):
        url = 'http://localhost:5000/'

        newID = hex(0x0001000100010000+random.randint(0,9))

        payload = {
            'ID': newID,
            'description': "nodo di prova",
            'signature': 0xc22377ffcd7a890b9d99660e37373dd3ff,
            'registered': 1
        }

        if method == "post":
            response = requests.post(url + 'topology/nodes/{}'.format(newID), json=payload)
        if method == "getn":
            response = requests.get(url + 'topology/nodes/0x0001000100010002')
        if method == "gett":
            response = requests.get(url + 'topology')

        print 'Status code: {}'.format(response.status_code)
        print 'Data:'
        try:
            print json.loads(response.content)
        except:
            print 'non json reply: {}'.format(response.content)

        return None





    def __gen_bs_reg_msg(self):
        src = 0xF001000100010001
        dst = 0xF001000100010001
        payload = {
            'message': src,
            'hash': 0x4849030f989ada889393b38000b,
            'signature': 0xc22377ffcd7a890b9d99660e37373dd3ff
        }
        payload = json.dumps(payload)
        msg = struct.pack("!QQHHB{}s".format(len(payload)), src, dst, PROTO.TSD, NSO_HDR_LEN + 1 + len(payload),
                          TSDMSG.DEV_REG, payload)




    def __gen_reg_msg(self):

        src = 0x0001000100010000+random.randint(0,9)
        dst = 0xF001000100010001

        payload = {
            'message': src,
            'hash': 0x4849030f989ada889393b38000b,
            'signature': 0xc22377ffcd7a890b9d99660e37373dd3ff
        }
        payload = json.dumps(payload)
        msg = struct.pack("!QQHHB{}s".format(len(payload)), src, dst, PROTO.TSD, NSO_HDR_LEN + 1 + len(payload), TSDMSG.DEV_REG, payload)
        return msg





    def __gen_random_msg(self):
        src = 0x0001000100010001
        dst = 0xF001000100010001
        msg = struct.pack("!QQHHB", src, dst, PROTO.TSD, NSO_HDR_LEN + 1, 0x56)
        return msg





    def __gen_son_msg(self):
        src = 0x0001000100010001
        dst = 0xF001000100010001
        msg = struct.pack("!QQHHB", src, dst, PROTO.SON, NSO_HDR_LEN + 1, 0x56)
        return msg





if __name__ == "__main__":

    # while True:
    #     address = "127.0.0.1"
    #
    #     # address = str(raw_input("Address? "))
    #     try:
    #         if address.count('.') < 3:
    #             raise  socket.error
    #
    #         break
    #     except socket.error:
    #         print "Not a valid IP..."
    #         pass
    #
    # while True:
    #     port_num = 2904
    #     # port_num = raw_input("Port? ")
    #     try:
    #         port_num = int(port_num)
    #         if port_num > 65535:
    #             raise ValueError
    #         break
    #     except ValueError:
    #         print "Invalid port number (0 < port < 65535)"
    #         pass

    Sender().console()


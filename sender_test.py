import socket
import sys, os
import json
import struct

sys.path.append(os.path.abspath(os.path.join("..")))


class Sender(object):

    def __init__(self, dest, port):
        self.dest = dest
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


        print "Connecting to ", dest , ":", port
        try:
            self.sock.connect((dest, port))
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
            if message == "c":
                self.sock.close()
                exit(0)
            print "Sending {}".format(" ".join("{:02x}".format(ord(c)) for c in message))
            try:
                self.sock.send(message)
                data = self.sock.recv(buffsize)
                print "Received {}".format(" ".join("{:02x}".format(ord(c)) for c in data))
            except socket.error, exc:
                print "socket.error: %s" % exc
                self.sock.close()
                exit(1)

    def __gen_bs_reg_msg(self):
        src = 0xF001000100010001
        dst = 0xF001000100010001
        payload = {
            'message': src,
            'hash': 0x4849030f989ada889393b38000b,
            'signature': 0xc22377ffcd7a890b9d99660e37373dd3ff
        }
        payload = json.dumps(payload)
        msg = struct.pack("!QQHHB{}s".format(len(payload)), src, dst, PROTO.TSD.value, NSO_HDR_LEN + 1 + len(payload),
                          MSGTYPE.DEV_REG.value, payload)
        return msg

    def __gen_reg_msg(self):

        src = 0x0001000100010001
        dst = 0xF001000100010001

        payload = {
            'message': src,
            'hash': 0x4849030f989ada889393b38000b,
            'signature': 0xc22377ffcd7a890b9d99660e37373dd3ff
        }
        payload = json.dumps(payload)
        msg = struct.pack("!QQHHB{}s".format(len(payload)), src, dst, PROTO.TSD.value, NSO_HDR_LEN + 1 + len(payload), MSGTYPE.DEV_REG.value, payload)
        return msg

    def __gen_random_msg(self):
        src = 0x0001000100010001
        dst = 0xF001000100010001
        msg = struct.pack("!QQHHB", src, dst, PROTO.TSD.value, NSO_HDR_LEN + 1, 0x56)
        return msg

    def __gen_son_msg(self):
        src = 0x0001000100010001
        dst = 0xF001000100010001
        msg = struct.packs("!QQHHB", src, dst, PROTO.SON.value, NSO_HDR_LEN + 1, 0x56)
        return msg


if __name__ == "__main__":

    while True:
        address = "127.0.0.1"

        # address = str(raw_input("Address? "))
        try:
            if address.count('.') < 3:
                raise  socket.error
            #socket.inet_aton(address)
            break
        except socket.error:
            print "Not a valid IP..."
            pass

    while True:
        port_num = 2311
        # port_num = raw_input("Port? ")
        try:
            port_num = int(port_num)
            if port_num > 65535:
                raise ValueError
            break
        except ValueError:
            print "Invalid port number (0 < port < 65535)"
            pass

    Sender(address, port_num).console()


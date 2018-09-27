#!/usr/bin/env python3

import socketserver, sys, os
import struct
import logging
import threading
import argparse
import socket
from threading import Thread

sys.path.append(os.path.abspath(os.path.join("..", "..")))
from common.Def import *

logging.basicConfig(level=logging.DEBUG)
parser = argparse.ArgumentParser()
parser.add_argument("-p", "--port", type=int, required=True, help="translator will be running on which port")
args = parser.parse_args()

HOST_ADDR = ""
HOST_PORT = args.port
#TODO: add cmd arguments
WEB_ADDR = ""
WEB_PORT = 80

class service(socketserver.BaseRequestHandler):

    def handle(self):

        def __prepare_report(data):
            #8B devid 8B usage#
            dev_id, usage = struct.unpack("=Qd", data)
            #client address
            #pack data (devid, client_ipv4, client_port, usage)
            #TODO: write the code
            return None

        def __tx_data_to_webserver(websock, appsock):
            data = appsock.recv(16) #8B devid and 8B usage
            report = __prepare_report(data)
            websock.sendall(report)

        def __rx_data_from_webserver(websock, appsock):
            cmd = websock.recv(2) #2B cmd(type, value)
            appsock.sendall(cmd)

        def __rx_thr_main(websock, appsock):
            while True:
                try:
                    __rx_data_from_webserver(websock, appsock)
                except Exception as e:
                    print(e)
                    break

        #create a connection to webserver
        websock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        websock.connect((WEB_ADDR, WEB_PORT))

        # start a rx thread for webserver
        threading.Thread(target=__rx_thr_main, args=(websock, self.request)).start()

        logging.info("accept a connection from a device")

        while True:
            try:
                #data = self.request.recv(1024)
                #logging.debug(data)
                #self.request.sendall(data)
                __tx_data_to_webserver(websock, self.request)
            except Exception as e:
                print(e)
                break


class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    pass

if __name__ == "__main__":
    ThreadedTCPServer.allow_reuse_addr = True
    server = ThreadedTCPServer((HOST_ADDR, HOST_PORT), service)
    server.serve_forever()

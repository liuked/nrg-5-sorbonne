import sys, os
sys.path.append(os.path.abspath(os.path.join("..")))
from vSON_southbound import vson
from vSON_northbound import RESTfulAPI
import logging
from optparse import OptionParser
import threading


### set log destination
logging.basicConfig(filename="vSON.log", level=logging.DEBUG)



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
            print "vSON_listener:main: Invalid port number..."
            pass


    logging.debug("Starting Southbound listener deamon...")
    t = threading.Thread(target=vson('', port_num).listen)
    t.setDaemon(True)
    t.start()

    logging.debug("Starting Nortbound Rest Interface...")
    RESTfulAPI()

import sys, os
import logging
from flask import Flask
from flask_restful import Api, Resource, reqparse

sys.path.append(os.path.abspath(os.path.join("..")))
from vSON_graph import TopologyGraph, topo
from common.Def import *


class RESTfulAPI:

    def __init__(self):
        self.app = Flask(__name__)
        self.api = Api(self.app)

        self.api.add_resource(RESTNode, "/topology/nodes/<string:str_ID>")
        self.api.add_resource(RESTTopo, "/topology")

        self.app.run()
        logging.debug("Opening a thread for rest API")





class RESTTopo(Resource):

    def get(self):
        logging.info("Received GET request for topology")
        res = topo.get_topo_all()

        logging.debug("Building response: {}".format(res))
        if res == STATUS.TOPO_EMPTY:
            return "Topology not found", 404
        return res, 200





class RESTNode(Resource):


    def get(self, str_ID):
        ID = int(str_ID, 0)
        logging.info("Received GET request for node: {:16X}".format(ID))
        res = topo.get_node(ID)
        logging.debug("Building response: {}".format(res))
        if res == STATUS.NODE_NOT_FOUND:
            return "Node not found", 404
        return res, 200


    def post(self, str_ID):
        ID = int(str_ID, 0)
        prs = reqparse.RequestParser()
        prs.add_argument("descr")
        prs.add_argument("signature")
        prs.add_argument("registered")
        a = prs.parse_args()

        logging.info("Received POST request, {}".format(a))
        res = topo.push_node(ID, sign= a["signature"], reg=a["registered"], msg= a["descr"])

        logging.debug("Building response: {}".format(res))

        if res == STATUS.NODE_ALREADY_EXISTENT:
            return "Node with ID: {:16X} already exist".format(ID), 400

        if res == STATUS.INTERNAL_ERROR:
            return "Server Error", 500

        return res, 200

    def put(self, str_ID):

        ID = int(str_ID, 0)

        prs = reqparse.RequestParser()
        a = prs.parse_args()

        status, node = topo.put_node_info(ID, a)

        if status == STATUS.SUCCESS:
            return node, 200
        else:
            return 'Node not present, unauthorized to add one', 401



    def delete(self, str_ID):
        ID = int(str_ID, 0)
        logging.info("Received DELETE request for node: {}".format(ID))
        stat, data = topo.delete_node(ID)
        logging.debug("Building response: {}".format(data))
        if stat == STATUS.NODE_NOT_FOUND:
            return "Node not found", 404
        return data, 200









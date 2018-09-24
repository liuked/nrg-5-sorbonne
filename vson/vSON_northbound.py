import sys, os
import logging
from flask import Flask
from flask_restful import Api, Resource, reqparse

sys.path.append(os.path.abspath(os.path.join("..")))
from vSON_graph import TopologyGraph
from common.Def import *


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
        logging.info("Received DELETE request for node: {}".format(ID))
        res = topo.delete_node(ID)
        logging.debug("Building response: {}".format(res))
        if res == ERROR.NODE_NOT_FOUND:
            return "Node not found", 404
        return res, 200









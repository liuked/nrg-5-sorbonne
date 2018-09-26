#!/usr/bin/env python#-*- coding:utf-8 -*-from enum import Enum# 8 src + 8 dst + 2 proto + 2 lenNSO_HDR_LEN = 20vAAA_URL = 'http://localhost:8080'NETGRAPH_PATH = '/var/www/html/data/vSON_netgraph.json'class PROTO(Enum):    TSD = 0xF000    SON = 0xF001    SU2IP = 0x0001class SONMSG(Enum):    TOPO_REPO = 0x00    TOPO_REPO_REPLY = 0x01    UNSUPPORTED_MSGTYPE_ERROR = 0xA0class TSDMSG(Enum):    DEV_REG = 0x00    DEV_REG_REPLY = 0x01    BS_REG = 0x02    BS_REG_REPLY = 0x03    UNSUPPORTED_MSGTYPE_ERROR = 0xA0class ANSWER(Enum):    SUCCESS = 0x00    FAIL = 0x01class STATUS(Enum):    SUCCESS = 0x000    NODE_NOT_FOUND = 0x404    NODE_ALREADY_EXISTENT = 0x400    INTERNAL_ERROR = 0x500    TOPO_EMPTY = 0x404    NODE_NOT_REGISTERED = 0x401    SOUTHBOUND_NOT_NSO = 0xF01    UNRECOGNIZED_BS = 0xFF1    INTERFACE_ALREADY_PRESENT = 0x004    INVALID_LINK = 0x100    INVALID_NODE_ID = 0xF08class frm(Enum):    HEADER = '\033[95m'    OKBLUE = '\033[94m'    OKGREEN = '\033[92m'    WARNING = '\033[93m'    FAIL = '\033[91m'    ENDC = '\033[0m'    BOLD = '\033[1m'    UNDERLINE = '\033[4m'class NSOException(Exception):    def __init__(self, code, msg):        self.msg = msg        self.code = code    def __str__(self):        return repr(self.code) + repr(self.msg)
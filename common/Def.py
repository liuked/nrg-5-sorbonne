#!/usr/bin/env python#-*- coding:utf-8 -*-from enum import Enumclass INTFTYPE(Enum):    WIFI = 1# 8 src + 8 dst + 2 proto + 2 lenNSO_HDR_LEN = 20vAAA_URL = 'http://localhost:8080'class PROTO(Enum):    TSD = 0xF000    SON = 0xF001    SU2IP = 0x0001TOPO_REPO = 0x00class MSGTYPE(Enum):    DEV_REG = 0x00    DEV_REG_REPLY = 0x01    BS_REG = 0x02    BS_REG_REPLY = 0x03    UNSUPPORTED_MSGTYPE_ERROR = 0xA0class ANSWER(Enum):    SUCCESS = 0x00    FAIL = 0x01class ERRTYPE(Enum):    AUTH_FAILED = 0x11class frm(Enum):    HEADER = '\033[95m'    OKBLUE = '\033[94m'    OKGREEN = '\033[92m'    WARNING = '\033[93m'    FAIL = '\033[91m'    ENDC = '\033[0m'    BOLD = '\033[1m'    UNDERLINE = '\033[4m'
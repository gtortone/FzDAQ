#!/usr/bin/python -B

import zmq
from ZSI.ServiceContainer import AsServer
from eccservice import EccService

if __name__ == '__main__':

    context = zmq.Context()
    AsServer(port=8080,  services=[EccService(context)])

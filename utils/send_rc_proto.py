#!/usr/bin/python2

import zmq
import sys
import FzRCS_pb2 as RCS
import FzNodeReport_pb2 as NodeReport

from time import sleep

if len(sys.argv) == 3:
    host = sys.argv[1]
    port = sys.argv[2]
else:
    print("usage: send_rc <host> <port>")
    sys.exit()

requests = []

'''
req = {'channel': RCS.Request.RC, 'operation': RCS.Request.READ, 'module': RCS.Request.CNT, 'variable': "report"}
requests.append(req)

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.READ, 'module': RCS.Request.CNT, 'variable': "nodestatus"}
requests.append(req)

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.WRITE, 'module': RCS.Request.CNT, 'variable': "event", 'value': "reset"}
requests.append(req)

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.WRITE, 'module': RCS.Request.CNT, 'variable': "event", 'value': "configure"}
requests.append(req)

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.WRITE, 'module': RCS.Request.CNT, 'variable': "event", 'value': "start"}
requests.append(req)
'''

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.READ, 'module': RCS.Request.NM, 'variable': "mode"}
requests.append(req)

req = {'channel': RCS.Request.RC, 'operation': RCS.Request.READ, 'module': RCS.Request.NM, 'variable': "status"}
requests.append(req)

context = zmq.Context()
print("Connecting to server...")
socket = context.socket(zmq.REQ)

url = "tcp://" + host + ":" + port
print("url = " + url)
socket.connect(url)
print("")

for req in requests:

    print("Sending command...")
    request = RCS.Request()
    request.channel = req["channel"];
    request.operation = req["operation"];
    request.variable = req["variable"]
    request.module = req["module"]
    if "hostname" in req.keys():
        request.hostname = req["hostname"]
    if "value" in req.keys():
        request.value = req["value"]

    socket.send_string(request.SerializeToString(), zmq.NOBLOCK)

    sleep(0.1)

    try:
        message = socket.recv(zmq.NOBLOCK)
    except zmq.Again:
         print("ZMQ recv exception")

    else:
        if(request.variable == "report"):

            response = NodeReport.NodeSet();
            response.ParseFromString(str(message))

            print("Received report...") 
            for n in response.node:
                print("\thostname: " + n.hostname);
                print("\tprofile: " + n.profile)
                print("\tstatus: " + n.status)

        else:

            response = RCS.Response()
            response.ParseFromString(str(message))

            print("Received reply...")
            print("Value: " + response.value)
       
            if(response.node):
               for n in response.node:
                  print("\thostname: " + n.hostname)
                  print("\tprofile: " + n.profile)
                  print("\tstatus: " + n.status)

            print("Error code: " + str(response.errorcode));
            print("Reason: " + response.reason);
            print("")

# set linger to 1 second
context.destroy(1)

print("Bye!")

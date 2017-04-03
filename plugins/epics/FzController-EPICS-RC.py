#!/usr/bin/python2 -B
'''
start with:
    EPICS_CAS_INTF_ADDR_LIST="`/bin/hostname`" ./FzEpics-RC.py
'''

import zmq

import sys
sys.path.append('../pyproto')

import FzRCS_pb2 as RCS
import FzNodeReport_pb2 as NodeReport
import FzUtils as util

from threading import Thread, Event, Lock
from pcaspy import Driver, SimpleServer, Alarm, Severity
from time import sleep

fzc_host = "nblupo"
#fzc_host = "nblupo-wls"
fzc_port = 5555

url = "tcp://" + fzc_host + ":" + str(fzc_port)
timeout = 0.1   # 100 milliseconds

prefix = 'DAQ:'
pvdb = {
        'RC:mode'           : { 'type' : 'enum', 'enums': ['unlock', 'lock'] },
        'RC:configure'      : { 'type' : 'int', 'value': 0 },
        'RC:start'     	    : { 'type' : 'int', 'value': 0 },
        'RC:stop'           : { 'type' : 'int', 'value': 0 },
        'RC:reset'          : { 'type' : 'int', 'value': 0 },
        'RC:state'          : { 'type' : 'enum', 'enums': ['IDLE', 'READY', 'RUNNING', 'PAUSED'], 'value' : 0  },
        'RC:transition'     : { 'type' : 'enum', 'enums': ['OK', 'ERROR'], 'value' : 0  },
        'nodelist'          : { 'type' : 'char', 'count' : 255 },
        'iocstatus'         : { 'type' : 'char', 'count' : 255 },
        'reader:in:data'    : { 'type' : 'int', 'value': 0 },
        'reader:in:databw'  : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'reader:in:ev'      : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'reader:in:evbw'    : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'reader:out:data'   : { 'type' : 'int', 'value': 0 },
        'reader:out:databw' : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'reader:out:ev'     : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'reader:out:evbw'   : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'fsm:states:invalid': { 'type' : 'int', 'value': 0 },
        'fsm:events:empty'  : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'fsm:events:good'   : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'fsm:events:bad'    : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'writer:in:data'    : { 'type' : 'int', 'value': 0 },
        'writer:in:databw'  : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'writer:in:ev'      : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'writer:in:evbw'    : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'writer:out:data'   : { 'type' : 'int', 'value': 0 },
        'writer:out:databw' : { 'type' : 'int', 'value': 0 , 'unit': 'Mbyte/s' },
        'writer:out:ev'     : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'writer:out:evbw'   : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
}

# a table to associate each PV value to a Run Control event
pvevents = {
        'RC:configure'      : { 'command': 'configure' },
        'RC:start'          : { 'command': 'start' },
        'RC:stop'           : { 'command': 'stop' },
        'RC:reset'          : { 'command': 'reset' },
}
 
mode_enum = ['unlock', 'lock']
state_enum = ['IDLE', 'READY', 'RUNNING', 'PAUSED']
transition_enum = ['OK', 'ERROR']

class myDriver(Driver):

    def __init__(self, ctx):

        Driver.__init__(self)
        self.context = ctx
        self.initSocket()
        self.iocstatus = ""
        self.report = NodeReport.NodeSet()

        self.sockmutex = Lock()
        self.repmutex = Lock()
        self.eid = Event()
        self.tid = Thread(target = self.runIOC) 
        self.tid.setDaemon(True)
        self.tid.start()

    def initSocket(self):
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect(url)

    def runIOC(self):

        self.setParamStatus("RC:mode", Alarm.NO_ALARM, Severity.NO_ALARM)
        self.setParamStatus("RC:transition", Alarm.NO_ALARM, Severity.NO_ALARM)

        while True:

            try:
                status = self.get_status()
            except zmq.Again:
                self.iocstatus = "FAIL: FzController not reachable" 
            else:
                self.setParam("RC:state", state_enum.index(status))
                self.iocstatus = "OK: FzController working normally"
            
            self.setParam("iocstatus", self.iocstatus)

            try:
                self.get_report()
            except zmq.Again:
                pass
            else:
                self.update_ioc()

            self.updatePVs()

            self.eid.wait(1)

    def write(self, reason, value):
        
        if(reason == "RC:mode"):
            self.setParam(reason, value)

        elif(self.getParam("RC:mode") == mode_enum.index('unlock')):

            for pv,info in pvevents.items():
                if(pv == reason):
                    err = None
                    try:
                        err = self.set_status(info['command'])
                    except zmq.Again:
                        pass
                    finally:
                        #print("DEBUG: err = " + str(err))
                        if(err == RCS.Response.OK):
                            self.setParam("RC:transition", 0)
                        else:
                            self.setParam("RC:transition", 1)

                    self.setParam(pv, 0);
                    self.updatePVs()

    def read(self, reason):
        if(reason == "nodelist"):
            try:
                value = self.get_nodelist()
            except zmq.Again:
                pass
            else:
                self.setParam("nodelist", value)
        return self.getParam(reason)

    def set_status(self, status):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.WRITE
        req.module = RCS.Request.CNT
        req.variable = "event"
        req.value = status

        self.sockmutex.acquire()
        self.socket.send_string(req.SerializeToString(), zmq.NOBLOCK)
        sleep(timeout)

        try:
            message = self.socket.recv(zmq.NOBLOCK)
        except zmq.Again:
            self.socket.close()
            self.initSocket()
            self.sockmutex.release()
            raise
        
        self.sockmutex.release()

        res = RCS.Response();
        res.ParseFromString(str(message))

        return res.errorcode

    def get_status(self):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.READ
        req.module = RCS.Request.CNT
        req.variable = "status"

        self.sockmutex.acquire()
        self.socket.send_string(req.SerializeToString(), zmq.NOBLOCK)
        sleep(timeout)

        try:
            message = self.socket.recv(zmq.NOBLOCK)
        except zmq.Again:
            self.socket.close()
            self.initSocket()
            self.sockmutex.release()
            raise

        self.sockmutex.release()

        res = RCS.Response()
        res.ParseFromString(str(message))

        return res.value

    def get_report(self):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.READ
        req.module = RCS.Request.CNT
        req.variable = "perfdata"

        self.sockmutex.acquire()
        self.socket.send_string(req.SerializeToString(), zmq.NOBLOCK)
        sleep(timeout)

        try:
            message = self.socket.recv(zmq.NOBLOCK)
        except zmq.Again:
            self.socket.close()
            self.initSocket()
            self.sockmutex.release()
            raise

        self.sockmutex.release()

        self.repmutex.acquire()
        self.report.ParseFromString(str(message))
        self.repmutex.release()

    def get_nodelist(self):

        nodelist = ""
        self.repmutex.acquire()
        for n in self.report.node:
            nodelist = str(nodelist + n.hostname + " [" + n.profile + "] ")
        self.repmutex.release()

        return nodelist

    def update_ioc(self):

        self.repmutex.acquire()

        for n in self.report.node:

            if(n.reader):
                data = util.human_byte(n.reader.in_bytes)
                self.setParam("reader:in:data", data["value"])
                self.setParamInfo("reader:in:data", {'unit': data["unit"]}) 

                data = util.to_Mbit(n.reader.in_bytes_bw)
                self.setParam("reader:in:databw", data)

                self.setParam("reader:in:ev", n.reader.in_events)
                self.setParam("reader:in:evbw", n.reader.in_events_bw)

                data = util.human_byte(n.reader.out_bytes)
                self.setParam("reader:out:data", data["value"])
                self.setParamInfo("reader:out:data", {'unit': data["unit"]})

                data = util.to_Mbit(n.reader.out_bytes_bw)
                self.setParam("reader:out:databw", data)

                self.setParam("reader:out:ev", n.reader.out_events)
                self.setParam("reader:out:evbw", n.reader.out_events_bw)

            if(len(n.fsm) > 0):
                ev_good = ev_bad = ev_empty = st_invalid = 0
                for fsm in n.fsm:
                    ev_good += fsm.events_good
                    ev_bad += fsm.events_bad
                    ev_empty += fsm.events_empty
                    st_invalid += fsm.state_invalid

                self.setParam("fsm:events:good", ev_good)
                self.setParam("fsm:events:bad", ev_bad)
                self.setParam("fsm:events:empty", ev_empty)
                self.setParam("fsm:states:invalid", st_invalid)

            if(n.writer):
                data = util.human_byte(n.writer.in_bytes)
                self.setParam("writer:in:data", data["value"])
                self.setParamInfo("writer:in:data", {'unit': data["unit"]})

                data = util.to_Mbit(n.writer.in_bytes_bw)
                self.setParam("writer:in:databw", data)

                self.setParam("writer:in:ev", n.writer.in_events)
                self.setParam("writer:in:evbw", n.writer.in_events_bw)

                data = util.human_byte(n.writer.out_bytes)
                self.setParam("writer:out:data", data["value"])
                self.setParamInfo("writer:out:data", {'unit': data["unit"]})
                
                data = util.to_Mbyte(n.writer.out_bytes_bw)
                self.setParam("writer:out:databw", data)

                self.setParam("writer:out:ev", n.writer.out_events)
                self.setParam("writer:out:evbw", n.writer.out_events_bw)

        self.repmutex.release()

if __name__ == '__main__':
    server = SimpleServer()
    server.createPV(prefix, pvdb)

    context = zmq.Context()
    driver = myDriver(context)

    # process CA transactions
    while True:
        server.process(0.1)

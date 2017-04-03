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

fznm_host = "nblupo"
#fznm_host = "nblupo-wls"
fznm_port = 5550

url = "tcp://" + fznm_host + ":" + str(fznm_port)
timeout = 0.1   # 100 milliseconds

prefix = str(fznm_host + ':')
pvdb = {
        'RC:mode'           : { 'type' : 'enum', 'enums': ['unlock', 'lock'] },
        'RC:state'          : { 'type' : 'enum', 'enums': ['IDLE', 'READY', 'RUNNING', 'PAUSED'], 'value' : 0  },
        'RC:transition'     : { 'type' : 'enum', 'enums': ['OK', 'ERROR'], 'value' : 0  },
        'iocstatus'         : { 'type' : 'char', 'count' : 255 },
        'profile'           : { 'type' : 'char', 'count' : 255 },
        'reader:in:data'    : { 'type' : 'int', 'value': 0 },
        'reader:in:databw'  : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'reader:in:ev'      : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'reader:in:evbw'    : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'reader:out:data'   : { 'type' : 'int', 'value': 0 },
        'reader:out:databw' : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'reader:out:ev'     : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'reader:out:evbw'   : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'parser:in:data'    : { 'type' : 'int', 'value': 0 },
        'parser:in:databw'  : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'parser:in:ev'      : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'parser:in:evbw'    : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'parser:out:data'   : { 'type' : 'int', 'value': 0 },
        'parser:out:databw' : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'parser:out:ev'     : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'parser:out:evbw'   : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'fsm:in:data'    : { 'type' : 'int', 'value': 0 },
        'fsm:in:databw'  : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'fsm:in:ev'      : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'fsm:in:evbw'    : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
        'fsm:out:data'   : { 'type' : 'int', 'value': 0 },
        'fsm:out:databw' : { 'type' : 'int', 'value': 0, 'unit': 'Mbit/s' },
        'fsm:out:ev'     : { 'type' : 'int', 'value': 0, 'unit': 'ev' },
        'fsm:out:evbw'   : { 'type' : 'int', 'value': 0, 'unit': 'ev/s' },
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

state_enum = ['IDLE', 'READY', 'RUNNING', 'PAUSED']
transition_enum = ['OK', 'ERROR']

class myDriver(Driver):

    def __init__(self, ctx):

        Driver.__init__(self)
        self.context = ctx
        self.initSocket()
        self.iocstatus = ""
        self.report = NodeReport.Node()

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
        self.setParamStatus("profile", Alarm.NO_ALARM, Severity.NO_ALARM)
        while True:

            try:
                status = self.get_status()
            except zmq.Again:
                self.iocstatus = "FAIL: FzNodeManager not reachable" 
            else:
                self.setParam("RC:state", state_enum.index(status))
                self.iocstatus = "OK: FzNodeManager working normally"
            
            self.setParam("iocstatus", self.iocstatus)

            try:
                self.get_report()
            except zmq.Again:
                pass
            else:
                self.update_ioc()

            self.updatePVs()

            self.eid.wait(1)

    def read(self, reason):
        if(reason == "profile"):
            return self.get_profile()

        return self.getParam(reason)


    def get_status(self):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.READ
        req.module = RCS.Request.NM
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

    def get_profile(self):

        profile = ""
        self.repmutex.acquire()
        profile = str(self.report.profile)
        self.repmutex.release()

        return profile 

    def get_report(self):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.READ
        req.module = RCS.Request.NM
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

    def update_ioc(self):

        self.repmutex.acquire()

        if(self.report.reader):
            data = util.human_byte(self.report.reader.in_bytes)
            self.setParam("reader:in:data", data["value"])
            self.setParamInfo("reader:in:data", {'unit': data["unit"]}) 

            data = util.to_Mbit(self.report.reader.in_bytes_bw)
            self.setParam("reader:in:databw", data)

            self.setParam("reader:in:ev", self.report.reader.in_events)
            self.setParam("reader:in:evbw", self.report.reader.in_events_bw)

            data = util.human_byte(self.report.reader.out_bytes)
            self.setParam("reader:out:data", data["value"])
            self.setParamInfo("reader:out:data", {'unit': data["unit"]})

            data = util.to_Mbit(self.report.reader.out_bytes_bw)
            self.setParam("reader:out:databw", data)

            self.setParam("reader:out:ev", self.report.reader.out_events)
            self.setParam("reader:out:evbw", self.report.reader.out_events_bw)

        if(len(self.report.fsm) > 0):
            ev_good = ev_bad = ev_empty = st_invalid = 0
            for fsm in self.report.fsm:
                ev_good += fsm.events_good
                ev_bad += fsm.events_bad
                ev_empty += fsm.events_empty
                st_invalid += fsm.state_invalid

            self.setParam("fsm:events:good", ev_good)
            self.setParam("fsm:events:bad", ev_bad)
            self.setParam("fsm:events:empty", ev_empty)
            self.setParam("fsm:states:invalid", st_invalid)

        if(self.report.writer):
            data = util.human_byte(self.report.writer.in_bytes)
            self.setParam("writer:in:data", data["value"])
            self.setParamInfo("writer:in:data", {'unit': data["unit"]})

            data = util.to_Mbit(self.report.writer.in_bytes_bw)
            self.setParam("writer:in:databw", data)

            self.setParam("writer:in:ev", self.report.writer.in_events)
            self.setParam("writer:in:evbw", self.report.writer.in_events_bw)

            data = util.human_byte(self.report.writer.out_bytes)
            self.setParam("writer:out:data", data["value"])
            self.setParamInfo("writer:out:data", {'unit': data["unit"]})
            
            data = util.to_Mbyte(self.report.writer.out_bytes_bw)
            self.setParam("writer:out:databw", data)

            self.setParam("writer:out:ev", self.report.writer.out_events)
            self.setParam("writer:out:evbw", self.report.writer.out_events_bw)

        self.repmutex.release()

if __name__ == '__main__':
    server = SimpleServer()
    server.createPV(prefix, pvdb)

    context = zmq.Context()
    driver = myDriver(context)

    # process CA transactions
    while True:
        server.process(0.1)

#!/usr/bin/python2

import os
import zmq

import sys
sys.path.append('../pyproto')

import FzRCS_pb2 as RCS
import FzNodeReport_pb2 as NodeReport

from time import sleep
from ecc_server import *

fzc_host = "nblupo"
#fzc_host = "nblupo-wls"
fzc_port = 5555

class EccService(ecc):

    #ECC states definition
    ECC_S_OFF = 0
    ECC_S_IDLE = 1
    ECC_S_DESCRIBED = 2
    ECC_S_PREPARED = 3
    ECC_S_READY = 4
    ECC_S_RUNNING = 5
    ECC_S_PAUSED = 6

    eccState = ECC_S_IDLE;

    def __init__(self, ctx):
        ecc.__init__(self)
        self.url = "tcp://" + fzc_host + ":" + str(fzc_port)
        self.timeout = 0.1   # 100 milliseconds
        self.context = ctx
        self.initSocket()

    def initSocket(self):
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect(self.url)

    def soap_ServerExit(self, ps, **kw):
        request, response = ecc.soap_ServerExit(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        print("ECC service exit !")
        self.socket.close()
        os._exit(0)        

    def soap_GetConfigIDs(self, ps, **kw):
        request, response = ecc.soap_GetConfigIDs(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = "<ConfigIdSet>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_1\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_1\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_1\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_2\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_1\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_2\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_3\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_3\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_2\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_4\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_4\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_4\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_5\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_6\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_6\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_3\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_6\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_7\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_8\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_8\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_2\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_9\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_00\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_6\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_10\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_7\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_7\
                </SubConfigId>\
        </ConfigId>\
        <ConfigId>\
                <SubConfigId type=\"describe\">\
                        DESC_11\
                </SubConfigId>\
                <SubConfigId type=\"prepare\">\
                        PREP_11\
                </SubConfigId>\
                <SubConfigId type=\"configure\">\
                        CONF_10\
                </SubConfigId>\
        </ConfigId>\
</ConfigIdSet>"
        return request, response

    def soap_Describe(self, ps, **kw):
        request, response = ecc.soap_Describe(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_IDLE:
            self.eccState = self.ECC_S_DESCRIBED
            try:
                self.set_status("reset")
            except zmq.Again:
                pass
        return request, response

    def soap_Prepare(self, ps, **kw):
        request, response = ecc.soap_Prepare(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_DESCRIBED:
            self.eccState = self.ECC_S_PREPARED
            try:
                self.set_status("reset")
            except zmq.Again:
                pass
        return request, response

    def soap_Configure(self, ps, **kw):
        request, response = ecc.soap_Configure(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_PREPARED:
            self.eccState = self.ECC_S_READY
            try:
                self.set_status("configure")
            except zmq.Again:
                pass
        return request, response

    def soap_Start(self, ps, **kw):
        request, response = ecc.soap_Start(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_READY:
            self.eccState = self.ECC_S_RUNNING
            try:
                self.set_status("start")
            except zmq.Again:
                pass
        return request, response

    def soap_Stop(self, ps, **kw):
        request, response = ecc.soap_Stop(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_RUNNING:
            self.eccState = self.ECC_S_READY
            try:
                self.set_status("stop")
            except zmq.Again:
                pass
        elif self.eccState == self.ECC_S_PAUSED:
            self.eccState = self.ECC_S_READY
            try:
                self.set_status("configure")
            except zmq.Again:
                pass
        return request, response

    def soap_Pause(self, ps, **kw):
        request, response = ecc.soap_Pause(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_RUNNING:
            self.eccState = self.ECC_S_PAUSED
            try:
                self.set_status("stop")
            except zmq.Again:
                pass
        return request, response

    def soap_Resume(self, ps, **kw):
        request, response = ecc.soap_Resume(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_PAUSED:
            self.eccState = self.ECC_S_RUNNING
            try:
                self.set_status("start")
            except zmq.Again:
                pass
        return request, response

    def soap_Breakup(self, ps, **kw):
        request, response = ecc.soap_Breakup(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_READY:
            self.eccState = self.ECC_S_PREPARED
            try:
                self.set_status("reset")
            except zmq.Again:
                pass
        return request, response

    def soap_Undo(self, ps, **kw):
        request, response = ecc.soap_Undo(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        if self.eccState == self.ECC_S_PREPARED:
            self.eccState = self.ECC_S_DESCRIBED
            try:
                self.set_status("reset")
            except zmq.Again:
                pass
        elif self.eccState == self.ECC_S_DESCRIBED:
            self.eccState = self.ECC_S_IDLE
            try:
                self.set_status("reset")
            except zmq.Again:
                pass
        return request, response

    def soap_GetState(self, ps, **kw):
        request, response = ecc.soap_GetState(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.State = self.eccState
        response.Transition = 0
        return request, response

    def soap_Special(self, ps, **kw):
        request, response = ecc.soap_Special(self, ps, **kw)
        response.ErrorCode = 0
        response.ErrorMessage = ""
        response.Text = ""
        return request, response

    def set_status(self, status):

        req = RCS.Request()
        req.channel = RCS.Request.RC
        req.operation = RCS.Request.WRITE
        req.module = RCS.Request.CNT
        req.variable = "event"
        req.value = status

        self.socket.send_string(req.SerializeToString(), zmq.NOBLOCK)
        sleep(self.timeout)

        try:
            message = self.socket.recv(zmq.NOBLOCK)
        except zmq.Again:
            self.initSocket()
            raise

        res = RCS.Response();
        res.ParseFromString(str(message))

        return res.errorcode

AUTOMAKE_OPTIONS = subdir-objects
DEFAULT_INCLUDES = -I. -Isrc $(protobuf_CFLAGS) $(configpp_CFLAGS) $(LOG4CPP_CFLAGS) $(zmq_CFLAGS)

bin_PROGRAMS = FzDAQ-mt FzController FzTransmitter

FzDAQ_mt_SOURCES = \
src/proto/FzRCS.pb.cc\
src/proto/FzNodeReport.pb.cc \
src/proto/FzEventSet.pb.cc \
src/reader/FzReader.cpp \
src/utils/fossa.c \
src/parser/FzFSM.cpp \
src/parser/FzParser.cpp \
src/writer/FzWriter.cpp \
src/rc/RCFSM.cpp \
src/nodemanager/FzNodeManager.cpp \
src/main/FzConfig.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp \
src/main/FzDAQ-mt.cpp  

FzController_SOURCES = \
src/proto/FzRCS.pb.cc\
src/proto/FzNodeReport.pb.cc \
src/logger/FzHttpPost.cpp \
src/controller/FzController.cpp \
src/controller/log.cpp \
src/controller/collector.cpp \
src/controller/rcs.cpp \
src/controller/cli.cpp \
src/controller/weblog.cpp \
src/controller/map.cpp \
src/controller/main.cpp \
src/main/FzConfig.cpp \
src/rc/RCFSM.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp

FzTransmitter_SOURCES = \
src/proto/FzEventSet.pb.cc \
src/transmitter/Tcp.cpp \
src/transmitter/fdtTransmitter.cpp \
src/transmitter/main.cpp

include Makefile.pb
include Makefile.init

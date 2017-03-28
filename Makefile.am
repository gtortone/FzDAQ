AUTOMAKE_OPTIONS = subdir-objects
DEFAULT_INCLUDES = -I. -Isrc

bin_PROGRAMS = FzDAQ-mt FzController

FzDAQ_mt_SOURCES = \
src/proto/FzRCS.pb.cpp\
src/proto/FzNodeReport.pb.cpp \
src/proto/FzEventSet.pb.cpp \
src/reader/FzReader.cpp \
src/utils/fossa.c \
src/parser/FzFSM.cpp \
src/parser/FzParser.cpp \
src/writer/FzWriter.cpp \
src/rc/RCFSM.cpp \
src/nodemanager/FzNodeManager.cpp \
src/rc/FzEpics.cpp \
src/rc/softIoc_registerRecordDeviceDriver.cpp \
src/main/FzConfig.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp \
src/main/FzDAQ-mt.cpp  

FzController_SOURCES = \
src/proto/FzRCS.pb.cpp\
src/proto/FzNodeReport.pb.cpp \
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
src/rc/FzEpics.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp

include Makefile.pb
include Makefile.init

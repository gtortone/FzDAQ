AUTOMAKE_OPTIONS = subdir-objects
DEFAULT_INCLUDES = -I. -Isrc

bin_PROGRAMS = FzDAQ-mt FzController

FzDAQ_mt_SOURCES = \
src/reader/FzReader.cpp \
src/utils/fossa.c \
src/parser/FzFSM.cpp \
src/parser/FzParser.cpp \
src/writer/FzWriter.cpp \
src/rc/RCFSM.cpp \
src/controller/FzNodeManager.cpp \
src/rc/FzEpics.cpp \
src/rc/softIoc_registerRecordDeviceDriver.cpp \
src/proto/FzNodeReport.pb.cpp \
src/proto/FzEventSet.pb.cpp \
src/main/FzConfig.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp \
src/main/FzDAQ-mt.cpp  

FzController_SOURCES = \
src/controller/FzController.cpp \
src/main/FzConfig.cpp \
src/proto/FzNodeReport.pb.cpp \
src/logger/FzHttpPost.cpp \
src/rc/RCFSM.cpp \
src/rc/FzEpics.cpp \
src/logger/FzLogger.cpp \
src/utils/FzUtils.cpp \
src/rc/softIoc_registerRecordDeviceDriver.cpp

include Makefile.pb
include Makefile.init

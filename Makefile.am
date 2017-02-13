AUTOMAKE_OPTIONS=subdir-objects

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

FzDAQ_mt_CXXFLAGS = -std=c++0x -Wall -Isrc -I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux -I$(EPICS_BASE)/include/compiler/gcc -I/usr/local/include/activemq-cpp-3.9.0 -I/usr/local/apr/include/apr-1
FzDAQ_mt_LDADD = -L/usr/local/lib -lusb-1.0 -lprotobuf -lpthread -lz -lboost_thread -llog4cpp -lboost_system -lboost_program_options -lzmq -lconfig++ -L $(EPICS_BASE)/lib/linux-x86_64 -ldbRecStd -ldbCore -lCom -lca -lactivemq-cpp

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

FzController_CXXFLAGS = -std=c++0x -Wall -Isrc -I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux -I$(EPICS_BASE)/include/compiler/gcc -I/usr/local/include/activemq-cpp-3.9.0 -I/usr/local/apr/include/apr-1
FzController_LDADD = -L/usr/local/lib -lboost_thread -lboost_system -lboost_program_options -lprotobuf -lzmq -lconfig++ -lpthread -lcurl -L $(EPICS_BASE)/lib/linux-x86_64 -ldbRecStd -ldbCore -lCom -lca -lactivemq-cpp -llog4cpp

include Makefile.pb

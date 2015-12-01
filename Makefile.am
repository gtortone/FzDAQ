bin_PROGRAMS = FzDAQ-mt FzController
FzDAQ_mt_SOURCES = FzReader.cpp fossa.c FzFSM.cpp FzParser.cpp FzWriter.cpp RCFSM.cpp FzNodeManager.cpp FzEpics.cpp softIoc_registerRecordDeviceDriver.cpp FzNodeReport.pb.cpp FzEventSet.pb.cpp FzConfig.cpp FzLogger.cpp FzUtils.cpp FzDAQ-mt.cpp  
FzDAQ_mt_CXXFLAGS = -std=c++0x -Wall -I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux -I$(EPICS_BASE)/include/compiler/gcc -I/usr/local/include/activemq-cpp-3.9.0 -I/usr/local/apr/include/apr-1
FzDAQ_mt_LDADD = -L/usr/local/lib -lusb-1.0 -lprotobuf -lpthread -lz -lboost_thread -llog4cpp -lboost_system -lboost_program_options -lzmq -lconfig++ -L $(EPICS_BASE)/lib/linux-x86_64 -ldbRecStd -ldbCore -lCom -lca -lactivemq-cpp

FzController_SOURCES = FzController.cpp FzConfig.cpp FzNodeReport.pb.cpp FzHttpPost.cpp RCFSM.cpp FzEpics.cpp FzLogger.cpp FzUtils.cpp softIoc_registerRecordDeviceDriver.cpp
FzController_CXXFLAGS = -std=c++0x -Wall -I$(EPICS_BASE)/include -I$(EPICS_BASE)/include/os/Linux -I$(EPICS_BASE)/include/compiler/gcc -I/usr/local/include/activemq-cpp-3.9.0 -I/usr/local/apr/include/apr-1
FzController_LDADD = -L/usr/local/lib -lboost_thread -lboost_system -lboost_program_options -lprotobuf -lzmq -lconfig++ -lpthread -lcurl -L $(EPICS_BASE)/lib/linux-x86_64 -ldbRecStd -ldbCore -lCom -lca -lactivemq-cpp -llog4cpp

include Makefile.pb

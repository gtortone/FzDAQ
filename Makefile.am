bin_PROGRAMS = FzDAQ-mt FzController
FzDAQ_mt_SOURCES = FzReader.cpp fossa.c FzFSM.cpp FzParser.cpp FzWriter.cpp FzNodeManager.cpp FzNodeReport.pb.cpp FzEventSet.pb.cpp FzConfig.cpp FzUtils.cpp FzDAQ-mt.cpp  
FzDAQ_mt_CXXFLAGS = -std=c++0x -Wall
FzDAQ_mt_LDADD = -L/usr/local/lib -lusb-1.0 -lprotobuf -lpthread -lz -lboost_thread -llog4cpp -lboost_system -lboost_program_options -lzmq -lconfig++

FzController_SOURCES = FzController.cpp FzConfig.cpp FzNodeReport.pb.cpp mongoose.c
FzController_CXXFLAGS = -std=c++0x -Wall
FzController_LDADD = -L/usr/local/lib -lboost_thread -lboost_system -lboost_program_options -lprotobuf -lzmq -lconfig++ -lpthread

include Makefile.pb

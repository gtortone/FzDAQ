bin_PROGRAMS = FzDAQ-mt
FzDAQ_mt_SOURCES = FzReader.cpp FzParser.cpp FzWriter.cpp FzEventWord.cpp FzEventSet.pb.cpp FzDAQ-mt.cpp 
FzDAQ_mt_CXXFLAGS = -std=c++0x -I/usr/local/root/include
FzDAQ_mt_LDFLAGS = -L/usr/local/lib -L/usr/local/root/lib -lusb-1.0 -lprotobuf -lpthread -lz -lboost_thread -lboost_system -llog4cpp -lboost_program_options -lzmq 

include Makefile.pb

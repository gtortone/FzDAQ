C++ = g++

CFLAGS = -O3

SERIALLIBS = -lpthread -lprotobuf 

SOURCES = ../src/proto/FzEventSet.pb.cpp read-fazia-pb.cpp
INCDIR = -I.. -I../src/proto

read-fazia-pb: 
	$(C++) $(CFLAGS) $(INCDIR) $(SOURCES) $(SERIALLIBS) -o $@

clean:
	rm -f *.o read-fazia-pb 

C++ = g++

CFLAGS = -O3

SERIALLIBS = -lpthread -lprotobuf 

INCDIR = -I.. -I../src/proto -I../src/utils

all: read-fazia-pb publish-fazia-pb

read-fazia-pb: read-fazia-pb.cpp
	$(C++) $(CFLAGS) $(INCDIR) ../src/proto/FzEventSet.pb.cc read-fazia-pb.cpp $(SERIALLIBS) -o $@

publish-fazia-pb: publish-fazia-pb.cpp
	$(C++) $(CFLAGS) $(INCDIR) ../src/proto/FzEventSet.pb.cc publish-fazia-pb.cpp $(SERIALLIBS) -lzmq -o $@

clean:
	rm -f *.o read-fazia-pb publish-fazia-pb

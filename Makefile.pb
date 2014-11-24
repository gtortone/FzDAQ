proto: 
	protoc FzEventSet.proto --cpp_out=.
	mv FzEventSet.pb.cc FzEventSet.pb.cpp

deploy:
	ssh daq@fzdaq01 "rm -rf devel/Fazia/miniDAQ-mt/*"
	scp README ChangeLog NEWS AUTHORS COPYING configure.in Makefile.am Makefile.pb *.cpp *.h *.hpp daq@fzdaq01:/home/daq/devel/Fazia/miniDAQ-mt
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; rm -f *.o"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; autoreconf -ivf"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; ./configure"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; make"
	ssh root@fzdaq01 "cd /home/daq/devel/Fazia/miniDAQ-mt ; make remote-install"

remote-install:
	cp FzDAQ-mt /opt/FzDAQ/


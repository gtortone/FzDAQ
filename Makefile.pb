space:
	@echo -e "removing event files..."
	@rm -rf pbout/* 
	@echo -e "removing record files..."
	@rm -rf record/*
	@echo -e "removing log files..."
	@rm -rf logs/*
	@echo -e "OK"

proto: 
	protoc FzEventSet.proto --cpp_out=.
	mv FzEventSet.pb.cc FzEventSet.pb.cpp
	sed -i 's/assert/\/\/assert/' FzEventSet.pb.h
	protoc FzNodeReport.proto --cpp_out=.
	mv FzNodeReport.pb.cc FzNodeReport.pb.cpp

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


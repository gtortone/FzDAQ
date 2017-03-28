space$(EXEEXT):
	@echo -e "removing event files..."
	@rm -rf pbout/* 
	@echo -e "removing record files..."
	@rm -rf record/*
	@echo -e "removing log files..."
	@rm -rf logs/*
	@echo -e "OK"

src/proto/FzEventSet.pb.cpp$(EXEEXT) src/proto/FzEventSet.pb.h$(EXEEXT): 
	protoc src/proto/FzEventSet.proto --cpp_out=.
	mv src/proto/FzEventSet.pb.cc src/proto/FzEventSet.pb.cpp
	sed -i 's/assert/\/\/assert/' src/proto/FzEventSet.pb.h

src/proto/FzNodeReport.pb.cpp$(EXEEXT) src/proto/FzNodeReport.pb.h$(EXEEXT):
	protoc src/proto/FzNodeReport.proto --cpp_out=.
	mv src/proto/FzNodeReport.pb.cc src/proto/FzNodeReport.pb.cpp

src/proto/FzRCS.pb.cpp$(EXEEXT) src/proto/FzRCS.pb.h$(EXEEXT):
	protoc src/proto/FzRCS.proto --cpp_out=.
	mv src/proto/FzRCS.pb.cc src/proto/FzRCS.pb.cpp

clean-local:
	-rm -f src/proto/*.cpp src/proto/*.h

deploy$(EXEEXT):
	ssh daq@fzdaq01 "rm -rf devel/Fazia/miniDAQ-mt/*"
	scp README ChangeLog NEWS AUTHORS COPYING configure.in Makefile.am Makefile.pb *.cpp *.h *.hpp daq@fzdaq01:/home/daq/devel/Fazia/miniDAQ-mt
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; rm -f *.o"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; autoreconf -ivf"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; ./configure"
	ssh daq@fzdaq01 "cd devel/Fazia/miniDAQ-mt ; make"
	ssh root@fzdaq01 "cd /home/daq/devel/Fazia/miniDAQ-mt ; make remote-install"

remote-install$(EXEEXT):
	cp FzDAQ-mt /opt/FzDAQ/

ttt$(EXEEXT):
	@touch gennaro-tortone

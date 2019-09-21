space$(EXEEXT):
	@echo -e "removing event files..."
	@rm -rf pbout/* 
	@echo -e "removing record files..."
	@rm -rf record/*
	@echo -e "removing log files..."
	@rm -rf logs/*
	@echo -e "OK"

src/proto/FzEventSet.pb.cc$(EXEEXT) src/proto/FzEventSet.pb.h$(EXEEXT): 
	protoc src/proto/FzEventSet.proto --cpp_out=.
	sed -i 's/assert/\/\/assert/' src/proto/FzEventSet.pb.h

src/proto/FzNodeReport.pb.cc$(EXEEXT) src/proto/FzNodeReport.pb.h$(EXEEXT):
	protoc src/proto/FzNodeReport.proto --cpp_out=.

src/proto/FzRCS.pb.cc$(EXEEXT) src/proto/FzRCS.pb.h$(EXEEXT):
	protoc src/proto/FzRCS.proto --cpp_out=.

clean-local:
	-rm -f src/proto/*.cc src/proto/*.h

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

src/proto/FzEventSet.pb.cc$(EXEEXT) src/proto/FzEventSet.pb.h$(EXEEXT): 
	protoc src/proto/FzEventSet.proto --cpp_out=.
	sed -i 's/assert/\/\/assert/' src/proto/FzEventSet.pb.h

src/proto/FzNodeReport.pb.cc$(EXEEXT) src/proto/FzNodeReport.pb.h$(EXEEXT):
	protoc src/proto/FzNodeReport.proto --cpp_out=.

src/proto/FzRCS.pb.cc$(EXEEXT) src/proto/FzRCS.pb.h$(EXEEXT):
	protoc src/proto/FzRCS.proto --cpp_out=.

clean-local:
	-rm -f src/proto/*.cc src/proto/*.h

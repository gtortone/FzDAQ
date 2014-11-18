#ifndef FZPROTOBUF_H_
#define FZPROTOBUF_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include "FzEventSet.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream.h>

class FzProtobuf {

private:
   std::string subdir;
   std::string runtag;
   std::string steptag;

   unsigned int fileid;
   std::stringstream filename;
   std::fstream output;

public:

FzProtobuf (std::string dir, std::string run, std::string step) {

   subdir = dir;
   runtag = run;
   steptag = step;

   fileid = 0;
}

void setup_newfile(void) {

   if(output.good())
      output.close();

   // setup of filename and output stream
   filename.str("");
   filename << subdir << '/' << runtag << '-' << time(NULL) << '-' << steptag << '-' << fileid << ".pb";
   output.open(filename.str().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

   fileid++;
};

bool serialize_delimited(std::ostream& stream, DAQ::FzEventSet &message, uint32_t bsize) {

   //assert(message->IsInitialized());

   google::protobuf::io::OstreamOutputStream ostreamWrapper(&stream);
   google::protobuf::io::CodedOutputStream codedOStream(&ostreamWrapper);

   // Write the message size first.
   //assert(bsize > 0);
   codedOStream.WriteLittleEndian32(message.ByteSize());
   //codedOStream.WriteLittleEndian32(bsize);

   // Write the message.

   // new code
   message.SerializeToCodedStream(&codedOStream);
   // new code

   // message.SerializeWithCachedSizes(&codedOStream);

   return stream.good();
};

bool parse_delimited(std::istream& stream, DAQ::FzEventSet *message) {

   uint32_t messageSize = 0;

   {
   // Read the message size.
   google::protobuf::io::IstreamInputStream istreamWrapper(&stream, sizeof(uint32_t));
   google::protobuf::io::CodedInputStream codedIStream(&istreamWrapper);

   // Don't consume more than sizeof(uint32_t) from the stream.
   google::protobuf::io::CodedInputStream::Limit oldLimit = codedIStream.PushLimit(sizeof(uint32_t));
   codedIStream.ReadLittleEndian32(&messageSize);
   codedIStream.PopLimit(oldLimit);
   //assert(messageSize > 0);
   if (messageSize <= 0) return stream.eof();
   //assert(istreamWrapper.ByteCount() == sizeof(uint32_t));
   }

   {
   // Read the message.
   google::protobuf::io::IstreamInputStream istreamWrapper(&stream, messageSize);
   google::protobuf::io::CodedInputStream codedIStream(&istreamWrapper);

   // Read the message, but don't consume more than messageSize bytes from the stream.
   google::protobuf::io::CodedInputStream::Limit oldLimit = codedIStream.PushLimit(messageSize);
   message->ParseFromCodedStream(&codedIStream);
   codedIStream.PopLimit(oldLimit);
   //assert(istreamWrapper.ByteCount() == messageSize);
   }

   return stream.good();
};

void WriteDataset(DAQ::FzEventSet &ev) {

   serialize_delimited(output, ev, ev.ByteSize());
};

};

#endif

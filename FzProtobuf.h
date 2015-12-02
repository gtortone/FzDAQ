#ifndef FZPROTOBUF_H_
#define FZPROTOBUF_H_

#define DIR_OK		0
#define DIR_EXISTS	1
#define DIR_FAIL	2

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include "FzEventSet.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
// header for compatibility - libprotobuf 2.6.1
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>

class FzProtobuf {

public:

bool serialize_delimited(std::ostream& stream, DAQ::FzEventSet &message, uint32_t bsize) {

   //assert(message->IsInitialized());

   google::protobuf::io::OstreamOutputStream ostreamWrapper(&stream);
   google::protobuf::io::CodedOutputStream codedOStream(&ostreamWrapper);

   // Write the message size first.
   //assert(bsize > 0);
   codedOStream.WriteLittleEndian32(message.ByteSize());
   //codedOStream.WriteLittleEndian32(bsize);

   // Write the message.
   message.SerializeToCodedStream(&codedOStream);

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

void WriteDataset(std::fstream &output, DAQ::FzEventSet &ev) {

   serialize_delimited(output, ev, ev.ByteSize());
};

void WriteDataset(std::fstream &output, std::string &message) {

   google::protobuf::io::OstreamOutputStream ostreamWrapper(&output);
   google::protobuf::io::CodedOutputStream codedOStream(&ostreamWrapper);

   codedOStream.WriteLittleEndian32(message.length());
   codedOStream.WriteRaw(message.c_str(), message.length());
};

};	//end of class

#endif

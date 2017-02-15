#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include "FzEventSet.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

using namespace std;


bool parse_delimited(istream& stream, DAQ::FzEventSet *message) {

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
}

// MAIN

int main(int argc, char* argv[]) {

   time_t tm = time(NULL);
   time_t mark1, mark2;
   int i,j;
   unsigned int fileid = 0;
   unsigned long int hitset_size = 0;
   unsigned long int tothits = 0;
   uint32_t bsize;

   static char const* const FzFec_str[] = { "FEC#0", "FEC#1", "FEC#2", "FEC#3", "FEC#4", "FEC#5", "FEC#6", "FEC#7", "", "", "", "", "", "", "", "ADC"  };
   static char const* const FzDataType_str[] = { "QH1", "I1", "QL1", "Q2", "I2", "Q3", "ADC", "UNK" };
   static char const* const FzTelescope_str[] = { "A", "B" };
   static char const* const FzDetector_str[] = { "Si1", "Si2", "CsI" };

   // Verify that the version of the library that we linked against is
   // compatible with the version of the headers we compiled against.
   GOOGLE_PROTOBUF_VERIFY_VERSION;

   DAQ::FzEvent ev;
   DAQ::FzBlock *blk;
   DAQ::FzFee *fee;
   DAQ::FzHit *hit;
   DAQ::FzData *d;
   DAQ::Energy *en;
   DAQ::Waveform *wf;

   // ==== READ hits from file 0 ====
   
   DAQ::FzEventSet EVSET; 

   string filename(argv[1]);
   fstream input(filename.c_str(), ios::in | ios::binary);
   
   cout << "open file: " << filename << endl;

   while (parse_delimited(input, &EVSET) == input.good()) {
 
    for(int s=0; s < EVSET.ev_size(); s++) {

      ev = EVSET.ev(s);

      cout << "EC: " << ev.ec() << endl;

      for (i = 0; i < ev.block_size(); i++) {
      
         const DAQ::FzBlock& rdblk = ev.block(i);
 
         if(rdblk.has_len_error() && rdblk.has_crc_error() )
            cout << "\tBLKID: " << rdblk.blkid() << " - LEN_ERR: " << rdblk.len_error() << " - CRC_ERR: " << rdblk.crc_error() << endl;
         else
            cout << "\tBLKID: " << rdblk.blkid() << endl;
 
         for(j = 0; j < rdblk.fee_size(); j++) {

            const DAQ::FzFee& rdfee = rdblk.fee(j);
            cout << "\t\t" << FzFec_str[rdfee.feeid()] << " - LEN_ERR: " << rdfee.len_error() << " - CRC_ERR: " << rdfee.crc_error() << endl;

            for(int k = 0; k < rdfee.hit_size(); k++) {
            
               const DAQ::FzHit& rdhit = rdfee.hit(k);
               cout << "\t\t\tEC: " << rdhit.ec();

               cout << " - TEL: ";
               if(rdhit.has_telid())
                  cout << FzTelescope_str[rdhit.telid()];
               else { cout << "<ERR>" << endl; continue; }
               
               cout << " - DET: ";
               if(rdhit.has_detid())
                  cout << FzDetector_str[rdhit.detid()];
               else { cout << "<ERR>" << endl; continue; }
     
               cout << " - FEE: ";
               if(rdhit.has_feeid()) 
                  cout << FzFec_str[rdhit.feeid()];
               else { cout << "<ERR>" << endl; continue; }

               cout << " - GTTAG: " << rdhit.gttag();
               cout << " - DETTAG: " << rdhit.dettag() << endl;

               for(int m = 0; m < rdhit.data_size(); m++) {

                  const DAQ::FzData& rdata = rdhit.data(m);
                  cout << "\t\t\t\tDATATYPE: " << FzDataType_str[rdata.type()];

                  if(!rdata.has_energy() && !rdata.has_waveform()) {
                     cout << " [NO DATA]" << endl;
                     continue;
                  }

                  if(rdata.has_energy()) {

	             const DAQ::Energy& ren = rdata.energy();
		     cout << " - ENERGY: ";

                     for(int e=0; e < ren.value_size(); e++)
                        cout << ren.value(e) << ", ";

                     cout << " - LEN_ERR: " << ren.len_error();
                  }

                  if(rdata.has_waveform()) {

                    const DAQ::Waveform& rwf = rdata.waveform();
                    cout << " - PRETRIG: " << rwf.pretrig();

                    if(rdata.type() == DAQ::FzData::ADC) {

                       cout << " - SINE: " << rdata.sine() << " - COSINE: " << rdata.cosine();
                    }

                    cout << " - WAVEFORM: " << " - LEN_ERR: " << rwf.len_error() << " len = " << rwf.sample_size() << endl;
                    cout << endl << "\t\t\t\t              ";
                    signed int supp;
                    for(int n=0; n < rwf.sample_size(); n++) {
                       if(n%16 == 0 && n) 
                          cout << endl << "\t\t\t\t              ";

                       if(rwf.sample(n) > 8191)
                          supp = rwf.sample(n) | 0xFFFFC000;
                       else
                          supp = rwf.sample(n);
                       
                       cout << supp << ", ";
                       //cout << hex << "(0x" << rwf.sample(n) << ")";
                       //cout << hex << "0x" << supp << ", ";
                    }
                  }

   		  cout << endl;
               
               }
            }
           
         }

         for(int j=0; j<ev.trinfo_size(); j++) {

            cout << "TRIG ID: " << ev.trinfo(j).id() << "\tATTR: " << ev.trinfo(j).attr() << "\tVALUE: " << ev.trinfo(j).value() << endl;

         }
      }
   } 

 }
   google::protobuf::ShutdownProtobufLibrary();

   return 0;
}

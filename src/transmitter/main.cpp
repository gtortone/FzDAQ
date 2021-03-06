#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <arpa/inet.h>
#include "boost/program_options.hpp"
#include "utils/zmq.hpp"
#include "proto/FzEventSet.pb.h"
#include "transmitter/Tcp.h"
#include "transmitter/fdtTransmitter.h"
#include "transmitter/fdt.h"

namespace po = boost::program_options;

int main(int argc, char*argv[]) {
   
   zmq::context_t context(1);
   zmq::message_t event;
   zmq::socket_t *sub;
   DAQ::FzEventSet ev;

   // handling of command line parameters
   po::options_description desc("\nFzDAQ => FDT- allowed options", 100);

   desc.add_options()
    ("help", "produce help message")
    ("sh", po::value<std::string>(), "[mandatory] FzDAQ spy hostname")
    ("sp", po::value<std::string>(), "[optional] FzDAQ spy port (default: 5563)")
    ("fh", po::value<std::string>(), "[mandatory] FDT server hostname")
    ("fp", po::value<uint16_t>(), "[mandatory] FDT server port")
   ;

   po::variables_map vm;

   try {

      po::store(po::parse_command_line(argc, argv, desc), vm);

   } catch (po::error& e) { 

      std::cout << "ERROR: " << e.what() << std::endl << std::endl;
      std::cout << desc << std::endl;
      exit(1);
   }

   po::notify(vm);

   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      exit(0);
   }

   std::string ep;
   
   if(vm.count("sh")) {

      ep = "tcp://" + vm["sh"].as<std::string>() + ":";
      if(vm.count("sp"))
         ep = ep + vm["sp"].as<std::string>();
      else
         ep = ep + "5563";
	 
      std::cout << "I: FzWriter spy endpoint: " << ep << std::endl;

   } else {

      std::cout << "ERROR: FzDAQ spy hostname missing!" << std::endl;
      std::cout << desc << std::endl;
      exit(1);
   }

   try {
      sub = new zmq::socket_t(context, ZMQ_SUB);
   } catch (zmq::error_t &e) {
      std::cout << "ERROR: failed to create ZMQ subscriber socket:" << e.what() << std::endl;
      exit(1);
   }
   
   try { 
      sub->connect(ep.c_str());
   } catch (zmq::error_t &e) {
      std::cout << "ERROR: failed to connect with FzDAQ spy publisher:" << e.what() << std::endl;
      exit(1);
   }

   sub->setsockopt(ZMQ_SUBSCRIBE, "", 0);

   std::string fdthost;
   uint16_t fdtport;

   if(vm.count("fh") && vm.count("fp")) {

      fdthost = vm["fh"].as<std::string>();
      fdtport = vm["fp"].as<uint16_t>();

      std::cout << "I: FDT server endpoint: " << "fdt://" << fdthost << ":" << fdtport << std::endl;

   } else {

      std::cout << "ERROR: FDT hostname and port missing!" << std::endl;
      std::cout << desc << std::endl;
      exit(1);
   }

   extern int TcpWind;
   TcpWind = 0x8000;	// 32 Kb
   fdtTransmitter *trm = new fdtTransmitter();
   int err = OK;
   unsigned char mfmevent[0x400000]; // 4 MB buffer
   
   // don't raise SIGPIPE when sending into broken TCP connections
   ::signal(SIGPIPE, SIG_IGN); 

   trm->setTimeout(10);
   err = trm->connect(fdthost.c_str(), fdtport);

   if (err != OK) {

      std::cout << "E: FDT failed with error " << err << std::endl;

      switch (err) {
	 case R_TCP_DNS    : cout << " (R_TCP_DNS)"    << endl; break;
      	 case R_TCP_SOCKET : cout << " (R_TCP_SOCKET)" << endl; break;
         case R_TCP_BIND   : cout << " (R_TCP_BIND)"   << endl; break;
         case R_TCP_CONNECT: cout << " (R_TCP_CONNECT)"<< endl; break;
         default           : cout << " (UNKNOWN)"      << endl;
      }

      delete trm;
      sub->close();

      exit(1);
   }  

   cout << "I: FDT ok" << endl;

   while (1) {

      sub->recv(&event);

      std::cout << "I: got event..." << std::endl;

      if(ev.ParseFromArray(event.data(), event.size()) == true) {
      
	 std::cout << "I: ev.ev_size() = " << ev.ev_size() << std::endl;
         std::cout << "I: parsing succedeed - event size = " << event.size() << std::endl;

	 long long mfmeventsize = 20 + 4 + ev.ByteSize();	// MFM header + len of pb data + pb data
         if(mfmeventsize % 2)
	    mfmeventsize++;

	
	 std::cout << "I: MFM serialized data length: " << mfmeventsize << std::endl;
	 // start of MFM header
	 mfmevent[0] = 0x41;					// metaType: big endian
	 mfmevent[1] = (mfmeventsize/2 & 0xFF0000) >> 16;	// frameSize: total size of frame in 16 bit
	 mfmevent[2] = (mfmeventsize/2 & 0x00FF00) >> 8;
	 mfmevent[3] = (mfmeventsize/2 & 0x0000FF);
	 mfmevent[4] = 0x00;					// subsystemNumber: set to 0
	 mfmevent[5] = 0x00;					// frameType: set to 0x0040
	 mfmevent[6] = 0x40;
	 mfmevent[7] = 0x00;					// revision: set to 0

 	 for(int i=0; i<=12; i++)
	    mfmevent[i+8] = 0;

         for(int j=0; j<ev.ev(0).trinfo_size(); j++) {

	    if(ev.ev(0).trinfo(j).id() == 0x200) {		// centrum0

	       mfmevent[8] |= (ev.ev(0).trinfo(j).value() & 0x3F) << 2;

	    } else if(ev.ev(0).trinfo(j).id() == 0x201) {	// centrum1

	       // Centrum Timestamp
	       mfmevent[8] |= (ev.ev(0).trinfo(j).value() >> 28);
	       mfmevent[9] = (ev.ev(0).trinfo(j).value() >> 20) & 0xFF;
	       mfmevent[10] = (ev.ev(0).trinfo(j).value() >> 12) & 0xFF;  
	       mfmevent[11] = (ev.ev(0).trinfo(j).value() >> 4) & 0xFF; 
	       mfmevent[12] |= (ev.ev(0).trinfo(j).value() & 0xF) << 4;

	    } else if(ev.ev(0).trinfo(j).id() == 0x202) {	// centrum2

	       mfmevent[12] |= (ev.ev(0).trinfo(j).value() >> 26);
	       mfmevent[13] = (ev.ev(0).trinfo(j).value() >> 18) & 0xFF; 
	       // Centrum Event Counter
	       mfmevent[14] = (ev.ev(0).trinfo(j).value() >> 10) & 0xFF;
	       mfmevent[15] = (ev.ev(0).trinfo(j).value() >> 2) & 0xFF;
	       mfmevent[16] |= (ev.ev(0).trinfo(j).value() & 0x3) << 6;

	    } else if(ev.ev(0).trinfo(j).id() == 0x203) {	// centrum3

	       mfmevent[16] |= (ev.ev(0).trinfo(j).value() >> 24);
	       mfmevent[17] = (ev.ev(0).trinfo(j).value() >> 16) & 0xFF;
	       // Centrum CRC
	       mfmevent[18] = (ev.ev(0).trinfo(j).value() >> 8) & 0xFF;
	       mfmevent[19] = ev.ev(0).trinfo(j).value() & 0xFF;
	    }
         }

	 // end of MFM header
	 mfmevent[20] = (ev.ByteSize() & 0x000000FF);
	 mfmevent[21] = (ev.ByteSize() & 0x0000FF00) >> 8;
	 mfmevent[22] = (ev.ByteSize() & 0x00FF0000) >> 16;
	 mfmevent[23] = (ev.ByteSize() & 0xFF000000) >> 24;

	 memcpy(mfmevent + 24, event.data(), mfmeventsize - 24);	// dataBlock: FAZIA data block

	 err = trm->send(mfmevent, mfmeventsize);

	 if(err != OK) {

            std::cout << "E: no FDT connection" << std::endl;
	    std::cout << "I: try to reconnect..." << std::endl;
   	    err = trm->connect(fdthost.c_str(), fdtport);

	    if (err != OK) {

	       std::cout << "E: FDT failed with error " << err;

               switch (err) {

	          case R_TCP_DNS    : std::cout << " (R_TCP_DNS)"    << std::endl; break;
	          case R_TCP_SOCKET : std::cout << " (R_TCP_SOCKET)" << std::endl; break;
	          case R_TCP_BIND   : std::cout << " (R_TCP_BIND)"   << std::endl; break;
	          case R_TCP_CONNECT: std::cout << " (R_TCP_CONNECT)"<< std::endl; break;
	          default           : std::cout << " (UNKNOWN)"      << std::endl;
               }

	    } else std::cout << "I: FDT ok" << std::endl;
	 }

         ev.Clear();

      } else {

         std::cout << "E: parsing failed" << std::endl;
      }

   }	// end while(1)

   return 0;
}

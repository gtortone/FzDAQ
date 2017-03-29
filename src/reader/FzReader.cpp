#include "FzReader.h"
#include "utils/FzTypedef.h"
#include "utils/FzUtils.h"
#include <fstream>
#include <sstream> 

#define REGHDR_FMT              0xF800
#define REGHDR_TAG              0xC800  

#ifdef AMQLOG_ENABLED
FzReader::FzReader(std::string nurl, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn) :
   context(ctx), AMQconn(JMSconn) {
#else
FzReader::FzReader(std::string nurl, std::string cfgfile, zmq::context_t &ctx) :
   context(ctx) {
#endif
  
   neturl = nurl;

   log.setFileConnection("fzreader", "/var/log/fzdaq/fzreader.log");

#ifdef AMQLOG_ENABLED
   if(AMQconn.get())
      log.setJMSConnection("FzReader", JMSconn);
#endif

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str()); 	// syntax checks on main

   } else hascfg = false;

   log.write(INFO, "thread allocated");

   std::string ep;

   try {
   
      reader = new zmq::socket_t(context, ZMQ_PUSH);
      int linger = 0;
      reader->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));	// linger equal to 0 for a fast socket shutdown

   }  catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzReader: failed to start ZeroMQ producer: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzreader.producer");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzReader: producer endpoint not present in config file" << std::endl;
         exit(1);
      }
 
      std::cout << INFOTAG << "FzReader: producer endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "inproc://fzreader";
      std::cout << INFOTAG << "FzReader: producer endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      reader->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzReader: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // prevent init method run thread
   rcstate = IDLE;
   thread_init = false;

   // init
   if (init() != 0) {

      std::cout << ERRTAG << "FzReader: device initialization failed" << std::endl;
      exit(1);

   } else std::cout << INFOTAG << "FzReader: device initialization done" << std::endl;

   report.Clear();
};

void FzReader::close(void) {
   thr->interrupt();
   thr->join();
}

void FzReader::record(bool val) {
   cb_data.rec = val;
};

int FzReader::init(void) {

   return(initNet());
};

int FzReader::initNet(void) {

   cb_data.log_ptr = &log;
   cb_data.reader_ptr = reader;
   cb_data.report_ptr = &report;

   ns_mgr_init(&udpserver, NULL);
   udpserver.user_data = &cb_data;

   if(ns_bind(&udpserver, neturl.c_str(), udp_handler) == NULL) {

      std::cout << ERRTAG << "FzReader: unable to bind UDP socket" << std::endl;
      exit(1);
   }

   std::cout << INFOTAG << "FzReader: UDP socket bind successfully" << std::endl;
 
   if(!thread_init) {

      thread_init = true;
      thr = new boost::thread(boost::bind(&FzReader::process, this));
   }

   return(0);
};

/* static */ void FzReader::udp_handler(struct ns_connection *nc, int ev, void *ev_data) {

   static std::string prefix = "record/event-";
   static unsigned int fileno = 0;

   static FzRawData chunk;
   unsigned short int *bufusint;
   unsigned short int value;
   long int evlen;
/*
   uint32_t seqno;
   static uint32_t old_seqno = 0;
*/
   int offset;

   struct iobuf *io = &nc->recv_iobuf;

   // retrieve callback data
   struct _cb_data *pdata = (struct _cb_data *) nc->mgr->user_data;

   // FzLogger *log = (FzLogger *) pdata->log_ptr;
   zmq::socket_t *reader= (zmq::socket_t *) pdata->reader_ptr;
   Report::FzReader *report = (Report::FzReader *) pdata->report_ptr; 
   bool rec = (bool) pdata->rec;

   switch (ev) {

      case NS_RECV:

         bufusint = reinterpret_cast<unsigned short int*>(io->buf);
         
         evlen = ((io->len/2 * 2) == io->len)?io->len/2:io->len/2 + 1;

/*
         seqno = bufusint[1] + (bufusint[0] << 16);

         if(seqno != old_seqno + 1) {

            std::stringstream msg;
            msg << "FzReader: UDP out of sequence prev:" << old_seqno << " - curr " << seqno;
            log->write(ERROR, msg.str());
         }

         old_seqno = seqno;
*/

         offset = 2;
         if( (bufusint[0] == 0x8080) || (bufusint[1] == 0x8080) || (bufusint[2] == 0x8080) )
            offset++;

         for (long int i=offset; i < evlen; i++) {

            value = bufusint[i];
            chunk.push_back(value);

            // debug:
            //std::cout << std::setw(4) << std::setfill('0') << std::hex << value << std::endl;

            if( (value & REGHDR_FMT) == REGHDR_TAG ) {

               //std::cout << "FzReader: end of event" << std::endl;

               if(rec) {

                  std::ostringstream convert;
                  convert << prefix << std::setw(4) << std::setfill('0') << std::hex << chunk[1] << std::dec << "-" << fileno;

                  std::string filename = convert.str();

                  std::ofstream myfile;
                  myfile.open(filename);

                  for(unsigned int ix=0; ix<chunk.size(); ix++) {
                     myfile << std::setw(4) << std::setfill('0') << std::hex << chunk[ix] << ' ';
                  }

                  fileno++;
                  myfile.close();
               }                   // end if rec 

               report->set_in_events( report->in_events() + 1 );

               char *blob_data = reinterpret_cast<char *>(chunk.data());
               long int blob_size = chunk.size() * 2;
               reader->send(blob_data, blob_size);

               report->set_out_events( report->out_events() + 1);
               report->set_out_bytes( report->out_bytes() + blob_size );

               chunk.clear();
           }                       // end if eoe

        }	// end for

        report->set_in_bytes( report->in_bytes() + io->len );

        // Discard data from recv buffer
        iobuf_remove(io, io->len);      
  
        // if total event received data is greater than maximum discard event
        if(chunk.size() >= MAX_EVENT_SIZE) {
 
           chunk.clear();
/*
           std::stringstream msg;
           msg << "data discarded due to overcoming maximum event size: " << MAX_EVENT_SIZE << " bytes";
           log->write(ERROR, msg.str());
*/
        }

      break;

      default:
         break;
   }
}

void FzReader::process(void) {
 
   while(true) {

      try {

         if(rcstate == RUNNING) {

            ns_mgr_poll(&udpserver, 5);
 
            boost::this_thread::interruption_point();

         } else {

              boost::this_thread::sleep(boost::posix_time::seconds(1));
              boost::this_thread::interruption_point();

         } 

      } catch(boost::thread_interrupted& interruption) {

         reader->close();
         break;

      } catch(std::exception& e) {

      }
   }	// end while
};

Report::FzReader FzReader::get_report(void) {
   return(report);
};

void FzReader::set_rcstate(RCstate s) {

   rcstate = s;
}

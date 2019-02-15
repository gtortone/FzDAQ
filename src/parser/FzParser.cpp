#include "FzParser.h"
#include "utils/FzProtobuf.h"
#include "logger/FzLogger.h"

#ifdef AMQLOG_ENABLED
FzParser::FzParser(unsigned int id, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn, int evf, std::string logdir) :
   context(ctx), AMQconn(JMSconn), evformat(evf), logbase(logdir) {
#else
FzParser::FzParser(unsigned int id, std::string cfgfile, zmq::context_t &ctx, int evf, std::string logdir) :
   context(ctx), evformat(evf), logbase(logdir) {
#endif

   std::stringstream filename, msg;
   filename << logbase << "/fzparser.log." << id;

   log.setFileConnection("fzparser", filename.str());

#ifdef AMQLOG_ENABLED
   if(AMQconn.get()) {

      msg << "FzParser." << id;
      log.setJMSConnection(msg.str(), JMSconn);
      msg.str("");
   }
#endif

   cfg.readFile(cfgfile.c_str());    // syntax checks on main

   msg << "thread " << id << " allocated";
   log.write(INFO, msg.str());
   msg.str("");

   sm.initlog(&log, logbase, id);
   sm.init(evformat);

   std::string ep;

   try {

      parser = new zmq::socket_t(context, ZMQ_PULL);
      int tval = 1000;
      parser->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 1 second timeout on recv
      int linger = 0;
      parser->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   }  catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzParser: failed to start ZeroMQ consumer: " << e.what () << std::endl;
        exit(1);
   }

   ep = getZMQEndpoint(cfg, "fzdaq.fzparser.consumer");

   if(ep.empty()) {

      ep = "inproc://fzreader";
      std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [default]" << std::endl;

   } else std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [cfg file]" << std::endl;

   try {

      parser->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzParser: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   try {

      writer = new zmq::socket_t(context, ZMQ_PUSH);
      int linger = 0;
      writer->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   }  catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzParser: failed to start ZeroMQ producer: " << e.what () << std::endl;
        exit(1);
   }

   ep = getZMQEndpoint(cfg, "fzdaq.fzparser.producer");

   if(ep.empty()) {

      ep = "inproc://fzwriter";
      std::cout << INFOTAG << "FzParser: producer endpoint: " << ep << " [default]" << std::endl;

   } else std::cout << INFOTAG << "FzParser: producer endpoint: " << ep << " [cfg file]" << std::endl;

   try {

      writer->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzParser: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   thread_init = false;
   rcstate = IDLE;

   log.write(INFO, "state machine started");

   psr_report.Clear();
};

void FzParser::close(void) {
   thr->interrupt();
   thr->join();
}

void FzParser::init(void) {

   if(!thread_init) {
   
      thread_init = true;
      thr = new boost::thread(boost::bind(&FzParser::process, this));
   }
}

void FzParser::process(void) {

   bool rc = false;

   while(true) {

      try {    

         if(rcstate == RUNNING) {

            zmq::message_t message;
            std::string str;
     
            try {

               rc = parser->recv(&message);

            } catch (zmq::error_t &e) {
                 
                std::stringstream msg;
                msg << "error receiving from FzReader: " << e.what ();
                log.write(WARN, msg.str());
            }

            if(rc) {

               unsigned short int *bufusint = reinterpret_cast<unsigned short int*>(message.data());
               uint32_t bufsize = message.size() / 2;
               int retval;

               psr_report.set_in_bytes( psr_report.in_bytes() + message.size() );
               psr_report.set_in_events( psr_report.in_events() + 1 );
           
               sm.init(evformat);
               sm.import(bufusint, bufsize, &ev);
               retval = sm.process();

               // check if process from FSM is ok...
               if( (retval == PARSE_OK) && (sm.event_is_empty == false) ) {
   
                  DAQ::FzEventSet eventset;
                  DAQ::FzEvent *tmpev;

                  tmpev = eventset.add_ev();
                  tmpev->MergeFrom(ev);

                  bool retval;
                  retval = eventset.SerializeToString(&str);

                  if(retval == false) {
 
                     std::stringstream msg;
                     msg << "serialization error - EC: " << std::hex << ev.ec();
                     log.write(WARN, msg.str());
                  }

                  writer->send(str.data(), str.size());

                  psr_report.set_out_bytes( psr_report.out_bytes() + str.size() );
                  psr_report.set_out_events( psr_report.out_events() + 1 );

                  str.clear();
                  eventset.Clear();

               } else if (retval == PARSE_FAIL) {

                  std::stringstream msg;
                  msg << "serialization error - EC: " << std::hex << ev.ec();
                  log.write(WARN, msg.str());
               }

               ev.Clear();
            }

            boost::this_thread::interruption_point();

         } else {

              boost::this_thread::sleep(boost::posix_time::seconds(1));
              boost::this_thread::interruption_point();

         } 

      } catch(boost::thread_interrupted& interruption) {

         parser->close();
         writer->close();
         break;

      } catch(std::exception& e) {

      }	
   }	// end while
}

Report::FzParser FzParser::get_report(void) {
   return(psr_report);
}

Report::FzFSM FzParser::get_fsm_report(void) {
   return(sm.get_report());
}

void FzParser::set_rcstate(RCstate s) {

   rcstate = s;
}

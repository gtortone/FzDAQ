#include "FzParser.h"
#include "FzProtobuf.h"
#include "FzLogger.h"

FzParser::FzParser(unsigned int id, log4cpp::Priority::Value log_priority, std::string cfgfile, zmq::context_t &ctx) :
   context(ctx), logparser(log4cpp::Category::getInstance("fzparser" + id)) {

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;

   std::stringstream filename;

   filename << "logs/fzparser.log." << id;
   appender = new log4cpp::FileAppender("fzparser" + id, filename.str());
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   appender->setThreshold(log_priority);
   logparser.addAppender(appender);

   logparser.setAdditivity(false);

   logparser << INFO << "FzParser::constructor - success";

   sm.initlog(&logparser);
   sm.init();

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

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzparser.consumer");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzParser: consumer endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "inproc://fzreader";
      std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [default]" << std::endl;
   }

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

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzparser.producer");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzParser: producer endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzParser: producer endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "inproc://fzwriter";
      std::cout << INFOTAG << "FzParser: producer endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      writer->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzParser: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   thread_init = false;
   status = STOP;

   logparser << INFO << "FzParser - state machine started";

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

void FzParser::set_status(enum DAQstatus_t val) {

   status = val;
}

void FzParser::process(void) {

   while(true) {

      try {    

         if(status == START) {

            zmq::message_t message;
            std::string str;
            bool rc;
     
            rc = parser->recv(&message);

            if(rc) {

               unsigned short int *bufusint = reinterpret_cast<unsigned short int*>(message.data());
               uint32_t bufsize = message.size() / 2;

               psr_report.set_in_bytes( psr_report.in_bytes() + message.size() );
               psr_report.set_in_events( psr_report.in_events() + 1 );
           
               sm.init();
               sm.import(bufusint, bufsize, &ev);

               // check if process from FSM is ok...
               if( (sm.process() == PARSE_OK) && (sm.event_is_empty == false) ) {
   
                  bool retval;

                  retval = ev.SerializeToString(&str);

                  if(retval == false)
                     std::cout << "FzParser: serialization error - EC: " << ev.ec() << std::endl;

                  writer->send(str.data(), str.size());

                  str.clear();
               }

               psr_report.set_out_bytes( psr_report.out_bytes() + str.size() );
               psr_report.set_out_events( psr_report.out_events() + 1 );

               ev.Clear();
            }

            boost::this_thread::interruption_point();

         } else if (status == STOP) {

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

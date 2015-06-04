#include "FzNodeManager.h"

FzNodeManager::FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx) :
   context(ctx), rd(rd), psr_array(psr_array), wr(wr) {

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;
   
   profile = prof;
   thread_init = false;
   report_init = false;

   char buf[64];
   int retval = gethostname(buf, sizeof(buf));

   if(retval == -1) {

      std::cout << ERRTAG << "FzNodeManager: failed to get hostname" << std::endl;
      exit(1);
   }

   hostname = buf;
   std::cout << INFOTAG << "FzNodeManager: hostname is " << hostname << std::endl;

   std::string ep;
  
   // run control socket
 
   try {

      subrc = new zmq::socket_t(context, ZMQ_SUB);
      int linger = 0;
      subrc->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to start ZeroMQ run control subscriber: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fznodemanager.runcontrol");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzNodeManager: run control endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzNodeManager: run control subscriber endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://eth0:6000";
      std::cout << INFOTAG << "FzNodeManager: run control subscriber endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      subrc->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // report monitoring socket

   try {

      pushmon = new zmq::socket_t(context, ZMQ_PUSH);
      int tval = 1000;
      pushmon->setsockopt(ZMQ_SNDTIMEO, &tval, sizeof(tval));		// 1 second timeout on send
      int linger = 0;
      pushmon->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to start ZeroMQ report push: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fznodemanager.stats");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzNodeManager: report endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzNodeManager: report push endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://eth0:7000";
      std::cout << INFOTAG << "FzNodeManager: report push endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      //pushmon->bind(ep.c_str());
      pushmon->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        //std::cout << ERRTAG << "FzNodeManager: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        std::cout << ERRTAG << "FzNodeManager: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }
};

void FzNodeManager::close(void) {

   thr->interrupt();
   thr->join();
}

void FzNodeManager::init(void) {

   if (!thread_init) {

      thread_init = true;
      thr = new boost::thread(boost::bind(&FzNodeManager::process, this));
   }
};

void FzNodeManager::process(void) { 

   unsigned int i;
   unsigned int nparsers = psr_array.size();
   Report::FzReader rd_report_t0, rd_report_t1;
   Report::FzParser psr_report_t0[nparsers], psr_report_t1[nparsers];
   Report::FzFSM fsm_report_t0[nparsers], fsm_report_t1[nparsers];
   Report::FzWriter wr_report_t0, wr_report_t1;

   // performance evaluation
   //struct timeval tv_start, tv_stop;
   //

   while(true) {

      try {

         boost::this_thread::interruption_point();

         if(status == START) {
 
            // performance evaluation
            //gettimeofday(&tv_start, NULL);         
            //

            nodereport.set_hostname(hostname);
            nodereport.set_profile(profile);
            nodereport.set_parser_num(nparsers);

            if( (profile == "compute") || (profile == "all") ) {

               rd_report_t0.CopyFrom( rd->get_report() );

               for(i=0; i<nparsers; i++) {
                  psr_report_t0[i].CopyFrom( psr_array[i]->get_report() );
                  fsm_report_t0[i].CopyFrom( psr_array[i]->get_fsm_report() );
               }
            }

            if( (profile == "storage") || (profile == "all") ) {

               wr_report_t0.CopyFrom( wr->get_report() );
            }

            boost::this_thread::sleep(boost::posix_time::seconds(1));

            if( (profile == "compute") || (profile == "all") ) {

               rd_report_t1.CopyFrom( rd->get_report() );
   
               nodereport.mutable_reader()->set_consumer_ip( rd_report_t1.consumer_ip() );
               nodereport.mutable_reader()->set_in_bytes( rd_report_t1.in_bytes() );
               nodereport.mutable_reader()->set_in_bytes_bw( rd_report_t1.in_bytes() - rd_report_t0.in_bytes() );
               nodereport.mutable_reader()->set_in_events( rd_report_t1.in_events() );
               nodereport.mutable_reader()->set_in_events_bw( rd_report_t1.in_events() - rd_report_t0.in_events() );
   
               nodereport.clear_parser();
               nodereport.clear_fsm();
   
               for(i=0; i<nparsers; i++) {
 
                 psr_report_t1[i].CopyFrom( psr_array[i]->get_report() );
                 fsm_report_t1[i].CopyFrom( psr_array[i]->get_fsm_report() );

                 nodereport.add_parser(); 
                 nodereport.add_fsm(); 

                 nodereport.mutable_parser(i)->set_in_bytes( psr_report_t1[i].in_bytes() ); 
                 nodereport.mutable_parser(i)->set_in_bytes_bw( psr_report_t1[i].in_bytes() - psr_report_t0[i].in_bytes() ); 
                 nodereport.mutable_parser(i)->set_out_bytes( psr_report_t1[i].out_bytes());
                 nodereport.mutable_parser(i)->set_out_bytes_bw( psr_report_t1[i].out_bytes() - psr_report_t0[i].out_bytes() );
                 nodereport.mutable_parser(i)->set_in_events( psr_report_t1[i].in_events() );
                 nodereport.mutable_parser(i)->set_in_events_bw( psr_report_t1[i].in_events() - psr_report_t0[i].in_events() );
                 nodereport.mutable_parser(i)->set_out_events( psr_report_t1[i].out_events() );
                 nodereport.mutable_parser(i)->set_out_events_bw( psr_report_t1[i].out_events() - psr_report_t0[i].out_events() );

                 nodereport.mutable_fsm(i)->set_in_bytes( fsm_report_t1[i].in_bytes() );
                 nodereport.mutable_fsm(i)->set_in_bytes_bw( fsm_report_t1[i].in_bytes() - fsm_report_t0[i].in_bytes() );
                 nodereport.mutable_fsm(i)->set_out_bytes( fsm_report_t1[i].out_bytes());
                 nodereport.mutable_fsm(i)->set_out_bytes_bw( fsm_report_t1[i].out_bytes() - fsm_report_t0[i].out_bytes() );
                 nodereport.mutable_fsm(i)->set_in_events( fsm_report_t1[i].in_events() );
                 nodereport.mutable_fsm(i)->set_in_events_bw( fsm_report_t1[i].in_events() - fsm_report_t0[i].in_events() );
                 nodereport.mutable_fsm(i)->set_out_events( fsm_report_t1[i].out_events() );
                 nodereport.mutable_fsm(i)->set_out_events_bw( fsm_report_t1[i].out_events() - fsm_report_t0[i].out_events() );
                 nodereport.mutable_fsm(i)->set_state_invalid( fsm_report_t1[i].state_invalid() );
                 nodereport.mutable_fsm(i)->set_events_empty( fsm_report_t1[i].events_empty() );
                 nodereport.mutable_fsm(i)->set_events_good( fsm_report_t1[i].events_good() );
                 nodereport.mutable_fsm(i)->set_events_bad( fsm_report_t1[i].events_bad() );
               }
            }

            if( (profile == "storage") || (profile == "all" ) ) {
    
               wr_report_t1.CopyFrom( wr->get_report() );
   
               nodereport.mutable_writer()->set_in_bytes( wr_report_t1.in_bytes() );
               nodereport.mutable_writer()->set_in_bytes_bw( wr_report_t1.in_bytes() - wr_report_t0.in_bytes() );
               nodereport.mutable_writer()->set_in_events( wr_report_t1.in_events() );
               nodereport.mutable_writer()->set_in_events_bw( wr_report_t1.in_events() - wr_report_t0.in_events() );
               nodereport.mutable_writer()->set_out_bytes( wr_report_t1.out_bytes() );
               nodereport.mutable_writer()->set_out_bytes_bw( wr_report_t1.out_bytes() - wr_report_t0.out_bytes() );
               nodereport.mutable_writer()->set_out_events( wr_report_t1.out_events() );
               nodereport.mutable_writer()->set_out_events_bw( wr_report_t1.out_events() - wr_report_t0.out_events() );
            }

            // send report to FzController
            std::string str;
            nodereport.SerializeToString(&str);

            pushmon->send(str.data(), str.size());
            str.clear();
 
            report_init = true;
   
   /*
            // performance evaluation
            gettimeofday(&tv_stop, NULL);
 
            uint32_t diff_sec = tv_stop.tv_sec - tv_start.tv_sec;
            uint32_t diff_usec = tv_stop.tv_usec - tv_start.tv_usec;
            double telaps = diff_sec + (diff_usec / 1000000L);
            std::cout << "FzNodeManager measurement loop time: " << telaps << " s" << std::endl;
   */
         } 

      } catch(boost::thread_interrupted& interruption) {

         subrc->close();
         pushmon->close();
         break;

      } catch(std::exception& e) {

      }

   }	// end while
};

void FzNodeManager::set_status(enum DAQstatus_t val) {

   status = val;
}

Report::Node FzNodeManager::get_nodereport(void) {
   return(nodereport);
}

bool FzNodeManager::has_data(void) {
   return(report_init);
}

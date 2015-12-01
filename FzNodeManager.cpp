#include "FzNodeManager.h"
#include "FzUtils.h"

FzNodeManager::FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx, cms::Connection *JMSconn) :
   context(ctx), rd(rd), psr_array(psr_array), wr(wr), AMQconn(JMSconn) {

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;
   
   if(AMQconn.get())
      log.setJMSConnection("FzNodeManager", JMSconn);

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

   // node report endpoint setup

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

      pushmon->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // run control socket

   try {

      rcontrol = new zmq::socket_t(context, ZMQ_REP);
      int tval = 500;
      rcontrol->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));           // 500 milliseconds timeout on recv
      int linger = 0;
      rcontrol->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to start ZeroMQ run control reply: " << e.what () << std::endl;
        exit(1);
   }

   // run control endpoint setup

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fznodemanager.runcontrol");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzNodeManager: run control endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzNodeManager: run control request/reply endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://*:5550";
      std::cout << INFOTAG << "FzNodeManager: run control request/reply endpoint: " << ep << " [default]" << std::endl;
   }

   try {
  
      rcontrol->bind(ep.c_str());

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzNodeManager: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
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
      ioc = new boost::thread(boost::bind(&FzNodeManager::epics_ioc, this)); 
   }
};

void FzNodeManager::process(void) { 

   unsigned int i;
   unsigned int nparsers = psr_array.size();
   Report::FzReader rd_report_t0, rd_report_t1;
   Report::FzParser psr_report_t0[nparsers], psr_report_t1[nparsers];
   Report::FzFSM fsm_report_t0[nparsers], fsm_report_t1[nparsers];
   Report::FzWriter wr_report_t0, wr_report_t1;
   zmq::message_t rcreq;
   int retval;

   while(true) {

      try {

         boost::this_thread::interruption_point();

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
            nodereport.mutable_reader()->set_out_bytes( rd_report_t1.out_bytes() );
            nodereport.mutable_reader()->set_out_bytes_bw( rd_report_t1.out_bytes() - rd_report_t0.out_bytes() );
            nodereport.mutable_reader()->set_out_events( rd_report_t1.out_events() );
            nodereport.mutable_reader()->set_out_events_bw( rd_report_t1.out_events() - rd_report_t0.out_events() );
   
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

         // update EPICS IOC
         update_stats_ioc();

         // send report to FzController
         std::string str;
         nodereport.SerializeToString(&str);

         pushmon->send(str.data(), str.size());
         str.clear();
 
         report_init = true;

         // receive run control messages
         retval = rcontrol->recv(&rcreq);
         std::string request = std::string(static_cast<char*>(rcreq.data()), rcreq.size());

         if(retval) {	// request received...

            std::stringstream msg;
            msg << "RC request received: " << request;
            logmtx.lock(); 
               log.write(INFO, msg.str());
            logmtx.unlock();

            RCcommand cmd = reset;

            if(request == "configure") {

               cmd = configure;

            } else if(request == "start") {
 
               cmd = start;

            } else if(request == "stop") {

               cmd = stop;

            } else if(request == "reset") {

               cmd = reset;
            }

            rc_process(cmd);

            // create response
            std::string reply = std::string("OK");
            zmq::message_t rcrep(reply.size());   

            memcpy (rcrep.data(), reply.data(), reply.size());
            rcontrol->send(rcrep);
         }

      } catch(boost::thread_interrupted& interruption) {

         rcontrol->close();
         pushmon->close();
         break;

      } catch(std::exception& e) {

      }
   }	// end while
};

void FzNodeManager::epics_ioc(void) {

   const char *base_dbd = DBD_FILE;
   char *dbd_file = const_cast<char*>(base_dbd);
   char xmacro[PVNAME_STRINGSZ + 4];
   
   std::string rc_status_pv;	

   if (dbLoadDatabase(dbd_file, NULL, NULL)) {
      std::cout << ERRTAG << "FzNodeManager: failed to load FAZIA dbd file" << std::endl;
      exit(1);
   }

   softIoc_registerRecordDeviceDriver(pdbbase);
   epicsSnprintf(xmacro, sizeof xmacro, "HOSTNAME=%s", hostname.c_str());
 
   if (dbLoadRecords(DB_FILE, xmacro)) {
      std::cout << ERRTAG << "FzNodeManager: failed to load NodeManager db file" << std::endl;
      exit(1);
   }

   iocInit();
   epicsThreadSleep(0.2);

   if(ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL) {

      std::cout << ERRTAG << "FzNodeManager: failed to create EPICS CA context" << std::endl;
      exit(1);
   }

   // update DAQ node profile PV
   PVwrite_db(hostname + ":profile", profile);

   while(true) {

      epicsThreadSleep(0.5);
   }
}

Report::Node FzNodeManager::get_nodereport(void) {
   return(nodereport);
}

bool FzNodeManager::has_data(void) {
   return(report_init);
}

void FzNodeManager::rc_do(RCcommand cmd) {

   if( (profile == "compute") || (profile == "all") ) {

      rd->rc_do(cmd);
      rd->set_rcstate(rc.state());

      for(unsigned int i=0; i<psr_array.size(); i++) {
         psr_array[i]->rc_do(cmd);
         psr_array[i]->set_rcstate(rc.state());
      }
   }

   if( (profile == "storage") || (profile == "all") ) {

      wr->rc_do(cmd);
      wr->set_rcstate(rc.state());
   }
}

void FzNodeManager::update_rc_ioc(void) {

   PVwrite_db(hostname + ":RC:transition.DISP", 0);
   PVwrite_db(hostname + ":RC:transition", rc.error());
   PVwrite_db(hostname + ":RC:transition.DISP", 1);

   PVwrite_db(hostname + ":RC:state.DISP", 0);
   PVwrite_db(hostname + ":RC:state", rc.state());
   PVwrite_db(hostname + ":RC:state.DISP", 1);  
}

void FzNodeManager::update_stats_ioc(void) {

   if( (profile == "compute") || (profile == "all") ) {

      Report::FzReader rd_report = nodereport.reader();
      Report::FzParser psr_report;
      Report::FzFSM fsm_report;

      double value;
      std::string unit;

      double in_data, in_data_bw, in_ev, in_ev_bw;
      double out_data, out_data_bw, out_ev, out_ev_bw;
      double ev_good, ev_bad, ev_empty, st_invalid;

      // FzReader data statistics

      human_byte(rd_report.in_bytes(), &value, &unit);
      PVwrite_db(hostname + ":reader:in:data", value);
      PVwrite_db(hostname + ":reader:in:data.EGU", unit);

      PVwrite_db(hostname + ":reader:in:databw", rd_report.in_bytes_bw() * 8E-6);	// Mbit/s
 
      PVwrite_db(hostname + ":reader:in:ev", (double)rd_report.in_events());

      human_byte(rd_report.in_events_bw(), &value);
      PVwrite_db(hostname + ":reader:in:evbw", value);

      human_byte(rd_report.out_bytes(), &value, &unit);
      PVwrite_db(hostname + ":reader:out:data", value);
      PVwrite_db(hostname + ":reader:out:data.EGU", unit);

      PVwrite_db(hostname + ":reader:out:databw", rd_report.out_bytes_bw() * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":reader:out:ev", (double)rd_report.out_events());

      human_byte(rd_report.out_events_bw(), &value);
      PVwrite_db(hostname + ":reader:out:evbw", value);

      // FzParser pool data statistics

      in_data = in_data_bw = in_ev = in_ev_bw = 0;
      out_data = out_data_bw = out_ev = out_ev_bw = 0;

      for(unsigned int i=0; i<nodereport.parser_num(); i++) {

         psr_report = nodereport.parser(i);
 
         in_data += psr_report.in_bytes();
         in_data_bw += psr_report.in_bytes_bw();
         in_ev += psr_report.in_events();
         in_ev_bw += psr_report.in_events_bw();

         out_data += psr_report.out_bytes();
         out_data_bw += psr_report.out_bytes_bw();
         out_ev += psr_report.out_events();
         out_ev_bw += psr_report.out_events_bw();
      }

      human_byte(in_data, &value, &unit);
      PVwrite_db(hostname + ":parser:in:data", value);
      PVwrite_db(hostname + ":parser:in:data.EGU", unit);

      PVwrite_db(hostname + ":parser:in:databw", in_data_bw * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":parser:in:ev", (double)in_ev);

      human_byte(in_ev_bw, &value);
      PVwrite_db(hostname + ":parser:in:evbw", value);

      human_byte(out_data, &value, &unit);
      PVwrite_db(hostname + ":parser:out:data", value);
      PVwrite_db(hostname + ":parser:out:data.EGU", unit);

      PVwrite_db(hostname + ":parser:out:databw", out_data_bw * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":parser:out:ev", (double)out_ev);

      human_byte(out_ev_bw, &value);
      PVwrite_db(hostname + ":parser:out:evbw", value);

      // FzFSM pool data statistics
      
      in_data = in_data_bw = in_ev = in_ev_bw = 0;
      out_data = out_data_bw = out_ev = out_ev_bw = 0;
      ev_good = ev_bad = ev_empty = st_invalid = 0;

      for(unsigned int i=0; i<nodereport.parser_num(); i++) {
 
         fsm_report = nodereport.fsm(i);

         in_data += fsm_report.in_bytes();
         in_data_bw += fsm_report.in_bytes_bw();
         in_ev += fsm_report.in_events();
         in_ev_bw += fsm_report.in_events_bw();

         out_data += fsm_report.out_bytes();
         out_data_bw += fsm_report.out_bytes_bw();
         out_ev += fsm_report.out_events();
         out_ev_bw += fsm_report.out_events_bw();

         ev_good += fsm_report.events_good();
         ev_bad += fsm_report.events_bad();
         ev_empty += fsm_report.events_empty();
         st_invalid += fsm_report.state_invalid(); 
      }
      
      human_byte(in_data, &value, &unit);
      PVwrite_db(hostname + ":fsm:in:data", value);
      PVwrite_db(hostname + ":fsm:in:data.EGU", unit);

      PVwrite_db(hostname + ":fsm:in:databw", in_data_bw * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":fsm:in:ev", (double)in_ev);

      human_byte(in_ev_bw, &value);
      PVwrite_db(hostname + ":fsm:in:evbw", value);

      human_byte(out_data, &value, &unit);
      PVwrite_db(hostname + ":fsm:out:data", value);
      PVwrite_db(hostname + ":fsm:out:data.EGU", unit);

      PVwrite_db(hostname + ":fsm:out:databw", out_data_bw * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":fsm:out:ev", (double)out_ev);

      human_byte(out_ev_bw, &value);
      PVwrite_db(hostname + ":fsm:out:evbw", value);
 
      PVwrite_db(hostname + ":fsm:events:good", ev_good); 
      PVwrite_db(hostname + ":fsm:events:bad", ev_bad); 
      PVwrite_db(hostname + ":fsm:events:empty", ev_empty); 
      PVwrite_db(hostname + ":fsm:states:invalid", st_invalid); 
   }

   if( (profile == "storage") || (profile == "all") ) {

      Report::FzWriter wr_report = nodereport.writer();

      double value;
      std::string unit;

      // FzWriter data statistics

      human_byte(wr_report.in_bytes(), &value, &unit);
      PVwrite_db(hostname + ":writer:in:data", value);
      PVwrite_db(hostname + ":writer:in:data.EGU", unit);

      PVwrite_db(hostname + ":writer:in:databw", wr_report.in_bytes_bw() * 8E-6);	// Mbit/s

      PVwrite_db(hostname + ":writer:in:ev", (double)wr_report.in_events());

      human_byte(wr_report.in_events_bw(), &value);
      PVwrite_db(hostname + ":writer:in:evbw", value);

      human_byte(wr_report.out_bytes(), &value, &unit);
      PVwrite_db(hostname + ":writer:out:data", value);
      PVwrite_db(hostname + ":writer:out:data.EGU", unit);

      PVwrite_db(hostname + ":writer:out:databw", wr_report.out_bytes_bw() * 1E-6);	// Mbyte/s;

      PVwrite_db(hostname + ":writer:out:ev", (double)wr_report.out_events());

      human_byte(wr_report.out_events_bw(), &value);
      PVwrite_db(hostname + ":writer:out:evbw", value);      
   }
}

//
// wrapper methods for RC FSM
//
RCtransition FzNodeManager::rc_process(RCcommand cmd) {

   RCstate prev_state = rc.state();
   RCtransition trans_err = rc.process(cmd);
   RCstate cur_state = rc.state();

   if( (cmd == reset) || ((trans_err == RCOK) && (cur_state != prev_state)) ) {

      // a state transition occurred...
      std::stringstream msg;
      msg << "local RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
      logmtx.lock();
         log.write(INFO, msg.str());
      logmtx.unlock();
      rc_do(cmd);
   }

   update_rc_ioc();

   return(rc.error());
}

RCstate FzNodeManager::rc_state(void) {

   return(rc.state());
}

RCtransition FzNodeManager::rc_error(void) {

   return(rc.error());
}

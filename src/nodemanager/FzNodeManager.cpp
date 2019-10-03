#include "proto/FzRCS.pb.h"
#include "utils/FzUtils.h"
#include "FzNodeManager.h"

#include <boost/algorithm/string.hpp>

#ifdef AMQLOG_ENABLED
FzNodeManager::FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx, cms::Connection *JMSconn) :
   context(ctx), rd(rd), psr_array(psr_array), wr(wr), AMQconn(JMSconn) {
#else
FzNodeManager::FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx) :
   context(ctx), rd(rd), psr_array(psr_array), wr(wr) {
#endif

   cfg.readFile(cfgfile.c_str());    // syntax checks on main

   log.setFileConnection("fznodemanager", "/var/log/fzdaq/fznodemanager.log");

#ifdef AMQLOG_ENABLED
   if(AMQconn.get())
      log.setJMSConnection("FzNodeManager", JMSconn);
#endif

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

   std::string ep, netint;
  
   if(cfg.lookupValue("fzdaq.fznodemanager.runcontrol_mode", rcmode)) {

      if( (rcmode != std::string(RC_MODE_LOCAL)) && (rcmode != std::string(RC_MODE_REMOTE)) ) {

         rcmode = std::string(RC_MODE_LOCAL);
      }

   } else {

      rcmode = std::string(RC_MODE_LOCAL);
   }

   if (rcmode == std::string(RC_MODE_REMOTE))
      state = IDLE;

   std::cout << INFOTAG << "FzNodemanager: Run Control mode set to: " << rcmode << std::endl;

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

   // FzController collector IP setup

   std::string fzc_ip;
   cfg.lookupValue("fzdaq.fznodemanager.stats.ip", fzc_ip);

   if(fzc_ip.empty()) {

         std::cout << ERRTAG << "FzNodeManager: FzController collector IP not present in config file" << std::endl;
         exit(1);
   }

   std::cout << INFOTAG << "FzNodeManager: FzController collector IP: " << fzc_ip << " [cfg file]" << std::endl;

   ep = "tcp://" + fzc_ip + ":" + std::to_string(FZC_COLLECTOR_PORT);

   try {

      pushmon->connect(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // run control socket (REQ/REP)

   try {

      rcontrol = new zmq::socket_t(context, ZMQ_REP);
      int linger = 0;
      rcontrol->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to start ZeroMQ run control reply: " << e.what () << std::endl;
        exit(1);
   }

   // run control endpoint setup (REQ/REP)

   if(cfg.lookupValue("fzdaq.fznodemanager.interface", netint)) {

      std::cout << INFOTAG << "FzNodeManager: network interface: " << netint << " [cfg file]" << std::endl;
      ep = "tcp://" + netint + ":" + std::to_string(FZNM_REP_PORT);
      std::cout << INFOTAG << "FzNodeManager: run control req/rep endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      // default setting
      ep = "tcp://eth0:" + std::to_string(FZNM_REP_PORT);
      std::cout << INFOTAG << "FzNodeManager: run control req/rep endpoint: " << ep << " [default]" << std::endl;
   }

   try {
  
      rcontrol->bind(ep.c_str());

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzNodeManager: failed to bind ZeroMQ req/rep endpoint " << ep << ": " << e.what () << std::endl;
      exit(1);
   }
   
   // run control socket (PULL)
   
   try {

      pcontrol = new zmq::socket_t(context, ZMQ_PULL);
      int linger = 0;
      pcontrol->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzNodeManager: failed to start ZeroMQ run control pull: " << e.what () << std::endl;
        exit(1);
   }

   // run control endpoint setup (PULL)

   if(cfg.lookupValue("fzdaq.fznodemanager.interface", netint)) {

      std::cout << INFOTAG << "FzNodeManager: network interface: " << netint << " [cfg file]" << std::endl;
      ep = "tcp://" + netint + ":" + std::to_string(FZNM_PULL_PORT);
      std::cout << INFOTAG << "FzNodeManager: run control pull endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      // default setting
      ep = "tcp://eth0:" + std::to_string(FZNM_PULL_PORT);
      std::cout << INFOTAG << "FzNodeManager: run control pull endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      pcontrol->bind(ep.c_str());

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzNodeManager: failed to bind ZeroMQ pull endpoint " << ep << ": " << e.what () << std::endl;
      exit(1);
   }
};

void FzNodeManager::close(void) {

   thr_comm->interrupt();
   thr_comm->join();
   thr_perfdata->interrupt();
   thr_perfdata->join();
}

void FzNodeManager::init(void) {

   if (!thread_init) {

      thread_init = true;
      thr_comm = new boost::thread(boost::bind(&FzNodeManager::process_comm, this));
      thr_perfdata = new boost::thread(boost::bind(&FzNodeManager::process_perfdata, this));
   }
};

void FzNodeManager::process_comm(void) {
   
   zmq::message_t req;

   //  Initialize poll set
   zmq::pollitem_t items[2];

   items[0].socket = (*rcontrol);
   items[0].fd = 0;
   items[0].events = ZMQ_POLLIN;
   items[1].socket = (*pcontrol);
   items[1].fd = 0;
   items[1].events = ZMQ_POLLIN;

   while(true) {

      try {

         boost::this_thread::interruption_point();
	 
   	 // process messages from both sockets
	 zmq::poll(items, 2, 100);

	 if(items[0].revents & ZMQ_POLLIN) {
	    
            // receive run control & setup messages (REQ/REP socket)
	    rcontrol->recv(&req);

            zmq::message_t res;
            res = handle_request(req);

            // a reply is mandatory in every case...    
            rcontrol->send(res);
	    items[0].revents = 0;
	 }

	 if(items[1].revents & ZMQ_POLLIN) {
	    
	    // receive run control & setup messages (PULL socket)
	    pcontrol->recv(&req);

            handle_request(req);
	    items[1].revents = 0;
	 }

      } catch(boost::thread_interrupted& interruption) {

         rcontrol->close();
         pcontrol->close();
         break;

      } catch(std::exception& e) {

      }
   }  // end while
}

void FzNodeManager::process_perfdata(void) { 

   unsigned int i;
   unsigned int nparsers = psr_array.size();
   Report::FzReader rd_report_t0, rd_report_t1;
   Report::FzParser psr_report_t0[nparsers], psr_report_t1[nparsers];
   Report::FzFSM fsm_report_t0[nparsers], fsm_report_t1[nparsers];
   Report::FzWriter wr_report_t0, wr_report_t1;

   while(true) {

      try {

         boost::this_thread::interruption_point();

         nodereport.set_hostname(hostname);
         nodereport.set_profile(profile);
         nodereport.set_status(state_labels[rc_state()]);
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
            nodereport.mutable_writer()->set_out_events( wr_report_t1.out_events() );

            if(wr->get_store()) {

               nodereport.mutable_writer()->set_out_bytes_bw( wr_report_t1.out_bytes() - wr_report_t0.out_bytes() );
               nodereport.mutable_writer()->set_out_events_bw( wr_report_t1.out_events() - wr_report_t0.out_events() );

            } else {

               nodereport.mutable_writer()->set_out_bytes_bw(0);
               nodereport.mutable_writer()->set_out_events_bw(0);
            }
         }

         // send report to FzController
         std::string str;
         nodereport.SerializeToString(&str);

         pushmon->send(str.data(), str.size());
         str.clear();
 
         report_init = true;

      } catch(boost::thread_interrupted& interruption) {

         rcontrol->close();
         pushmon->close();
         break;

      } catch(std::exception& e) {

      }
   }	// end while
};

zmq::message_t FzNodeManager::handle_request(zmq::message_t& request) {
   
   RCS::Request req;
   RCS::Response res;
   std::string str;

   res.set_errorcode(RCS::Response::ERR);
   res.set_reason("request format error");

   if(req.ParseFromArray(request.data(), request.size()) == false) {

      res.set_errorcode(RCS::Response::ERR);
      res.set_reason("request deserialization error");
      
      std::string str;
      res.SerializeToString(&str);

      zmq::message_t msg(str.size());
      memcpy(msg.data(), str.data(), str.size());

      return(msg);
   } 

   if(req.module() != RCS::Request::NM) {

      res.set_errorcode(RCS::Response::ERR);
      res.set_reason("module destination error");
      
      std::string str;
      res.SerializeToString(&str);

      zmq::message_t msg(str.size());
      memcpy(msg.data(), str.data(), str.size());

      return(msg);
   }

   if(req.channel() == RCS::Request::RC) {		// RUN CONTROL

      if(req.operation() == RCS::Request::READ) {

         if(req.variable() == "status") {

            res.set_value(state_labels[rc_state()]);
            res.set_errorcode(RCS::Response::OK);
            res.set_reason("OK");

         } else if(req.variable() == "mode") {

            res.set_value(rcmode);
            res.set_errorcode(RCS::Response::OK);
            res.set_reason("OK");

	 } else if(req.variable() == "perfdata") {

	    std::string str;
	    nodereport.SerializeToString(&str);

	    zmq::message_t msg(str.size());
            memcpy(msg.data(), str.data(), str.size());

	    return(msg);

         } else res.set_reason("variable not found");

      } else if(req.operation() == RCS::Request::WRITE) {

           if(rcmode == std::string(RC_MODE_REMOTE)) {

              if(req.variable() == "status") {

                 if(req.value() == "idle") {
                 
                    state = IDLE;
                    rc_state_do(state);
                    res.set_errorcode(RCS::Response::OK);
                    res.set_reason("OK");

                 } else if(req.value() == "ready") {

                    state = READY;
                    rc_state_do(state);
                    res.set_errorcode(RCS::Response::OK);
                    res.set_reason("OK");

                 } else if(req.value() == "running") {

                    state = RUNNING;
                    rc_state_do(state);
                    res.set_errorcode(RCS::Response::OK);
                    res.set_reason("OK");

                 } else if(req.value() == "paused") {

                    state = PAUSED;
                    rc_state_do(state);
                    res.set_errorcode(RCS::Response::OK);
                    res.set_reason("OK");

                 } else res.set_reason("status value not correct");

              } else res.set_reason("variable not found");

           } else {		// RC_MODE_LOCAL

              res.set_errorcode(RCS::Response::DENY);
              res.set_reason("FzDAQ Run Control set to local mode"); 
           }
      }

   } else if(req.channel() == RCS::Request::SETUP) {	// SETUP

      if(req.operation() == RCS::Request::READ) {

      } else if(req.operation() == RCS::Request::WRITE) {
      
         if( (profile == "storage") || (profile == "all") ) {

            if(req.variable() == "store") {

                  wr->set_store(true);
                  res.set_errorcode(RCS::Response::OK);
                  res.set_reason("OK");

            } else if(req.variable() == "nostore") {

                  wr->set_store(false);
                  res.set_errorcode(RCS::Response::OK);
                  res.set_reason("OK");

            }
         }
      }
   }

   res.SerializeToString(&str);
   zmq::message_t msg(str.size());
   memcpy(msg.data(), str.data(), str.size());

   return(msg);
}

Report::Node FzNodeManager::get_nodereport(void) {
   return(nodereport);
}

bool FzNodeManager::has_data(void) {
   return(report_init);
}

void FzNodeManager::rc_event_do(RCcommand cmd) {

   if( (profile == "compute") || (profile == "all") ) {

      if(rcmode == std::string(RC_MODE_LOCAL))
         rd->set_rcstate(rc.state());

      for(unsigned int i=0; i<psr_array.size(); i++) {

         if(rcmode == std::string(RC_MODE_LOCAL))
            psr_array[i]->set_rcstate(rc.state());
      }
   }

   if( (profile == "storage") || (profile == "all") ) {

      if(rcmode == std::string(RC_MODE_LOCAL))
         wr->set_rcstate(rc.state());
   }
}

void FzNodeManager::rc_state_do(RCstate state) {

   if( (profile == "compute") || (profile == "all") ) {

      rd->set_rcstate(state);

      for(unsigned int i=0; i<psr_array.size(); i++) 
         psr_array[i]->set_rcstate(state);
   }

   if( (profile == "storage") || (profile == "all") ) 
      wr->set_rcstate(state);
}

//
// wrapper methods for RC FSM
//
RCtransition FzNodeManager::rc_process(RCcommand cmd) {

   RCstate prev_state = rc.state();
   RCtransition trans_err = rc.process(cmd);
   RCstate cur_state = rc.state();

   if( (cmd == RCcommand::reset) || ((trans_err == RCOK) && (cur_state != prev_state)) ) {

      // a state transition occurred...
      std::stringstream msg;
      msg << "local RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
      logmtx.lock();
         log.write(INFO, msg.str());
      logmtx.unlock();
      rc_event_do(cmd);
   }

   return(rc.error());
}

RCstate FzNodeManager::rc_state(void) {

   if(rcmode == std::string(RC_MODE_REMOTE))
      return(state);
   else
      return(rc.state());
}

std::string FzNodeManager::rc_mode(void) {
   return(rcmode);
}

RCtransition FzNodeManager::rc_error(void) {

   return(rc.error());
}

#include "FzController.h"

// local functions
static void prepare_rcwrite(RCS::Request& req);
static void print_status_table(Report::NodeSet r);
static void print_header(void);
static void print_reader_status(Report::FzReader r);
static void print_parser_status(Report::FzParser r);
static void print_fsm_status(Report::FzFSM r);
static void print_writer_status(Report::FzWriter r);

void FzController::start_cli(void) {

   thrcli = new boost::thread(&FzController::process_cli, this);
}

void FzController::process_cli(void) {

   std::cout << BOLDMAGENTA << "FzController - START command line interface " << RESET << std::endl;
   std::cout << std::endl;

   std::string line, lastcmd;
   bool cmderr;
   RCS::Request req;
   RCS::Response res;
   Report::NodeSet report;
   zmq::message_t msg_req, msg_res;

   char buf[256];
   std::string hostname;

   gethostname(buf, sizeof(buf));
   hostname = buf;

   try {

      for ( ; std::cout << "FzController > " && std::getline(std::cin, line); ) {

         if (!line.empty()) {

            cmderr = true;

            if(!line.compare("r")) {

               cmderr = false;
               line = lastcmd;
            }

            if(!line.compare("help")) {

               cmderr = false;
               std::cout << std::endl;
               std::cout << "configure:\tconfigure acquisition" << std::endl;
               std::cout << "start:\t\tconfigure and start acquisition" << std::endl;
               std::cout << "stop:\t\tstop acquisition" << std::endl;
               std::cout << "reset:\t\treset acquisition" << std::endl;
               std::cout << "stats:\t\tprint acquisition statistics" << std::endl;
               std::cout << "status:\t\tprint acquisition status" << std::endl;
               std::cout << "help:\t\tprint usage" << std::endl;
               std::cout << "quit:\t\tquit from FzController" << std::endl;
               std::cout << std::endl;
            }

            if(!line.compare("configure")) {

               cmderr = false;
               prepare_rcwrite(req);
	       req.set_hostname(hostname);

               req.set_value("configure");
               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {		

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK)       
                     std::cout << INFOTAG << "Fazia DAQ configured" << std::endl;
                  else
                     std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of configure RC command failed" << std::endl;
            }

            if(!line.compare("start")) {

               cmderr = false;
               prepare_rcwrite(req);
	       req.set_hostname(hostname);

               req.set_value("configure");
               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK) {				// send start

                     req.set_value("start");
                     msg_req = to_zmqmsg(req);

                     if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

                        res = to_rcsres(msg_res);

                        if(res.errorcode() == RCS::Response::OK)
                           std::cout << INFOTAG << "Fazia DAQ configured and started" << std::endl;
                        else
                           std::cout << ERRTAG << res.reason() << std::endl;

                     } else std::cout << ERRTAG << "send of start RC command failed" << std::endl;

                  } else std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of configure RC command failed" << std::endl;
            }

            if(!line.compare("stop")) {

               cmderr = false;
               prepare_rcwrite(req);
	       req.set_hostname(hostname);

               req.set_value("stop");
               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {		

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK)       
                     std::cout << INFOTAG << "Fazia DAQ stopped" << std::endl;
                  else
                     std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of stop RC command failed" << std::endl;
            }

            if(!line.compare("reset")) {

               cmderr = false;
               prepare_rcwrite(req);
	       req.set_hostname(hostname);

               req.set_value("reset");
               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK)
                     std::cout << INFOTAG << "Fazia DAQ reset" << std::endl;
                  else
                     std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of reset RC command failed" << std::endl;
            }

            if(!line.compare("status")) {
 
               cmderr = false;
               std::string daqstatus;

               std::cout << std::endl;

               req.set_channel(RCS::Request::RC);
               req.set_operation(RCS::Request::READ);
               req.set_module(RCS::Request::CNT);
               req.set_hostname(hostname);
               req.set_variable("status");

               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK) {

                     daqstatus = res.value();
                     std::cout << INFOTAG << "Fazia DAQ Controller status: " << res.value() << std::endl;

                  } else std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of reset RC command failed" << std::endl;

               req.Clear();

               std::cout << std::endl;

               req.set_channel(RCS::Request::RC);
               req.set_operation(RCS::Request::READ);
               req.set_module(RCS::Request::CNT);
               req.set_hostname(hostname);
               req.set_variable("nodestatus");

               msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

                  res = to_rcsres(msg_res);

                  if(res.errorcode() == RCS::Response::OK) {

                     for(int i=0; i < res.node_size(); i++) {

                        std::cout << "hostname: \t" << res.node(i).hostname() << std::endl;
                        std::cout << "profile: \t" << res.node(i).profile() << std::endl;
                        std::cout << "status: \t" << res.node(i).status();
                        if(res.node(i).status() != daqstatus)
                           std::cout << "  <== Run Control sync problem !!! reset FzDAQ !!!";
                        std::cout << std::endl;
                        std::cout << "----------" << std::endl;
                     }   

                  } else std::cout << ERRTAG << res.reason() << std::endl;

               } else std::cout << ERRTAG << "send of status RC command failed" << std::endl;
            }

            if(!line.compare("stats")) {
             
               cmderr = false;

	       req.set_channel(RCS::Request::RC);
               req.set_operation(RCS::Request::READ);
               req.set_module(RCS::Request::CNT);
               req.set_hostname(hostname);
               req.set_variable("perfdata");

	       msg_req = to_zmqmsg(req);

               if(send_unicast(msg_req, msg_res, req.hostname(), FZC_RC_PORT) == 0) {

		  report.ParseFromArray(msg_res.data(), msg_res.size());
	          print_status_table(report);

	       } else std::cout << ERRTAG << "send of stats RC command failed" << std::endl;
            }

            if(!line.compare("quit")) {
               cmderr = false;
               cli_exit = true;
               break;
            }

            if(cmderr == true) {
              std::cout << ERRTAG << "command not found" << std::endl;
            } else lastcmd = line;

            req.Clear();
            res.Clear();
         }

      } // end for

   } catch (boost::thread_interrupted& interruption) {

         return;
   } 	
}

static void prepare_rcwrite(RCS::Request& req) {

   req.set_channel(RCS::Request::RC);
   req.set_operation(RCS::Request::WRITE);
   req.set_module(RCS::Request::CNT);
   req.set_variable("event");
}

static void print_status_table(Report::NodeSet r) {

   std::cout << std::endl;

   // "compute" and "all" profiles

   for(int i=0; i < r.node_size(); i++) {

      Report::Node n = r.node(i);

      if( (n.profile() == "compute") || (n.profile() == "all") ) {

	 uint64_t ev_empty, ev_good, ev_bad;
         
	 std::cout << "== Host: " << n.hostname() << " - Profile: " << n.profile() << " - Status: " << n.status() << std::endl << std::endl;
	 print_header();

	 print_reader_status(r.node(i).reader());

	 ev_empty = ev_good = ev_bad = 0;

	 for(int j=0; j < r.node(i).parser_size(); j++) {
	    print_parser_status(r.node(i).parser(j));
	    print_fsm_status(r.node(i).fsm(j));

	    ev_empty += r.node(i).fsm(j).events_empty();
	    ev_good  += r.node(i).fsm(j).events_good();
	    ev_bad   += r.node(i).fsm(j).events_bad(); 
	 }

	 std::cout << "Event statistics \tempty: " << ev_empty << "\tgood: " << ev_good << "\t\tbad: " << ev_bad << std::endl;
      }
   }
   std::cout << std::endl;

   // "storage" profile

   for(int i=0; i < r.node_size(); i++) {

      Report::Node n = r.node(i);

      if(n.profile() == "all") {

	// skip header
	print_writer_status(r.node(i).writer());

      } else if( (n.profile() == "storage") ) {

	 std::cout << "== Host: " << n.hostname() << " - Profile: " << n.profile() << " - Status: " << n.status() << std::endl << std::endl;
	 print_header();

	 print_writer_status(r.node(i).writer());
      }
   }
   std::cout << std::endl;
}

static void print_header(void) {

   std::cout << std::left << std::setw(15) << "thread" <<
   std::setw(30) << "          IN events" <<
   std::setw(30) << "           IN data" <<
   std::setw(30) << "          OUT events" <<
   std::setw(30) << "           OUT data" <<
   std::endl;

   std::cout << std::left << std::setw(15) << "-" <<
   std::setw(15) << "     num" <<
   std::setw(15) << "    rate" <<
   std::setw(15) << "    size" <<
   std::setw(15) << "     bw" <<
   std::setw(15) << "     num" <<
   std::setw(15) << "    rate" <<
   std::setw(15) << "    size" <<
   std::setw(15) << "     bw" <<
   std::endl;
}

static void print_reader_status(Report::FzReader r) {

   std::stringstream in_evbw, out_evbw, in_databw, out_databw;

   in_evbw << r.in_events_bw() << " ev/s";
   in_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(r.in_bytes_bw() * 8) << "/s)";
   out_evbw << r.out_events_bw() << " ev/s";
   out_databw << human_byte(r.out_bytes_bw()) << "/s"; // << " (" << human_bit(r.out_bytes_bw() * 8) << "/s)";

   std::cout << std::left << std::setw(15) << "FzReader" <<
   std::setw(15) << std::right << r.in_events() <<
   std::setw(15) << std::right << in_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << in_databw.str() <<
   std::setw(15) << std::right << r.out_events() <<
   std::setw(15) << std::right << out_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.out_bytes()) <<
   std::setw(15) << std::right << out_databw.str() <<
   std::endl;

   std::cout << "---" << std::endl;
}

static void print_parser_status(Report::FzParser r) {

   std::stringstream in_evbw, out_evbw, in_databw, out_databw;

   in_evbw << r.in_events_bw() << " ev/s";
   in_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(psr_report.in_bytes_bw() * 8) << "/s)"; 
   out_evbw << r.in_events_bw() << " ev/s";
   out_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(psr_report.in_bytes_bw() * 8) << "/s)"; 

   std::cout << std::left << std::setw(15) << "FzParser" <<
   std::setw(15) << std::right << r.in_events() <<
   std::setw(15) << std::right << in_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << in_databw.str() <<
   std::setw(15) << std::right << r.out_events() <<
   std::setw(15) << std::right << out_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << out_databw.str() <<
   std::endl;
}

static void print_fsm_status(Report::FzFSM r) {

   std::stringstream in_evbw, out_evbw, in_databw, out_databw;

   in_evbw << r.in_events_bw() << " ev/s";
   in_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 
   out_evbw << r.in_events_bw() << " ev/s";
   out_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 

   std::cout << std::left << std::setw(15) << "  FSM" <<
   std::setw(15) << std::right << r.in_events() <<
   std::setw(15) << std::right << in_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << in_databw.str() <<
   std::setw(15) << std::right << r.out_events() <<
   std::setw(15) << std::right << out_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << out_databw.str() <<
   std::endl;

   std::cout << "---" << std::endl;
}

static void print_writer_status(Report::FzWriter r) {

   std::stringstream in_evbw, out_evbw, in_databw, out_databw;

   in_evbw << r.in_events_bw() << " ev/s";
   in_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 
   out_evbw << r.in_events_bw() << " ev/s";
   out_databw << human_byte(r.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 

   std::cout << std::left << std::setw(15) << "FzWriter" <<
   std::setw(15) << std::right << r.in_events() <<
   std::setw(15) << std::right << in_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << in_databw.str() <<
   std::setw(15) << std::right << r.out_events() <<
   std::setw(15) << std::right << out_evbw.str() <<
   std::setw(15) << std::right << human_byte(r.in_bytes()) <<
   std::setw(15) << std::right << out_databw.str() <<
   std::endl;

   std::cout << "---" << std::endl;
}

void FzController::close_cli(void) {

   thrcli->interrupt();
   thrcli->join();
}


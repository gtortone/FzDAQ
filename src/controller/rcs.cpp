#include "FzController.h"

#include <boost/algorithm/string.hpp>	// tolower

zmq::message_t FzController::handle_request(zmq::message_t& request) {

   RCS::Request req;
   RCS::Response res;

   res.set_errorcode(RCS::Response::ERR);
   res.set_reason("request format error");

   if(req.ParseFromArray(request.data(), request.size()) == false) {

      res.set_reason("request deserialization error");

      return(to_zmqmsg(res));
   }

   if(!req.has_module()) {

      res.set_reason("request destination module not present");

      return(to_zmqmsg(res));      
   }

   if(req.module() == RCS::Request::CNT) {

      if(req.channel() == RCS::Request::RC) {		// RUN CONTROL

         if(req.operation() == RCS::Request::READ) {

            if(req.variable() == "status") {

               res.set_value(state_labels[rc.state()]);
               res.set_errorcode(RCS::Response::OK);
               res.set_reason("OK");

            } else if(req.variable() == "nodestatus") {

               std::map<std::string, Report::Node>::iterator it;
               Report::Node rep;
               
               it = map.begin();
               while(it != map.end()) {

                  rep = it->second;
                  RCS::Node *node = res.add_node();

                  node->set_hostname(rep.hostname());
                  node->set_profile(rep.profile());
                  node->set_status(rep.status());

                  it++;
               }

               res.set_errorcode(RCS::Response::OK);
               res.set_reason("OK");

            } else if(req.variable() == "perfdata") {

               Report::NodeSet rs;
               Report::Node *rn;
               std::map<std::string, Report::Node>::iterator it;

               it = map.begin();
               while(it != map.end()) {

                  rn = rs.add_node();
                  rn->MergeFrom(it->second);

                  it++;
               }
               
               // only for this request return a serialized Report::NodeSet
               std::string str;
               rs.SerializeToString(&str);

               zmq::message_t msg(str.size());
               memcpy(msg.data(), str.data(), str.size());

               return(msg);

            } else res.set_reason("variable not found");

         } else if(req.operation() == RCS::Request::WRITE) {

            if(req.variable() == "event") {

               if(map.size() == 0) {

                  res.set_reason("no DAQ node available");

               } else {

                  RCtransition rct = RCOK;

                  if(req.value() == "reset") {

                     rct = rc.process(RCcommand::reset);

                  } else if(req.value() == "configure") {

                    rct = rc.process(RCcommand::configure);

                  } else if(req.value() == "start") {

                    rct = rc.process(RCcommand::start);

                  } else if(req.value() == "stop") {

                    rct = rc.process(RCcommand::stop);

                  } else res.set_reason("event value not correct");

                  if(rct == RCERROR) {

                     res.set_reason("wrong requested RC transition");

                  } else { 
                     
                     req.set_module(RCS::Request::NM);
                     req.set_variable("status");	// event on FzController -> status on FzNodeManager

                     std::string s = state_labels[rc.state()];
                     boost::to_lower(s);
                     req.set_value(s);

                     zmq::message_t msg_req = to_zmqmsg(req);
                     push_broadcast(msg_req, FZNM_PULL_PORT);

		     // error code is always OK due to time constraints...
                     res.set_errorcode(RCS::Response::OK);
                     res.set_reason("RC transition ok");
                  }
               }		

            } else res.set_reason("variable not found");
         }

      } else if(req.channel() == RCS::Request::SETUP) {	// SETUP

      }

   } else if(req.module() == RCS::Request::NM) {	// routing to FzNodeManager

      if(req.hostname().empty()) {

         res.set_reason("destination hostname not present");

      } else {

         zmq::message_t msg_req = to_zmqmsg(req);
         zmq::message_t msg_res;

         if(send_unicast(msg_req, msg_res, req.hostname(), FZNM_REP_PORT) == 0)	// success
            return(msg_res);
         else res.set_reason("destination hostname not available");
      }
   }

   return(to_zmqmsg(res));
}

int FzController::send_unicast(zmq::message_t &request, zmq::message_t& response, std::string hostname, uint16_t port) {

   int tval = 3000;
   std::stringstream ep;

   ep << "tcp://" << hostname << ":" << port;

   zmq::socket_t socket(context, ZMQ_REQ);
   socket.setsockopt(ZMQ_SNDTIMEO, &tval, sizeof(tval));            // 3 seconds timeout on send
   socket.setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 3 seconds timeout on recv
   socket.connect (ep.str().c_str());

   if(socket.send(request)) {

      if(socket.recv(&response)) {

         if(response.size() > 0) {
            socket.close();
            return 0;   // success
         }
      }
   } 
 
   socket.close();
   return 1;
}

void FzController::push_unicast(zmq::message_t &request, std::string hostname, uint16_t port) {

   std::stringstream ep;

   ep << "tcp://" << hostname << ":" << port;

   zmq::socket_t socket(context, ZMQ_PUSH);
   socket.connect (ep.str().c_str());

   socket.send(request);

   socket.close();
}

void FzController::push_broadcast(zmq::message_t& request, uint16_t port) {

   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;
   zmq::message_t response;
   zmq::message_t req(request.size());
   
   it = map.begin();
   while(it != map.end()) {

      rep = it->second;

      // zmq_msg_t structure passed to zmq_send() is nullified during the call
      req.copy(&request);
      push_unicast(req, rep.hostname(), port);

      it++;
   }
}

zmq::message_t FzController::to_zmqmsg(RCS::Request& data) {

   std::string str;
   data.SerializeToString(&str);

   zmq::message_t msg(str.size());
   memcpy(msg.data(), str.data(), str.size());

   return(msg);
}

zmq::message_t FzController::to_zmqmsg(RCS::Response& data) {

   std::string str;
   data.SerializeToString(&str);

   zmq::message_t msg(str.size());
   memcpy(msg.data(), str.data(), str.size());

   return(msg);
}

RCS::Response FzController::to_rcsres(zmq::message_t& msg) {

   RCS::Response res;

   res.ParseFromArray(msg.data(), msg.size());

   return(res);   
}

void FzController::configure_rcs(void) {

   std::string ep, netint;

   // run control socket

   try {

      rcontrol = new zmq::socket_t(context, ZMQ_REP);
      int tval = 500;
      rcontrol->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));     // 500 milliseconds timeout on recv
      int linger = 0;
      rcontrol->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to start ZeroMQ run control reply: " << e.what () << std::endl;
        exit(1);
   }

   // run control endpoint setup

   if(hascfg && cfg.lookupValue("fzdaq.fzcontroller.interface", netint)) {

      std::cout << INFOTAG << "FzController: network interface: " << netint << " [cfg file]" << std::endl;
      ep = "tcp://" + netint + ":" + std::to_string(FZC_RC_PORT);
      std::cout << INFOTAG << "FzController: run control endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      // default setting
      ep = "tcp://eth0:" + std::to_string(FZC_RC_PORT);
      std::cout << INFOTAG << "FzController: run control endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      rcontrol->bind(ep.c_str());

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzNodeManager: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
      exit(1);
   }
}

void FzController::start_rcs(void) {

   thrrc = new boost::thread(&FzController::process_rcs, this);
}

void FzController::process_rcs(void) {

   int retval;
   zmq::message_t req;
 
   while(true) {        // thread loop

      try {

         boost::this_thread::interruption_point();

         // receive run control & setup messages
         retval = rcontrol->recv(&req);

         if(retval) {   // request received...

            zmq::message_t res;
            res = handle_request(req);

            // a reply is mandatory in every case...    
            rcontrol->send(res);
         }

      } catch(boost::thread_interrupted& interruption) {

         break;

      } catch(std::exception& e) {

      }
   }    // end while
}

void FzController::close_rcs(void) {

   thrrc->interrupt();
   thrrc->join();

   rcontrol->close();
}

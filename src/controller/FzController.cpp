#include <iostream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>
#include "boost/program_options.hpp"
#include "boost/thread.hpp"

#include "epicsThread.h"
#include "epicsExit.h"
#include "epicsStdio.h"
#include "dbStaticLib.h"
#include "subRecord.h"
#include "dbAccess.h"
#include "asDbLib.h"
#include "iocInit.h"
#include "initHooks.h"
#include "iocsh.h"
#include "cadef.h"

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "proto/FzNodeReport.pb.h"
#include "logger/FzHttpPost.h"
#include "rc/FzEpics.h"
#include "utils/zmq.hpp"
#include "rc/RCFSM.h"
#include "logger/FzJMS.h"
#include "logger/FzLogger.h"
#include "utils/FzUtils.h"

#define DBD_FILE "/etc/default/fazia/softIoc.dbd"
#define DB_FILE  "/etc/default/fazia/Fazia-C.db"

extern "C" int softIoc_registerRecordDeviceDriver(struct dbBase *pdbbase);

namespace po = boost::program_options;

zmq::context_t context(1);

void process(std::string cfgfile);
bool map_insert(std::string const &k, Report::Node const &v);
void map_clean(void);

void weblog(void);
bool haswl;
std::string wl_url, wl_user;
int wl_interval;
boost::thread *thrwl;

// global vars 
Report::Node nodereport;
static double const EXPIRY = 10; 	// seconds
std::map<std::string, Report::Node> map;
std::deque<std::pair<std::map<std::string, Report::Node>::iterator, time_t>> deque;

RCFSM rc;
bool ioc_is_ready = false;
void epics_ioc(void);
static void rc_configure_cb(struct event_handler_args eha);
static void rc_start_cb(struct event_handler_args eha);
static void rc_stop_cb(struct event_handler_args eha);
static void rc_reset_cb(struct event_handler_args eha);
void rc_do(RCcommand cmd);
void update_stats_ioc(void);

FzLogger clog;
boost::shared_mutex mutex;

int main(int argc, char *argv[]) {

   libconfig::Config cfg;
   std::string cfgfile = "";

   activemq::core::ActiveMQConnectionFactory JMSfactory;
   std::shared_ptr<cms::Connection> JMSconn;
   std::string brokerURI;

   boost::thread *thrmon;
   boost::thread *ioc;

   // handling of command line parameters
   po::options_description desc("\nFzController - allowed options", 100);

   desc.add_options()
    ("help", "produce help message")
    ("cfg", po::value<std::string>(), "[optional] configuration file")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      exit(0);
   }
   
   std::cout << std::endl;
   std::cout << BOLDMAGENTA << "FzController - START configuration" << RESET << std::endl;

   if (vm.count("cfg")) {

      cfgfile = vm["cfg"].as<std::string>();

      std::cout << INFOTAG << "FzController configuration file: " << cfgfile << std::endl;

      try {

         cfg.readFile(cfgfile.c_str());

      } catch(libconfig::ParseException& ex) {

         std::cout << ERRTAG << "configuration file line " << ex.getLine() << ": " << ex.getError() << std::endl;
         exit(1);

      } catch(libconfig::FileIOException& ex) {

         std::cout << ERRTAG << "cannot open configuration file " << cfgfile << std::endl;
         exit(1);
      }
   }

   // configure ActiveMQ JMS log
   if(cfg.lookupValue("fzdaq.global.log.url", brokerURI)) {

      std::cout << INFOTAG << "FzDAQ global logging URI: " << brokerURI << "\t[cfg file]" << std::endl;

      activemq::library::ActiveMQCPP::initializeLibrary();
      JMSfactory.setBrokerURI(brokerURI);

      try {

        JMSconn.reset(JMSfactory.createConnection());

      } catch (cms::CMSException e) {

        std::cout << ERRTAG << "log server error: " << e.what() << std::endl;
        JMSconn.reset();
        //activemq::library::ActiveMQCPP::shutdownLibrary();
      }
   }

   if(JMSconn.get()) {

      JMSconn->start();
      std::cout << INFOTAG << "log server connection successfully" << std::endl;
      clog.setJMSConnection("FzController", JMSconn.get());
   }

   thrmon = new boost::thread(process, cfgfile);
   ioc = new boost::thread(epics_ioc);

   std::cout << INFOTAG << "FzController: service ready" << std::endl;
   sleep(1);
   std::cout << BOLDMAGENTA << "FzDAQ - END configuration" << RESET << std::endl;
   std::cout << std::endl;
  
   // start command line interface

   std::cout << BOLDMAGENTA << "FzController - START command line interface " << RESET << std::endl;
   std::cout << std::endl;

   std::string line, lastcmd;
   for ( ; std::cout << "FzController > " && std::getline(std::cin, line); ) {
      if (!line.empty()) {

         if(!line.compare("r"))
            line = lastcmd;

         if(!line.compare("help")) {

            std::cout << std::endl;
            std::cout << "help:   \tprint usage" << std::endl;
            std::cout << "quit:   \tquit from FzController" << std::endl;
            std::cout << std::endl;
           }

         if(!line.compare("quit")) {
            break;
         }

         lastcmd = line;
      }
   }

   std::cout << INFOTAG << "FzController: closing threads: \t";
   ioc->interrupt();
   ioc->join();
   thrmon->interrupt();
   thrmon->join();  
   thrwl->interrupt();
   thrwl->join();
   std::cout << " DONE" << std::endl;
   
   context.close();

   std::cout << "Goodbye.\n";

   return(0);
}

//
// Monitoring report thread
//

void process(std::string cfgfile) {

   libconfig::Config cfg;
   bool hascfg;


   zmq::socket_t *pullmon;      // pull socket for monitoring reports
   std::string ep;

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;


   // report monitoring socket

   try {

      pullmon = new zmq::socket_t(context, ZMQ_PULL);
      int tval = 1000;
      pullmon->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 1 second timeout on recv

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to start ZeroMQ report pull: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzcontroller.stats");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzController: report endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzController: report pull endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://eth0:7000";
      std::cout << INFOTAG << "FzController: report pull endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      pullmon->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // parse weblog parameters
     
   haswl = false;

   if(hascfg) {

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.url", wl_url)) {

         haswl = true;
         std::cout << INFOTAG << "Weblog URL: " << wl_url << std::endl;

      } else {

         std::cout << INFOTAG << "Weblog URL not defined" << std::endl;
      }
   }

   if(haswl) {

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.username", wl_user)) {

         std::cout << INFOTAG << "Weblog username: " << wl_user << std::endl;

      } else {

         std::cout << ERRTAG << "Weblog username not defined" << std::endl;
         haswl = false;
      }

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.interval", wl_interval)) {

         std::cout << INFOTAG << "Weblog report time interval: " << wl_interval << " seconds" << std::endl;

      } else {

         std::cout << ERRTAG << "Weblog report time interval not defined" << std::endl;
         haswl = false;
      }
   }   

   if(haswl)
      thrwl = new boost::thread(weblog); 

   boost::shared_lock<boost::shared_mutex> lock{mutex};
      clog.write(INFO, "node report monitoring loop started");
   lock.unlock();

   while(true) {	// thread loop

      try {

         zmq::message_t repmsg;
         bool rc;

         boost::this_thread::interruption_point();

         // receive protobuf
         rc = pullmon->recv(&repmsg);

         if(rc) {

            if(nodereport.ParseFromArray(repmsg.data(), repmsg.size()) == true) {

               map_insert(nodereport.hostname(), nodereport);
            } 
         }
      } catch(boost::thread_interrupted& interruption) {

         pullmon->close();
         break;

      } catch(std::exception& e) {

      }

      map_clean();	// remove expired reports

      // update EPICS IOC 
      if(ioc_is_ready)		// synchronize with IOC initialization
         update_stats_ioc();

   }	// end while
}

bool map_insert(std::string const &k, Report::Node const &v) {

   std::pair<std::map<std::string, Report::Node>::iterator, bool> result = map.insert(std::make_pair(k, v));

   if (result.second) {	// element does not exist in map - insert in deque

      deque.push_back(std::make_pair(result.first, time(NULL)));

   } else {	// element exists in map - update element in map and update time in deque

      map[k] = v;

      for(uint8_t i=0; i<deque.size(); i++) {

         if(deque[i].first->first == result.first->first)
            deque[i].second = time(NULL);
      }      
   }

   return result.second;
}

void map_clean(void) {

   for(uint8_t i=0; i<deque.size(); i++) {

      if(difftime(time(NULL), deque[i].second) > EXPIRY) {

         map.erase(deque[i].first);
         deque.erase(deque.begin() + i);
      }
   }
}

// 
// WebLog thread
//

void weblog(void) {

   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;
   uint32_t tot_state_invalid, tot_events_empty, tot_events_good, tot_events_bad;
   bool send = false;

   FZHttpPost post;
   post.SetServerURL(wl_url);
   post.SetPHPArrayName("par");
   post.SetDBUserName(wl_user);

   while(1) {

         try {
  
            boost::this_thread::interruption_point();

            send = false;
            it = map.begin();
            while(it != map.end()) {

               rep = it->second;

               if(rep.has_reader()) {

                  if(!send) {
 
                     send = true;
                     post.NewPost();
                  }                    

                  post.SetDBCategory("daq_reader");

                  post.AddToPost("hostname", rep.hostname());
                  post.AddToPost("in_data", std::to_string(rep.reader().in_bytes()));
                  post.AddToPost("in_databw", std::to_string(rep.reader().in_bytes_bw()));
                  post.AddToPost("in_event", std::to_string(rep.reader().in_events()));
                  post.AddToPost("in_eventbw", std::to_string(rep.reader().in_events_bw()));

                  tot_state_invalid = tot_events_empty = tot_events_good = tot_events_bad = 0;

                  for(unsigned int i=0; i<rep.parser_num();i++) {

                     tot_state_invalid += rep.fsm(i).state_invalid();
                     tot_events_empty += rep.fsm(i).events_empty();
                     tot_events_good += rep.fsm(i).events_good();
                     tot_events_bad += rep.fsm(i).events_bad();
                  }

                  post.AddToPost("state_invalid", std::to_string(tot_state_invalid));
                  post.AddToPost("events_empty", std::to_string(tot_events_empty));
                  post.AddToPost("events_good", std::to_string(tot_events_good));
                  post.AddToPost("events_bad", std::to_string(tot_events_bad));

                  post.NewLine();
               }

               it++;
            }

            if(send)
               post.SendPost();

            send = false;
            it = map.begin();
            while(it != map.end()) {

               if(rep.has_writer()) {

                  if(!send) {
 
                     send = true;
                     post.NewPost();
                  }                    
 
                  post.SetDBCategory("daq_writer");

                  post.AddToPost("hostname", rep.hostname());
                  post.AddToPost("out_data", std::to_string(rep.writer().out_bytes()));
                  post.AddToPost("out_databw", std::to_string(rep.writer().out_bytes_bw()));
                  post.AddToPost("out_event", std::to_string(rep.writer().out_events()));
                  post.AddToPost("out_eventbw", std::to_string(rep.writer().out_events_bw()));

                  post.NewLine();
               }
              
               it++;
            }

            if(send)
               post.SendPost();

            boost::this_thread::sleep(boost::posix_time::seconds(wl_interval));

        } catch(boost::thread_interrupted& interruption) {

           break;
        }
   }	// end while
}

//
// EPICS IOC thread
//

static void myHookFunction(initHookState state) {
  switch(state) {
    case initHookAfterIocRunning:
      ioc_is_ready = true;
      break;
    default:
      break;
  }
}

int myHookInit(void) {
  return(initHookRegister(myHookFunction));
}

void epics_ioc(void) {

   const char *base_dbd = DBD_FILE;
   char *dbd_file = const_cast<char*>(base_dbd);

   chid rc_configure_chid;	// channel id for global run control CONFIGURE signal
   chid rc_start_chid;		// channel id for global run control START signal
   chid rc_stop_chid;		// channel id for global run control STOP signal
   chid rc_reset_chid;		// channel id for global run control RESET signal
   std::string rc_configure_pv;
   std::string rc_start_pv;
   std::string rc_stop_pv;
   std::string rc_reset_pv;

   if (dbLoadDatabase(dbd_file, NULL, NULL)) {
      std::cout << ERRTAG << "FzController: failed to load FAZIA dbd file" << std::endl;
      exit(1);
   }

   softIoc_registerRecordDeviceDriver(pdbbase);

   if (dbLoadRecords(DB_FILE, NULL)) {
      std::cout << ERRTAG << "FzController: failed to load NodeManager db file" << std::endl;
      exit(1);
   }

   myHookInit();
   iocInit();

   while(!ioc_is_ready) 
      ;			// loop until IOC hook

   std::cout << "FzController: IOC initialized and started" << std::endl;

   if(ca_context_create(ca_enable_preemptive_callback) != ECA_NORMAL) {

      std::cout << ERRTAG << "FzController: failed to create EPICS CA context" << std::endl;
      exit(1);
   }

   rc_configure_pv = "DAQ:RC:configure";
   ca_create_channel(rc_configure_pv.c_str(), NULL, NULL, 20, &rc_configure_chid);
   rc_start_pv= "DAQ:RC:start";
   ca_create_channel(rc_start_pv.c_str(), NULL, NULL, 20, &rc_start_chid);
   rc_stop_pv = "DAQ:RC:stop";
   ca_create_channel(rc_stop_pv.c_str(), NULL, NULL, 20, &rc_stop_chid); 
   rc_reset_pv = "DAQ:RC:reset";
   ca_create_channel(rc_reset_pv.c_str(), NULL, NULL, 20, &rc_reset_chid);

   // first param '1' is DBR_INT from db_access.h
 
   if( ca_create_subscription(1, 1, rc_configure_chid, DBE_VALUE, rc_configure_cb, NULL, NULL) != ECA_NORMAL ) {

      std::cout << "FATAL: EPICS CA callback not available" << std::endl;
      exit(1);
   }

   if( ca_create_subscription(1, 1, rc_start_chid, DBE_VALUE, rc_start_cb, NULL, NULL) != ECA_NORMAL ) {

      std::cout << "FATAL: EPICS CA callback not available" << std::endl;
      exit(1);
   }

   if( ca_create_subscription(1, 1, rc_stop_chid, DBE_VALUE, rc_stop_cb, NULL, NULL) != ECA_NORMAL ) {

      std::cout << "FATAL: EPICS CA callback not available" << std::endl;
      exit(1);
   }

   if( ca_create_subscription(1, 1, rc_reset_chid, DBE_VALUE, rc_reset_cb, NULL, NULL) != ECA_NORMAL ) {

      std::cout << "FATAL: EPICS CA callback not available" << std::endl;
      exit(1);
   }

   while(true) {

      try {

         boost::this_thread::interruption_point();
         epicsThreadSleep(0.5);

      } catch (boost::thread_interrupted& interruption) {

        break;
      }
   }	// end while
}

static void rc_configure_cb(struct event_handler_args eha) {

   int *pdata = (int *) eha.dbr;
   bool cfg = (bool) *pdata;
   RCstate prev_state = rc.state();
   RCstate cur_state;
   std::stringstream msg;

   if(cfg) {

      RCcommand cmd = configure;
      RCtransition trans_err = rc.process(cmd);
      cur_state = rc.state();

      boost::shared_lock<boost::shared_mutex> lock(mutex);
         msg << "global RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
         clog.write(INFO, msg.str()); 
      lock.unlock();

      PVwrite_db("DAQ:RC:transition.DISP", 0);
      PVwrite_db("DAQ:RC:transition", trans_err);
      PVwrite_db("DAQ:RC:transition.DISP", 1);

      rc_do(cmd);

      // update RC FSM state
      PVwrite_db("DAQ:RC:state.DISP", 0);
      PVwrite_db("DAQ:RC:state", rc.state());
      PVwrite_db("DAQ:RC:state.DISP", 1);
   }

}

static void rc_start_cb(struct event_handler_args eha) {

   int *pdata = (int *) eha.dbr;
   bool sta = (bool) *pdata;
   RCstate prev_state = rc.state();
   RCstate cur_state;
   std::stringstream msg;

   if(sta) {

      RCcommand cmd = start;
      RCtransition trans_err = rc.process(cmd);
      cur_state = rc.state();

      boost::shared_lock<boost::shared_mutex> lock{mutex};
         msg << "global RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
         clog.write(INFO, msg.str());
      lock.unlock();
 
      PVwrite_db("DAQ:RC:transition.DISP", 0);
      PVwrite_db("DAQ:RC:transition", trans_err);
      PVwrite_db("DAQ:RC:transition.DISP", 1);

      rc_do(cmd);

      // update RC FSM state
      PVwrite_db("DAQ:RC:state.DISP", 0);
      PVwrite_db("DAQ:RC:state", rc.state());
      PVwrite_db("DAQ:RC:state.DISP", 1);
   }
}

static void rc_stop_cb(struct event_handler_args eha) {

   int *pdata = (int *) eha.dbr;
   bool stp = (bool) *pdata;
   RCstate prev_state = rc.state();
   RCstate cur_state;
   std::stringstream msg;

   if(stp) {

      RCcommand cmd = stop;
      RCtransition trans_err = rc.process(cmd);
      cur_state = rc.state();

      boost::shared_lock<boost::shared_mutex> lock{mutex};
         msg << "global RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
         clog.write(INFO, msg.str());
      lock.unlock();

      PVwrite_db("DAQ:RC:transition.DISP", 0);
      PVwrite_db("DAQ:RC:transition", trans_err);
      PVwrite_db("DAQ:RC:transition.DISP", 1);

      rc_do(cmd);

      // update RC FSM state
      PVwrite_db("DAQ:RC:state.DISP", 0);
      PVwrite_db("DAQ:RC:state", rc.state());
      PVwrite_db("DAQ:RC:state.DISP", 1);
   }
}

static void rc_reset_cb(struct event_handler_args eha) {

   int *pdata = (int *) eha.dbr;
   bool rst = (bool) *pdata;
   RCstate prev_state = rc.state();
   RCstate cur_state;
   std::stringstream msg; 

   if(rst) {

      RCcommand cmd = reset;
      RCtransition trans_err = rc.process(cmd);
      cur_state = rc.state();

      boost::shared_lock<boost::shared_mutex> lock{mutex};
         msg << "global RC state transition: " << state_labels_l[prev_state] << " -> " << state_labels_l[cur_state];
         clog.write(INFO, msg.str());
      lock.unlock();

      PVwrite_db("DAQ:RC:transition.DISP", 0);
      PVwrite_db("DAQ:RC:transition", trans_err);
      PVwrite_db("DAQ:RC:transition.DISP", 1);

      rc_do(cmd);

      // update RC FSM state
      PVwrite_db("DAQ:RC:state.DISP", 0);
      PVwrite_db("DAQ:RC:state", rc.state());
      PVwrite_db("DAQ:RC:state.DISP", 1);
   }
}

void rc_do(RCcommand cmd) {

   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;
   std::string cmd_str;
   std::stringstream msg;

   cmd_str = cmd_labels[cmd];

   if(map.size() == 0) {
  
      boost::shared_lock<boost::shared_mutex> lock{mutex};
         msg << "no DAQ node available - RC command not sent";
         clog.write(WARN, msg.str());
      lock.unlock();
   }

   it = map.begin();
   while(it != map.end()) {

      rep = it->second;
      std::stringstream ep;

      zmq::socket_t socket(context, ZMQ_REQ);
      int tval = 1000;
      socket.setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 1 second timeout on recv

      ep << "tcp://" << rep.hostname() << ":5550";
      socket.connect (ep.str().c_str());
 
      zmq::message_t message(cmd_str.size());      
      memcpy(message.data(), cmd_str.data(), cmd_str.size());
      
      msg << "sending " << cmd_str << " RC command to " << rep.hostname();
      boost::shared_lock<boost::shared_mutex> lock{mutex};
         clog.write(INFO, msg.str());
      lock.unlock(); 
      msg.str("");

      bool rc = socket.send(message);

      if(rc) {

         socket.recv(&message);

      } else {

         msg << "Problem connecting " << rep.hostname() << " RC command failed";
         boost::shared_lock<boost::shared_mutex> lock{mutex};
            clog.write(ERROR, msg.str());
         lock.unlock();
      }

      it++;
   } 
}

void update_stats_ioc(void) {

   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;
   Report::Node DAQreport;
   double ev_good, ev_bad, ev_empty, st_invalid;
   std::stringstream nodelist;

   if(map.empty()) {

      PVwrite_db("DAQ:nodelist", "");
      return;
   }

   it = map.begin();
   ev_good = ev_bad = ev_empty = st_invalid = 0;

   while(it != map.end()) {

      rep = it->second;

      nodelist << rep.hostname() << " [" << rep.profile() << "] ";

      if(rep.has_reader()) {	// compute DAQ node

         Report::FzReader rdrep = rep.reader();

         DAQreport.mutable_reader()->set_in_bytes( DAQreport.mutable_reader()->in_bytes() + rdrep.in_bytes() );
         DAQreport.mutable_reader()->set_in_bytes_bw( DAQreport.mutable_reader()->in_bytes_bw() + rdrep.in_bytes_bw() );
         DAQreport.mutable_reader()->set_in_events( DAQreport.mutable_reader()->in_events() + rdrep.in_events() );
         DAQreport.mutable_reader()->set_in_events_bw( DAQreport.mutable_reader()->in_events_bw() + rdrep.in_events_bw() );
         DAQreport.mutable_reader()->set_out_bytes( DAQreport.mutable_reader()->out_bytes() + rdrep.out_bytes() );
         DAQreport.mutable_reader()->set_out_bytes_bw( DAQreport.mutable_reader()->out_bytes_bw() + rdrep.out_bytes_bw() );
         DAQreport.mutable_reader()->set_out_events( DAQreport.mutable_reader()->out_events() + rdrep.out_events() );
         DAQreport.mutable_reader()->set_out_events_bw( DAQreport.mutable_reader()->out_events_bw() + rdrep.out_events_bw() );

         for(int i=0; i<rep.parser_size(); i++) {

            ev_good += rep.fsm(i).events_good();
            ev_bad += rep.fsm(i).events_bad();
            ev_empty += rep.fsm(i).events_empty();
            st_invalid += rep.fsm(i).state_invalid();
         }
      }

      if(rep.has_writer()) {	// storage DAQ node

         Report::FzWriter wrrep = rep.writer();

         DAQreport.mutable_writer()->set_in_bytes( DAQreport.mutable_writer()->in_bytes() + wrrep.in_bytes() );
         DAQreport.mutable_writer()->set_in_bytes_bw( DAQreport.mutable_writer()->in_bytes_bw() + wrrep.in_bytes_bw() );
         DAQreport.mutable_writer()->set_in_events( DAQreport.mutable_writer()->in_events() + wrrep.in_events() );
         DAQreport.mutable_writer()->set_in_events_bw( DAQreport.mutable_writer()->in_events_bw() + wrrep.in_events_bw() );
         DAQreport.mutable_writer()->set_out_bytes( DAQreport.mutable_writer()->out_bytes() + wrrep.out_bytes() );
         DAQreport.mutable_writer()->set_out_bytes_bw( DAQreport.mutable_writer()->out_bytes_bw() + wrrep.out_bytes_bw() );
         DAQreport.mutable_writer()->set_out_events( DAQreport.mutable_writer()->out_events() + wrrep.out_events() );
         DAQreport.mutable_writer()->set_out_events_bw( DAQreport.mutable_writer()->out_events_bw() + wrrep.out_events_bw() );
      }

      it++;
   }

   double value;
   std::string unit;
 
   // FzReader data statistics

   PVwrite_db("DAQ:nodelist", nodelist.str());

   human_byte(DAQreport.reader().in_bytes(), &value, &unit);
   PVwrite_db("DAQ:reader:in:data", value);
   PVwrite_db("DAQ:reader:in:data.EGU", unit);

   PVwrite_db("DAQ:reader:in:databw", DAQreport.reader().in_bytes_bw() * 8E-6);       // Mbit/s

   PVwrite_db("DAQ:reader:in:ev", (double)DAQreport.reader().in_events());

   human_byte(DAQreport.reader().in_events_bw(), &value);
   PVwrite_db("DAQ:reader:in:evbw", value);

   human_byte(DAQreport.reader().out_bytes(), &value, &unit);
   PVwrite_db("DAQ:reader:out:data", value);
   PVwrite_db("DAQ:reader:out:data.EGU", unit);

   PVwrite_db("DAQ:reader:out:databw", DAQreport.reader().out_bytes_bw() * 8E-6);     // Mbit/s

   PVwrite_db("DAQ:reader:out:ev", (double)DAQreport.reader().out_events());

   human_byte(DAQreport.reader().out_events_bw(), &value);
   PVwrite_db("DAQ:reader:out:evbw", value);

   PVwrite_db("DAQ:fsm:events:good", ev_good);
   PVwrite_db("DAQ:fsm:events:bad", ev_bad);
   PVwrite_db("DAQ:fsm:events:empty", ev_empty);
   PVwrite_db("DAQ:fsm:states:invalid", st_invalid);

   // FzWriter data statistics

   human_byte(DAQreport.writer().in_bytes(), &value, &unit);
   PVwrite_db("DAQ:writer:in:data", value);
   PVwrite_db("DAQ:writer:in:data.EGU", unit);

   PVwrite_db("DAQ:writer:in:databw", DAQreport.writer().in_bytes_bw() * 8E-6);       // Mbit/s

   PVwrite_db("DAQ:writer:in:ev", (double)DAQreport.writer().in_events());

   human_byte(DAQreport.writer().in_events_bw(), &value);
   PVwrite_db("DAQ:writer:in:evbw", value);

   human_byte(DAQreport.writer().out_bytes(), &value, &unit);
   PVwrite_db("DAQ:writer:out:data", value);
   PVwrite_db("DAQ:writer:out:data.EGU", unit);

   PVwrite_db("DAQ:writer:out:databw", DAQreport.writer().out_bytes_bw() * 1E-6);     // Mbyte/s;

   PVwrite_db("DAQ:writer:out:ev", (double)DAQreport.writer().out_events());

   human_byte(DAQreport.writer().out_events_bw(), &value);
   PVwrite_db("DAQ:writer:out:evbw", value);
}

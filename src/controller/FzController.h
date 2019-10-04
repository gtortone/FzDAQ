#ifndef FZCONTROLLER_H_
#define FZCONTROLLER_H_

#include "boost/thread.hpp"

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "proto/FzNodeReport.pb.h"
#include "proto/FzRCS.pb.h"
#include "utils/zmq.hpp"
#include "utils/FzTypedef.h"
#include "rc/RCFSM.h"
#include "utils/FzUtils.h"
#include "logger/FzLogger.h"

class FzController {

private:

   libconfig::Config cfg;
   bool hascfg;

   zmq::context_t &context;
   FzLogger clog;

   RCFSM rc;                    // used in remote Run Control mode
   RCstate state;               // used in remote Run Control mode

   boost::shared_mutex logmutex;
   boost::mutex mapmutex;

   boost::thread *thrcol;	// collector thread
   boost::thread *thrrc;	// run control thread
   boost::thread *thrcli;	// command line thread
#ifdef WEBLOG_ENABLED
   boost::thread *thrwl;	// weblog thread
   std::string wl_url, wl_user;
   int wl_interval;
   bool haswl;
#endif  

   zmq::socket_t *pullmon;      // pull socket for monitoring reports
   zmq::socket_t *rcontrol;     // req/rep socket for run control
 
   double const EXPIRY = 10;    // seconds
   std::map<std::string, Report::Node> map;
   std::deque<std::pair<std::map<std::string, Report::Node>::iterator, time_t>> deque;

   bool cli_exit = false;

public:

   FzController(zmq::context_t &ctx, std::string cfgfile);
   FzController(zmq::context_t &ctx);
 
   bool map_insert(std::string const &k, Report::Node const &v);
   void map_clean(void);
 
   zmq::message_t handle_request(zmq::message_t& request);

   int send_unicast(zmq::message_t& request, zmq::message_t& response, std::string hostname, uint16_t port);
   void push_unicast(zmq::message_t& request, std::string hostname, uint16_t port);
   void push_broadcast(zmq::message_t& request, uint16_t port);

   zmq::message_t to_zmqmsg(RCS::Request& data);
   zmq::message_t to_zmqmsg(RCS::Response& data);

   RCS::Response to_rcsres(zmq::message_t& msg);

   void configure_log(void);
   void configure_collector(void);
   void configure_rcs(void);  
#ifdef WEBLOG_ENABLED
   void configure_weblog(void);
#endif

   void start_collector(void);
   void start_rcs(void);
   void start_cli(void);
#ifdef WEBLOG_ENABLED
   void start_weblog(void);
#endif

   void process_collector(void);
   void process_rcs(void);
   void process_cli(void);
#ifdef WEBLOG_ENABLED
   void process_weblog(void);
#endif

   void close_collector(void);
   void close_rcs(void);
   void close_cli(void);
#ifdef WEBLOG_ENABLED
   void close_weblog(void);
#endif

   void configure(void);
   void start(void);
   void stop(void);
   void close(void);

   bool quit(void);
};

#endif


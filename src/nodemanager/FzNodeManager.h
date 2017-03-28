#ifndef FZMANAGER_H_
#define FZMANAGER_H_

#include <unistd.h>
#include "boost/thread.hpp"

#define	RC_MODE_LOCAL	"local"
#define	RC_MODE_REMOTE	"remote"

#include "main/FzConfig.h"
#include "reader/FzReader.h"
#include "parser/FzParser.h"
#include "writer/FzWriter.h"
#include "rc/RCFSM.h"
#include "utils/FzTypedef.h"
#include "logger/FzLogger.h"

class FzNodeManager {

private:

   boost::thread *thr_perfdata;
   boost::thread *thr_comm;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *pushmon;	// push socket for monitoring reports
   zmq::socket_t *rcontrol;	// req/rep socket for run control
   zmq::socket_t *pcontrol;	// pull socket for run control

   libconfig::Config cfg;
   std::string profile;
   bool hascfg;

   // threads communication
   FzReader *rd;
   std::vector<FzParser *> psr_array;
   FzWriter *wr;

   FzLogger log;
#ifdef AMQLOG_ENABLED
   std::unique_ptr<cms::Connection> AMQconn;
#endif
   boost::mutex logmtx;

   std::string hostname;
   Report::Node nodereport;
   bool report_init;   

   std::string rcmode;
   RCFSM rc;			// used in local Run Control mode
   RCstate state;		// used in remote Run Control mode

   void process_perfdata(void);   
   void process_comm(void);

   zmq::message_t handle_request(zmq::message_t& request);

   void rc_event_do(RCcommand cmd);	// used in local Run Control mode
   void rc_state_do(RCstate state);	// used in remote Run Control mode

public:

#ifdef AMQLOG_ENABLED
   FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx, cms::Connection *JMSconn);
#else
   FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx);
#endif
   void init(void);
   void close(void);

   Report::Node get_nodereport(void);
 
   bool has_data(void);

   // wrapper methods for RC FSM
   RCtransition rc_process(RCcommand cmd);
   RCstate rc_state(void);
   std::string rc_mode(void);
   RCtransition rc_error(void);
};

#endif

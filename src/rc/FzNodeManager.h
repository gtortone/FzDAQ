#ifndef FZMANAGER_H_
#define FZMANAGER_H_

#include <unistd.h>
#include "boost/thread.hpp"

#ifdef EPICS_ENABLED

#include "epicsThread.h"
#include "epicsExit.h"
#include "epicsStdio.h"
#include "dbStaticLib.h"
#include "subRecord.h"
#include "dbAccess.h"
#include "asDbLib.h"
#include "iocInit.h"
#include "iocsh.h"
#include "rc/FzEpics.h"

#undef DBR_CTRL_DOUBLE
#undef DBR_CTRL_LONG
#undef DBR_GR_DOUBLE
#undef DBR_GR_LONG
#undef DBR_PUT_ACKS
#undef DBR_PUT_ACKT
#undef DBR_SHORT
#undef INVALID_DB_REQ
#undef VALID_DB_REQ

#include "cadef.h"

#define DBD_FILE "/etc/default/fazia/softIoc.dbd"
#define DB_FILE	 "/etc/default/fazia/Fazia-NM.db"

extern "C" int softIoc_registerRecordDeviceDriver(struct dbBase *pdbbase);

#endif	// EPICS_ENABLED

#include "main/FzConfig.h"
#include "reader/FzReader.h"
#include "parser/FzParser.h"
#include "writer/FzWriter.h"
#include "rc/RCFSM.h"
#include "utils/FzTypedef.h"
#include "logger/FzLogger.h"

class FzNodeManager {

private:

   boost::thread *thr;
   boost::thread *ioc;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *pushmon;	// push socket for monitoring reports
   zmq::socket_t *rcontrol;	// req/rep socket for run control

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

   RCFSM rc;

   void process(void);   

#ifdef EPICS_ENABLED
   void epics_ioc(void);   

   static void ca_connection_cb(struct connection_handler_args args);
   static void rc_event_cb(struct event_handler_args eha);

   void update_rc_ioc(void);
   void update_stats_ioc(void);
#endif

   void rc_do(RCcommand cmd);

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
   RCtransition rc_error(void);
};

#endif

#ifndef FZMANAGER_H_
#define FZMANAGER_H_

#include <unistd.h>
#include "boost/thread.hpp"

#include "epicsThread.h"
#include "epicsExit.h"
#include "epicsStdio.h"
#include "dbStaticLib.h"
#include "subRecord.h"
#include "dbAccess.h"
#include "asDbLib.h"
#include "iocInit.h"
#include "iocsh.h"
#include "cadef.h"

#include "FzConfig.h"
#include "FzReader.h"
#include "FzParser.h"
#include "FzWriter.h"
#include "FzEpics.h"
#include "RCFSM.h"
#include "FzTypedef.h"
#include "FzLogger.h"

#define DBD_FILE "/etc/default/fazia/softIoc.dbd"
#define DB_FILE	 "/etc/default/fazia/Fazia-NM.db"

extern "C" int softIoc_registerRecordDeviceDriver(struct dbBase *pdbbase);

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
   std::unique_ptr<cms::Connection> AMQconn;
   boost::mutex logmtx;

   std::string hostname;
   Report::Node nodereport;
   bool report_init;   

   RCFSM rc;

   void process(void);   
   void epics_ioc(void);   

   static void ca_connection_cb(struct connection_handler_args args);
   static void rc_event_cb(struct event_handler_args eha);

   void rc_do(RCcommand cmd);
   void update_rc_ioc(void);
   void update_stats_ioc(void);

public:

   FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx, cms::Connection *JMSconn);

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

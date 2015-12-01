#ifndef FZPARSER_H_
#define FZPARSER_H_

#include <sstream>

#include <boost/thread.hpp>

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzFSM.h"
#include "FzProtobuf.h"
#include "FzLogger.h"
#include "FzNodeReport.pb.h"
#include "zmq.hpp"
#include "FzJMS.h"
#include "FzLogger.h"
#include "FzTypedef.h" 

class FzParser {

private:

   libconfig::Config cfg;
   bool hascfg;

   boost::thread *thr;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *parser;
   zmq::socket_t *writer;

   RCstate rcstate;

   FzFSM sm;
   DAQ::FzEvent ev;
 
   FzLogger log;
   std::unique_ptr<cms::Connection> AMQconn;

   Report::FzParser psr_report;

   void process(void);

public:

   FzParser(unsigned int id, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn);

   void init(void);
   void close(void);

   void rc_do(RCcommand cmd);
   void set_rcstate(RCstate s);

   Report::FzParser get_report(void);
   Report::FzFSM get_fsm_report(void);
}; 

#endif

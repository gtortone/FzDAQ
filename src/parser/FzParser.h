#ifndef FZPARSER_H_
#define FZPARSER_H_

#include <sstream>

#include <boost/thread.hpp>

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "parser/FzFSM.h"
#include "utils/FzProtobuf.h"
#include "logger/FzLogger.h"
#include "proto/FzNodeReport.pb.h"
#include "utils/zmq.hpp"
#include "logger/FzJMS.h"
#include "logger/FzLogger.h"
#include "utils/FzTypedef.h" 

class FzParser {

private:

   libconfig::Config cfg;

   boost::thread *thr;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *parser;
   zmq::socket_t *writer;

   RCstate rcstate;

   int evformat;
   FzFSM sm;
   DAQ::FzEvent ev;
 
   FzLogger log;
   std::string logbase;
#ifdef AMQLOG_ENABLED
   std::unique_ptr<cms::Connection> AMQconn;
#endif

   Report::FzParser psr_report;

   void process(void);

public:

#ifdef AMQLOG_ENABLED
   FzParser(unsigned int id, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn, int evf, std::string logdir);
#else
   FzParser(unsigned int id, std::string cfgfile, zmq::context_t &ctx, int evf, std::string logdir);
#endif

   void init(void);
   void close(void);

   void set_rcstate(RCstate s);

   Report::FzParser get_report(void);
   Report::FzFSM get_fsm_report(void);
}; 

#endif

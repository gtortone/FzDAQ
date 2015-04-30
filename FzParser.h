#ifndef FZPARSER_H_
#define FZPARSER_H_

#include <sstream>

#include <boost/thread.hpp>

#include <log4cpp/Category.hh>
#include "log4cpp/Appender.hh"
#include <log4cpp/PatternLayout.hh>
#include "log4cpp/FileAppender.hh"
#include <log4cpp/PropertyConfigurator.hh>

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzFSM.h"
#include "FzProtobuf.h"
#include "FzLogger.h"
#include "FzNodeReport.pb.h"
#include "zmq.hpp"

class FzParser {

private:

   libconfig::Config cfg;
   bool hascfg;

   boost::thread *thr;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *parser;
   zmq::socket_t *writer;

   FzFSM sm;
   DAQ::FzEvent ev;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;
 
   Report::FzParser psr_report;

   DAQstatus_t status;

   void process(void);

public:
   log4cpp::Category &logparser;

   FzParser(unsigned int id, log4cpp::Priority::Value log_priority, std::string cfgfile, zmq::context_t &ctx);

   void init(void);
   void close(void);
   void set_status(enum DAQstatus_t val);

   Report::FzParser get_report(void);
   Report::FzFSM get_fsm_report(void);
}; 

#endif

#ifndef FZMANAGER_H_
#define FZMANAGER_H_

#include <unistd.h>
#include "boost/thread.hpp"

#include "FzConfig.h"
#include "FzReader.h"
#include "FzParser.h"
#include "FzWriter.h"

class FzNodeManager {

private:

   boost::thread *thr;
   bool thread_init;

   zmq::context_t &context;
   zmq::socket_t *subrc;	// subscribe socket for run control messages
   zmq::socket_t *pushmon;	// push socket for monitoring reports

   libconfig::Config cfg;
   std::string profile;
   bool hascfg;

   FzReader *rd;
   std::vector<FzParser *> psr_array;
   FzWriter *wr;

   std::string hostname;
   Report::Node nodereport;
   bool report_init;   

   DAQstatus_t status;

   void process(void);   

public:

   FzNodeManager(FzReader *rd, std::vector<FzParser *> psr_array, FzWriter *wr, std::string cfgfile, std::string prof, zmq::context_t &ctx);

   void init(void);
   void close(void);

   void set_status(enum DAQstatus_t val);

   Report::Node get_nodereport(void);
 
   bool has_data(void);
};

#endif

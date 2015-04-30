#ifndef FZWRITER_H_
#define FZWRITER_H_

#include <boost/thread.hpp>

#include <log4cpp/Category.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/PropertyConfigurator.hh>

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzProtobuf.h"
#include "FzNodeReport.pb.h"

#include "zmq.hpp"

class FzWriter {

private:

   libconfig::Config cfg;
   bool hascfg;

   boost::thread *thr;
   bool thread_init;
   zmq::context_t &context;
   zmq::socket_t *writer;
   zmq::socket_t *pub;

   FzProtobuf *pb;

   log4cpp::Category &logwriter;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;
 
   unsigned long int event_file_size;
   unsigned long int event_dir_size;

   std::string basedir;
   std::string runtag;
   bool subid;

   unsigned int fileid;
   unsigned int dirid;
   unsigned int dirsubid;
   std::string dirstr;

   std::stringstream filename;
   std::fstream output; 

   Report::FzWriter report;

   DAQstatus_t status;

   void process(void);

public:
   FzWriter(std::string bdir, std::string run, long int id, bool subid, std::string cfgfile, zmq::context_t &ctx);

   void init(void);
   void close(void);
   void set_status(enum DAQstatus_t val);

   void set_eventfilesize(unsigned long int size);
   void set_eventdirsize(unsigned long int size);

   void setup_newfile(void);
   int setup_newdir(void);

   Report::FzWriter get_report(void);
};

#endif

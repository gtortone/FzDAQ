#ifndef FZWRITER_H_
#define FZWRITER_H_

#include <log4cpp/Category.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/PropertyConfigurator.hh>

#include "FzTypedef.h"
#include "FzCbuffer.h"
#include "FzProtobuf.h"

#include "zmq.hpp"

class FzWriter {

private:

   boost::thread *thr;
   bool thread_init;

   FzCbuffer<DAQ::FzEvent> &cbw;
   FzProtobuf *pb;

   log4cpp::Category &logwriter;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;
 
   unsigned long int event_file_size;
   unsigned long int event_dir_size;

   std::string basedir;
   std::string runtag;

   DAQstatus_t status;

   void process(void);

public:
   FzWriter(FzCbuffer<DAQ::FzEvent> &cb, std::string basedir, std::string run, long int id, bool subid);

   int init(void);
   void set_status(enum DAQstatus_t val);

   void set_eventfilesize(unsigned long int size);
   void set_eventdirsize(unsigned long int size);
};

#endif

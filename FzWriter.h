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

/*#include "MessageTypes.h"
#include "TSocket.h"
#include "TH1.h"
#include "TMessage.h" */

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

   std::string subdir;
   std::string runtag;
   std::string steptag;

   DAQstatus_t status;

   /*TSocket *sock; 
   TH1 *hist; */

   void process(void);

public:
   FzWriter(FzCbuffer<DAQ::FzEvent> &cb, std::string dir, std::string run, std::string step);

   int init(void);
   void set_status(enum DAQstatus_t val);

   void set_eventfilesize(unsigned long int size);
};

#endif

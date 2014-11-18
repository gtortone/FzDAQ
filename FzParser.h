#ifndef FZPARSER_H_
#define FZPARSER_H_

#include <sstream>

#include <boost/thread.hpp>

#include <log4cpp/Category.hh>
#include "log4cpp/Appender.hh"
#include <log4cpp/PatternLayout.hh>
#include "log4cpp/FileAppender.hh"
#include <log4cpp/PropertyConfigurator.hh>

#include "FzTypedef.h"
#include "FzEventWord.h"
#include "FzFSM.h"
#include "FzCbuffer.h"
#include "FzProtobuf.h"
#include "FzLogger.h"

class FzParser {

private:

   boost::thread *thr;
   bool thread_init;

   FzCbuffer<FzRawData> &cbr;  	  // circular buffer of array of raw data
   FzRawData chunk;

   FzFSM sm;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;
 
   DAQstatus_t status;

   void process(void);

public:
   log4cpp::Category &logparser;
   FzCbuffer<DAQ::FzEvent> *cbw;  // circular buffer of parsed events

   FzParser(FzCbuffer<FzRawData> &cb_r, FzCbuffer <DAQ::FzEvent> *cb_w, unsigned int id, log4cpp::Priority::Value log_priority);

   void init(void);
   void set_status(enum DAQstatus_t val);

   uint32_t get_fsm_tr_invalid_tot(void);
   uint32_t *get_fsm_tr_invalid_stats(void);
   uint32_t get_fsm_event_clean_num(void);
   uint32_t get_fsm_event_witherr_num(void);
   uint32_t get_fsm_event_empty_num(void);
}; 

#endif

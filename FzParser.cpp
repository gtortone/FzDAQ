#include "FzParser.h"
#include "FzLogger.h"

FzParser::FzParser(FzCbuffer<FzRawData> &cb_r, FzCbuffer <DAQ::FzEvent> *cb_w, unsigned int id, log4cpp::Priority::Value log_priority) :
   cbr(cb_r),
   logparser(log4cpp::Category::getInstance("fzparser" + id)) {

   std::stringstream filename;

   filename << "logs/fzparser.log." << id;
   appender = new log4cpp::FileAppender("fzparser" + id, filename.str());
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   appender->setThreshold(log_priority);
   logparser.addAppender(appender);

   logparser.setAdditivity(false);

   logparser << INFO << "FzParser::constructor - success";

   chunk.reserve(MAX_EVENT_SIZE);    // reserve space to avoid memory reallocation

   cbw = cb_w;

   sm.initlog(&logparser);
   sm.initcbw(cbw);
   sm.start();

   thread_init = false;
   status = STOP;

   logparser << INFO << "FzParser - state machine started";
};

void FzParser::init(void) {

   if(!thread_init) {
   
      thread_init = true;
      thr = new boost::thread(boost::bind(&FzParser::process, this));
   }
}

void FzParser::set_status(enum DAQstatus_t val) {

   status = val;
}

void FzParser::process(void) {

   while(true) {

      if(status == START) {

         unsigned long i;
         wtype_t wtype;
     
         chunk.clear();
         chunk = cbr.receive();

         for(i=0; i<chunk.size(); i++) {

               // debug:
               // std::cout << std::setw(4) << std::setfill('0') << std::hex << chunk[i] << std::endl; 

               FzEventWord w(chunk[i]);
               wtype = w.getType();

               switch(wtype) {

                  case WT_UNKNOWN:
                     sm.process_event(gotUNKNOWN(w));
                     break; 

                  case WT_REGHDR:
                     sm.process_event(gotREGHDR(w));
                     break; 

                  case WT_BLKHDR:
                     sm.process_event(gotBLKHDR(w));
                     break;

                  case WT_EC:
                     sm.process_event(gotEC(w));
                     break;

                  case WT_TELHDR:
                     sm.process_event(gotTELHDR(w));
                     break;

                  case WT_DETHDR:
                     sm.process_event(gotDETHDR(w));
                     break;

                  case WT_DATA:
                     sm.process_event(gotDATA(w));
                     break;

                  case WT_LENGTH:
                     sm.process_event(gotLENGTH(w));
                     break;

                  case WT_CRCFE:
                     sm.process_event(gotCRCFE(w));
                     break;

                  case WT_CRCBL:
                     sm.process_event(gotCRCBL(w));
                     break;

                  case WT_EMPTY:
                     //sm.process_event(gotEMPTY(w));
                     break; 
               } // end of switch 
            }  // end of for(i)  

      } else if (status == STOP) {

           boost::this_thread::sleep(boost::posix_time::seconds(1));

      } else if(status == QUIT) {

           break;
      }
   }	// end of while
}

uint32_t FzParser::get_fsm_tr_invalid_tot(void) {
   return(sm.tr_invalid_tot);
}

uint32_t* FzParser::get_fsm_tr_invalid_stats(void) {
   return(sm.tr_invalid_stats);
}

uint32_t FzParser::get_fsm_event_clean_num(void) {
   return(sm.event_clean_num);
}

uint32_t FzParser::get_fsm_event_witherr_num(void) {
   return(sm.event_witherr_num);
} 

uint32_t FzParser::get_fsm_event_empty_num(void) {
   return(sm.event_empty_num);
}

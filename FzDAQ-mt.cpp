#include <iostream>

#include "boost/program_options.hpp"
#include "boost/thread.hpp"
#include <log4cpp/PropertyConfigurator.hh>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "FzEventSet.pb.h"

#include "FzTypedef.h"
#include "FzCbuffer.h"
#include "FzLogger.h"
#include "FzReader.h"
#include "FzParser.h"
#include "FzWriter.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

   unsigned int nthreads;
   std::string inputch;
   std::string subdir;
   std::string runtag;
   std::string steptag;

   struct timeval tv_start, tv_stop;
   unsigned int diff_msec = 0;
   uint64_t curr_num_bytes = 0;
   uint64_t persec_num_bytes = 0;
   unsigned long prev_num_bytes = 0;
   unsigned long diff_num_bytes = 0;
   unsigned long curr_num_ev = 0;
   unsigned long prev_num_ev = 0;
   unsigned long diff_num_ev = 0;

   bool rec = false;
   DAQstatus_t DAQstatus = STOP;

   log4cpp::Priority::Value logparser_prio;

   // configure log 
   log4cpp::PropertyConfigurator::configure("log4cpp.properties");
   logparser_prio = log4cpp::Category::getInstance(std::string("fzparser")).getChainedPriority();

   FzCbuffer<FzRawData> cbr(1500);	// circular buffer of array of raw data
   FzCbuffer<DAQ::FzEvent> cbw(10000);	// circular buffer of parsed events

   // handling of command line parameters
   po::options_description desc("\nFzDAQ - allowed options");
   
   desc.add_options()
    ("help", "produce help message")
    ("nt", po::value<unsigned int>(), "number of parser threads")
    ("input", po::value<std::string>(), "input channel of raw data (usb or file)")
    ("subdir", po::value<std::string>(), "directory for output events")
    ("runtag", po::value<std::string>(), "tag for run identification (e.g. LNS, GANIL)")
    ("steptag", po::value<std::string>(), "tag for step identification (e.g. DEBUG, TEST, PROD)")
    ("esize", po::value<unsigned long int>(), "[optional] max size of event file - default: 1000000 byte - 1 megabyte")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);    

   if (vm.count("help")) {
      std::cout << desc << "\n";
      return 0;
   }

   if (vm.count("nt")) {
      nthreads = vm["nt"].as<unsigned int>();
      std::cout << "nt is " << nthreads << std::endl;
   } else {
      std::cout << "input parameter error: nt not set" << std::endl;
      return -1;
   }

   if (vm.count("input")) {
      inputch = vm["input"].as<std::string>();
      std::cout << "input is " << inputch << std::endl;
   } else {
      std::cout << "input parameter error: input not set" << std::endl;
      return -1;
   }

   if (vm.count("subdir")) {
      subdir = vm["subdir"].as<std::string>();
      std::cout << "subdir is " << subdir << std::endl;
   } else {
      std::cout << "input parameter error: subdir not set" << std::endl;
      return -1;
   }

   if (vm.count("runtag")) {
      runtag = vm["runtag"].as<std::string>();
      std::cout << "runtag is " << runtag << std::endl;
   } else {
      std::cout << "input parameter error: runtag not set" << std::endl;
      return -1;
   }
   
   if (vm.count("steptag")) {
      steptag = vm["steptag"].as<std::string>();
      std::cout << "steptag is " << steptag << std::endl;
   } else {
      std::cout << "input parameter error: steptag not set" << std::endl;
      return -1;
   }

   FzReader *rd = new FzReader(cbr);

   // setup of USB channel
   if (rd->setup(inputch) != 0) {
      std::cout << "FATAL ERROR: DAQ channel setup failed" << std::endl;
      return(-1);
   }

   if (rd->init() != 0) {
  
      std::cout << "FATAL ERROR: DAQ channel initialization failed" << std::endl;
      return(-1);
   }

   FzWriter *wr = new FzWriter(cbw, subdir, runtag, steptag);
   std::vector<FzParser *> psr_array;	// vector of (Fzparser *)

   wr->init();
 
   if( vm.count("esize") )
      wr->set_eventfilesize(vm["esize"].as<unsigned long int>()); 

   for(int i=0; i < nthreads; i++) {
      psr_array.push_back(new FzParser(cbr, &cbw, i, logparser_prio));
      psr_array[i]->init();
   }

   gettimeofday(&tv_start, NULL);

   std::string line, lastcmd;
   for ( ; std::cout << "FzDAQ > " && std::getline(std::cin, line); ) {
        if (!line.empty()) { 

           if(!line.compare("r")) 
              line = lastcmd;

           if(!line.compare("help")) {

              std::cout << std::endl;
              std::cout << "help:   \tprint usage" << std::endl;
              std::cout << "status: \tprint acquisition status" << std::endl;
              std::cout << "start:  \tstart acquisition" << std::endl;
              std::cout << "stop:   \tstop acquisition" << std::endl;
              std::cout << "stats:  \tprint event data acquisition statistics" << std::endl;
              std::cout << "r:      \trepeat last command" << std::endl;
              std::cout << "rec:    \trecord raw data to files" << std::endl;
              std::cout << "quit:   \tquit from FzDAQ" << std::endl;
              std::cout << std::endl;
           }

           if(!line.compare("status"))
              std::cout << "current DAQ status: " << state_labels[DAQstatus] << std::endl;

           if(!line.compare("start")) {
              std::cout << "sending start to FzReader and " << nthreads << "xFzParser" << std::endl;
              rd->set_status(START);
 
              for(int i=0; i < nthreads; i++)
                 psr_array[i]->set_status(START);

              wr->set_status(START);

              DAQstatus = START;
           }

           if(!line.compare("stop")) {
              std::cout << "sending stop to FzReader and " << nthreads << "xFzParser" << std::endl;
              rd->set_status(STOP);

              for(int i=0; i < nthreads; i++)
                 psr_array[i]->set_status(STOP);

              wr->set_status(STOP);

              DAQstatus = STOP;
           }

           if(!line.compare("stats")) {

              uint32_t tr_invalid_tot = 0;
              uint32_t event_clean_num = 0;
              uint32_t event_witherr_num = 0;
              uint32_t event_empty_num = 0;
 
              for(int i=0; i < nthreads; i++) {
                 tr_invalid_tot += psr_array[i]->get_fsm_tr_invalid_tot();
                 event_clean_num += psr_array[i]->get_fsm_event_clean_num();
                 event_witherr_num += psr_array[i]->get_fsm_event_witherr_num();
                 event_empty_num += psr_array[i]->get_fsm_event_empty_num();
              }

              curr_num_bytes = rd->get_tot_bytes();
              diff_num_bytes = curr_num_bytes - prev_num_bytes;
              prev_num_bytes = curr_num_bytes;

              curr_num_ev = event_clean_num + event_witherr_num + event_empty_num;
              diff_num_ev = curr_num_ev - prev_num_ev;
              prev_num_ev = curr_num_ev;

              gettimeofday(&tv_stop, NULL);
              diff_msec = (tv_stop.tv_sec - tv_start.tv_sec)*1000;
              diff_msec += (tv_stop.tv_usec - tv_start.tv_usec)/1000;
   	      gettimeofday(&tv_start, NULL);

              std::cout << std::endl;
              std::cout << "=== USB data transfer statistics ===" << std::endl;
              std::cout << "total bytes transferred: " << curr_num_bytes << std::endl;
              std::cout << "current bandwith in bytes/s: " << rd->get_persec_bytes() << std::endl;
              std::cout << std::endl;

              std::cout << "=== QUEUED events statistics ===" << std::endl;
              std::cout << "total number of events ready to be parsed: " << cbr.size() << std::endl;
              std::cout << std::endl;

              std::cout << "=== PARSED events statistics ===" << std::endl;
              std::cout << "event bandwith in events/s: " << (diff_num_ev*1000)/diff_msec << std::endl;
              std::cout << "total number of invalid state machine transitions: " << tr_invalid_tot << std::endl;
              std::cout << "total number of parsed empty events: " << event_empty_num << std::endl;
              std::cout << "total number of parsed events without errors: " << event_clean_num << std::endl;
              std::cout << "total number of parsed events with one or more errors: " << event_witherr_num << std::endl; 
              std::cout << std::endl;

              std::cout << "=== STORED event statistics ===" << std::endl;
              std::cout << "total number of events ready to be stored: " << cbw.size() << std::endl;
              std::cout << std::endl;
           }

           if(!line.compare("rec")) {

              if(!rec) {

                 rd->record(true);
                 rec = true;
                 std::cout << "record enabled" << std::endl;
 
              } else {

  		 rd->record(false);
                 rec = false;
                 std::cout << "record disabled" << std::endl;
              }
           }

           if(!line.compare("quit")) {
              std::cout << "quit from FzDAQ" << std::endl;
              DAQstatus = QUIT;
              break;
           }

           lastcmd = line;
        }
    }

   std::cout << "closing FzReader thread: \t";
   rd->set_status(QUIT);
   std::cout << " DONE" << std::endl;

   std::cout << "closing FzParser threads: \t";
   for(int i=0; i < nthreads; i++)
      psr_array[i]->set_status(STOP);
   std::cout << " DONE" << std::endl;

   std::cout << "closing FzWriter thread: \t";
   wr->set_status(QUIT);
   std::cout << " DONE" << std::endl; 
   
   std::cout << "Goodbye.\n";

   return 0;
}

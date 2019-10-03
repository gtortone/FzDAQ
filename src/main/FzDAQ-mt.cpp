#include <iostream>

#include "boost/program_options.hpp"
#include "boost/thread.hpp"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "proto/FzEventSet.pb.h"

#include "FzConfig.h"
#include "utils/FzTypedef.h"
#include "utils/FzUtils.h"
#include "logger/FzLogger.h"
#include "nodemanager/FzNodeManager.h"
#include "reader/FzReader.h"
#include "parser/FzParser.h"
#include "writer/FzWriter.h"
#include "logger/FzJMS.h"
#include "utils/zmq.hpp"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

   zmq::context_t context(1);	// for ZeroMQ communications

   unsigned int i;
   std::string neturl;
   unsigned int nthreads;
   std::string subdir;
   std::string runtag;
   uint32_t runid;
   uint32_t esize, dsize;

   libconfig::Config cfg;
   bool hascfg = false;
   bool iscompute = false;
   bool isstorage = false;
   std::string cfgfile;
   std::string profile;
   std::string netdev, devip;
   unsigned int port;
   std::string brokerURI;
   int evformat = 0;
   std::string evformat_str;
   std::string logbasedir;

   FzReader *rd = NULL;
   std::vector<FzParser *> psr_array;	// vector of (Fzparser *)
   FzWriter *wr = NULL;
   FzNodeManager *nm;

   Report::Node nodereport;

   bool rec = false;

#ifdef AMQLOG_ENABLED
   activemq::core::ActiveMQConnectionFactory JMSfactory;
   //std::shared_ptr<cms::Connection> JMSconn;
   cms::Connection *JMSconn = NULL;
#endif

   // handling of command line parameters
   po::options_description desc("\nFzDAQ - allowed options", 100);
   
   desc.add_options()
    ("help", "produce help message")
    ("cfg", po::value<std::string>(), "[mandatory] configuration file")
    ("profile", po::value<std::string>(), "[mandatory] profile to start {compute, storage, all}")
   ;

   po::variables_map vm;

   try {
      
      po::store(po::parse_command_line(argc, argv, desc), vm);
   
   } catch (po::error& e) {

      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      std::cerr << desc << std::endl;
      exit(1);
   }
   
   po::notify(vm);    


   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      exit(0);
   }
 
   std::cout << std::endl;
   std::cout << BOLDMAGENTA << "FzDAQ - START configuration" << RESET << std::endl;

   // by default run all profile on single machine
   profile = "all";
   iscompute = isstorage = true;

   if (vm.count("cfg")) {
      
      hascfg = true;
      cfgfile = vm["cfg"].as<std::string>();

      std::cout << INFOTAG << "FzDAQ configuration file: " << cfgfile << std::endl;
 
      try {

         cfg.readFile(cfgfile.c_str());

      } catch(libconfig::ParseException& ex) {

         std::cout << ERRTAG << "configuration file line " << ex.getLine() << ": " << ex.getError() << std::endl;
         exit(1);

      } catch(libconfig::FileIOException& ex) {

         std::cout << ERRTAG << "cannot open configuration file " << cfgfile << std::endl;
         exit(1);
      }
   }
 
   if(hascfg) {
 
      if(!vm.count("profile")) {

         std::cout << ERRTAG << "if configuration file is specified profile is mandatory" << std::endl;
         exit(1);

      } else {

         profile = vm["profile"].as<std::string>();
         iscompute = isstorage =  false;	// check parameter for profile to run

         if(profile == "compute")
            iscompute = true;
         else if(profile == "storage")
            isstorage = true;
         else if(profile == "all")
            iscompute = isstorage = true;
         else {
   
            std::cout << ERRTAG << "profile unknown" << std::endl;
            exit(1);
         }
      }
   }  

#ifdef AMQLOG_ENABLED
   // configure ActiveMQ JMS log
   if(cfg.lookupValue("fzdaq.global.log.url", brokerURI)) {

      std::cout << INFOTAG << "FzDAQ global logging URI: " << brokerURI << "\t[cfg file]" << std::endl;

      activemq::library::ActiveMQCPP::initializeLibrary();
      JMSfactory.setBrokerURI(brokerURI);

      try {
 
        JMSconn = JMSfactory.createConnection();

      } catch (cms::CMSException e) {

        std::cout << ERRTAG << "log server error" << std::endl; 
        JMSconn = NULL;
      } 
   } 

   if(JMSconn) {
    
      JMSconn->start();
      std::cout << INFOTAG << "log server connection successfully" << std::endl;
   }
#endif

   // configure log base directory
   if(cfg.lookupValue("fzdaq.global.log.basedir", logbasedir)) {

      std::cout << INFOTAG << "log files base directory: " << logbasedir << std::endl;

   } else logbasedir = "/var/log/fazia";

   // configure event format
   if(cfg.lookupValue("fzdaq.global.evformat", evformat_str)) {

      if(evformat_str == "basic")
         evformat = 0;
      else if(evformat_str == "tag")
         evformat = 1;
      else { 
         std::cout << ERRTAG << "FzDAQ: unable to parse event format" << std::endl;
         exit(1);
      }
   } else {
         std::cout << ERRTAG << "FzDAQ: event format not specified in configuration file" << std::endl;
         exit(1);
   }

   std::cout << INFOTAG << "*** Selected event format: " << evformat_str << " ***" << std::endl; 

   std::cout << INFOTAG << "FzDAQ profile selected: " << profile << std::endl;

   if(iscompute) {		

      neturl = "";

      // configure FzReader and FzParser

      if(cfg.lookupValue("fzdaq.fzreader.consumer.url", neturl)) {

	 std::cout << INFOTAG << "FzReader network UDP url: " << neturl << "\t[cfg file]" << std::endl;

      } else {

	 neturl = "udp://eth0:50000";
	 std::cout << INFOTAG << "FzReader network UDP url: udp://eth0:5000\t[default]" << std::endl;
      }

      if(!urlparse(neturl, &netdev, &port)) {

	 std::cout << ERRTAG << "FzReader: unable to parse UDP url" << std::endl;
	 exit(1);
      }

      devip = devtoip(netdev);

      if(devip.empty()) {

	 std::cout << ERRTAG << "FzReader: unable to get ip address of " << netdev << std::endl;
	 exit(1);
      }

      // reassemble neturl with IP 
      std::ostringstream s;
      s << "udp://" << devip << ":" << port;
      neturl = s.str();
 
      std::cout << INFOTAG << "FzReader: UDP socket will be bind on IP " << devip << " (" << netdev << ")" << std::endl;

      if(cfg.lookupValue("fzdaq.fzparser.nthreads", nthreads)) {

         std::cout << INFOTAG << "FzParser nthreads: " << nthreads << "\t[cfg file]" << std::endl;

      } else {

         nthreads = 1;
         std::cout << INFOTAG << "FzParser nthreads: " << nthreads << "\t[default]" << std::endl;
      }
   }

   if(isstorage) {         

      // configure FzWriter

      if(cfg.lookupValue("fzdaq.fzwriter.subdir", subdir)) {

         std::cout << INFOTAG << "FzWriter subdir: " << subdir << "\t[cfg file]" << std::endl;

      } else {

         std::cout << ERRTAG << "FzWriter subdir not set" << std::endl;
         exit(1);
      }

      if(cfg.lookupValue("fzdaq.fzwriter.runtag", runtag)) {

         std::cout << INFOTAG << "FzWriter runtag: " << runtag << "\t[cfg file]" << std::endl;

      } else {

         runtag = "run";
         std::cout << INFOTAG << "FzWriter runtag: " << runtag << "\t[default]" << std::endl;
      }
   
      if(cfg.lookupValue("fzdaq.fzwriter.runid", runid)) {
  
          std::cout << INFOTAG << "FzWriter runid: " << runid << "\t[cfg file]" << std::endl;

      } else {

         std::cout << ERRTAG << "FzWriter runid not set" << std::endl;
         exit(1);
      }

      if(cfg.lookupValue("fzdaq.fzwriter.esize", esize)) {

         std::cout << INFOTAG << "FzWriter esize: " << esize << "\t[cfg file]" << std::endl;

      } else {

         esize = 10; // default is 10 MBytes
         std::cout << INFOTAG << "FzWriter esize: " << esize << "\t[default]" << std::endl;
      } 

      if(cfg.lookupValue("fzdaq.fzwriter.dsize", dsize)) {

         std::cout << INFOTAG << "FzWriter dsize: " << dsize << "\t[cfg file]" << std::endl;

      } else {

         dsize = 100; // default is 100 MBytes
         std::cout << INFOTAG << "FzWriter dsize: " << dsize << "\t[default]" << std::endl;
      } 

      // create FzWriter base directory

      int status;
      status = mkdir(subdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

      if(status == 0)
         std::cout << INFOTAG << "FzWriter: " << subdir << " directory created" << std::endl;
      else if(errno == EEXIST)
         std::cout << WARNTAG << "FzWriter: " << subdir << " directory already exist" << std::endl;
      else if(status != 0) {
         std::cout << ERRTAG << "FzWriter: ";
         perror("on base directory: ");
      }
   }

   std::cout << BOLDMAGENTA << "FzDAQ - END configuration" << RESET << std::endl;
   std::cout << std::endl;  

   std::cout << BOLDMAGENTA << "FzDAQ - START threads allocation" << RESET << std::endl;

   if(iscompute) {

#ifdef AMQLOG_ENABLED
      // create FzReader thread
      rd = new FzReader(neturl, cfgfile, context, JMSconn, logbasedir);
#else
      rd = new FzReader(neturl, cfgfile, context, logbasedir);
#endif

      if(!rd) {
         std::cout << ERRTAG << "FzReader: thread allocation failed" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzReader: thread ready" << std::endl;

      // create FzParser threads pool
      for(i=0; i < nthreads; i++) {

#ifdef AMQLOG_ENABLED
         psr_array.push_back(new FzParser(i, cfgfile, context, JMSconn, evformat, logbasedir));
#else
         psr_array.push_back(new FzParser(i, cfgfile, context, evformat, logbasedir));       
#endif

         if(!psr_array[i]) {

            std::cout << ERRTAG << "FzParser: threads allocation failed" << std::endl;
            exit(1);
         }

         psr_array[i]->init();
      }

      std::cout << INFOTAG << "FzParser: threads ready" << std::endl;
  }

   if(isstorage) {

#ifdef AMQLOG_ENABLED
      // create FzWriter thread 
      wr = new FzWriter(subdir, runtag, runid, cfgfile, context, JMSconn, logbasedir);
#else
      wr = new FzWriter(subdir, runtag, runid, cfgfile, context, logbasedir);
#endif

      if(!wr) {
         std::cout << ERRTAG << "FzWriter: thread allocation failed" << std::endl;
         exit(1);
      }

      wr->init();

      if(dsize > esize) {
 
         wr->set_eventfilesize(esize * 1000000);
         wr->set_eventdirsize(dsize * 1000000);

      } else {

         std::cout << ERRTAG << "FzWriter: dsize must be greater than esize" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzWriter: thread ready" << std::endl;
   }

   // create FzNodeManager thread

#ifdef AMQLOG_ENABLED
   nm = new FzNodeManager(rd, psr_array, wr, cfgfile, profile, context, JMSconn);
#else
   nm = new FzNodeManager(rd, psr_array, wr, cfgfile, profile, context);
#endif

   if(!nm) {
      std::cout << ERRTAG << "FzNodeManager: thread allocation failed" << std::endl;
      exit(1);
   }

   nm->init();

   std::cout << INFOTAG << "FzNodeManager: thread ready" << std::endl;

   std::cout << BOLDMAGENTA << "FzDAQ - END threads allocation" << RESET << std::endl;
   std::cout << std::endl;  

   // start command line interface 

   std::cout << BOLDMAGENTA << "FzDAQ - START command line interface " << RESET << std::endl;
   std::cout << std::endl;  

   std::string line, lastcmd;
   bool cmderr;
   RCtransition rcerr;

   for ( ; std::cout << "FzDAQ > " && std::getline(std::cin, line); ) {
        if (!line.empty()) { 

           cmderr = true;

           if(!line.compare("r")) {
              cmderr = false;
              line = lastcmd;
           }

           if(!line.compare("help")) {

              cmderr = false;
              std::cout << std::endl;
              std::cout << "help:   \tprint usage" << std::endl;
              std::cout << "status: \tprint acquisition status" << std::endl;

              if(nm->rc_mode() == std::string(RC_MODE_LOCAL)) {
                 std::cout << "configure:  \tconfigure acquisition" << std::endl;
                 std::cout << "start:  \tconfigure and start acquisition" << std::endl;
                 std::cout << "stop:   \tstop acquisition" << std::endl;
                 std::cout << "reset:   \treset acquisition" << std::endl;
              }

              std::cout << "stats:  \tprint event data acquisition statistics" << std::endl;
              std::cout << "r:      \trepeat last command" << std::endl;
              std::cout << "rec:    \trecord raw data to files" << std::endl;
              std::cout << "quit:   \tquit from FzDAQ" << std::endl;
              std::cout << std::endl;
           }

           if(!line.compare("status")) {
              cmderr = false;
              std::cout << "current DAQ status: " << state_labels[nm->rc_state()] << std::endl;
              std::cout << "Run Control mode = " << nm->rc_mode() << std::endl;
           }

           if(nm->rc_mode() == std::string(RC_MODE_LOCAL)) {

              if(!line.compare("configure")) {

                 cmderr = false;
                 rcerr = nm->rc_process(RCcommand::configure);
 
                 if(rcerr == RCOK)
                    std::cout << INFOTAG << "Fazia DAQ configured" << std::endl;
                 else
                    std::cout << ERRTAG << "configure RC command failed" << std::endl;
              }

              if(!line.compare("start")) {

                 cmderr = false;
                 // to simplify start
                 rcerr = nm->rc_process(RCcommand::configure);
                 rcerr = nm->rc_process(RCcommand::start);

                 if(rcerr == RCOK)
                    std::cout << INFOTAG << "Fazia DAQ configured and started" << std::endl;
                 else
                    std::cout << ERRTAG << "configure/start RC command failed" << std::endl;
              }

              if(!line.compare("stop")) {

                 cmderr = false;
                 rcerr = nm->rc_process(RCcommand::stop);
        
                 if(rcerr == RCOK)
                    std::cout << INFOTAG << "Fazia DAQ stopped" << std::endl;
                 else
                    std::cout << ERRTAG << "stop RC command failed" << std::endl;
              }

              if(!line.compare("reset")) {

                 cmderr = false;
                 nm->rc_process(RCcommand::reset);
                 std::cout << INFOTAG << "Fazia DAQ reset" << std::endl;
              }

           }	// RC_MODE_LOCAL

           if(!line.compare("stats")) {

              cmderr = false;
              std::stringstream in_evbw, out_evbw, in_databw, out_databw;

              if(!nm->has_data()) {

                 std::cout << "FzNodeManager is collecting statistics... try later" << std::endl;
                 lastcmd = line;
                 continue;
              }

              nodereport = nm->get_nodereport(); 
              std::cout << std::endl;

              if(iscompute) {

                 std::stringstream psrname;

                 Report::FzReader rd_report;
                 Report::FzParser psr_report;
                 Report::FzFSM fsm_report;

                 std::cout << std::left << std::setw(15) << "thread" << 
			std::setw(30) << "          IN events" << 
			std::setw(30) << "           IN data" << 
			std::setw(30) << "          OUT events" << 
			std::setw(30) << "           OUT data" << 
		 std::endl;

                 std::cout << std::left << std::setw(15) << "-" << 
		 	std::setw(15) << "     num" << 
		 	std::setw(15) << "    rate" << 
			std::setw(15) << "    size" << 
			std::setw(15) << "     bw" << 
			std::setw(15) << "     num" << 
		 	std::setw(15) << "    rate" << 
			std::setw(15) << "    size" << 
			std::setw(15) << "     bw" << 
		 std::endl;

		 // FzReader statistics
                 rd_report = nodereport.reader();

                 in_evbw << rd_report.in_events_bw() << " ev/s";
                 in_databw << human_byte(rd_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(rd_report.in_bytes_bw() * 8) << "/s)";
                 out_evbw << rd_report.out_events_bw() << " ev/s";
                 out_databw << human_byte(rd_report.out_bytes_bw()) << "/s"; // << " (" << human_bit(rd_report.out_bytes_bw() * 8) << "/s)";

                 std::cout << std::left << std::setw(15) << "FzReader" <<
			std::setw(15) << std::right << rd_report.in_events() <<
			std::setw(15) << std::right << in_evbw.str() <<
			std::setw(15) << std::right << human_byte(rd_report.in_bytes()) <<
			std::setw(15) << std::right << in_databw.str() <<
                        std::setw(15) << std::right << rd_report.out_events() <<
                        std::setw(15) << std::right << out_evbw.str() <<
                        std::setw(15) << std::right << human_byte(rd_report.out_bytes()) <<
                        std::setw(15) << std::right << out_databw.str() <<
		 std::endl;

                 in_evbw.str("");
                 in_databw.str("");
                 out_evbw.str("");
                 out_databw.str("");
 
		 std::cout << "---" << std::endl;

                 for(i=0; i<nthreads; i++) {
 
		    // FzParser statistics
                    psr_report = nodereport.parser(i);

                    in_evbw << psr_report.in_events_bw() << " ev/s";
                    in_databw << human_byte(psr_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(psr_report.in_bytes_bw() * 8) << "/s)"; 
                    out_evbw << psr_report.in_events_bw() << " ev/s";
                    out_databw << human_byte(psr_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(psr_report.in_bytes_bw() * 8) << "/s)"; 

                    psrname << "FzParser #" << i; 
		    std::cout << std::left << std::setw(15) << psrname.str() <<
                    	std::setw(15) << std::right << psr_report.in_events() <<
	                std::setw(15) << std::right << in_evbw.str() <<
	                std::setw(15) << std::right << human_byte(psr_report.in_bytes()) <<
	                std::setw(15) << std::right << in_databw.str() <<
	                std::setw(15) << std::right << psr_report.out_events() <<
	                std::setw(15) << std::right << out_evbw.str() <<
	                std::setw(15) << std::right << human_byte(psr_report.in_bytes()) <<
	                std::setw(15) << std::right << out_databw.str() <<
	            std::endl;

                    psrname.str("");	// clear stringstream
		    in_evbw.str("");
                    in_databw.str("");
                    out_evbw.str("");
                    out_databw.str("");

                    // FSM statistics
                    fsm_report = nodereport.fsm(i);

                    in_evbw << fsm_report.in_events_bw() << " ev/s";
                    in_databw << human_byte(fsm_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 
                    out_evbw << fsm_report.in_events_bw() << " ev/s";
                    out_databw << human_byte(fsm_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 

                    std::cout << std::left << std::setw(15) << "  FSM" <<
			std::setw(15) << std::right << fsm_report.in_events() <<
	                std::setw(15) << std::right << in_evbw.str() <<
	                std::setw(15) << std::right << human_byte(fsm_report.in_bytes()) <<
	                std::setw(15) << std::right << in_databw.str() <<
	                std::setw(15) << std::right << fsm_report.out_events() <<
	                std::setw(15) << std::right << out_evbw.str() <<
	                std::setw(15) << std::right << human_byte(fsm_report.in_bytes()) <<
	                std::setw(15) << std::right << out_databw.str() <<
	            std::endl;

                    psrname.str("");    // clear stringstream
                    in_evbw.str("");
                    in_databw.str("");
                    out_evbw.str("");
                    out_databw.str("");

		    std::cout << "---" << std::endl;
                 } 
              }			// end if(iscompute)

              if(isstorage) {

                 // FzWriter statistics
                 Report::FzWriter wr_report;

                 wr_report = nodereport.writer();

                 in_evbw << wr_report.in_events_bw() << " ev/s";
                 in_databw << human_byte(wr_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 
                 out_evbw << wr_report.in_events_bw() << " ev/s";
                 out_databw << human_byte(wr_report.in_bytes_bw()) << "/s"; // << " (" << human_bit(fsm_report.in_bytes_bw() * 8) << "/s)"; 

                 std::cout << std::left << std::setw(15) << "FzWriter" <<
                     std::setw(15) << std::right << wr_report.in_events() <<
                     std::setw(15) << std::right << in_evbw.str() <<
                     std::setw(15) << std::right << human_byte(wr_report.in_bytes()) <<
                     std::setw(15) << std::right << in_databw.str() <<
                     std::setw(15) << std::right << wr_report.out_events() <<
                     std::setw(15) << std::right << out_evbw.str() <<
                     std::setw(15) << std::right << human_byte(wr_report.in_bytes()) <<
                     std::setw(15) << std::right << out_databw.str() <<
                 std::endl;

                 in_evbw.str("");
                 in_databw.str("");
                 out_evbw.str("");
                 out_databw.str("");

                 std::cout << "---" << std::endl;
              }
           }

           if(!line.compare("rec")) {

              cmderr = false;
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
              cmderr = false;
              break;
           }

           if(cmderr == true) {
              std::cout << ERRTAG << "command not found" << std::endl;
           } else lastcmd = line;
        }
    }

   std::cout << INFOTAG << "FzNodeManager: closing thread: \t";
   nm->close();
   nm->rc_process(RCcommand::stop);
   std::cout << " DONE" << std::endl;

   if(iscompute) {
   
      std::cout << INFOTAG << "FzReader: closing thread: \t";
      rd->close();
      std::cout << " DONE" << std::endl;

      std::cout << INFOTAG << "FzParser: closing threads: \t";
      for(i=0; i < nthreads; i++) {
         psr_array[i]->close();
      }
      std::cout << " DONE" << std::endl;
   }

   if(isstorage) {

      std::cout << INFOTAG << "FzWriter: closing thread: \t";
      wr->close();
      std::cout << " DONE" << std::endl; 
   }
   
   context.close();
   std::cout << INFOTAG << "ZeroMQ shutdown" << std::endl;

   std::cout << "Goodbye.\n";

   return(0);
}

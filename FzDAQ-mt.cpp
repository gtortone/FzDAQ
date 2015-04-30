#include <iostream>

#include "boost/program_options.hpp"
#include "boost/thread.hpp"
#include <log4cpp/PropertyConfigurator.hh>

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "FzEventSet.pb.h"

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzUtils.h"
#include "FzLogger.h"
#include "FzNodeManager.h"
#include "FzReader.h"
#include "FzParser.h"
#include "FzWriter.h"
#include "zmq.hpp"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

   zmq::context_t context(1);	// for ZeroMQ communications

   unsigned int i;
   std::string devname;
   std::string neturl;
   unsigned int nthreads;
   std::string subdir;
   std::string runtag;
   uint32_t runid;
   bool subid = false;
   uint32_t esize, dsize;

   libconfig::Config cfg;
   bool hascfg = false;
   bool iscompute = false;
   bool isstorage = false;
   std::string cfgfile;
   std::string profile;
   std::string netdev, devip;
   unsigned int port;

   FzReader *rd;
   std::vector<FzParser *> psr_array;	// vector of (Fzparser *)
   FzWriter *wr;
   FzNodeManager *mon;

   Report::Node nodereport;

   bool rec = false;
   DAQstatus_t DAQstatus = STOP;

   log4cpp::Priority::Value logparser_prio;

   // configure log 
   log4cpp::PropertyConfigurator::configure("log4cpp.properties");
   logparser_prio = log4cpp::Category::getInstance(std::string("fzparser")).getChainedPriority();

   // handling of command line parameters
   po::options_description desc("\nFzDAQ - allowed options", 100);
   
   desc.add_options()
    ("help", "produce help message")
    ("dev", po::value<std::string>(), "acquisition device {usb, net} (default: usb)")
    ("neturl", po::value<std::string>(), "network UDP consumer url (default: udp://eth0:50000)")
    ("nt", po::value<unsigned int>(), "number of parser threads\ndefault: 1")
    ("subdir", po::value<std::string>(), "base output directory")
    ("runtag", po::value<std::string>(), "label for run directory identification (e.g. LNS, GANIL)\ndefault: run")
    ("runid", po::value<uint32_t>(), "id for run identification (e.g. 100, 205)")
    ("esize", po::value<uint32_t>(), "[optional] max size of event file in Mbytes\ndefault: 10 Mb")
    ("dsize", po::value<uint32_t>(), "[optional] max size of event directory in Mbytes\ndefault: 100 Mb")
    ("ensubid", "[optional] enable subid for run identification (eg. run000220.0, run000220.1 ...)")
    ("cfg", po::value<std::string>(), "[optional] configuration file")
    ("profile", po::value<std::string>(), "[optional] profile to start {compute, storage, all}")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);    


   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      //std::cout << "example: ./FzDAQ-mt --subdir pbout --runid 150" << std::endl;
      //std::cout << " 'pbout' directory will contain event subdirectories starting from 'run000150'" << std::endl;
      //std::cout << " each eventset file has a max size of 10 MB (default) and each event subdirectory has a max size of 100 MB (default)" << std::endl << std::endl;
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

   /*
     options will be set by priority:
        - command line parameter
        - configuration file
        - default value
   */

   std::cout << INFOTAG << "FzDAQ profile selected: " << profile << std::endl;

   if(iscompute) {		

      devname = neturl = "";

      // configure FzReader and FzParser

      if(vm.count("dev")) {

        devname = vm["dev"].as<std::string>();
        if( (devname == "usb") || (devname == "net") ) {

           std::cout << INFOTAG << "FzReader device: " << devname << "\t[cmd param]" << std::endl;

        } else {

           std::cout << ERRTAG << "FzReader device unknown: " << devname << "\t[cmd param]" << std::endl;
           exit(1);
        }

      } else if(cfg.lookupValue("fzdaq.fzreader.consumer.device", devname)) {

         std::cout << INFOTAG << "FzReader device: " << devname << "\t[cfg file]" << std::endl;

      } else {

         devname = "usb";
         std::cout << INFOTAG << "FzReader device: usb\t[default]" << std::endl;
      }

      if(devname == "net") {  	// fetch neturl parameter

         if(vm.count("neturl")) {

            neturl = vm["neturl"].as<std::string>();
            if(neturl.substr(0,3) == "udp") {

               std::cout << INFOTAG << "FzReader network UDP url: " << neturl << "\t[cmd param]" << std::endl;

            } else {

               std::cout << ERRTAG << "FzReader network UDP url not valid: " << neturl << "\t[cmd param]" << std::endl;
               exit(1);
            }

         } else if(cfg.lookupValue("fzdaq.fzreader.consumer.url", neturl)) {

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
      }

      if (vm.count("nt")) {

         nthreads = vm["nt"].as<unsigned int>();
         std::cout << INFOTAG << "FzParser nthreads: " << nthreads << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzparser.nthreads", nthreads)) {

         std::cout << INFOTAG << "FzParser nthreads: " << nthreads << "\t[cfg file]" << std::endl;

      } else {

         nthreads = 1;
         std::cout << INFOTAG << "FzParser nthreads: " << nthreads << "\t[default]" << std::endl;
      }
   }

   if(isstorage) {         

      // configure FzWriter

      if (vm.count("subdir")) {

         subdir = vm["subdir"].as<std::string>();
         std::cout << INFOTAG << "FzWriter subdir: " << subdir << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzwriter.subdir", subdir)) {

         std::cout << INFOTAG << "FzWriter subdir: " << subdir << "\t[cfg file]" << std::endl;

      } else {

         std::cout << ERRTAG << "FzWriter subdir not set" << std::endl;
         exit(1);
      }

      if (vm.count("runtag")) {

         runtag = vm["runtag"].as<std::string>();
         std::cout << INFOTAG << "FzWriter runtag: " << runtag << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzwriter.runtag", runtag)) {

         std::cout << INFOTAG << "FzWriter runtag: " << runtag << "\t[cfg file]" << std::endl;

      } else {

         runtag = "run";
         std::cout << INFOTAG << "FzWriter runtag: " << runtag << "\t[default]" << std::endl;
      }
   
      if (vm.count("runid")) {

         //runid = vm["runid"].as<unsigned long int>();
         runid = vm["runid"].as<uint32_t>();
         std::cout << INFOTAG << "FzWriter runid: " << runid << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzwriter.runid", runid)) {
  
          std::cout << INFOTAG << "FzWriter runid: " << runid << "\t[cfg file]" << std::endl;

      } else {

         std::cout << ERRTAG << "FzWriter runid not set" << std::endl;
         exit(1);
      }

      if (vm.count("esize")) {

         esize = vm["esize"].as<uint32_t>();
         std::cout << INFOTAG << "FzWriter esize: " << esize << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzwriter.esize", esize)) {

         std::cout << INFOTAG << "FzWriter esize: " << esize << "\t[cfg file]" << std::endl;

      } else {

         esize = 10; // default is 10 MBytes
         std::cout << INFOTAG << "FzWriter esize: " << esize << "\t[default]" << std::endl;
      } 

      if (vm.count("dsize")) {

         esize = vm["dsize"].as<uint32_t>();
         std::cout << INFOTAG << "FzWriter dsize: " << dsize << "\t[cmd param]" << std::endl;

      } else if(cfg.lookupValue("fzdaq.fzwriter.dsize", dsize)) {

         std::cout << INFOTAG << "FzWriter dsize: " << dsize << "\t[cfg file]" << std::endl;

      } else {

         dsize = 100; // default is 100 MBytes
         std::cout << INFOTAG << "FzWriter dsize: " << dsize << "\t[default]" << std::endl;
      } 

      if (vm.count("ensubid"))
         subid = true;
      else
         subid = false;

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

      // create FzReader thread
      rd = new FzReader(devname, neturl, cfgfile, context);

      if(!rd) {
         std::cout << ERRTAG << "FzReader: thread allocation failed" << std::endl;
         exit(1);
      }

      rd->set_status(STOP);

      std::cout << INFOTAG << "FzReader: thread ready" << std::endl;

      // create FzParser threads pool
      for(i=0; i < nthreads; i++) {

         psr_array.push_back(new FzParser(i, logparser_prio, cfgfile, context));

         if(!psr_array[i]) {

            std::cout << ERRTAG << "FzParser: threads allocation failed" << std::endl;
            exit(1);
         }

         psr_array[i]->init();
         psr_array[i]->set_status(STOP);
      }

      std::cout << INFOTAG << "FzParser: threads ready" << std::endl;
  }

   if(isstorage) {

      // create FzWriter thread 
      wr = new FzWriter(subdir, runtag, runid, subid, cfgfile, context);

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

      wr->set_status(STOP);

      std::cout << INFOTAG << "FzWriter: thread ready" << std::endl;
   }

   // create FzNodeManager thread

   mon = new FzNodeManager(rd, psr_array, wr, cfgfile, profile, context);

   if(!mon) {
      std::cout << ERRTAG << "FzNodeManager: thread allocation failed" << std::endl;
      exit(1);
   }

   mon->init();
   mon->set_status(START);

   std::cout << INFOTAG << "FzNodeManager: thread ready" << std::endl;

   std::cout << BOLDMAGENTA << "FzDAQ - END threads allocation" << RESET << std::endl;
   std::cout << std::endl;  

   // start command line interface 

   std::cout << BOLDMAGENTA << "FzDAQ - START command line interface " << RESET << std::endl;
   std::cout << std::endl;  

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

           if(!line.compare("status")) {
              std::cout << "current DAQ status: " << state_labels[DAQstatus] << std::endl;
              //std::cout << "file eventset max size: " << esize/1000000 << " MB" << std::endl;
              //std::cout << "directory eventset max size: " << dsize/1000000 << " MB" << std::endl;
           }

           if(!line.compare("start")) {

              if(iscompute) {

                 std::cout << "sending start to FzReader and " << nthreads << "xFzParser" << std::endl;
                 rd->set_status(START);
 
                 for(i=0; i < nthreads; i++)
                    psr_array[i]->set_status(START);
              }
 
              if(isstorage) {

                 std::cout << "sending start to FzWriter" << std::endl;
                 wr->set_status(START);
              }

              DAQstatus = START;
           }

           if(!line.compare("stop")) {

              if(iscompute) {

                 std::cout << "sending stop to FzReader and " << nthreads << "xFzParser" << std::endl;
                 rd->set_status(STOP);

                 for(i=0; i < nthreads; i++)
                    psr_array[i]->set_status(STOP);
              }

              if(isstorage) {

                 std::cout << "sending stop to FzWriter" << std::endl;
                 wr->set_status(STOP);
              }

              DAQstatus = STOP;
           }

          if(!line.compare("stats")) {

              std::stringstream in_evbw, out_evbw, in_databw, out_databw;

              if(!mon->has_data()) {

                 std::cout << "FzNodeManager is collecting statistics... try later" << std::endl;
                 lastcmd = line;
                 continue;
              }

              nodereport = mon->get_nodereport(); 
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

                 std::cout << std::left << std::setw(15) << "FzReader" <<
			std::setw(15) << std::right << rd_report.in_events() <<
			std::setw(15) << std::right << in_evbw.str() <<
			std::setw(15) << std::right << human_byte(rd_report.in_bytes()) <<
			std::setw(15) << std::right << in_databw.str() <<
		 std::endl;

                 in_evbw.str("");
                 in_databw.str("");
 
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
              DAQstatus = QUIT;
              break;
           }

           lastcmd = line;
        }
    }

   std::cout << INFOTAG << "FzNodeManager: closing thread: \t";
   mon->close();
   std::cout << " DONE" << std::endl;

   if(iscompute) {
   
      std::cout << INFOTAG << "FzReader: closing thread: \t";
      rd->set_status(STOP);
      rd->close();
      std::cout << " DONE" << std::endl;

      std::cout << INFOTAG << "FzParser: closing threads: \t";
      for(i=0; i < nthreads; i++) {		// STOP and CLOSE each FzParser to do a fast exit
         psr_array[i]->set_status(STOP);
      }
      for(i=0; i < nthreads; i++) {
         psr_array[i]->close();
      }
      std::cout << " DONE" << std::endl;
   }

   if(isstorage) {

      std::cout << INFOTAG << "FzWriter: closing thread: \t";
      wr->set_status(STOP);
      wr->close();
      std::cout << " DONE" << std::endl; 
   }
   
   context.close();
   std::cout << INFOTAG << "ZeroMQ shutdown" << std::endl;

   std::cout << "Goodbye.\n";

   return(0);
}

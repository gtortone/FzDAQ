#include "FzWriter.h"
#include "logger/FzLogger.h"
#include "utils/FzUtils.h"	// debug

#include <fstream>
#include <sstream>
#include <dirent.h>

#ifdef AMQLOG_ENABLED
FzWriter::FzWriter(std::string bdir, std::string run, long int id, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn, std::string logdir) :
   context(ctx), AMQconn(JMSconn), logbase(logdir) {
#else
FzWriter::FzWriter(std::string bdir, std::string run, long int id, std::string cfgfile, zmq::context_t &ctx, std::string logdir) :
   context(ctx), logbase(logdir) {
#endif

   int status;
   std::stringstream msg;

   basedir = bdir;
   runtag = run;
   dirid = id;

   log.setFileConnection("fzwriter", logbase + "/fzwriter.log");

#ifdef AMQLOG_ENABLED
   if(AMQconn.get())
      log.setJMSConnection("FzWriter", JMSconn);
#endif

   cfg.readFile(cfgfile.c_str());    // syntax checks on main

   pb = new FzProtobuf();
   
   // prevent init method run thread
   rcstate = IDLE;
   thread_init = false;

   start_newdir = true;
   status = setup_newdir();

   if(status == DIR_FAIL) {

      msg << "directory " << dirstr << " filesystem  error";
      log.write(ERROR, msg.str());
   
   } else {
   
      msg << "directory " << dirstr << " successfully allocated for new event files";
      log.write(INFO, msg.str());
   }

   setup_newfile();

   log.write(INFO, "thread allocated");

   std::string ep;

   try {

      writer = new zmq::socket_t(context, ZMQ_PULL);
      int tval = 1000;
      writer->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));		// 1 second timeout on recv

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzWriter: failed to start ZeroMQ consumer: " << e.what () << std::endl;
      exit(1);

   }

   ep = getZMQEndpoint(cfg, "fzdaq.fzwriter.consumer");
      
   if(ep.empty()) {

      ep = "inproc://fzwriter";
      std::cout << INFOTAG << "FzWriter: consumer endpoint: " << ep << " [default]" << std::endl;

   } else std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [cfg file]" << std::endl;

   try {

      writer->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzWriter: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   try {

      pub = new zmq::socket_t(context, ZMQ_PUB);
      int linger = 0;
      pub->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));   // linger equal to 0 for a fast socket shutdown

   } catch (zmq::error_t &e) {

      std::cout << ERRTAG << "FzWriter: failed to start ZeroMQ event spy: " << e.what () << std::endl;
      exit(1);

   }

   std::string netint;
   if(cfg.lookupValue("fzdaq.fzwriter.spy.interface", netint)) {

      std::cout << INFOTAG << "FzWriter: event spy network interface: " << netint << " [cfg file]" << std::endl;
      ep = "tcp://" + netint + ":" + std::to_string(FZW_SPY_PORT);
      std::cout << INFOTAG << "FzWriter: event spy endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://*:" + std::to_string(FZW_SPY_PORT);
      std::cout << INFOTAG << "FzWriter: event spy endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      pub->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzWriter: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   std::cout << INFOTAG << "FzWriter: initialization done" << std::endl;

   report.Clear();
};

void FzWriter::close(void) {
   thr->interrupt();
   thr->join();
}

void FzWriter::init(void) {

    if (!thread_init) {

      thread_init = true;
      thr = new boost::thread(boost::bind(&FzWriter::process, this));
   }
}

void FzWriter::set_eventfilesize(unsigned long int size) {
   event_file_size = size;
};

void FzWriter::set_eventdirsize(unsigned long int size) {
   event_dir_size = size;
};

void FzWriter::setup_newfile(void) {

   if(output.good())
      output.close();

   // setup of filename and output stream
   filename.str("");
   filename << dirstr << '/' << "FzEventSet-" << time(NULL) << '-' << fileid << ".pb";
   output.open(filename.str().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

   fileid++;
};

// default argument: (firstrun = false)
int FzWriter::setup_newdir(void) {

   static bool firstrun = true;

   if(firstrun) {

      dirid = get_max_runid();
      firstrun = false;
      std::cout << "get_max_runid() = " << dirid << std::endl;
      exit(0); // FIX
   }
   
   int status = DIR_OK;
   std::ostringstream ss;

   ss << runtag << std::setw(6) << std::setfill('0') << dirid;

   dirstr = basedir + '/' + ss.str();

   // create directory
   errno = 0;
   mkdir(dirstr.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

   if(errno == EEXIST)
      status = DIR_EXISTS;
   else if(status != 0)
      status = DIR_FAIL;

   // prepare for next directory name
   dirid++;

   return(status);
}

int FzWriter::get_max_runid(void) {

   DIR *dir = opendir(basedir.c_str());

   struct dirent *entry = readdir(dir);

   int num = dirid;
   int max = dirid;
   while (entry != NULL) {
      if (entry->d_type == DT_DIR) {
         if( strstr(entry->d_name, runtag.c_str()) != NULL) {
            sscanf(entry->d_name, "%*[^0123456789]%d", &num);
            if (num > max)
               max = num;
         }
      }
      entry = readdir(dir);
   }  // end while

   closedir(dir);

   return max;
}

void FzWriter::process(void) {

   while(true) {

      try {

         boost::this_thread::interruption_point();

         if(rcstate == RUNNING) {

            bool rc;
            zmq::message_t message;

            rc = writer->recv(&message);

            if(rc) {

               report.set_in_bytes( report.in_bytes() + message.size() );
               report.set_in_events( report.in_events() + 1 );

               std::string msg_str(static_cast<char*>(message.data()), message.size());
               pb->WriteDataset(output, msg_str);

               report.set_out_bytes( report.out_bytes() + message.size() );
               report.set_out_events( report.out_events() + 1 );

               esize += message.size();
               dsize += message.size();
         
               if(dsize > event_dir_size) {

                  setup_newdir();
                  setup_newfile();
                  esize = dsize = 0;

               } else if(esize > event_file_size) {

                  setup_newfile();
                  esize = 0;
               }

               // to fazia-spy	-- at this point due to 'move' semantic on message
               pub->send(message);
            }

            boost::this_thread::interruption_point();

         } else {

            boost::this_thread::sleep(boost::posix_time::seconds(1));
            boost::this_thread::interruption_point();

         } 

      } catch(boost::thread_interrupted& interruption) { 

         writer->close();
         pub->close();
         break;

      } catch(std::exception& e) {

      }

   } // end while 
};

Report::FzWriter FzWriter::get_report(void) {
   return(report);
};

void FzWriter::set_rcstate(RCstate s) {

   if( ( (rcstate == PAUSED) || (rcstate == READY) ) && (s == RUNNING) ) {

      if(!start_newdir) {

         // at every start a new run must be allocated
         // PAUSED -> start -> RUNNING
         // READY  -> start -> RUNNING
         setup_newdir();
         setup_newfile();
         esize = dsize = 0;
      }

      start_newdir = false;     // prevent first 'start' create a new run directory
   }

   rcstate = s;
}

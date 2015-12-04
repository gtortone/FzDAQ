#include "FzWriter.h"
#include "FzLogger.h"
#include "FzUtils.h"	// debug

#include <fstream>
#include <sstream>

FzWriter::FzWriter(std::string bdir, std::string run, long int id, bool subid, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn) :
   context(ctx), AMQconn(JMSconn) {

   int status;

   basedir = bdir;
   runtag = run;
   dirid = id;
   this->subid = subid;

   log.setFileConnection("fzwriter", "/var/log/fzdaq/fzwriter.log");

   if(AMQconn.get())
      log.setJMSConnection("FzWriter", JMSconn);

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;

   pb = new FzProtobuf();
   
   // prevent init method run thread
   rcstate = IDLE;
   thread_init = false;

   status = setup_newdir();

   if(status == DIR_EXISTS) {

      std::stringstream msg;
      msg << "directory " << dirstr << " already exists - searching for new directory name...";
      log.write(WARN, msg.str());

      while(setup_newdir() == DIR_EXISTS)
         ;
      
      msg.str("");
      msg << "directory " << dirstr << " successfully allocated for new event files";
      log.write(INFO, msg.str());

   } else if(status == DIR_FAIL) {

      std::stringstream msg;
      msg << "directory " << dirstr << " filesystem  error";
      log.write(ERROR, msg.str());
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

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzwriter.consumer");
      
      if(ep.empty()) {

         std::cout << ERRTAG << "FzWriter: consumer endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzParser: consumer endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "inproc://fzwriter";
      std::cout << INFOTAG << "FzWriter: consumer endpoint: " << ep << " [default]" << std::endl;
 
   }

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

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzwriter.spy");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzWriter: event spy endpoint not present in config file" << std::endl;
         exit(1);
      }
   
      std::cout << INFOTAG << "FzParser: event spy endpoint: " << ep << " [cfg file]" << std::endl;

   } else {
   
      ep = "tcp://*:5563";
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

int FzWriter::setup_newdir(void) {

   int status = DIR_OK;
   std::ostringstream ss;

   if(!subid)
      ss << runtag << std::setw(6) << std::setfill('0') << dirid;
   else
      ss << runtag << std::setw(6) << std::setfill('0') << dirid << '.' << dirsubid;

   dirstr = basedir + '/' + ss.str();

   // create directory
   errno = 0;
   mkdir(dirstr.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

   if(errno == EEXIST)
      status = DIR_EXISTS;
   else if(status != 0)
      status = DIR_FAIL;

   // prepare for next directory name
   if(subid)
      dirsubid++;
   else
      dirid++;

   return(status);
}

void FzWriter::process(void) {

   while(true) {

      try {

         boost::this_thread::interruption_point();

         if(rcstate == RUNNING) {

            bool rc;
            static unsigned long esize;	// current eventset file dize
            static unsigned long dsize;	// current eventset directory size

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

void FzWriter::rc_do(RCcommand cmd) {

}

void FzWriter::set_rcstate(RCstate s) {

   rcstate = s;
}

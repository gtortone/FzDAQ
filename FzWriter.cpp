#include "FzWriter.h"
#include "FzLogger.h"

FzWriter::FzWriter(FzCbuffer<DAQ::FzEvent> &cb, std::string dir, std::string run, std::string step) :
   cbw(cb),
   logwriter(log4cpp::Category::getInstance("fzwriter")) {

   pb = new FzProtobuf(dir, run, step);

   event_file_size = 1000000LL;
   pb->setup_newfile();

   appender = new log4cpp::FileAppender("fzwriter", "logs/fzwriter.log");
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   logwriter.addAppender(appender);

   logwriter << INFO << "FzWriter::constructor - success";

   thread_init = false;
   status = STOP;
};

int FzWriter::init(void) {

    if (!thread_init) {

      thread_init = true;
      thr = new boost::thread(boost::bind(&FzWriter::process, this));
   }
}

void FzWriter::set_status(enum DAQstatus_t val) {

   status = val;
}

void FzWriter::set_eventfilesize(unsigned long int size) {
   event_file_size = size;
};

void FzWriter::process(void) {

   static zmq::context_t zmq_ctx(1);
   static zmq::socket_t zmq_pub(zmq_ctx, ZMQ_PUB);

   static std::string str;

   zmq_pub.bind("tcp://*:5563");

   while(true) {

      if(status == START) {

         static unsigned long size;

         signed int supp;
         unsigned long i;
         DAQ::FzEventSet eventset;
         DAQ::FzEvent *tmpev;
         DAQ::FzEvent event;

         // local copy 
         event = cbw.receive();

         event.SerializeToString(&str);

         int sz = str.length();
         zmq::message_t *query = new zmq::message_t(sz);
         memcpy(query->data(), str.c_str(), sz);
         zmq_pub.send(*query);
 
         tmpev = eventset.add_ev();
         tmpev->MergeFrom(event);

         pb->WriteDataset(eventset);

         size += eventset.ByteSize();
         if(size > event_file_size) {
            pb->setup_newfile();
            size = 0;
         }

         eventset.Clear(); 

      } else if(status == STOP) {

           boost::this_thread::sleep(boost::posix_time::seconds(1));

      } else if(status == QUIT) {

           break;
      }
   }
};

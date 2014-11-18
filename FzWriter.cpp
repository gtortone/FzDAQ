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

   /* sock = new TSocket("localhost", 9090);
   sock->SetCompressionLevel(1);  */

   // hist = new TH1I("hist","This is the px distribution",500,0,500); 

}

void FzWriter::set_status(enum DAQstatus_t val) {

   status = val;
}

void FzWriter::set_eventfilesize(unsigned long int size) {
   event_file_size = size;
};

void FzWriter::process(void) {

   /* static TMessage mess;
   static std::vector<unsigned int> wave; 

   mess.SetWhat(kMESS_OBJECT);
   TMessage::EnableSchemaEvolutionForAll(kTRUE); */

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
 
/*
         // send waveform to histogram server
 
         const DAQ::FzBlock& rdblk = event.block(0);
         const DAQ::FzFee& rdfee = rdblk.fee(0);
         const DAQ::FzHit& rdhit = rdfee.hit(2);
         const DAQ::FzData& rdata = rdhit.data(0);

         if(rdata.has_waveform()) {

            const DAQ::Waveform& rwf = rdata.waveform(); 

            for(int n=0; n < rwf.sample_size(); n++) {

               signed int fval;

               if(rdata.type() != DAQ::FzData::ADC) {

                  if(rwf.sample(n) > 8191)
                     supp = rwf.sample(n) | 0xFFFFC000;
                  else
                     supp = rwf.sample(n);

               } else supp = rwf.sample(n);

               if(n==0)
                  fval = supp;
               else
                  supp -= fval;

               //if(n<195) {
                  hist->SetBinContent(n,supp);
                  wave.push_back(supp);
               //}
            }

            mess.WriteObject(hist);     // write object in message buffer
            sock->Send(mess);          // send message

            mess.Reset();
 
            //mess.WriteObject((TObject *) &wave);     // write object in message buffer
            //sock->Send(mess);          // send message
            
            mess.Reset();

            //sock->Send("Finished");          // tell server we are finished
            hist->Reset();
            wave.clear();
         }
*/
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

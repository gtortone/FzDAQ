#ifndef FZWRITER_H_
#define FZWRITER_H_

#include <boost/thread.hpp>

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "utils/FzProtobuf.h"
#include "proto/FzNodeReport.pb.h"
#include "logger/FzLogger.h"
#include "utils/zmq.hpp"
#include "utils/FzTypedef.h"

class FzWriter {

private:

   libconfig::Config cfg;

   boost::thread *thr;
   bool thread_init;
   bool start_newdir;
   zmq::context_t &context;
   zmq::socket_t *writer;
   zmq::socket_t *pub;

   FzProtobuf *pb;

   FzLogger log;
   std::string logbase;
  
   unsigned long int event_file_size;
   unsigned long int event_dir_size;
   unsigned long int esize, dsize;

   std::string basedir;
   std::string runtag;

   unsigned int fileid;
   unsigned int dirid;
   std::string dirstr;
   bool store;

   std::stringstream filename;
   std::fstream output; 

   Report::FzWriter report;

   RCstate rcstate;

   void process(void);

public:

   FzWriter(std::string bdir, std::string run, long int id, std::string cfgfile, zmq::context_t &ctx, std::string logdir);

   void init(void);
   void close(void);

   void set_eventfilesize(unsigned long int size);
   void set_eventdirsize(unsigned long int size);

   void setup_newfile(void);
   int setup_newdir();
   int get_max_runid(void);
   
   void set_store(bool value);
   bool get_store(void);

   Report::FzWriter get_report(void);

   void set_rcstate(RCstate s);
};

#endif

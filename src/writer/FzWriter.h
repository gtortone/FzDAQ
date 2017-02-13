#ifndef FZWRITER_H_
#define FZWRITER_H_

#include <boost/thread.hpp>

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "utils/FzProtobuf.h"
#include "proto/FzNodeReport.pb.h"
#include "logger/FzJMS.h"
#include "logger/FzLogger.h"
#include "utils/zmq.hpp"
#include "utils/FzTypedef.h"

class FzWriter {

private:

   libconfig::Config cfg;
   bool hascfg;

   boost::thread *thr;
   bool thread_init;
   bool start_newdir;
   zmq::context_t &context;
   zmq::socket_t *writer;
   zmq::socket_t *pub;

   FzProtobuf *pb;

   FzLogger log;
   std::unique_ptr<cms::Connection> AMQconn;
  
   unsigned long int event_file_size;
   unsigned long int event_dir_size;
   unsigned long int esize, dsize;

   std::string basedir;
   std::string runtag;
   bool subid;

   unsigned int fileid;
   unsigned int dirid;
   unsigned int dirsubid;
   std::string dirstr;

   std::stringstream filename;
   std::fstream output; 

   Report::FzWriter report;

   RCstate rcstate;

   void process(void);

public:
   FzWriter(std::string bdir, std::string run, long int id, bool subid, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn);

   void init(void);
   void close(void);

   void set_eventfilesize(unsigned long int size);
   void set_eventdirsize(unsigned long int size);

   void setup_newfile(void);
   int setup_newdir(void);

   Report::FzWriter get_report(void);

   void rc_do(RCcommand cmd);
   void set_rcstate(RCstate s);
};

#endif

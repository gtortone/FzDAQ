#ifndef FZREADER_H_
#define FZREADER_H_

#include "boost/thread.hpp"

#include "main/FzConfig.h"
#include "utils/FzTypedef.h"
#include "parser/FzParser.h"
#include "proto/FzNodeReport.pb.h"
#include "utils/zmq.hpp"
#include "utils/fossa.h"
#include "logger/FzJMS.h"
#include "logger/FzLogger.h"
#include "utils/FzTypedef.h"

#define CYPRESS_VID     0x04B4
#define CYPRESS_PID     0x8613 

#define EP_IN           (0x08 | LIBUSB_ENDPOINT_IN)
#define USB_TIMEOUT     100    // 100 ms 
#define BUFFER_SIZE     16384

#include <vector>
#ifdef USB_ENABLED
   #include <libusb-1.0/libusb.h>
#endif

struct _cb_data {

   FzLogger *log_ptr;
   zmq::socket_t *reader_ptr;
   Report::FzReader *report_ptr;
   bool rec;
};

class FzReader {

private:

   std::string devname;
   std::string neturl;
   libconfig::Config cfg;
   bool hascfg;

   boost::thread *thr;
   bool thread_init;
   zmq::context_t &context;
   zmq::socket_t *reader;

   RCstate rcstate;

#ifdef USB_ENABLED
   libusb_device_handle *usbh;
   libusb_context *ctx;
   libusb_transfer *xfr;
   unsigned char ubuf[BUFFER_SIZE];
   int usbstatus;
   static void cb_usbin(struct libusb_transfer *xfr);
#endif

   struct _cb_data cb_data;

   FzLogger log;
#ifdef AMQLOG_ENABLED
   std::unique_ptr<cms::Connection> AMQconn;
#endif

   struct ns_mgr udpserver;

   Report::FzReader report;

   int setupUsb(void);
   int setupNet(void);

   int initUsb(void);
   int initNet(void);

   static void udp_handler(struct ns_connection *nc, int ev, void *ev_data);

   void process(void);

public:

#ifdef AMQLOG_ENABLED
   FzReader(std::string dname, std::string nurl, std::string cfgfile, zmq::context_t &ctx, cms::Connection *JMSconn);
#else
   FzReader(std::string dname, std::string nurl, std::string cfgfile, zmq::context_t &ctx);
#endif

   int setup(void);
   int init(void);
   void close(void);
   void record(bool val);

   void rc_do(RCcommand cmd);
   void set_rcstate(RCstate s);

   Report::FzReader get_report(void); 

#ifdef USB_ENABLED
   void usb_close(void);
#endif
};

#endif

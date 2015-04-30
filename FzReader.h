#ifndef FZREADER_H_
#define FZREADER_H_

#include "log4cpp/Appender.hh"
#include <log4cpp/PatternLayout.hh>
#include "log4cpp/FileAppender.hh"
#include <log4cpp/PropertyConfigurator.hh>
#include "boost/thread.hpp"

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzParser.h"
#include "FzNodeReport.pb.h"
#include "zmq.hpp"
#include "fossa.h"

#define CYPRESS_VID     0x04B4
#define CYPRESS_PID     0x8613 

#define EP_IN           (0x08 | LIBUSB_ENDPOINT_IN)
#define USB_TIMEOUT     100    // 100 ms 
#define BUFFER_SIZE     16384

#include <vector>
#include <libusb-1.0/libusb.h>

struct _cb_data {

   log4cpp::Category *logreader_ptr;
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

   libusb_device_handle *usbh;
   libusb_context *ctx;
   libusb_transfer *xfr;
   unsigned char ubuf[BUFFER_SIZE];
   int usbstatus;

   struct _cb_data cb_data;

   log4cpp::Category &logreader;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;

   struct ns_mgr udpserver;

   Report::FzReader report;

   DAQstatus_t status;

   int setupUsb(void);
   int setupNet(void);

   int initUsb(void);
   int initNet(void);

   static void cb_usbin(struct libusb_transfer *xfr);
   static void udp_handler(struct ns_connection *nc, int ev, void *ev_data);

   void process(void);

public:

   FzReader(std::string dname, std::string nurl, std::string cfgfile, zmq::context_t &ctx);

   int setup(void);
   int init(void);
   void close(void);
   void record(bool val);

   void set_status(enum DAQstatus_t val);

   Report::FzReader get_report(void); 

   void usb_close(void);
};

#endif

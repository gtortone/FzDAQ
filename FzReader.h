#ifndef FZREADER_H_
#define FZREADER_H_

#include "log4cpp/Appender.hh"
#include <log4cpp/PatternLayout.hh>
#include "log4cpp/FileAppender.hh"
#include <log4cpp/PropertyConfigurator.hh>
#include "boost/thread.hpp"

#include "FzTypedef.h"
#include "FzEventWord.h"
#include "FzCbuffer.h"
#include "FzParser.h"

#define CYPRESS_VID     0x04B4
#define CYPRESS_PID     0x8613 

#define EP_IN           (0x08 | LIBUSB_ENDPOINT_IN)
#define USB_TIMEOUT     100    // 100 ms 
#define BUFFER_SIZE     16384

#include <vector>
#include <libusb-1.0/libusb.h>

struct cb_data {

   std::clock_t start_time;
   FzCbuffer<FzRawData> *cb_ptr;
   uint64_t *tot_bytes_ptr;
   uint64_t *persec_bytes_ptr;
   bool rec;
};

class FzReader {

private:

   boost::thread *thr;
   bool thread_init;

   FzCbuffer<FzRawData> &cbr;
   uint64_t persec_bytes;
   uint64_t tot_bytes;
 
   libusb_device_handle *usbh;
   libusb_transfer *xfr;
   unsigned char ubuf[BUFFER_SIZE];
   int usbstatus;

   struct cb_data usb_cb_data;

   log4cpp::Category &logreader;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;

   DAQstatus_t status;

   void process(void);

public:

   FzReader(FzCbuffer<FzRawData> &cb);

   int setup(std::string ch);
   int init(void);
   void record(bool val);

   void set_status(enum DAQstatus_t val);

   static void cb_usbin(struct libusb_transfer *xfr);
   void usb_close(void);

   uint64_t get_tot_bytes(void);
   uint64_t get_persec_bytes(void);
};

#endif

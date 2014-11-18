#include "FzReader.h"
#include <fstream>
#include <sstream> 

FzReader::FzReader(FzCbuffer<FzRawData> &cb) : 
   cbr(cb),
   logreader(log4cpp::Category::getInstance("fzreader")) {
  
   appender = new log4cpp::FileAppender("fzreader", "logs/fzreader.log");
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   logreader.addAppender(appender);

   logreader << INFO << "FzReader::constructor - success";
   
   usbstatus = -1;

   thread_init = false;
   status = STOP;
};

void FzReader::record(bool val) {
   usb_cb_data.rec = val;
};

// USB methods

int FzReader::setup(std::string ch) {

   if(ch == "usb") {

      libusb_device **devs; // pointer to pointer of device, used to retrieve a list of devices
      libusb_context *ctx = NULL; // a libusb session
      libusb_device_descriptor desc;

      std::ostringstream oss; // string stream for logging
      ssize_t cnt, i; // holding number of devices in list
      bool found = false;
      int retval;

      retval = libusb_init(&ctx); // initialize a library session

      if (retval < 0) {
         logreader << FATAL << "init error";
         return -1;

      }

      libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

      cnt = libusb_get_device_list(ctx, &devs); // get the list of devices

      if (cnt < 0) {
         logreader << FATAL << "get device error";
         return -1;
      }

      logreader << INFO << cnt << " devices in list";

      for (i = 0; i < cnt; i++) {

         retval = libusb_get_device_descriptor(devs[i], &desc);

         if (retval < 0) {
            logreader << FATAL << "failed to get device descriptor";
            return -1;
         }

         if ((desc.idVendor == CYPRESS_VID) && (desc.idProduct == CYPRESS_PID)) {
            found = true;
            break;
         }
      }

      if (!found) {
         logreader << FATAL << "Cypress USB controller NOT found";
         return -1;
      } else
         logreader << INFO << "Cypress USB controller FOUND";

      retval = libusb_open(devs[i], &usbh);

      if (retval != 0) {
         logreader << FATAL << "error opening Cypress USB device";
         return -1;
      }

      libusb_free_device_list(devs, 1); // free the list, unref the devices in it

      if (libusb_kernel_driver_active(usbh, 0)) {

         retval = libusb_detach_kernel_driver(usbh, 0);
         if (retval != 0) {
            logreader << FATAL << "error detaching Cypress USB device";
            return -1;
         }
      }

      retval = libusb_set_configuration(usbh, 1);
      if (retval != 0) {
         logreader << ERROR << "error configuring Cypress USB interface";
         return -1;
      } else
         logreader << INFO << "USB interface configured";

      retval = libusb_claim_interface(usbh, 0);
      if (retval != 0) {
         logreader << ERROR << "error claiming Cypress USB interface";
         return -1;
      } else
         logreader << INFO << "USB interface claimed";

      retval = libusb_set_interface_alt_setting(usbh, 0, 1);
      if (retval != 0) {
         logreader << ERROR << "error setting interface alternate settings";
         return -1;
      } else
         logreader << INFO <<  "interface settings ok";

      return 0;

   } else if (ch == "file") { return 0; }
}

/* static */ void FzReader::cb_usbin(struct libusb_transfer *xfr) {

   static std::string prefix = "record/event-";
   static unsigned int fileno = 0;
   static double tmp_bw_bytes = 0;

   static FzRawData chunk;
   unsigned int value;

   struct cb_data *pdata = (struct cb_data *) xfr->user_data;

   chunk.reserve(MAX_EVENT_SIZE);

   std::clock_t start = (std::clock_t) pdata->start_time;
   FzCbuffer<FzRawData> *cb = (FzCbuffer<FzRawData> *) pdata->cb_ptr;
   uint64_t *tr_bytes = (uint64_t *) pdata->tot_bytes_ptr;
   uint64_t *bw_bytes = (uint64_t *) pdata->persec_bytes_ptr;
   bool rec = (bool) pdata->rec;

   if(xfr->status == LIBUSB_TRANSFER_COMPLETED) {

      for (int i=0; i < (unsigned)(xfr->actual_length - 1); i=i+2) {

         value = (xfr->buffer[i] << 8) + (xfr->buffer[i + 1]);
         chunk.push_back(value);

         // debug:
         // std::cout << std::setw(4) << std::setfill('0') << std::hex << value << std::endl;

         if( (value & CRCBL_TAG) == CRCBL_TAG ) {

            if(rec) {

               std::ostringstream convert;
               convert << prefix << std::setw(4) << std::setfill('0') << std::hex << chunk[1] << std::dec << "-" << fileno;

               std::string filename = convert.str();

               std::ofstream myfile;
               myfile.open(filename);

               for(int ix=0; ix<chunk.size(); ix++) {
                  myfile << std::setw(4) << std::setfill('0') << std::hex << chunk[ix] << ' ';
               }

               fileno++;
               myfile.close();
            }

               cb->send(chunk);    // send data to thread FzParser 
               chunk.clear();
         }
      }

      *tr_bytes += xfr->actual_length;
      *bw_bytes = *tr_bytes / ( (std::clock() - start) / (double) CLOCKS_PER_SEC);
   }

   if (libusb_submit_transfer(xfr) < 0) {
      std::cout << "error re-submitting URB" << std::endl;
      libusb_free_transfer(xfr);
   }
} 

int FzReader::init(void) {

   usb_cb_data.start_time = std::clock();
   usb_cb_data.cb_ptr = &cbr;
   usb_cb_data.tot_bytes_ptr = &tot_bytes;
   usb_cb_data.persec_bytes_ptr = &persec_bytes;

   xfr = libusb_alloc_transfer(0);

   libusb_fill_bulk_transfer(xfr, usbh, EP_IN, ubuf, BUFFER_SIZE, FzReader::cb_usbin, &usb_cb_data, USB_TIMEOUT);

   usbstatus = libusb_submit_transfer(xfr);

   if ((usbstatus == 0) && (!thread_init) ) {
      
      thread_init = true;
      thr = new boost::thread(boost::bind(&FzReader::process, this));
   }

   return(usbstatus);
};

void FzReader::process(void) {
 
   while(true) {

      if(status == START) {
 
         if( usbstatus || (libusb_handle_events(NULL) != 0) )
            std::cout << "USB fatal error !" << std::endl;

      } else if(status == STOP) {

           boost::this_thread::sleep(boost::posix_time::seconds(1));

      } else if(status == QUIT) {

           break;
      }
   }
};

void FzReader::usb_close(void) {

   libusb_free_transfer(xfr);
};

void FzReader::set_status(enum DAQstatus_t val) {

   status = val;
}

uint64_t FzReader::get_tot_bytes(void) {
   return(tot_bytes);
};

uint64_t FzReader::get_persec_bytes(void) { 
   return(persec_bytes);
};

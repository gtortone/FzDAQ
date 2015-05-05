#include "FzReader.h"
#include "FzTypedef.h"
#include "FzUtils.h"
#include <fstream>
#include <sstream> 

#define REGHDR_FMT              0xF800
#define REGHDR_TAG              0xC800  

FzReader::FzReader(std::string dname, std::string nurl, std::string cfgfile, zmq::context_t &ctx) :
   context(ctx), logreader(log4cpp::Category::getInstance("fzreader")) {
  
   devname = dname;
   neturl = nurl;

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str()); 	// syntax checks on main

   } else hascfg = false;

   appender = new log4cpp::FileAppender("fzreader", "logs/fzreader.log");
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   logreader.addAppender(appender);

   logreader << INFO << "FzReader::constructor - success";
   
   usbstatus = -1;

   std::string ep;

   try {
   
      reader = new zmq::socket_t(context, ZMQ_PUSH);
      int linger = 0;
      reader->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));	// linger equal to 0 for a fast socket shutdown

   }  catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzReader: failed to start ZeroMQ producer: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzreader.producer");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzReader: producer endpoint not present in config file" << std::endl;
         exit(1);
      }
 
      std::cout << INFOTAG << "FzReader: producer endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "inproc://fzreader";
      std::cout << INFOTAG << "FzReader: producer endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      reader->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzReader: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // prevent init method run thread
   status = STOP;
   thread_init = false;

   // setup
   if (setup() != 0) {

      std::cout << ERRTAG << "FzReader: device setup failed" << std::endl;
      exit(1);

   } else std::cout << INFOTAG << "FzReader: device setup done" << std::endl;

   // init
   if (init() != 0) {

      std::cout << ERRTAG << "FzReader: device initialization failed" << std::endl;
      exit(1);

   } else std::cout << INFOTAG << "FzReader: device initialization done" << std::endl;

   report.Clear();
};

void FzReader::close(void) {
   thr->interrupt();
   thr->join();
}

void FzReader::record(bool val) {
   cb_data.rec = val;
};

int FzReader::setup(void) {

   if(devname == "usb")
      return(setupUsb());
   if(devname == "net")
      return(setupNet());

   return(0);
};

int FzReader::setupNet(void) {

   return(0);
};

int FzReader::setupUsb(void) {

   libusb_device **devs; // pointer to pointer of device, used to retrieve a list of devices
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
      std::cout << ERRTAG << "FzReader: USB get device error" << std::endl;
      exit(1);
   }

   logreader << INFO << cnt << " devices in list";

   for (i = 0; i < cnt; i++) {

      retval = libusb_get_device_descriptor(devs[i], &desc);

      if (retval < 0) {
         logreader << FATAL << "failed to get device descriptor";
         std::cout << ERRTAG << "FzReader: USB failed to get device descriptor" << std::endl;
         exit(1);
      }

      if ((desc.idVendor == CYPRESS_VID) && (desc.idProduct == CYPRESS_PID)) {
         found = true;
         break;
      }
   }

   if (!found) {

      logreader << FATAL << "Cypress USB controller NOT found";
      std::cout << ERRTAG << "FzReader: Cypress USB controller NOT found" << std::endl;
      exit(1);

   } else {

      logreader << INFO << "Cypress USB controller FOUND";
      std::cout << INFOTAG << "FzReader: Cypress USB controller FOUND" << std::endl;
   }

   retval = libusb_open(devs[i], &usbh);

   if (retval != 0) {
      logreader << FATAL << "error opening Cypress USB device";
      std::cout << ERRTAG << "FzReader: error opening Cypress USB device" << std::endl;
      exit(1);
   }

   libusb_free_device_list(devs, 1); // free the list, unref the devices in it

   if (libusb_kernel_driver_active(usbh, 0)) {

      retval = libusb_detach_kernel_driver(usbh, 0);
      if (retval != 0) {
         logreader << FATAL << "error detaching Cypress USB device";
         std::cout << ERRTAG << "FzReader: error detaching Cypress USB device" << std::endl;
         exit(1);
      }
   }

   retval = libusb_set_configuration(usbh, 1);
   if (retval != 0) {

      logreader << ERROR << "error configuring Cypress USB interface";
      std::cout << ERRTAG << "FzReader: error configuring Cypress USB interface" << std::endl;
      exit(1);

   } else {
 
      logreader << INFO << "USB interface configured";
      std::cout << INFOTAG << "FzReader: USB interface configured" << std::endl; 
   }

   retval = libusb_claim_interface(usbh, 0);
   if (retval != 0) {

      logreader << ERROR << "error claiming Cypress USB interface";
      std::cout << ERRTAG << "FzReader: error claiming Cypress USB interface" << std::endl;
      exit(1);

   } else {

      logreader << INFO << "USB interface claimed";
      std::cout << INFOTAG << "FzReader: USB interface claimed" << std::endl;
   }

   retval = libusb_set_interface_alt_setting(usbh, 0, 1);
   if (retval != 0) {

      logreader << ERROR << "error setting interface alternate settings";
      std::cout << ERRTAG << "FzReader: error setting interface alternate settings" << std::endl;
      exit(1);

   } else {

      logreader << INFO <<  "interface settings ok";
      std::cout << INFOTAG << "FzReader: interface settings ok" << std::endl;
   }

   return 0;
}

/* static */ void FzReader::cb_usbin(struct libusb_transfer *xfr) {

   static std::string prefix = "record/event-";
   static unsigned int fileno = 0;

   static FzRawData chunk;
   unsigned short int *bufusint;
   unsigned short int value;

   struct _cb_data *pdata = (struct _cb_data *) xfr->user_data;

   chunk.reserve(MAX_EVENT_SIZE);

   log4cpp::Category *logreader = (log4cpp::Category *) pdata->logreader_ptr;
   zmq::socket_t *reader = (zmq::socket_t *) pdata->reader_ptr;
   Report::FzReader *report = (Report::FzReader *) pdata->report_ptr; 
   bool rec = (bool) pdata->rec;

   if(xfr->status == LIBUSB_TRANSFER_COMPLETED) {

      bufusint = reinterpret_cast<unsigned short int*>(xfr->buffer);
      //std::cout << std::setw(4) << std::setfill('0') << std::hex << bufusint[0] << std::endl;

      long int evlen = ((xfr->actual_length/2 * 2) == xfr->actual_length)?xfr->actual_length/2:xfr->actual_length/2 + 1;
      for (long int i=0; i < evlen; i++) {

         value = bufusint[i];
         chunk.push_back(value);
         
         // debug:
         //std::cout << std::setw(4) << std::setfill('0') << std::hex << value << std::endl;

         if( (value & REGHDR_FMT) == REGHDR_TAG ) {

            if(rec) {

               uint16_t ec;

               std::ostringstream convert;

               if( (chunk[0] & 0xE000) != 0xE000 )
	          ec = chunk[1];
               else
                  ec = chunk[0];

               convert << prefix << std::setw(4) << std::setfill('0') << std::hex << ec << std::dec << "-" << fileno;

               std::string filename = convert.str();

               std::ofstream myfile;
               myfile.open(filename);

               for(unsigned int ix=0; ix<chunk.size(); ix++) {
                  myfile << std::setw(4) << std::setfill('0') << std::hex << chunk[ix] << ' ';
               }

               fileno++;
               myfile.close();
            }			// end if rec 

            char *blob_data = reinterpret_cast<char *>(chunk.data());
            long int blob_size = chunk.size() * 2;
            reader->send(blob_data, blob_size);

            report->set_in_events( report->in_events() + 1 );

            chunk.clear();
        }			// end if eoe

      }		// end for

      // if total event received data is greater than maximum discard event
      if(chunk.size() >= MAX_EVENT_SIZE) {
 
         chunk.clear();
         logreader->error("data discarded due to overcoming maximum event size: %d bytes", MAX_EVENT_SIZE);
      }

      report->set_in_bytes( report->in_bytes() + xfr->actual_length );
   }

   if (libusb_submit_transfer(xfr) < 0) {
      std::cout << ERRTAG << "FzReading: re-submitting URB" << std::endl;
      libusb_free_transfer(xfr);
   }
} 

int FzReader::init(void) {

   if(devname == "usb")
      return(initUsb());
   if(devname == "net")
      return(initNet());

   return(0);
};


int FzReader::initUsb(void) {

   cb_data.logreader_ptr = &logreader;
   cb_data.reader_ptr = reader;
   cb_data.report_ptr = &report;

   xfr = libusb_alloc_transfer(0);

   libusb_fill_bulk_transfer(xfr, usbh, EP_IN, ubuf, BUFFER_SIZE, FzReader::cb_usbin, &cb_data, USB_TIMEOUT);

   usbstatus = libusb_submit_transfer(xfr);

   if ((usbstatus == 0) && (!thread_init) ) {
      
      thread_init = true;
      thr = new boost::thread(boost::bind(&FzReader::process, this));
   }

   return(usbstatus);
};

int FzReader::initNet(void) {

   cb_data.logreader_ptr = &logreader;
   cb_data.reader_ptr = reader;
   cb_data.report_ptr = &report;

   ns_mgr_init(&udpserver, NULL);
   udpserver.user_data = &cb_data;

   if(ns_bind(&udpserver, neturl.c_str(), udp_handler) == NULL) {

      std::cout << ERRTAG << "FzReader: unable to bind UDP socket" << std::endl;
      exit(1);
   }

   std::cout << INFOTAG << "FzReader: UDP socket bind successfully" << std::endl;
 
   if(!thread_init) {

      thread_init = true;
      thr = new boost::thread(boost::bind(&FzReader::process, this));
   }

   return(0);
};

/* static */ void FzReader::udp_handler(struct ns_connection *nc, int ev, void *ev_data) {

   static std::string prefix = "record/event-";
   static unsigned int fileno = 0;

   static FzRawData chunk;
   unsigned short int *bufusint;
   unsigned short int value;
   long int evlen;
   int offset;

   struct iobuf *io = &nc->recv_iobuf;

   // retrieve callback data
   struct _cb_data *pdata = (struct _cb_data *) nc->mgr->user_data;

   log4cpp::Category *logreader = (log4cpp::Category *) pdata->logreader_ptr;
   zmq::socket_t *reader= (zmq::socket_t *) pdata->reader_ptr;
   Report::FzReader *report = (Report::FzReader *) pdata->report_ptr; 
   bool rec = (bool) pdata->rec;

   switch (ev) {

      case NS_RECV:

         bufusint = reinterpret_cast<unsigned short int*>(io->buf);
         
         evlen = ((io->len/2 * 2) == io->len)?io->len/2:io->len/2 + 1;

         offset = 2;
         if( (bufusint[0] == 0x8080) || (bufusint[1] == 0x8080) || (bufusint[2] == 0x8080) )
            offset++;

         for (long int i=offset; i < evlen; i++) {

            value = bufusint[i];
            chunk.push_back(value);

            // debug:
            //std::cout << std::setw(4) << std::setfill('0') << std::hex << value << std::endl;

            if( (value & REGHDR_FMT) == REGHDR_TAG ) {

               //std::cout << "FzReader: end of event" << std::endl;

               if(rec) {

                  std::ostringstream convert;
                  convert << prefix << std::setw(4) << std::setfill('0') << std::hex << chunk[1] << std::dec << "-" << fileno;

                  std::string filename = convert.str();

                  std::ofstream myfile;
                  myfile.open(filename);

                  for(unsigned int ix=0; ix<chunk.size(); ix++) {
                     myfile << std::setw(4) << std::setfill('0') << std::hex << chunk[ix] << ' ';
                  }

                  fileno++;
                  myfile.close();
               }                   // end if rec 

               char *blob_data = reinterpret_cast<char *>(chunk.data());
               long int blob_size = chunk.size() * 2;
               reader->send(blob_data, blob_size);

               report->set_in_events( report->in_events() + 1 );

               chunk.clear();
           }                       // end if eoe

        }	// end for

        report->set_in_bytes( report->in_bytes() + io->len );

        // Discard data from recv buffer
        iobuf_remove(io, io->len);      
  
        // if total event received data is greater than maximum discard event
        if(chunk.size() >= MAX_EVENT_SIZE) {
 
           chunk.clear();
           logreader->error("data discarded due to overcoming maximum event size: %d bytes", MAX_EVENT_SIZE);
        }

      break;

      default:
         break;
   }
}

void FzReader::process(void) {
 
   while(true) {

      try {

         if(status == START) {
 
            if(devname == "usb") {

               if( usbstatus || (libusb_handle_events(ctx) != 0) )
                  std::cout << ERRTAG << "FzReader: USB fatal error !" << std::endl;

            } else if(devname == "net") {

               ns_mgr_poll(&udpserver, 1000);
            }
 
            boost::this_thread::interruption_point();

         } else if(status == STOP) {

              boost::this_thread::sleep(boost::posix_time::seconds(1));
              boost::this_thread::interruption_point();

         } 

      } catch(boost::thread_interrupted& interruption) {

         if(devname == "usb") {

            usb_close();

         } else if(devname == "net") {

            reader->close();
         }

         break;

      } catch(std::exception& e) {

      }
   }	// end while
};

Report::FzReader FzReader::get_report(void) {
   return(report);
};

void FzReader::usb_close(void) {

//   while(!libusb_cancel_transfer(xfr)) ;
   libusb_free_transfer(xfr); 
/*   libusb_release_interface(usbh, 0);
   libusb_close(usbh);
   libusb_exit(ctx); */
};

void FzReader::set_status(enum DAQstatus_t val) {

   status = val;
}

#include "FzController.h"

void FzController::configure_collector(void) {

   std::string ep, netint;

   // report monitoring socket

   try {

      pullmon = new zmq::socket_t(context, ZMQ_PULL);
      int tval = 1000;
      pullmon->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 1 second timeout on recv

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to start ZeroMQ report pull: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg && cfg.lookupValue("fzdaq.fzcontroller.interface", netint)) {
 
      std::cout << INFOTAG << "FzController: network interface: " << netint << " [cfg file]" << std::endl;
      ep = "tcp://" + netint + ":" + std::to_string(FZC_COLLECTOR_PORT);
      std::cout << INFOTAG << "FzController: report collector endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      // default setting
      ep = "tcp://eth0:" + std::to_string(FZC_COLLECTOR_PORT);
      std::cout << INFOTAG << "FzController: report collector endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      pullmon->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   boost::shared_lock<boost::shared_mutex> lock{logmutex};
      clog.write(INFO, "node report monitoring loop started");
   lock.unlock();
}

void FzController::start_collector(void) {

   thrcol = new boost::thread(&FzController::process_collector, this);
}

void FzController::process_collector(void) {

   Report::Node nodereport;

   boost::shared_lock<boost::shared_mutex> lock{logmutex};
      clog.write(INFO, "node report monitoring loop started");
   lock.unlock();

   while(true) {        // thread loop

      try {

         zmq::message_t repmsg;
         bool rc;

         boost::this_thread::interruption_point();

         // receive protobuf
         rc = pullmon->recv(&repmsg);

         if(rc) {

            if(nodereport.ParseFromArray(repmsg.data(), repmsg.size()) == true) {

               map_insert(nodereport.hostname(), nodereport);
            }
         }

      } catch(boost::thread_interrupted& interruption) {

         break;

      } catch(std::exception& e) {

      }

      map_clean();      // remove expired reports

   }    // end while
}

void FzController::close_collector(void) {

   thrcol->interrupt();
   thrcol->join();

   pullmon->close();
}

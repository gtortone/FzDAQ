#ifdef WEBLOG_ENABLED

#include "FzController.h"
#include "logger/FzHttpPost.h"

void FzController::configure_weblog(void) {

   haswl = false;

   if(hascfg) {

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.url", wl_url)) {

         haswl = true;
         std::cout << INFOTAG << "Weblog URL: " << wl_url << std::endl;

      } else {

         std::cout << INFOTAG << "Weblog URL not defined" << std::endl;
      }
   }

   if(haswl) {

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.username", wl_user)) {

         std::cout << INFOTAG << "Weblog username: " << wl_user << std::endl;

      } else {

         std::cout << ERRTAG << "Weblog username not defined" << std::endl;
         haswl = false;
      }

      if(cfg.lookupValue("fzdaq.fzcontroller.weblog.interval", wl_interval)) {

         std::cout << INFOTAG << "Weblog report time interval: " << wl_interval << " seconds" << std::endl;

      } else {

         std::cout << ERRTAG << "Weblog report time interval not defined" << std::endl;
         haswl = false;
      }
   }
}

void FzController::start_weblog(void) {

   if(haswl)
      thrwl = new boost::thread(&FzController::process_weblog, this);  
}

void FzController::process_weblog(void) {

   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;
   uint32_t tot_state_invalid, tot_events_empty, tot_events_good, tot_events_bad;
   bool send = false;

   FZHttpPost post;

   post.SetServerURL(wl_url);
   post.SetPHPArrayName("par");
   post.SetDBUserName(wl_user);

   while(1) {

         try {
  
            boost::this_thread::interruption_point();

            send = false;
            it = map.begin();
            while(it != map.end()) {

               rep = it->second;

               if(rep.has_reader()) {

                  if(!send) {
 
                     send = true;
                     post.NewPost();
                  }                    

                  post.SetDBCategory("daq_reader");

                  post.AddToPost("hostname", rep.hostname());
                  post.AddToPost("in_data", std::to_string(rep.reader().in_bytes()));
                  post.AddToPost("in_databw", std::to_string(rep.reader().in_bytes_bw()));
                  post.AddToPost("in_event", std::to_string(rep.reader().in_events()));
                  post.AddToPost("in_eventbw", std::to_string(rep.reader().in_events_bw()));

                  tot_state_invalid = tot_events_empty = tot_events_good = tot_events_bad = 0;

                  for(unsigned int i=0; i<rep.parser_num();i++) {

                     tot_state_invalid += rep.fsm(i).state_invalid();
                     tot_events_empty += rep.fsm(i).events_empty();
                     tot_events_good += rep.fsm(i).events_good();
                     tot_events_bad += rep.fsm(i).events_bad();
                  }

                  post.AddToPost("state_invalid", std::to_string(tot_state_invalid));
                  post.AddToPost("events_empty", std::to_string(tot_events_empty));
                  post.AddToPost("events_good", std::to_string(tot_events_good));
                  post.AddToPost("events_bad", std::to_string(tot_events_bad));

                  post.NewLine();
               }

               it++;
            }

            if(send)
               post.SendPost();

            send = false;
            it = map.begin();
            while(it != map.end()) {

               if(rep.has_writer()) {

                  if(!send) {
 
                     send = true;
                     post.NewPost();
                  }                    
 
                  post.SetDBCategory("daq_writer");

                  post.AddToPost("hostname", rep.hostname());
                  post.AddToPost("out_data", std::to_string(rep.writer().out_bytes()));
                  post.AddToPost("out_databw", std::to_string(rep.writer().out_bytes_bw()));
                  post.AddToPost("out_event", std::to_string(rep.writer().out_events()));
                  post.AddToPost("out_eventbw", std::to_string(rep.writer().out_events_bw()));

                  post.NewLine();
               }
              
               it++;
            }

            if(send)
               post.SendPost();

            boost::this_thread::sleep(boost::posix_time::seconds(wl_interval));

        } catch(boost::thread_interrupted& interruption) {

           break;
        }
   }	// end while
}

void FzController::close_weblog(void) {

   if(haswl) {
      thrwl->interrupt();
      thrwl->join();
   }
}

#endif

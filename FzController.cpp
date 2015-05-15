#include <iostream>
#include <sstream>
#include <time.h>
#include <sys/stat.h>
#include "boost/program_options.hpp"
#include "boost/thread.hpp"
#include "mongoose.h"

#include "FzConfig.h"
#include "FzTypedef.h"
#include "FzNodeReport.pb.h"
#include "zmq.hpp"

namespace po = boost::program_options;

zmq::context_t context(1);

void process(std::string cfgfile);
bool map_insert(std::string const &k, Report::Node const &v);
void map_clean(void);

void httpd(void);
static int ev_handler(struct mg_connection *conn, enum mg_event ev);
std::string JsonReport(void);

// global vars 
Report::Node nodereport;
static double const EXPIRY = 10; 	// seconds
std::map<std::string, Report::Node> map;
std::deque<std::pair<std::map<std::string, Report::Node>::iterator, time_t>> deque;

static const char *s_no_cache_header =
  "Cache-Control: max-age=0, post-check=0, "
  "pre-check=0, no-store, no-cache, must-revalidate\r\n";

int main(int argc, char *argv[]) {

   libconfig::Config cfg;
   std::string cfgfile = "";

   boost::thread *thrmon, *thrweb;

   // handling of command line parameters
   po::options_description desc("\nFzController - allowed options", 100);

   desc.add_options()
    ("help", "produce help message")
    ("cfg", po::value<std::string>(), "[optional] configuration file")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      exit(0);
   }
   
   std::cout << std::endl;
   std::cout << BOLDMAGENTA << "FzController - START configuration" << RESET << std::endl;

   if (vm.count("cfg")) {

      cfgfile = vm["cfg"].as<std::string>();

      std::cout << INFOTAG << "FzController configuration file: " << cfgfile << std::endl;

      try {

         cfg.readFile(cfgfile.c_str());

      } catch(libconfig::ParseException& ex) {

         std::cout << ERRTAG << "configuration file line " << ex.getLine() << ": " << ex.getError() << std::endl;
         exit(1);

      } catch(libconfig::FileIOException& ex) {

         std::cout << ERRTAG << "cannot open configuration file " << cfgfile << std::endl;
         exit(1);
      }
   }

   thrmon = new boost::thread(process, cfgfile);
   thrweb = new boost::thread(httpd); 

   std::cout << INFOTAG << "FzController: service ready" << std::endl;
   sleep(1);
   std::cout << BOLDMAGENTA << "FzDAQ - END configuration" << RESET << std::endl;
   std::cout << std::endl;
  
   // start command line interface

   std::cout << BOLDMAGENTA << "FzController - START command line interface " << RESET << std::endl;
   std::cout << std::endl;

   std::string line, lastcmd;
   for ( ; std::cout << "FzController > " && std::getline(std::cin, line); ) {
      if (!line.empty()) {

         if(!line.compare("r"))
            line = lastcmd;

         if(!line.compare("help")) {

            std::cout << std::endl;
            std::cout << "help:   \tprint usage" << std::endl;
            std::cout << "quit:   \tquit from FzController" << std::endl;
            std::cout << std::endl;
           }

         if(!line.compare("quit")) {
            break;
         }

         lastcmd = line;
      }
   }

   std::cout << INFOTAG << "FzController: closing threads: \t";
   thrmon->interrupt();
   thrmon->join();  
   thrweb->interrupt();
   thrweb->join();
   std::cout << " DONE" << std::endl;
   
   context.close();

   std::cout << "Goodbye.\n";

   return(0);
}

void process(std::string cfgfile) {

   libconfig::Config cfg;
   bool hascfg;

   zmq::socket_t *pubrc;        // publish socket for run control messages
   zmq::socket_t *pullmon;      // pull socket for monitoring reports
   std::string ep;

   if(!cfgfile.empty()) {

      hascfg = true;
      cfg.readFile(cfgfile.c_str());    // syntax checks on main

   } else hascfg = false;

   // run control socket
   
   try {

      pubrc = new zmq::socket_t(context, ZMQ_PUB);

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to start ZeroMQ run control publisher: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzcontroller.runcontrol"); 

      if(ep.empty()) {

         std::cout << ERRTAG << "FzController: run control endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzController: run control publisher endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://eth0:6000";
      std::cout << INFOTAG << "FzController: run control publisher endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      pubrc->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   // report monitoring socket

   try {

      pullmon = new zmq::socket_t(context, ZMQ_PULL);
      int tval = 1000;
      pullmon->setsockopt(ZMQ_RCVTIMEO, &tval, sizeof(tval));            // 1 second timeout on recv

   } catch (zmq::error_t &e) {

        std::cout << ERRTAG << "FzController: failed to start ZeroMQ report pull: " << e.what () << std::endl;
        exit(1);
   }

   if(hascfg) {

      ep = getZMQEndpoint(cfg, "fzdaq.fzcontroller.stats");

      if(ep.empty()) {

         std::cout << ERRTAG << "FzController: report endpoint not present in config file" << std::endl;
         exit(1);
      }

      std::cout << INFOTAG << "FzController: report pull endpoint: " << ep << " [cfg file]" << std::endl;

   } else {

      ep = "tcp://eth0:7000";
      std::cout << INFOTAG << "FzController: report pull endpoint: " << ep << " [default]" << std::endl;
   }

   try {

      //pullmon->connect(ep.c_str());
      pullmon->bind(ep.c_str());

   } catch (zmq::error_t &e) {

        //std::cout << ERRTAG << "FzController: failed to connect ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        std::cout << ERRTAG << "FzController: failed to bind ZeroMQ endpoint " << ep << ": " << e.what () << std::endl;
        exit(1);
   }

   while(true) {	// thread loop

      try {

         zmq::message_t repmsg;
         bool rc;

         boost::this_thread::interruption_point();

         rc = pullmon->recv(&repmsg);

         if(rc) {

            if(nodereport.ParseFromArray(repmsg.data(), repmsg.size()) == true) {
            
               map_insert(nodereport.hostname(), nodereport);
            } 
         }
      } catch(boost::thread_interrupted& interruption) {

         pubrc->close();
         pullmon->close();
         break;

      } catch(std::exception& e) {

      }

      map_clean();	// remove expired reports

   }	// end while
}

bool map_insert(std::string const &k, Report::Node const &v) {

   std::pair<std::map<std::string, Report::Node>::iterator, bool> result = map.insert(std::make_pair(k, v));

   if (result.second) {	// element does not exist in map - insert in deque

      deque.push_back(std::make_pair(result.first, time(NULL)));

   } else {	// element exists in map - update element in map and update time in deque

      map[k] = v;

      for(uint8_t i=0; i<deque.size(); i++) {

         if(deque[i].first->first == result.first->first)
            deque[i].second = time(NULL);
      }      
   }

   return result.second;
}

void map_clean(void) {

   for(uint8_t i=0; i<deque.size(); i++) {

      if(difftime(time(NULL), deque[i].second) > EXPIRY) {

         map.erase(deque[i].first);
         deque.erase(deque.begin() + i);
      }
   }

   /*
   while (!deque.empty() && difftime(time(NULL), deque.front().second) > EXPIRY) {
    map.erase(deque.front().first);
    deque.pop_front();
  }
  */
}

void httpd(void) {

   struct mg_server *server;

   // Create and configure HTTP server
   server = mg_create_server(NULL, ev_handler);
   mg_set_option(server, "listening_port", "8080");

   while(1) {

      try {
         
         boost::this_thread::interruption_point();
                  
         mg_poll_server(server, 1000);
      
      } catch(boost::thread_interrupted& interruption) {

         mg_destroy_server(&server);
         break;

      } catch(std::exception& e) {

      }
   }    
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {

   std::string json_str, uri, filename;
   struct stat buffer;

   switch (ev) {

      case MG_AUTH: 
         return MG_TRUE;

      case MG_REQUEST:

         uri = conn->uri;
         filename = uri;
         filename.erase(0,1);

         if(uri == "/") {

            mg_send_file(conn, "web/html/index.html", s_no_cache_header);
            return MG_MORE;

         } else if(uri == "/report") {

            mg_send_header(conn, s_no_cache_header, "");
            mg_printf_data(conn, "%s", JsonReport().c_str());
            return MG_TRUE;
         
         } else if(!stat(filename.c_str(), &buffer)) {

            mg_send_file(conn, filename.c_str(), s_no_cache_header);
            return MG_MORE;

         } else return MG_FALSE;

      default: 
         return MG_FALSE;
   }
}

std::string JsonReport(void) {

   std::stringstream str;
   std::map<std::string, Report::Node>::iterator it;
   Report::Node rep;

   str << "[ ";

   int i = 0;
   it = map.begin();
   while(it != map.end()) {

      i++;

      // check if first entry
      if(it != map.begin())
         str << " , ";
 
      rep = it->second;
 
      str << " { ";

      str << "\"hostname\": \"" << rep.hostname() << "\", ";
      str << "\"profile\": \"" << rep.profile() << "\", ";

      if(rep.has_reader()) {

        str << " \"reader\": { ";
        str << " \"in\": { ";
        str << " \"data\": " << rep.reader().in_bytes() <<  ", ";
        str << " \"databw\": " << rep.reader().in_bytes_bw() << ", ";
        str << " \"ev\": " << rep.reader().in_events() << ", ";
        str << " \"evbw\": " << rep.reader().in_events_bw();
        str << " } , ";
        str << " \"out\": { ";
        str << " \"data\": " << rep.reader().out_bytes() <<  ", ";
        str << " \"databw\": " << rep.reader().out_bytes_bw() << ", ";
        str << " \"ev\": " << rep.reader().out_events() << ", ";
        str << " \"evbw\": " << rep.reader().out_events_bw();
        str << " } } , ";
      }

      if(rep.has_writer()) {

        str << " \"writer\": { ";
        str << " \"in\": { ";
        str << " \"data\": " << rep.writer().in_bytes() <<  ", ";
        str << " \"databw\": " << rep.writer().in_bytes_bw() << ", ";
        str << " \"ev\": " << rep.writer().in_events() << ", ";
        str << " \"evbw\": " << rep.writer().in_events_bw();
        str << " }, ";
        str << " \"out\": { ";
        str << " \"data\": " << rep.writer().out_bytes() <<  ", ";
        str << " \"databw\": " << rep.writer().out_bytes_bw() << ", ";
        str << " \"ev\": " << rep.writer().out_events() << ", ";
        str << " \"evbw\": " << rep.writer().out_events_bw();
        str << " } } ";
      }

      str << "}";

      it++;    
   }

   str << " ]";   

   return(str.str());
}

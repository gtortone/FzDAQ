#include <iostream>
#include "boost/program_options.hpp"
#include "boost/thread.hpp"

#include "utils/FzTypedef.h"

#include "FzController.h"

FzController::FzController(zmq::context_t &ctx, std::string cfgfile) : 
   context(ctx) {

   try {

      cfg.readFile(cfgfile.c_str());

   } catch(libconfig::ParseException& ex) {

      std::cout << ERRTAG << "configuration file line " << ex.getLine() << ": " << ex.getError() << std::endl;
      exit(1);

   } catch(libconfig::FileIOException& ex) {

      std::cout << ERRTAG << "cannot open configuration file " << cfgfile << std::endl;
      exit(1);
   } 

   hascfg = 1;
}

FzController::FzController(zmq::context_t &ctx) :
   context(ctx) {
}

void FzController::configure(void) {

   configure_log();
   configure_collector();
   configure_rcs();
#ifdef WEBLOG_ENABLED
   configure_weblog();
#endif
}

void FzController::start(void) {

   // start thread pool
   start_collector();
   start_rcs();
   start_cli();
#ifdef WEBLOG_ENABLED
   start_weblog();
#endif
}

void FzController::close(void) {

   std::cout << INFOTAG << "FzController: closing application" << std::endl; 

   close_collector();
   close_rcs();
   close_cli();
#ifdef WEBLOG_ENABLED
   close_weblog();
#endif

   context.close();
}

bool FzController::quit(void) {
   return cli_exit;
}

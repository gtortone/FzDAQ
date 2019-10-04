#include "FzController.h"

void FzController::configure_log(void) {

   std::string logpropfile;

   // configure log property file
   if(cfg.lookupValue("fzdaq.global.logpropfile", logpropfile)) {

   	std::cout << INFOTAG << "log property file: " << logpropfile << std::endl;
   	FzLogger::setPropertyFile(logpropfile);
   } 

   clog.setFileConnection("fzcontroller", "/var/log/fzdaq/fzcontroller.log");
}


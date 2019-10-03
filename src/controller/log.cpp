#include "FzController.h"

void FzController::configure_log(void) {

   std::string logpropfile;

   // configure log property file
   if(cfg.lookupValue("fzdaq.global.logpropfile", logpropfile)) {

   	std::cout << INFOTAG << "log property file: " << logpropfile << std::endl;
   	FzLogger::setPropertyFile(logpropfile);
   } 

#ifdef AMQLOG_ENABLED

   // configure ActiveMQ JMS log
   if(cfg.lookupValue("fzdaq.global.log.url", brokerURI)) {

      std::cout << INFOTAG << "FzDAQ global logging URI: " << brokerURI << "\t[cfg file]" << std::endl;

      activemq::library::ActiveMQCPP::initializeLibrary();
      JMSfactory.setBrokerURI(brokerURI);

      try {

        JMSconn.reset(JMSfactory.createConnection());

      } catch (cms::CMSException e) {

        std::cout << ERRTAG << "log server error" << std::endl;
        JMSconn.reset();
      }
   }

   if(JMSconn.get()) {

      JMSconn->start();
      std::cout << INFOTAG << "log server connection successfully" << std::endl;
      clog.setJMSConnection("FzController", JMSconn.get());
   }
#endif

   clog.setFileConnection("fzcontroller", "/var/log/fzdaq/fzcontroller.log");
}


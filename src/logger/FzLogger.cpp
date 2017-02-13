#include "FzLogger.h"

#include <unistd.h>

FzLogger::FzLogger(void) {

   has_filelog = false;
   has_jmslog = false;
}

void FzLogger::setFileConnection(std::string instance, std::string filename) {

   log4cpp::PropertyConfigurator::configure("/etc/default/fazia/log4cpp.properties");
   logfile = &(root.getInstance(instance));
   priority = logfile->getChainedPriority();

   appender = new log4cpp::FileAppender(instance, filename);
   layout = new log4cpp::PatternLayout();
   layout->setConversionPattern("%d [%p] %m%n");
   appender->setLayout(layout);
   appender->setThreshold(priority);
   logfile->addAppender(appender);
   logfile->setAdditivity(false);

   has_filelog = true;
}

void FzLogger::setJMSConnection(std::string instance, cms::Connection *JMSconn) {

   JMSsession = JMSconn->createSession();
   JMSdest = JMSsession->createTopic(DEST_LOG);
   JMSproducer = JMSsession->createProducer(JMSdest);
   JMSproducer->setDeliveryMode(cms::DeliveryMode::NON_PERSISTENT);
   JMSmessage = JMSsession->createMapMessage();   

   char hn[64];
   gethostname(hn, sizeof(hn));
   
   JMSmessage->setString("TYPE", "alarm");
   JMSmessage->setString("NAME", instance);
   JMSmessage->setString("HOST", hn);

   has_jmslog = true;
}

void FzLogger::write(log4cpp::Priority::Value severity, std::string text) {

   if(has_filelog) 
      *logfile << severity << text;

   if(has_jmslog) {

      JMSmessage->setString("SEVERITY", log4cpp::Priority::getPriorityName(severity));
      JMSmessage->setString("TEXT", text);
      JMSproducer->send(JMSmessage);
   }
}

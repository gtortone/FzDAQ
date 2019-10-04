#include "FzLogger.h"

#include <unistd.h>

std::string FzLogger::propfile = "/etc/default/fazia/log4cpp.properties";

FzLogger::FzLogger(void) {

   has_filelog = false;
}

void FzLogger::setPropertyFile(std::string filename) {
   propfile = filename;
}

void FzLogger::setFileConnection(std::string instance, std::string filename) {

   log4cpp::PropertyConfigurator::configure(FzLogger::propfile);
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

void FzLogger::write(log4cpp::Priority::Value severity, std::string text) {

   if(has_filelog) 
      *logfile << severity << text;
}

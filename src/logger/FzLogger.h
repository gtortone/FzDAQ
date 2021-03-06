#ifndef FZLOGGER_H_
#define FZLOGGER_H_

#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/Appender.hh>
#include <log4cpp/FileAppender.hh>

#define EMERG 	 log4cpp::Priority::EMERG
#define FATAL 	 log4cpp::Priority::FATAL
#define ALERT 	 log4cpp::Priority::ALERT
#define CRIT 	 log4cpp::Priority::CRIT
#define ERROR 	 log4cpp::Priority::ERROR
#define WARN 	 log4cpp::Priority::WARN
#define NOTICE 	 log4cpp::Priority::NOTICE
#define INFO 	 log4cpp::Priority::INFO
#define DEBUG 	 log4cpp::Priority::DEBUG
#define NOTSET   log4cpp::Priority::NOTSET 

class FzLogger {

private:

   bool has_filelog;
   log4cpp::Category &root = log4cpp::Category::getRoot();
   log4cpp::Category *logfile;
   log4cpp::Appender *appender;
   log4cpp::PatternLayout *layout;
   log4cpp::Priority::Value priority;
   static std::string propfile;

public:

   FzLogger();

   static void setPropertyFile(std::string filename);
   void setFileConnection(std::string instance, std::string filename);
   void write(log4cpp::Priority::Value severity, std::string text);
};

#endif

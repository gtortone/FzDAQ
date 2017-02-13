#include "FzConfig.h"

std::string getZMQEndpoint(libconfig::Config &cfg, std::string path) {

   std::string val;

   if(cfg.lookupValue(path + ".url", val)) {

      if(val[0] == '$') {       // value is a reference

        std::string ref;

        if(cfg.lookupValue(val.substr(1), ref))
           return(ref);
        else
           return("");

      } else return(val);

   } else return("");
}



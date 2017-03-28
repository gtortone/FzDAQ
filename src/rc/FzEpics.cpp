#ifdef EPICS_ENABLED

#include <string>

#include "epicsThread.h"
#include "epicsExit.h"
#include "epicsStdio.h"
#include "dbStaticLib.h"
#include "subRecord.h"
#include "dbAccess.h"
#include "asDbLib.h"
#include "iocInit.h"
#include "iocsh.h"

#undef DBR_CTRL_DOUBLE
#undef DBR_CTRL_LONG
#undef DBR_GR_DOUBLE
#undef DBR_GR_LONG
#undef DBR_PUT_ACKS
#undef DBR_PUT_ACKT
#undef DBR_SHORT
#undef INVALID_DB_REQ
#undef VALID_DB_REQ

#include "cadef.h"

#include "FzEpics.h"

void PVwrite_db(const std::string pvname, int value) {

   DBADDR from;

   dbNameToAddr(pvname.c_str(),&from);

   // first param '1' is DBR_INT from db_access.h
   dbPutField(&from,1,(int *)(&value),1);
}

void PVwrite_db(const std::string pvname, double value) {

   DBADDR from;

   dbNameToAddr(pvname.c_str(),&from);

   dbPutField(&from,DBR_DOUBLE,(double *)(&value),1);
}

void PVwrite_db(const std::string pvname, std::string value) {

   DBADDR from;

   dbNameToAddr(pvname.c_str(),&from);
   dbPutField(&from,DBR_STRING,value.c_str(),1);
}

void PVwrite_db(const std::string pvname, long value) {

   DBADDR from;

   dbNameToAddr(pvname.c_str(),&from);
   dbPutField(&from,DBR_LONG,(long *)(&value),1);
}

int PVwrite(const std::string pvname, int value) {

   chid pv;

   int status = ca_create_channel(pvname.c_str(), NULL, NULL, 0, &pv);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_pend_io(1.0);
   
   if(status != ECA_NORMAL)
      return 1;

   status = ca_put(1, pv, (int *)&value);
   ca_flush_io();

   if(status != ECA_NORMAL)
      return 1;

   ca_clear_channel(pv);

   return 0;
}

int PVwrite(const std::string pvname, double value) {

   chid pv;

   int status = ca_create_channel(pvname.c_str(), NULL, NULL, 0, &pv);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_pend_io(1.0);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_put(DBR_DOUBLE, pv, (double *)&value);
   ca_flush_io();

   if(status != ECA_NORMAL)
      return 1;

   ca_clear_channel(pv);

   return 0;

}

int PVwrite(const std::string pvname, std::string value) {

   chid pv;

   int status = ca_create_channel(pvname.c_str(), NULL, NULL, 0, &pv);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_pend_io(1.0);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_put(DBR_STRING, pv, value.c_str());
   ca_flush_io();

   if(status != ECA_NORMAL)
      return 1;

   ca_clear_channel(pv);

   return 0;

}

int PVwrite(const std::string pvname, long value) {

   chid pv;

   int status = ca_create_channel(pvname.c_str(), NULL, NULL, 0, &pv);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_pend_io(1.0);

   if(status != ECA_NORMAL)
      return 1;

   status = ca_put(DBR_LONG, pv, (long *)&value);
   ca_flush_io();

   if(status != ECA_NORMAL)
      return 1;

   ca_clear_channel(pv);

   return 0;

}

#endif

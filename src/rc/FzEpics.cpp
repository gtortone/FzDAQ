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

/*
int PVread_db(const std::string pvname) {

}
*/

#endif

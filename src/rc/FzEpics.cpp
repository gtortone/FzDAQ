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

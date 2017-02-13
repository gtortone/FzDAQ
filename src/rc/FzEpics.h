#ifndef _FZEPICS_H
#define _FZEPICS_H

void PVwrite_db(const std::string pvname, int value);
void PVwrite_db(const std::string pvname, double value);
void PVwrite_db(const std::string pvname, std::string value);
void PVwrite_db(const std::string pvname, long value);
/*
int PVread_db(const std::string pvname);
*/

#endif

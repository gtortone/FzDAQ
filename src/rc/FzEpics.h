#ifdef EPICS_ENABLED

#ifndef _FZEPICS_H
#define _FZEPICS_H

void PVwrite_db(const std::string pvname, int value);
void PVwrite_db(const std::string pvname, double value);
void PVwrite_db(const std::string pvname, std::string value);
void PVwrite_db(const std::string pvname, long value);

int PVwrite(const std::string pvname, int value);
int PVwrite(const std::string pvname, double value);
int PVwrite(const std::string pvname, std::string value);
int PVwrite(const std::string pvname, long value);

#endif

#endif

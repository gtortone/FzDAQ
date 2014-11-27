#ifndef FZTYPEDEF_H_
#define FZTYPEDEF_H_

#define MAX_EVENT_SIZE	10000000LL		// 10 megabyte

// vector of raw data
typedef std::vector<unsigned short int> FzRawData;

// status of DAQ
enum DAQstatus_t { STOP, START, QUIT };
static char const* const state_labels[] = { "STOP", "START", "QUIT" };

#endif

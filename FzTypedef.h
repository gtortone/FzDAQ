#ifndef FZTYPEDEF_H_
#define FZTYPEDEF_H_

#define MAX_EVENT_SIZE	1000000LL		// 1 megabyte

// vector of raw data
typedef std::vector<unsigned int> FzRawData;

// status of DAQ
enum DAQstatus_t { STOP, START, QUIT };
static char const* const state_labels[] = { "STOP", "START", "QUIT" };

#endif

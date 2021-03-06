#ifndef FZTYPEDEF_H_
#define FZTYPEDEF_H_

#include <vector>

#define MAX_EVENT_SIZE	10000000LL		// 10 megabyte

#define FZW_SPY_PORT		5563
#define FZNM_REP_PORT		5550
#define FZNM_PULL_PORT		6660
#define FZC_COLLECTOR_PORT	7000
#define FZC_RC_PORT		5555

// vector of raw data
typedef std::vector<unsigned short int> FzRawData;

// DAQ Run Control
enum class RCcommand { configure = 0, start, stop, reset };
static char const* const state_labels[] = { "IDLE", "READY", "RUNNING", "PAUSED" };

enum RCstate { IDLE = 0,  READY, RUNNING, PAUSED };
static char const* const cmd_labels[] = { "configure", "start", "stop", "reset" };
static char const* const state_labels_l[] = { "idle", "ready", "running", "paused" };

enum RCtransition { RCERROR, RCOK };

// text colors
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#define CLEAR "\033[2J"  // clear screen escape code 

#define INFOTAG BOLDBLUE << "[INFO] " << RESET
#define ERRTAG BOLDRED << "[ERROR] " << RESET
#define WARNTAG BOLDYELLOW << "[WARNING] " << RESET

#endif

#ifndef FZUTILS_H_
#define FZUTILS_H_

#include <string>
//#include "FzEventSet.pb.h"
#include "FzFSM.h"	// for strings related to FEE, Telescope and Detectors

std::string human_byte(double size, double *value = 0, std::string *unit = 0);
std::string human_bit(double size);

int urlparse(std::string url, std::string *devname, unsigned int *port);
std::string devtoip(std::string netdev);

//void dumpEventOnScreen(DAQ::FzEvent *ev);

#endif

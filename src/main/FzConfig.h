#ifndef FZCONFIG_H_
#define FZCONFIG_H_

#include <iostream>
#include <libconfig.h++>

std::string getZMQEndpoint(libconfig::Config &cfg, std::string path);

#endif

#include <iostream>
#include "boost/program_options.hpp"

#include "FzController.h"

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

   FzController *fzc;
   zmq::context_t context(1);   // for ZeroMQ communications
   std::string cfgfile;   

   // handling of command line parameters
   po::options_description desc("\nFzController - allowed options", 100);

   desc.add_options()
    ("help", "produce help message")
    ("cfg", po::value<std::string>(), "[optional] configuration file")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   if (vm.count("help")) {
      std::cout << desc << std::endl << std::endl;
      exit(0);
   }

   std::cout << std::endl;
   std::cout << BOLDMAGENTA << "FzController - START configuration" << RESET << std::endl;

   if (vm.count("cfg")) {

      cfgfile = vm["cfg"].as<std::string>();

      std::cout << INFOTAG << "FzDAQ configuration file: " << cfgfile << std::endl;

      // create FzController instance
      fzc = new FzController(context, cfgfile);

   } else {

      // create FzController instance
      fzc = new FzController(context);
   }

   fzc->configure();
   fzc->start();

   // FzController main loop
   while(!fzc->quit()) 
      sleep(1);	

   fzc->close();

   return 0;
}


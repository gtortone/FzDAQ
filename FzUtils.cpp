#include "FzUtils.h"
#include<sys/socket.h>
#include<netdb.h>
#include<ifaddrs.h>

std::string human_byte(double size, double *value, std::string *unit) {

   const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
   char buf[50];

   int i=0;
   while (size >= 1024) {
      size /= 1024;
      i++;
   }

   if(value)
      *value = size;

   if(unit)
      *unit = units[i]; 

   sprintf(buf, "%.*f %s", i, size, units[i]);
   return buf;
}

std::string human_bit(double size) {

   const char* units[] = {"b", "kb", "Mb", "Gb", "Tb", "Pb", "Eb", "Zb", "Yb"};
   char buf[50];

   int i=0;
   while (size >= 1000) {
      size /= 1000;
      i++;
   }

   sprintf(buf, "%.*f %s", i, size, units[i]);
   return buf;
}

int urlparse(std::string url, std::string *devname, unsigned int *port) {

   char _devname[16];
   unsigned int _port;

   int nc = sscanf(url.c_str(), "udp://%99[^:]:%99d", _devname, &_port);

   if(nc != 2) {

      return(0);

   } else {

      *devname = _devname;
      *port = _port;
      return(1);
   }
}

std::string devtoip(std::string netdev) {

   int fm = AF_INET;
   struct ifaddrs *ifaddr, *ifa;
   int family , s;
   char host[NI_MAXHOST];

   if (getifaddrs(&ifaddr) == -1)
      return("");

   for(ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {

      if (ifa->ifa_addr == NULL) {
         continue;
      }

      family = ifa->ifa_addr->sa_family;

      if(ifa->ifa_name == netdev) {

         if (family == fm) {

            s = getnameinfo( ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6) , host , NI_MAXHOST , NULL , 0 , NI_NUMERICHOST);

            if (s != 0)
               return("");
         }
      }
    }

    freeifaddrs(ifaddr);

    return(host);
}

/*

void dumpEventOnScreen(DAQ::FzEvent *ev) {

   std::cout << "EC: " << ev->ec() << std::endl;

   for (int i = 0; i < ev->block_size(); i++) {

      const DAQ::FzBlock& rdblk = ev->block(i);

      if(rdblk.has_len_error() && rdblk.has_crc_error() )
         std::cout << "\tBLKID: " << rdblk.blkid() << " - LEN_ERR: " << rdblk.len_error() << " - CRC_ERR: " << rdblk.crc_error() << std::endl;
      else
         std::cout << "\tBLKID: " << rdblk.blkid() << std::endl;

      for(int j = 0; j < rdblk.fee_size(); j++) {

         const DAQ::FzFee& rdfee = rdblk.fee(j);
         std::cout << "\t\t" << FzFec_str[rdfee.feeid()] << " - LEN_ERR: " << rdfee.len_error() << " - CRC_ERR: " << rdfee.crc_error() << std::endl;

         for(int k = 0; k < rdfee.hit_size(); k++) {

            const DAQ::FzHit& rdhit = rdfee.hit(k);
            std::cout << "\t\t\tEC: " << rdhit.ec();

            std::cout << " - TEL: ";
            if(rdhit.has_telid())
               std::cout << FzTelescope_str[rdhit.telid()];
            else { std::cout << "<ERR>" << std::endl; continue; }

            std::cout << " - DET: ";
            if(rdhit.has_detid())
               std::cout << FzDetector_str[rdhit.detid()];
            else { std::cout << "<ERR>" << std::endl; continue; }

            std::cout << " - FEE: ";
            if(rdhit.has_feeid())
               std::cout << FzFec_str[rdhit.feeid()];
            else { std::cout << "<ERR>" << std::endl; continue; }

            std::cout << " - GTTAG: " << rdhit.gttag();
            std::cout << " - DETTAG: " << rdhit.dettag() << std::endl;

            for(int m = 0; m < rdhit.data_size(); m++) {

               const DAQ::FzData& rdata = rdhit.data(m);
               std::cout << "\t\t\t\tDATATYPE: " << FzDataType_str[rdata.type()];

               if(!rdata.has_energy() && !rdata.has_waveform()) {
                  std::cout << " [NO DATA]" << std::endl;
                  continue;
               }

               if(rdata.has_energy()) {

                  const DAQ::Energy& ren = rdata.energy();
                  std::cout << " - ENERGY: ";

                  for(int e=0; e < ren.value_size(); e++)
                     std::cout << ren.value(e) << ", ";

                  std::cout << " - LEN_ERR: " << ren.len_error();
               }

               if(rdata.has_waveform()) {

                 const DAQ::Waveform& rwf = rdata.waveform();
                 std::cout << " - PRETRIG: " << rwf.pretrig();

                 std::cout << " - WAVEFORM: " << " - LEN_ERR: " << rwf.len_error() << std::endl;
                 std::cout << std::endl << "\t\t\t\t              ";
                 signed int supp;
                 for(int n=0; n < rwf.sample_size(); n++) {
                    if(n%16 == 0 && n)
                       std::cout << std::endl << "\t\t\t\t              ";

                    if(rdata.type() != DAQ::FzData::ADC) {

                       if(rwf.sample(n) > 8191)
                          supp = rwf.sample(n) | 0xFFFFC000;
                       else
                          supp = rwf.sample(n);

                    } else supp = rwf.sample(n);

                    std::cout << supp << ", ";
                 }
               }

               std::cout << std::endl;

            }
         }
      }
   }

   std::cout << "### END of event ###" << std::endl;
}
*/

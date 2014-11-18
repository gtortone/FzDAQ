#include <stdint.h>
#include <iostream>
#include <fstream>
#include <iterator>

int main(void) {

   uint16_t crc_lsb = 0;
   uint16_t crc_msb = 0;
   uint16_t crc16 = 0;
   uint16_t crc8 = 0;
   uint16_t tmpval = 0;

   std::ifstream myfile("event-0044");

   std::istream_iterator<std::string> word_iter( myfile ), word_iter_end;

   for ( ; word_iter != word_iter_end; ++ word_iter ) {

      sscanf(word_iter->c_str(), "%X", &tmpval);

      crc16 = crc16 ^ tmpval;
      crc_lsb = crc16 & 0x00FF;
      crc_msb = (crc16 & 0xFF00) >> 8;
      crc8 = (crc_lsb ^ crc_msb) & 0x00FF;

      std::cout << "word " << * word_iter << " - partial crc8  " << crc8 << '\n';
   }

   return 0;
}

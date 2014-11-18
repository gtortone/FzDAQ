#include <sstream>

#include "FzEventWord.h"

wtype_t FzEventWord::Parse(unsigned word) {

   if( (word & REGHDR_FMT) == REGHDR_TAG) {

      type = WT_REGHDR;
      content[WC_REGID] = (word & REGID_MASK);      

   } else if ( (word & BLKHDR_FMT) == BLKHDR_TAG) {

      type = WT_BLKHDR;
      content[WC_BLKID] = (word & BLKID_MASK);

   } else if( (word & EC_FMT) == EC_TAG) {

      type = WT_EC;
      content[WC_EC] = (word & EC_MASK);
   
   } else if( (word & TELHDR_FMT) == TELHDR_TAG) {

      type = WT_TELHDR;
      if( (word & ADC_MASK) == ADC_TAG )
         content[WC_FEEID] = 0x000F;	// ADC 
      else
         content[WC_FEEID] = (word & FEEID_MASK) >> 1;

      content[WC_TELID] = (word & TELID_MASK);
   
   } else if( (word & DETHDR_FMT) == DETHDR_TAG) {

      type = WT_DETHDR;
      content[WC_DTYPE] = (word & DTYPE_MASK) >> 8; 
      content[WC_DETID] = (word & DETID_MASK) >> 5;

      if( (word & ADC_MASK) == ADC_TAG )
         content[WC_FEEID] = 0x000F;	// ADC 
      else
         content[WC_FEEID] = (word & FEEID_MASK) >> 1;

      content[WC_TELID] = (word & TELID_MASK);
   
   } else if( (word & DATA_FMT) == DATA_TAG) {

      type = WT_DATA;
      content[WC_DATA] = word;

   } else if( (word & LENGTH_FMT) == LENGTH_TAG) {

      type = WT_LENGTH;
      content[WC_LEN] = (word & LENGTH_MASK);

   } else if( (word & CRCFE_FMT) == CRCFE_TAG) {

      type = WT_CRCFE;
      content[WC_CRC] = (word & CRCFE_MASK) >> 4;

   } else if( (word & CRCBL_FMT) == CRCBL_TAG) {

     type = WT_CRCBL;
     content[WC_CRC] = (word & CRCBL_MASK) >> 4;

   } else if( (word & EMPTY_FMT) == EMPTY_TAG) {

     type = WT_EMPTY;

   } else { 
 
      type = WT_UNKNOWN;
      content[WC_UNK] = word;
   }

   return(type);
}

void FzEventWord::Clear(void) {

   type = WT_UNKNOWN;
   content.clear();
}

wtype_t FzEventWord::getType(void) {
   return(type);
}

std::string FzEventWord::getType_str(void) {
   return(wtype_str[type]);
}

FzEventWord::wcontent_t FzEventWord::getContent(void) {
   return(content);
}

std::string FzEventWord::str(void) {

   std::stringstream s;
   FzEventWord::wcontent_t::iterator it;

   s << "word type: " << wtype_str[type];

   s << " / wcontent: ";
   for ( it=content.begin() ; it != content.end(); it++ )
      s << wlabel_str[(*it).first] << " = " << (*it).second << " ";

   return(s.str());   
}

std::ostream &operator<<(std::ostream &os, FzEventWord &ew) {

   FzEventWord::wcontent_t::iterator it;

   os << "word type: " << wtype_str[ew.type]; 

   os << " / wcontent: ";
   
   for ( it=(ew.content).begin() ; it != (ew.content).end(); it++ ) 
      os << wlabel_str[(*it).first] << " = " << (*it).second << " ";  

   return(os);
}


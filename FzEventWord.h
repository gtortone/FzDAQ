#ifndef FZEVENTWORD_H_
#define FZEVENTWORD_H_

#include <iostream>
#include <string>
#include <map>

/*
   <word>_FMT: 			bit=1 for each position that compose the word identificator
   <word>_TAG:			copy of word identificator
   <word>_MASK:			bit=1 for each position that compose the word content
*/

#define REGHDR_FMT		0xF800
#define REGHDR_TAG              0xC800  
#define REGID_MASK		0x07FF

#define BLKHDR_FMT		0xF000
#define BLKHDR_TAG              0xC000
#define BLKID_MASK		0x07FF

#define EC_FMT			0xF000
#define EC_TAG                  0xE000
#define EC_MASK			0x0FFF

#define TELHDR_FMT		0xF800
#define TELHDR_TAG              0x9800
#define TELID_MASK		0x0001
#define FEEID_MASK		0x000E
#define ADC_MASK		0x001F
#define ADC_TAG			0x001F

#define DETHDR_FMT		0xF800
#define DETHDR_TAG		0x9000
#define DTYPE_MASK		0x0700  
#define DETID_MASK		0x00E0	
//#define TELID_MASK		0x0001
//#define FEEID_MASK		0x000E

#define DATA_FMT		0x8000
#define DATA_TAG                0x0000
#define DATA_MASK

#define LENGTH_FMT		0xF000
#define LENGTH_TAG		0xA000
#define LENGTH_MASK		0x0FFF

#define CRCFE_FMT		0xF000
#define CRCFE_TAG             	0xB000
#define CRCFE_MASK		0x0FF0

#define CRCBL_FMT		0xF000
#define CRCBL_TAG		0xD000
#define CRCBL_MASK		0x0FF0

#define EMPTY_FMT		0x8000
#define EMPTY_TAG		0x8000

const char wtype_str[][11] = {"Unknown", "RegHdr", "BlkHdr", "Ec", "TelHdr", "DetHdr", "Data", "Length", "Crcfe", "Crcbl", "Empty"};

enum wtype_t { 
        WT_UNKNOWN,
        WT_REGHDR,
	WT_BLKHDR,
	WT_EC,
	WT_TELHDR,
	WT_DETHDR,
	WT_DATA,
	WT_LENGTH,
	WT_CRCFE,
	WT_CRCBL,
	WT_EMPTY
};

const char wlabel_str[][10] = {"regid", "blockid", "ec", "telid", "feeid", "detid", "dtype", "data", "len", "crc"};

enum wlabel_t {
	WC_REGID,
	WC_BLKID,
	WC_EC,
	WC_TELID,
	WC_FEEID,
        WC_DETID,
        WC_DTYPE,
	WC_DATA,
	WC_LEN,
	WC_CRC,
        WC_UNK
};

class FzEventWord {

typedef std::map<wlabel_t, unsigned> wcontent_t;

private:
  
   unsigned wraw;	// raw word
   wtype_t type;
   wcontent_t content;

public:
   FzEventWord(void) {  Clear(); }
   FzEventWord(unsigned word) { wraw = word; Parse(word); }

   wtype_t Parse(unsigned word);
   void Clear(void);

   unsigned getWord(void) { return wraw; };
   wtype_t getType(void);
   std::string getType_str(void);
   wcontent_t getContent(void);

   std::string str(void);
   friend std::ostream &operator<<(std::ostream &os, FzEventWord &ew);
};

#endif

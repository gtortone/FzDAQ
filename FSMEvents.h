#ifndef FZFSMEVENTS_H_
#define FZFSMEVENTS_H_

#include "FzEventWord.h"

// ----- Events
class gotEvent {

protected:
   FzEventWord w;
   std::string evname;

public:
   gotEvent(FzEventWord _w, std::string _name) {
      w = _w;
      evname = _name;
   }
   
   ~gotEvent() { };
 
   FzEventWord getWord(void) const {
      return(w);
   }

   std::string getName(void) const {
      return(evname);
   }
};

class gotREGHDR : public gotEvent {
public:
   gotREGHDR(FzEventWord word) : gotEvent(word, "gotREGHDR") {}
};

class gotBLKHDR : public gotEvent {
public:
   gotBLKHDR(FzEventWord word) : gotEvent(word, "gotBLKHDR") {}
};

class gotEC : public gotEvent {
public:
   gotEC(FzEventWord word) : gotEvent(word, "gotEC") {}
};

class gotTELHDR : public gotEvent {
public:
   gotTELHDR(FzEventWord word) : gotEvent(word, "gotTELHDR") {}
};

class gotDETHDR : public gotEvent {
public:
   gotDETHDR(FzEventWord word) : gotEvent(word, "gotDETHDR") {}
};

class gotDATA : public gotEvent {
public:
   gotDATA(FzEventWord word) : gotEvent(word, "gotDATA") {}
};

class gotLENGTH : public gotEvent {
public:
   gotLENGTH(FzEventWord word) : gotEvent(word, "gotLENGTH") {}
};

class gotCRCFE : public gotEvent {
public:
   gotCRCFE(FzEventWord word) : gotEvent(word, "gotCRCFE") {}
};

class gotCRCBL : public gotEvent {
public:
   gotCRCBL(FzEventWord word) : gotEvent(word, "gotCRCBL") {}
};

class gotEMPTY : public gotEvent {
public:
   gotEMPTY(FzEventWord word) : gotEvent(word, "gotEMPTY") {}
};

class gotUNKNOWN : public gotEvent {
public:
   gotUNKNOWN(FzEventWord word) : gotEvent(word, "gotUNKNOWN") {}
};

#endif

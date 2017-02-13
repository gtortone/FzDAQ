#include "RCFSM.h"

RCFSM::RCFSM(void) {

   curstate = IDLE;
   lasterr = RCOK;
}

RCtransition RCFSM::process(RCcommand cmd) {

   uint8_t trans_id;  
   RCtransition trans_err = RCOK;
  
   if(cmd > (RCCOMMAND_NUM-1)) {
      lasterr = RCERROR;
      return lasterr;
   }

   trans_id = rc_ttable[cmd][curstate];

   switch(trans_id) {

      case 0:
         trans_err = RCERROR;
         break;

      case 1:
         curstate = READY;	// IDLE -> configure -> READY
         break;

      case 2:
         curstate = IDLE;	// IDLE -> reset -> IDLE
         break;

      case 3:
         curstate = RUNNING;	// READY -> start -> RUNNING
         break;

      case 4:
         curstate = IDLE;	// READY -> reset -> IDLE
         break;

      case 5:
         curstate = PAUSED;	// RUNNING -> stop -> PAUSED
         break;

      case 6:
         curstate = IDLE;	// RUNNING -> reset -> IDLE
         break;

      case 7:
         curstate = READY;	// PAUSED -> configure -> READY
         break;

      case 8:
         curstate = RUNNING;	// PAUSED -> start -> RUNNING
         break;

      case 9:
         curstate = IDLE;	// PAUSED -> reset -> IDLE
         break;

      default:
         break;

   } // end switch

   lasterr = trans_err;
   return(trans_err);
}

RCstate RCFSM::state(void) {

   return(curstate);
}

RCtransition RCFSM::error(void) {

   return(lasterr);
}

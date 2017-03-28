#ifndef RCFSM_H_
#define RCFSM_H_

#include <stdint.h>

#include "utils/FzTypedef.h"

#define RCCOMMAND_NUM	4	
#define RCSTATES_NUM	4

// state transitions id
// 
//	0	TRANSITION NOT VALID
//	1	IDLE 	->	configure	-> READY
//	2	IDLE 	-> 	reset 		-> IDLE
//	3	READY 	-> 	start	 	-> RUNNING
//	4	READY 	-> 	reset		-> IDLE
//	5	RUNNING	-> 	stop		-> PAUSED
//	6	RUNNING -> 	reset		-> IDLE
//	7	PAUSED 	->	configure	-> READY
//	8	PAUSED 	-> 	start		-> RUNNING
//	9	PAUSED 	->	reset		-> IDLE

// matrix of state transition (row = command , col = current state, ttable[row][col] = transition_id)

static const uint8_t rc_ttable[RCCOMMAND_NUM][RCSTATES_NUM] = 

// 					IDLE	READY  RUNNING	PAUSED	
   {	
/* id =  0 - configure */     	{	 1,	 0,	 0,	 7 	},
/* id =  1 - start */     	{	 0,	 3,	 0,	 8	},
/* id =  2 - stop */     	{	 0,	 0,	 5,	 0 	},
/* id =  3 - reset */		{	 2,	 4,	 6,	 9 	}
   };	

class RCFSM {

private:

   RCstate curstate;
   RCtransition lasterr;

public:

   RCFSM(void);

   RCtransition process(RCcommand cmd);
   RCtransition try_process(RCcommand cmd);
   
   RCstate state(void); 
   RCtransition error(void);
};

#endif

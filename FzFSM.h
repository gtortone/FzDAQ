#ifndef FZFSM_H_
#define FZFSM_H_

#include <iostream>

#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#include "FSMStates.h"
#include "FSMActions.h"
#include "FSMEvents.h"
#include "FzParser.h"
#include "FzLogger.h"
#include "FzEventWord.h"
#include "FzCbuffer.h"

#define LOG	*logfsm

namespace msm = boost::msm;
namespace msmf = boost::msm::front;
namespace mpl = boost::mpl;

// ----- State machine

struct FzFSM_:msmf::state_machine_def < FzFSM_ > {

   log4cpp::Category *logfsm;
   FzCbuffer <DAQ::FzEvent> *cbw;

   DAQ::FzEvent ev;
   DAQ::FzBlock *blk;
   DAQ::FzFee *fee;
   DAQ::FzHit *hit;
   DAQ::FzData *d;
   DAQ::Energy *en;
   DAQ::Waveform *wf;

   FzEventWord w;

   uint16_t tmp_ec;
   uint16_t tmp_rawec;

   bool energy1_done;
   bool energy2_done;
   bool pretrig_done;
   bool wflen_done;

   uint16_t rd_wflen;

   uint16_t blklen;
   uint16_t rd_blklen;
   uint16_t feelen;
   uint16_t rd_feelen;

   uint16_t blkcrc;
   uint16_t rd_blkcrc;
   uint16_t feecrc;
   uint16_t rd_feecrc;

   bool err_in_event;

   void initlog(log4cpp::Category *lc) {

      logfsm = lc;
   };

   void initcbw(FzCbuffer <DAQ::FzEvent> *cb_w) {

      cbw = cb_w;
   };

   // no need for exception handling or message queue
   typedef int no_exception_thrown;
   typedef int no_message_queue;
   // also manually enable deferred events
   typedef int activate_deferred_events;

   // Set initial state
   typedef IdleState initial_state;

   // Transition table
   struct transition_table:mpl::vector17 <
   //              Start       Event       Next          Action         Guard
      msmf::Row <  IdleState,  gotEC,      State1,       Action1,       msmf::none>,
      msmf::Row <  State1,     gotBLKHDR,  State8,       Action2,       msmf::none>,
      msmf::Row <  State1,     gotTELHDR,  State2,       Action3,       msmf::none>,
      msmf::Row <  State2,     gotDATA,    State3,       Action4,       msmf::none>,
      msmf::Row <  State3,     gotDATA,    State3,       Action5,       msmf::none>,
      msmf::Row <  State3,     gotDETHDR,  State4,       Action6,       msmf::none>,
      msmf::Row <  State4,     gotDATA,    State5,       Action8,       msmf::none>,
      msmf::Row <  State5,     gotTELHDR,  State2,       Action9,       msmf::none>,
      msmf::Row <  State5,     gotDETHDR,  State4,       Action10,      msmf::none>,
      msmf::Row <  State5,     gotDATA,    State5,       Action11,      msmf::none>,
      msmf::Row <  State5,     gotLENGTH,  State6,       Action12,      msmf::none>,
      msmf::Row <  State6,     gotCRCFE,   State7,       Action13,      msmf::none>,
      msmf::Row <  State7,     gotEC,      State1,       Action14,      msmf::none>,
      msmf::Row <  State8,     gotLENGTH,  State9,       Action15,      msmf::none>,
      msmf::Row <  State9,     gotCRCBL,   State10,      Action16,      msmf::none>,
      msmf::Row <  State10,    gotREGHDR,  IdleState,    msmf::none,    msmf::none>,
      msmf::Row <  State10,    gotEC,      State1,       Action18,      msmf::none>
   > { };

   // Replaces the default no-transition response
   template <class FSM, class Event> void
   no_transition(Event const &e, FSM &sm, int state) {

      FzEventWord w = e.getWord();

      LOG << DEBUG << "invalid transition from state " << state << " on event " << e.getName() << " (FSM RESET) " << w.getType_str() << " -- " << std::setw(4) << std::setfill('0') << std::hex << w.getWord(); 

      // update stats
      tr_invalid_tot++;      
      tr_invalid_stats[state]++;

      // reset FSM
      sm.start(); 

      // clear incomplete event
      sm.ev.Clear();
   } 

public:

      // invalid transitions: total number and counter for each state
      uint32_t tr_invalid_tot;
      uint32_t tr_invalid_stats[16];

      // parsed events without errors and with errors
      uint32_t event_empty_num;
      uint32_t event_clean_num;
      uint32_t event_witherr_num;   
};

// Pick a back-end
typedef msm::back::state_machine < FzFSM_ > FzFSM;

// additional methods and vars

static char const* const state_names[] = { "IdleState", "State1", "State2", "State3", "State4", "State5", "State6", "State7", "State8", "State9", "State10" };

/*std::string getstate(FzFSM const& sm) {
   return (state_names[sm.current_state()[0]]);
}*/

#endif

#ifndef FSMSTATES_H_
#define FSMSTATES_H_ 

#include <sstream>
#include <string>

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>

#define FSMLOG sm.logfsm

namespace msmf = boost::msm::front;

// ----- FSM states

struct IdleState:msmf::state <> { };

struct State1:msmf::state <> {

#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm>
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State1", getstate(sm).c_str());
   }

   // Exit action 
   template <class Event, class Fsm > 
   void on_exit(Event const &, Fsm &) const {
   }

#endif

};

struct State2:msmf::state <> {
      
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State2", getstate(sm).c_str());
   }

#endif

};

struct State3:msmf::state <> {
      
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State3", getstate(sm).c_str());
   }
   
#endif

};

struct State4:msmf::state <> {
      
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State4", getstate(sm).c_str());
   }

#endif

};

struct State5:msmf::state <> {
   
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State5", getstate(sm).c_str());
   }
 
#endif

};

struct State6:msmf::state <> {
   
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State6", getstate(sm).c_str());
   }

#endif

};

struct State7:msmf::state <> {
   
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State7", getstate(sm).c_str());
   }

#endif

};

struct State8:msmf::state <> {

#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State8", getstate(sm).c_str());
   }

#endif

};

struct State9:msmf::state <> {
   
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State9", getstate(sm).c_str());
   } 

#endif

};

struct State10:msmf::state <> {
   
#ifdef DEBUG_ALL

   // Entry action
   template <class Event, class Fsm > 
   void on_entry(Event const &e, Fsm &sm) const {

      FSMLOG->debug("%s -> State10", getstate(sm).c_str());
   }

#endif

};

struct End:msmf::terminate_state <> { };

#endif

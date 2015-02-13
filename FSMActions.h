#ifndef FSMACTIONS_H_
#define FSMACTIONS_H_

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "FzEventSet.pb.h"
#include "FzEventWord.h"
#include "FSMStates.h"
#include "FSMEvents.h"

#define ACTLOG sm.logfsm
//#define DEBUG_ALL

static char const* const FzFec_str[] = { "FEC#0", "FEC#1", "FEC#2", "FEC#3", "FEC#4", "FEC#5", "FEC#6", "FEC#7", "", "", "", "", "", "", "", "ADC", "UNK"  };
static char const* const FzDataType_str[] = { "QH1", "I1", "QL1", "Q2", "I2", "Q3", "ADC", "UNK" };
static char const* const FzTelescope_str[] = { "A", "B", "UNK" };
static char const* const FzDetector_str[] = { "Si1", "Si2", "CsI", "UNK" };

// ----- Actions

struct Action1 {
    
   // IdleState -> State1
   template <class Fsm>
   void operator()(const gotEC &e, Fsm &sm, IdleState&, State1&) const {

      sm.w = e.getWord();
 
#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.err_in_event = false;

      sm.blk = sm.ev.add_block();
      sm.blk->set_len_error(true);
      sm.blk->set_crc_error(true);

      sm.blklen = 1;
      sm.blkcrc = sm.w.getWord();

      sm.tmp_rawec = sm.w.getWord();
      sm.tmp_ec = sm.w.getContent()[WC_EC];
      sm.ev.set_ec(sm.w.getContent()[WC_EC]);	// useful for empty events
   }
};

struct Action2 {

   // State1 -> State8
   template <class Fsm>
   void operator()(const gotBLKHDR &e, Fsm &sm, State1&, State8&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      sm.blk->set_blkid(sm.w.getContent()[WC_BLKID]);
   }
};

struct Action3 {

   // State1 -> State2
   template <class Fsm>
   void operator()(const gotTELHDR &e, Fsm &sm, State1&, State2&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen) = 2;	// EC is just acquired
      sm.feecrc = sm.tmp_rawec ^ sm.w.getWord();

      sm.fee = sm.blk->add_fee();
      sm.fee->set_len_error(true);
      sm.fee->set_crc_error(true);

      // validate range of FEEID
      if( (sm.w.getContent()[WC_FEEID] < DAQ::FzFee::FEC0) || (sm.w.getContent()[WC_FEEID] > DAQ::FzFee::ADCF) )
         sm.fee->set_feeid(DAQ::FzFee::UNKFEC);	// unknown fec	
      else
         sm.fee->set_feeid((DAQ::FzFee::FzFec) sm.w.getContent()[WC_FEEID]);
      
      sm.hit = sm.fee->add_hit();
      sm.hit->set_ec(sm.tmp_ec);

      // validate range of TELID
      if( (sm.w.getContent()[WC_TELID] < DAQ::FzHit::A) || (sm.w.getContent()[WC_TELID] > DAQ::FzHit::B) )
         sm.hit->set_telid(DAQ::FzHit::UNKT);	// unknown telescope
      else
         sm.hit->set_telid((DAQ::FzHit::FzTelescope) sm.w.getContent()[WC_TELID]);

      sm.hit->set_feeid((DAQ::FzHit::FzFec) sm.fee->feeid());
      //sm.hit->set_feeid((DAQ::FzHit::FzFec) sm.w.getContent()[WC_FEEID]);
   }
};

struct Action4 {

   // State2 -> State3
   template <class Fsm>
   void operator()(const gotDATA &e, Fsm &sm, State2&, State3&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      // verify if GTTAG already set
      sm.hit->set_gttag(sm.w.getContent()[WC_DATA]);
   }
};

struct Action5 {

   // State3 -> State3
   template <class Fsm>
   void operator()(const gotDATA &e, Fsm &sm, State3&, State3&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();
 
      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      // verify if DETTAG already set
      sm.hit->set_dettag(sm.w.getContent()[WC_DATA]);
   }
};

struct Action6 {

   // State3 -> State4
   template <class Fsm>
   void operator()(const gotDETHDR &e, Fsm &sm, State3&, State4&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << " (S3->S4) " << sm.w.str() << sm.w.getContent()[WC_DETID] << " - " << sm.w.getContent()[WC_DTYPE];
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      DAQ::FzHit::FzDetector detid = (DAQ::FzHit::FzDetector) sm.w.getContent()[WC_DETID];
      //DAQ::FzData::FzDataType dtype = (DAQ::FzData::FzDataType) sm.w.getContent()[WC_DTYPE];

      // validate range of DETID
      //if( (sm.w.getContent()[WC_DETID] < DAQ::FzHit::Si1) || (sm.w.getContent()[WC_DETID] > DAQ::FzHit::CsI) )
      // sm.hit->set_detid(DAQ::FzHit::UNKD);
      //else
      //   sm.hit->set_detid(detid);
      
      // verify (DETHDR[FEEID] == sm.fee->feeid() == sm.hit->feeid())
      // verify (DETHDR[TELID] == sm.hit->telid())

      DAQ::FzData::FzDataType dtype;

      if( (detid == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 0))
         dtype = DAQ::FzData::QH1;
      else if( (detid == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 1))
         dtype = DAQ::FzData::I1;
      else if( (detid == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 2))
         dtype = DAQ::FzData::QL1;
      else if( (detid == DAQ::FzHit::Si2) && (sm.w.getContent()[WC_DTYPE] == 0))
         dtype = DAQ::FzData::Q2;
      else if( (detid == DAQ::FzHit::Si2) && (sm.w.getContent()[WC_DTYPE] == 1))
         dtype = DAQ::FzData::I2;
      else if( (detid == DAQ::FzHit::CsI) && (sm.w.getContent()[WC_DTYPE] == 0))
         dtype = DAQ::FzData::Q3;
      else dtype = DAQ::FzData::UNKDT;

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << " (S3->S4) " << detid << " - " << dtype << " / " << sm.w.getContent()[WC_DETID] << "-" << sm.w.getContent()[WC_DTYPE];
#endif
 
      sm.hit->set_detid((DAQ::FzHit::FzDetector) detid); 

      sm.d = sm.hit->add_data();

      if (sm.w.getContent()[WC_FEEID] == DAQ::FzFee::ADCF) {	// ADC

         sm.d->set_type(DAQ::FzData::ADC);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if (dtype == DAQ::FzData::QH1) {

         sm.d->set_type(DAQ::FzData::QH1);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::I1) {

         sm.d->set_type(DAQ::FzData::I1);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::QL1) {

         sm.d->set_type(DAQ::FzData::QL1);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::Q2) {

         sm.d->set_type(DAQ::FzData::Q2);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::I2) {

         sm.d->set_type(DAQ::FzData::I2);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::Q3) {

         sm.d->set_type(DAQ::FzData::Q3);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else {	// unknown datatype
    
         sm.d->set_type(DAQ::FzData::UNKDT);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);
         sm.err_in_event = true;
      }  

   }
};

/* struct Action7 {

   // State4 -> State2
   template <class Fsm>
   void operator()(const gotTELHDR &e, Fsm&, State4&, State2&) const {

      sm.w = e.getWord();

      ACTLOG->debugStream() << sm.w.str();
   }
}; */

struct Action8 {

   // State4 -> State5
   template <class Fsm>
   void operator()(const gotDATA &e, Fsm &sm, State4&, State5&) const {

      // init data flags
      sm.energy1_done = sm.energy2_done = sm.pretrig_done = sm.wflen_done = false;
      sm.rd_wflen = 0;

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str() << " - " << FzDataType_str[sm.d->type()];
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      if( (sm.d->type() == DAQ::FzData::I1) || (sm.d->type() == DAQ::FzData::QL1) || (sm.d->type() == DAQ::FzData::I2) || (sm.d->type() == DAQ::FzData::ADC) || (sm.d->type() == DAQ::FzData::UNKDT) ) {
         // waveform without energy - get PRETRIG
 
         if(!sm.pretrig_done) {

            sm.wf->set_pretrig(sm.w.getContent()[WC_DATA]);
            sm.pretrig_done = true;

         } // else error

      } else if ( (sm.d->type() == DAQ::FzData::QH1) || (sm.d->type() == DAQ::FzData::Q2) || (sm.d->type() == DAQ::FzData::Q3) ) {

         // energy and waveform - get ENERGYH

         if(!sm.energy1_done) {

            sm.en->add_value(sm.w.getContent()[WC_DATA]);

         } // else error
      }
   }
};

struct Action9 {

   // State5 -> State2
   template <class Fsm>
   void operator()(const gotTELHDR &e, Fsm &sm, State5&, State2&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord(); 

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      // verify waveform length of event just acquired

      if (sm.rd_wflen == sm.wf->sample_size())
         sm.wf->set_len_error(false);
      else {
         ACTLOG->warn("(S5->S2) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", sm.blk->blkid(), FzFec_str[sm.hit->feeid()], \
		FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], sm.ev.ec(), sm.ev.ec(), sm.rd_wflen, sm.rd_wflen, sm.wf->sample_size(), \
		sm.wf->sample_size(), sm.w.getWord());
         sm.wf->set_len_error(true);
         sm.err_in_event = true;
      }

      // verify energy length of event just acquired

      // for Q3 two energy values are required
      if ( (sm.d->type() == DAQ::FzData::Q3) ) {

         if (sm.en->value_size() == 2)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
		sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
		sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }

      } else if ( (sm.d->type() == DAQ::FzData::QH1) || (sm.d->type() == DAQ::FzData::Q2) ) {	

      // for QH1, Q2 one energy value is required

         if (sm.en->value_size() == 1)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
		sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
		sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }
      }

      // verify (TELHDR[FEEID] = fee->feeid())

      sm.hit = sm.fee->add_hit();
      sm.hit->set_ec(sm.ev.ec());
      
      // validate range of FEEID
      if( (sm.w.getContent()[WC_FEEID] < DAQ::FzFee::FEC0) || (sm.w.getContent()[WC_FEEID] > DAQ::FzFee::ADCF) )
         sm.hit->set_feeid((DAQ::FzHit::FzFec) DAQ::FzFee::UNKFEC); // unknown fec  
      else
         sm.hit->set_feeid((DAQ::FzHit::FzFec) sm.w.getContent()[WC_FEEID]);

      // validate range of TELID
      if( (sm.w.getContent()[WC_TELID] < DAQ::FzHit::A) || (sm.w.getContent()[WC_TELID] > DAQ::FzHit::B) )
         sm.hit->set_telid((DAQ::FzHit::FzTelescope) DAQ::FzHit::UNKT);   // unknown telescope
      else
         sm.hit->set_telid((DAQ::FzHit::FzTelescope) sm.w.getContent()[WC_TELID]);

      //sm.hit->set_telid((DAQ::FzHit::FzTelescope) sm.w.getContent()[WC_TELID]);            
      //sm.hit->set_feeid((DAQ::FzHit::FzFec) sm.w.getContent()[WC_FEEID]); 
   }
};

struct Action10 {

   // State5 -> State4
   template <class Fsm>
   void operator()(const gotDETHDR &e, Fsm &sm, State5&, State4&) const {

      unsigned int gttag;
      unsigned int dettag;
      DAQ::FzHit::FzFec feeid;
      //DAQ::FzHit::FzDetector detid;
      DAQ::FzHit::FzTelescope telid;
      DAQ::FzData::FzDataType dtype;

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << "(S5->S4) " << sm.w.str() << sm.w.getContent()[WC_DETID] << " - " << sm.w.getContent()[WC_DTYPE];
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      // verify waveform length of event just acquired

      if (sm.rd_wflen == sm.wf->sample_size())
         sm.wf->set_len_error(false);
      else {
         ACTLOG->warn("(S5->S4) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", sm.blk->blkid(), FzFec_str[sm.hit->feeid()], \
		FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], sm.ev.ec(), sm.ev.ec(), sm.rd_wflen, sm.rd_wflen, sm.wf->sample_size(), \
		sm.wf->sample_size(), sm.w.getWord());
         sm.wf->set_len_error(true);
         sm.err_in_event = true;
      }

      // verify energy length of event just acquired

      // for Q3 two energy values are required
      if ( (sm.d->type() == DAQ::FzData::Q3) ) {

         if (sm.en->value_size() == 2)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
                sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
                sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }

      } else if ( (sm.d->type() == DAQ::FzData::QH1) || (sm.d->type() == DAQ::FzData::Q2) ) {

      // for QH1, Q2, one energy value is required
         if (sm.en->value_size() == 1)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
                sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
                sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }
      } 

      // verify (DETHDR[FEEID] == fee->feeid() == sm.hit->feeid())
      // verify (DETHDR[TELID] == sm.hit->telid())

      // copy previous hit values
      gttag = sm.hit->gttag();
      dettag = sm.hit->dettag();

      sm.hit = sm.fee->add_hit();
      sm.hit->set_ec(sm.ev.ec());
      sm.hit->set_gttag(gttag);
      sm.hit->set_dettag(dettag);

      // validate range of FEEID
      if( (sm.w.getContent()[WC_FEEID] < DAQ::FzFee::FEC0) || (sm.w.getContent()[WC_FEEID] > DAQ::FzFee::ADCF) )
         feeid = DAQ::FzHit::UNKFEC;	// unknown fec  
      else
         feeid = (DAQ::FzHit::FzFec) sm.w.getContent()[WC_FEEID]; 

      // validate range of DETID
      /* if( (sm.w.getContent()[WC_DETID] < DAQ::FzHit::Si1) || (sm.w.getContent()[WC_DETID] > DAQ::FzHit::CsI) )
         detid = DAQ::FzHit::UNKD; 	// unknown detector
      else
         detid = (DAQ::FzHit::FzDetector) sm.w.getContent()[WC_DETID]; */

      // validate range of TELID
      if( (sm.w.getContent()[WC_TELID] < DAQ::FzHit::A) || (sm.w.getContent()[WC_TELID] > DAQ::FzHit::B) )
         telid = DAQ::FzHit::UNKT;	// unknown telescope
      else
         telid = (DAQ::FzHit::FzTelescope) sm.w.getContent()[WC_TELID];	

      // validate range of DTYPE
      //if ( (sm.w.getContent()[WC_DTYPE] < DAQ::FzData::QH1) || (sm.w.getContent()[WC_DTYPE] > DAQ::FzData::ADC) ) 
      //   dtype = DAQ::FzData::UNKDT;	// unknown datatype
      //else
      // dtype = (DAQ::FzData::FzDataType) sm.w.getContent()[WC_DTYPE];
/*
      if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::QH1;

      } else if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 1)) {

         dtype = DAQ::FzData::I1; 
      
      } else if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::Si1) && (sm.w.getContent()[WC_DTYPE] == 2)) {

         dtype = DAQ::FzData::QL1;

      } else if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::Si2) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::Q2; 
   
      } else if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::Si2) && (sm.w.getContent()[WC_DTYPE] == 1)) {

         dtype = DAQ::FzData::I2;
   
      } else if( (sm.w.getContent()[WC_DETID] == DAQ::FzHit::CsI) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::Q3;

      } else {
	
        dtype = DAQ::FzData::UNKDT;
      } */

      if( (sm.w.getContent()[WC_DETID] == 0) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::QH1;

      } else if( (sm.w.getContent()[WC_DETID] == 0) && (sm.w.getContent()[WC_DTYPE] == 1)) {

         dtype = DAQ::FzData::I1; 
      
      } else if( (sm.w.getContent()[WC_DETID] == 0) && (sm.w.getContent()[WC_DTYPE] == 2)) {

         dtype = DAQ::FzData::QL1;

      } else if( (sm.w.getContent()[WC_DETID] == 1) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::Q2; 
   
      } else if( (sm.w.getContent()[WC_DETID] == 1) && (sm.w.getContent()[WC_DTYPE] == 1)) {

         dtype = DAQ::FzData::I2;
   
      } else if( (sm.w.getContent()[WC_DETID] == 2) && (sm.w.getContent()[WC_DTYPE] == 0)) {

         dtype = DAQ::FzData::Q3;

      } else {
	
        dtype = DAQ::FzData::UNKDT;
      }

      //sm.hit->set_detid((DAQ::FzHit::FzDetector) detid);      
      sm.hit->set_detid((DAQ::FzHit::FzDetector) sm.w.getContent()[WC_DETID]);      
      sm.hit->set_telid((DAQ::FzHit::FzTelescope) telid);
      sm.hit->set_feeid((DAQ::FzHit::FzFec) feeid);

      sm.d = sm.hit->add_data();

      if (feeid == DAQ::FzHit::ADCF) {       // ADC

         sm.d->set_type(DAQ::FzData::ADC);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::QH1) {

         sm.d->set_type(DAQ::FzData::QH1);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::I1) {

         sm.d->set_type(DAQ::FzData::I1);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::QL1) {

         sm.d->set_type(DAQ::FzData::QL1);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);


      } else if(dtype == DAQ::FzData::Q2) {

         sm.d->set_type(DAQ::FzData::Q2);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::I2) {

         sm.d->set_type(DAQ::FzData::I2);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else if(dtype == DAQ::FzData::Q3) {

         sm.d->set_type(DAQ::FzData::Q3);
         sm.en = sm.d->mutable_energy();
         sm.en->set_len_error(true);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);

      } else {

         sm.d->set_type(DAQ::FzData::UNKDT);
         sm.wf = sm.d->mutable_waveform();
         sm.wf->set_len_error(true);
         sm.err_in_event = true;
      }

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << " (S5->S4) " << "detid" << " - " << dtype << " / " << sm.w.getContent()[WC_DETID] << "-" << sm.w.getContent()[WC_DTYPE] << " - " << FzDataType_str[sm.d->type()];
#endif

   }
};

struct Action11 {

   // State5 -> State5
   template <class Fsm>
   void operator()(const gotDATA &e, Fsm &sm, State5&, State5&) const {
   
      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str() << " - " << FzDataType_str[sm.d->type()];
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      (sm.feelen)++;
      sm.feecrc ^= sm.w.getWord();

      if( (sm.d->type() == DAQ::FzData::QH1) || (sm.d->type() == DAQ::FzData::Q2) ) {

         // energy and waveform		( 1 word energyH, 1 word energyL , 1 word pretrig, 1 word wflen , 1 word energy , n samples )

         if(!sm.energy1_done) {

            sm.en->set_value(0, (sm.en->value(0) << 15) + sm.w.getContent()[WC_DATA]);
            sm.energy1_done = true;

         } else if(!sm.pretrig_done) {

            sm.wf->set_pretrig(sm.w.getContent()[WC_DATA]);
            sm.pretrig_done = true;

         } else if(!sm.wflen_done) {

            // store wflen for later check
            sm.rd_wflen = sm.w.getContent()[WC_DATA];
            sm.wflen_done = true;

         } else { 
 
            sm.wf->add_sample(sm.w.getContent()[WC_DATA]);
         }

      } else if(sm.d->type() == DAQ::FzData::Q3) {

         // two energy values and waveform	( 1 word energy1H, 1 word energy1L, 1 word energy2H, 1 word energy2L, 1 word pretrig, 1 word wflen, n samples )

         if(!sm.energy1_done) {

            sm.en->set_value(0, (sm.en->value(0) << 15) + sm.w.getContent()[WC_DATA]);
            sm.energy1_done = true;
         
         } else if(!sm.energy2_done) {

            if (sm.en->value_size() == 1)		// only first energy is acquired
               sm.en->add_value(sm.w.getContent()[WC_DATA]);
            else {
               sm.en->set_value(1, (sm.en->value(1) << 15) + sm.w.getContent()[WC_DATA]);
               sm.energy2_done = true;
            }

         } else if(!sm.pretrig_done) {

            sm.wf->set_pretrig(sm.w.getContent()[WC_DATA]);
            sm.pretrig_done = true;

         } else if(!sm.wflen_done) {

            // store wflen for later check
            sm.rd_wflen = sm.w.getContent()[WC_DATA];
            sm.wflen_done = true;

         } else { 

            sm.wf->add_sample(sm.w.getContent()[WC_DATA]);
         }

      } else if( (sm.d->type() == DAQ::FzData::I1) || (sm.d->type() == DAQ::FzData::I2) || (sm.d->type() == DAQ::FzData::QL1) || (sm.d->type() == DAQ::FzData::ADC) || (sm.d->type() == DAQ::FzData::UNKDT) ) {

        // waveform without energy	( 1 word pretrig, 1 word wflen, n word sample )

        /* if(!sm.pretrig_done) {

           sm.wf->set_pretrig(sm.w.getContent()[WC_DATA]);
           sm.pretrig_done = true;

        } else */ if(!sm.wflen_done) {

            // store wflen for later check
            sm.rd_wflen = sm.w.getContent()[WC_DATA];
            sm.wflen_done = true;

        } else {

           sm.wf->add_sample(sm.w.getContent()[WC_DATA]);
        }

      }
   }

};

struct Action12 {

   // State5 -> State6
   template <class Fsm>
   void operator()(const gotLENGTH &e, Fsm &sm, State5&, State6&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.rd_feelen = sm.w.getContent()[WC_LEN];

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      // verify waveform length of event just acquired

      if (sm.rd_wflen == sm.wf->sample_size())
         sm.wf->set_len_error(false);
      else {
         ACTLOG->warn("(S5->S6) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", sm.blk->blkid(), FzFec_str[sm.hit->feeid()], \
		FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], sm.ev.ec(), sm.ev.ec(), sm.rd_wflen, sm.rd_wflen, sm.wf->sample_size(), \
		sm.wf->sample_size(), sm.w.getWord());
         sm.wf->set_len_error(true);
         sm.err_in_event = true;
      }

      // verify energy length of event just acquired

      // for Q3 two energy values are required
      if ( (sm.d->type() == DAQ::FzData::Q3) ) {

         if (sm.en->value_size() == 2)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
                sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
                sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }

      } else if ( (sm.d->type() == DAQ::FzData::QH1) || (sm.d->type() == DAQ::FzData::Q2) ) {

      // for QH1, Q2 one energy value is required

         if (sm.en->value_size() == 1)
            sm.en->set_len_error(false);
         else {
            ACTLOG->warn("B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
                sm.blk->blkid(), FzFec_str[sm.hit->feeid()], FzTelescope_str[sm.hit->telid()], FzDetector_str[sm.hit->detid()], FzDataType_str[sm.d->type()], \
                sm.ev.ec(), sm.en->value_size());
            sm.en->set_len_error(true);
            sm.err_in_event = true;
         }
      }

      // fee length validation is moved on CRCFE state transition (Action13)
   }
};

struct Action13 {

   // State6 -> State7
   template <class Fsm>
   void operator()(const gotCRCFE &e, Fsm &sm, State6&, State7&) const {
   
      uint8_t crc_lsb, crc_msb, crc_calc;

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      sm.rd_feecrc = sm.w.getContent()[WC_CRC];
      sm.rd_feecrc &= 0x00FF;   // convert to 8 bit

      // verify FEE length

      if ( (sm.feelen & 0xFFF) == sm.rd_feelen ) {		// module with 0xFFF due to 12 bit width of LENGTH word

         sm.fee->set_len_error(false);

      } else {

         //
	 // FEE FPGA firmware to modify to add this feature	
         // 
         ACTLOG->warn("EC: %d FEE len error (value read: %d - value calc: %d)", sm.ev.ec(), sm.rd_feelen, sm.feelen);
         sm.fee->set_len_error(true);
         sm.err_in_event = true;
      } 

      // verify FEE CRC

      crc_lsb = (sm.feecrc & 0x00FF);
      crc_msb = (sm.feecrc & 0xFF00) >> 8;
      crc_calc = (crc_lsb ^ crc_msb) & 0x00FF;

      if( sm.rd_feecrc == crc_calc ) {

         sm.fee->set_crc_error(false);

      } else {

         //
         // FEE FPGA firmware to modify to add this feature     
         // 
         ACTLOG->warn("EC: %d FEE crc error (value read: %d - value calc: %d)", sm.ev.ec(), sm.rd_feecrc, crc_calc);
         sm.fee->set_crc_error(true);
         sm.err_in_event = true;
      } 
   }
};

struct Action14 {

   // State7 -> State1
   template <class Fsm>
   void operator()(const gotEC &e, Fsm &sm, State7&, State1&) const {
   
      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.tmp_ec = sm.w.getContent()[WC_EC];
      sm.tmp_rawec = sm.w.getWord();

      (sm.blklen)++;
      sm.blkcrc ^= sm.w.getWord();

      // verify (sm.tmp_ec == sm.ev.ec())
   }
};

struct Action15 {

   // State8 -> State9
   template <class Fsm>
   void operator()(const gotLENGTH &e, Fsm &sm, State8&, State9&) const {
        
      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.rd_blklen = sm.w.getContent()[WC_LEN];

      sm.blkcrc ^= sm.w.getWord();
      
      // block length validation is moved on CRCBL state transition (Action16)
   }
};


struct Action16 {
    
   // State9 -> State10
   template <class Fsm>
   void operator()(const gotCRCBL &e, Fsm &sm, State9&, State10&) const {

      uint8_t crc_lsb, crc_msb, crc_calc;

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.rd_blkcrc = sm.w.getContent()[WC_CRC];
      sm.rd_blkcrc &= 0x00FF;	// convert to 8 bit

      // verify block length

      if ( ((sm.blklen & 0xFFF)  + 2 ) == sm.rd_blklen ) {	// module with 0xFFF due to 12 bit width of LENGTH word

         sm.blk->set_len_error(false);

      } else {

         ACTLOG->warn("B%d - EC: %d BLOCK len error (value read: %X - value calc: %X)", sm.blk->blkid(), sm.ev.ec(), sm.rd_blklen, sm.blklen);
         sm.blk->set_len_error(true);
         sm.err_in_event = true;
      }

      // verify block CRC
         
      crc_lsb = (sm.blkcrc & 0x00FF);
      crc_msb = (sm.blkcrc & 0xFF00) >> 8;
      crc_calc = (crc_lsb ^ crc_msb) & 0x00FF;

      if( sm.rd_blkcrc == crc_calc ) {		
 
        sm.blk->set_crc_error(false);
    
      } else {

         ACTLOG->warn("B%d - EC: %d BLOCK crc error (value read: %X - value calc: %X)", sm.blk->blkid(), sm.ev.ec(), sm.rd_blkcrc, crc_calc);
         sm.blk->set_crc_error(true);
         sm.err_in_event = true;
      }
   }
};

struct Action17 { 

   // State10 -> idle
   template <class Fsm>
   void operator()(const gotREGHDR &e, Fsm &sm, State10&, IdleState&) const {

      bool event_is_empty = true;

      sm.w = e.getWord();

      sm.ev.set_regid(sm.w.getContent()[WC_REGID]);

      for(int i=0; i<sm.ev.block_size(); i++) {		// for each block

         if(sm.ev.block(i).fee_size() > 0) {

	    event_is_empty = false;
            break; 
         }
      }	// end for

      if(event_is_empty) {

         sm.event_empty_num++;

#ifdef DEBUG_ALL
         ACTLOG->info("EMPTY EVENT with EC = %d", sm.ev.ec());
#endif
  
      } else {

         // copy event to write circular buffer
         sm.cbw->send(sm.ev);

#ifdef DEBUG_ALL
         ACTLOG->debug("EVENT with EC = %d sent to write buffer", sm.ev.ec());
#endif

         if(sm.err_in_event) sm.event_witherr_num++;
            else sm.event_clean_num++;         

         sm.ev.Clear();
         sm.err_in_event = false;

      }      
   }
};

struct Action18 {

   // State10 -> State1
   template <class Fsm>
   void operator()(const gotEC &e, Fsm &sm, State10&, State1&) const {

      sm.w = e.getWord();

#ifdef DEBUG_ALL
      ACTLOG->debugStream() << sm.w.str();
#endif

      sm.blk = sm.ev.add_block();
      sm.blk->set_len_error(true);
      sm.blk->set_crc_error(true);

      sm.blklen = 1;
      sm.blkcrc = sm.w.getWord();

      sm.tmp_ec = sm.w.getContent()[WC_EC];
      sm.tmp_rawec = sm.w.getWord();
      sm.ev.set_ec(sm.w.getContent()[WC_EC]);	// useful for empty events
   }
};

#endif

#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "FzFSM.h"

//#define FSM_DEBUG
#include <google/protobuf/text_format.h>  // for debug

FzFSM::FzFSM(void) {

   fsm_report.Clear();
}

void FzFSM::init(void) {

   event = 0;
   event_size = event_index = 0;   

   state_id = 0;
}

void FzFSM::initlog(FzLogger *l) {

   log = l;
}

inline uint8_t FzFSM::getword_id(unsigned short int word) {

   uint8_t nibble3 = nibble(3, word);

   if(nibble3 < 0x8)
      return(0);        // DATA

   switch(nibble3) {

      case 0x9 :
         if(bit(11, word) == 0)
            return(16);    // DETID
         else
            return(9);     // TELID
         break;

      case 0x8 :
         return(8);	// EMPTY
         break;
   
      case 0xA :
         return(10);	// LENGTH
         break;

      case 0xB :
         return(11);	// CRCFE
         break;
      
      case 0XC :
         if(bit(11, word) == 0)
            return(17);	// BLKID
         else
            return(12); // REGID
         break;

      case 0xD :
         return(13);	// CRCBL
         break;

      case 0xE :
         return(14);	// EC
         break;

      case 0xF :
         return(15);	// EOE
         break;
  
      default :
         return(15);
   }
}

void FzFSM::import(unsigned short int *evraw, uint32_t size, DAQ::FzEvent *e) {

   event = evraw;
   event_size = size;
   state_id = 0;	// reset FSM

   ev = e;
   ev->Clear();
}

int FzFSM::process(void) {

   int retval = PARSE_FAIL;
   event_index = 0;

   fsm_report.set_in_events( fsm_report.in_events() + 1 );
   fsm_report.set_in_bytes( fsm_report.in_bytes() + (event_size*2) );	// event_size[i] contains 2 bytes

#ifdef FSM_DEBUG
   sprintf(logbuf, "received event size: %d", event_size);
   log->write(DEBUG, logbuf);
#endif

   for(event_index=0; event_index<event_size; event_index++) {

      word_id = getword_id(event[event_index]);

      if(word_id == 8) {	// EMPTY word

#ifdef FSM_DEBUG
         log->write(DEBUG, "empty word (0x8080) detected");
#endif
         continue;	// no transition
      }
      
      trans_id = ttable[word_id][state_id];

      switch(trans_id) {

         case 0: // transition not valid

#ifdef FSM_DEBUG
            sprintf(logbuf, "transition not valid WORD = %4.4X - word_id = %d - curr_state = %d - trans_id = %d\n", event[event_index], int(word_id), int(state_id), int(trans_id));
            log->write(DEBUG, logbuf);
#endif

            state_id = 0; 	// reset FSM	
	    ev->Clear();	// clear incomplete event
            fsm_report.set_state_invalid( fsm_report.state_invalid() + 1 );
            break;		
 
         case 1:		// idle    ->      (*)             -> idle
            break;

         case 2:		// idle    ->      (EC)            -> S1
            trans02();
            state_id = 1;
            break;

         case 3:		// S1      ->      (TELID)         -> S2
            trans03();
            state_id = 2;
            break;
                     
         case 4:		// S2      ->      (DATA)          -> S3
            trans04();
            state_id = 3;
            break;
                     
         case 5:
            trans05();		// S3      ->      (DATA)          -> S3
            state_id = 3;
            break;
                     
         case 6:
            trans06();		// S3      ->      (DETID)         -> S4
            state_id = 4;
            break;
                     
         case 7:
            trans07();		// S4      ->      (DATA)          -> S5
            state_id = 5;
            break;
                     
         case 8:
            trans08(); 		// S5      ->      (DATA)          -> S5
            state_id = 5;	
            break;
                     
         case 9:
            trans09();		// S5      ->      (DETID)         -> S4
            state_id = 4;
            break;
                     
         case 10:
            trans10();		// S5      ->      (TELID)         -> S2
            state_id = 2;
            break;
                     
         case 11:
            trans11();		// S5      ->      (LENGTH)        -> S6
            state_id = 6;
            break;
                     
         case 12:		// S6      ->      (CRCFE)         -> S7
            trans12();
            state_id = 7;
            break;
                     
         case 13:
            trans13();		// S7      ->      (EC)            -> S1
            state_id = 1;
            break;
                     
         case 14:
            trans14();		// S1      ->      (BLKID)         -> S8
            state_id = 8;
            break;
                     
         case 15:
            trans15();		// S8      ->      (LENGTH)        -> S9
            state_id = 9;
            break;
                     
         case 16:		// S9      ->      (CRCBL)         -> S10
            trans16();
            state_id = 10;
            break;
                     
         case 17:		// S10     ->      (EC)            -> S1
            trans17();
            state_id = 1;
            break;
                     
         case 18:		// S10     ->      (REGID)         -> idle
            trans18();
            state_id = 0;
            retval = PARSE_OK;	
            break;

         default:
            std::cout << "FzFSM: default case reached" << std::endl;
            break; 

      } // end switch

   } // end while

   fsm_report.set_out_events( fsm_report.out_events() + 1 );
   fsm_report.set_out_bytes( fsm_report.out_bytes() + (event_size*2));	// event_size[i] contains 2 bytes

   return(retval);
}

/*
void FzFSM::trans00(void) {	// transition not valid

#ifdef FSM_DEBUG
   std::cout << "transition not valid" << std::endl;
#endif
}

void FzFSM::trans01(void) {	// idle    ->      (*)             -> idle

#ifdef FSM_DEBUG
   std::cout << "idle    ->      (*)             -> idle" << std::endl;
#endif
}
*/

void FzFSM::trans02(void) {	// idle    ->      (EC)            -> S1

#ifdef FSM_DEBUG
   sprintf(logbuf, "idle    ->      (EC)            -> S1 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int ec = event[event_index] & EC_MASK;

   err_in_event = false;

   blk = ev->add_block();
   blk->set_len_error(true);
   blk->set_crc_error(true);

   blklen = 1;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc = event[event_index];

   tmp_rawec = event[event_index];
   tmp_ec = ec;
   ev->set_ec(ec);   // useful for empty events      
}

void FzFSM::trans03(void) {	// S1      ->      (TELID)         -> S2

#ifdef FSM_DEBUG
   sprintf(logbuf, "S1      ->      (TELID)         -> S2 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   DAQ::FzHit::FzTelescope telid = (DAQ::FzHit::FzTelescope) (event[event_index] & TELID_MASK);
   DAQ::FzFee::FzFec feeid = (DAQ::FzFee::FzFec) ((event[event_index] & FEEID_MASK) >> 1);

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen = 2;  // EC is just acquired
   feecrc = tmp_rawec ^ event[event_index];

   fee = blk->add_fee();
   fee->set_len_error(true);
   fee->set_crc_error(true);

   // validate range of FEEID
   if( (feeid < DAQ::FzFee::FEC0) || (feeid > DAQ::FzFee::ADCF) )
      fee->set_feeid(DAQ::FzFee::UNKFEC); // unknown fec  
   else
      fee->set_feeid((DAQ::FzFee::FzFec) feeid);

   hit = fee->add_hit();
   hit->set_ec(tmp_ec);

   // validate range of TELID
   if( (telid < DAQ::FzHit::A) || (telid > DAQ::FzHit::B) )
      hit->set_telid(DAQ::FzHit::UNKT);   // unknown telescope
   else
      hit->set_telid((DAQ::FzHit::FzTelescope) telid);

   hit->set_feeid((DAQ::FzHit::FzFec) fee->feeid());
   //hit->set_feeid((DAQ::FzHit::FzFec) feeid);   
}

void FzFSM::trans04(void) {	// S2      ->      (DATA)          -> S3

#ifdef FSM_DEBUG
   sprintf(logbuf, "S2      ->      (DATA)          -> S3 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   // verify if GTTAG already set
   hit->set_gttag(event[event_index]);
}

void FzFSM::trans05(void) {	// S3      ->      (DATA)          -> S3

#ifdef FSM_DEBUG
   sprintf(logbuf, "S3      ->      (DATA)          -> S3 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   // verify if DETTAG already set
   hit->set_dettag(event[event_index]);
}

void FzFSM::trans06(void) {	// S3      ->      (DETID)         -> S4

#ifdef FSM_DEBUG
   sprintf(logbuf, "S3      ->      (DETID)         -> S4 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf); 
#endif

   DAQ::FzFee::FzFec feeid = (DAQ::FzFee::FzFec) ((event[event_index] & FEEID_MASK) >> 1);
   unsigned short int detid = (event[event_index] & DETID_MASK) >> 5;
   unsigned short int dtype = (event[event_index] & DTYPE_MASK) >> 8;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   //DAQ::FzData::FzDataType dtype = (DAQ::FzData::FzDataType) sm.w.getContent()[WC_DTYPE];

   // validate range of DETID
   //if( (sm.w.getContent()[WC_DETID] < DAQ::FzHit::Si1) || (sm.w.getContent()[WC_DETID] > DAQ::FzHit::CsI) )
   // sm.hit->set_detid(DAQ::FzHit::UNKD);
   //else
   //   sm.hit->set_detid(detid);
      
   // verify (DETHDR[FEEID] == sm.fee->feeid() == sm.hit->feeid())
   // verify (DETHDR[TELID] == sm.hit->telid())

   //DAQ::FzData::FzDataType dtype;

   if( (detid == DAQ::FzHit::Si1) && (dtype == 0))
      dtype = DAQ::FzData::QH1;
   else if( (detid == DAQ::FzHit::Si1) && (dtype == 1))
      dtype = DAQ::FzData::I1;
   else if( (detid == DAQ::FzHit::Si1) && (dtype == 2))
      dtype = DAQ::FzData::QL1;
   else if( (detid == DAQ::FzHit::Si2) && (dtype == 0))
      dtype = DAQ::FzData::Q2;
   else if( (detid == DAQ::FzHit::Si2) && (dtype == 1))
      dtype = DAQ::FzData::I2;
   else if( (detid == DAQ::FzHit::CsI) && (dtype == 0))
      dtype = DAQ::FzData::Q3;
   else dtype = DAQ::FzData::UNKDT;

   hit->set_detid((DAQ::FzHit::FzDetector) detid); 

   d = hit->add_data();

   if (feeid == DAQ::FzFee::ADCF) {	// ADC

      d->set_type(DAQ::FzData::ADC);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if (dtype == DAQ::FzData::QH1) {

      d->set_type(DAQ::FzData::QH1);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::I1) {

      d->set_type(DAQ::FzData::I1);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::QL1) {

      d->set_type(DAQ::FzData::QL1);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::Q2) {

      d->set_type(DAQ::FzData::Q2);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::I2) {

      d->set_type(DAQ::FzData::I2);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::Q3) {

      d->set_type(DAQ::FzData::Q3);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else {	// unknown datatype
    
      d->set_type(DAQ::FzData::UNKDT);
      wf = d->mutable_waveform();
      wf->set_len_error(true);
      err_in_event = true;
   }
}

void FzFSM::trans07(void) {	// S4      ->      (DATA)          -> S5

#ifdef FSM_DEBUG
   sprintf(logbuf, "S4      ->      (DATA)          -> S5 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   // init data flags
   energy1_done = energy2_done = pretrig_done = wflen_done = false;
   rd_wflen = 0;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   if( (d->type() == DAQ::FzData::I1) || (d->type() == DAQ::FzData::QL1) || (d->type() == DAQ::FzData::I2) || (d->type() == DAQ::FzData::ADC) || (d->type() == DAQ::FzData::UNKDT) ) {
      // waveform without energy - get PRETRIG
 
      if(!pretrig_done) {

         wf->set_pretrig(event[event_index]);
         pretrig_done = true;

      } // else error

   } else if ( (d->type() == DAQ::FzData::QH1) || (d->type() == DAQ::FzData::Q2) || (d->type() == DAQ::FzData::Q3) ) {

      // energy and waveform - get ENERGYH

      if(!energy1_done) {

         en->add_value(event[event_index]);

      } // else error
   }
}

void FzFSM::trans08(void) {	// S5      ->      (DATA)          -> S5

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      (DATA)          -> S5 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   save_blkcrc = blkcrc;
   blkcrc ^= event[event_index];

   feelen++;
   save_feecrc = feecrc;
   feecrc ^= event[event_index];

   if( (d->type() == DAQ::FzData::QH1) || (d->type() == DAQ::FzData::Q2) ) {

      // energy and waveform		( 1 word energyH, 1 word energyL , 1 word pretrig, 1 word wflen , 1 word energy , n samples )

      if(!energy1_done) {

         en->set_value(0, (en->value(0) << 15) + event[event_index]);
         energy1_done = true;

      } else if(!pretrig_done) {

         wf->set_pretrig(event[event_index]);
         pretrig_done = true;

      } else if(!wflen_done) {

         // store wflen for later check
         rd_wflen = event[event_index];
         wflen_done = true;

      } else { 
 
         blkcrc = save_blkcrc;
         feecrc = save_feecrc;

         // boost patch 
         while(event[event_index] <= 0x7FFF) {

            wf->add_sample(event[event_index]);		// collect waveform samples
            
            blklen++;
            save_blkcrc = blkcrc;
            blkcrc ^= event[event_index];

            feelen++;
            save_feecrc = feecrc;
            feecrc ^= event[event_index];

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      gathering waveform     -> S5 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif
            event_index++;

         }	// end while

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      exit gathering waveform     - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

	 event_index--;
	 blklen--;
	 feelen--;
	 //blkcrc = save_blkcrc;
	 //feecrc = save_feecrc;
      }

   } else if(d->type() == DAQ::FzData::Q3) {

      // two energy values and waveform	( 1 word energy1H, 1 word energy1L, 1 word energy2H, 1 word energy2L, 1 word pretrig, 1 word wflen, n samples )

      if(!energy1_done) {

         en->set_value(0, (en->value(0) << 15) + event[event_index]);
         energy1_done = true;
         
      } else if(!energy2_done) {

         if (en->value_size() == 1)		// only first energy is acquired
            en->add_value(event[event_index]);
         else {
            en->set_value(1, (en->value(1) << 15) + event[event_index]);
            energy2_done = true;
         }

      } else if(!pretrig_done) {

         wf->set_pretrig(event[event_index]);
         pretrig_done = true;

      } else if(!wflen_done) {

         // store wflen for later check
         rd_wflen = event[event_index];
         wflen_done = true;

      } else { 

         blkcrc = save_blkcrc;
         feecrc = save_feecrc;

         // boost patch 
         while(event[event_index] <= 0x7FFF) {

            wf->add_sample(event[event_index]);		// collect waveform samples
            
            blklen++;
            save_blkcrc = blkcrc;
            blkcrc ^= event[event_index];

            feelen++;
            save_feecrc = feecrc;
            feecrc ^= event[event_index];

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      gathering waveform     -> S5 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif
            event_index++;

         }	// end while

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      exit gathering waveform     - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

	 event_index--;
	 blklen--;
	 feelen--;
	 //blkcrc = save_blkcrc;
	 //feecrc = save_feecrc;
      }

   } else if( (d->type() == DAQ::FzData::I1) || (d->type() == DAQ::FzData::I2) || (d->type() == DAQ::FzData::QL1) || (d->type() == DAQ::FzData::ADC) || (d->type() == DAQ::FzData::UNKDT) ) {

     // waveform without energy	( 1 word pretrig, 1 word wflen, n word sample )

     /* if(!sm.pretrig_done) {

        sm.wf->set_pretrig(sm.w.getContent()[WC_DATA]);
        sm.pretrig_done = true;

     } else */ if(!wflen_done) {

         // store wflen for later check
         rd_wflen = event[event_index];
         wflen_done = true;

     } else {

         blkcrc = save_blkcrc;
         feecrc = save_feecrc;

        // boost patch 
        while(event[event_index] <= 0x7FFF) {

            wf->add_sample(event[event_index]);		// collect waveform samples
            
            blklen++;
            save_blkcrc = blkcrc;
            blkcrc ^= event[event_index];

            feelen++;
            save_feecrc = feecrc;
            feecrc ^= event[event_index];

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      gathering waveform     -> S5 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif
            event_index++;

         }	// end while

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      exit gathering waveform     - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

	 event_index--;
	 blklen--;
	 feelen--;
	 //blkcrc = save_blkcrc;
	 //feecrc = save_feecrc;
     }
   }
}

void FzFSM::trans09(void) {	// S5      ->      (DETID)         -> S4

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      (DETID)         -> S4 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned int gttag;
   unsigned int dettag;
   DAQ::FzFee::FzFec feeid = (DAQ::FzFee::FzFec) ((event[event_index] & FEEID_MASK) >> 1);
   DAQ::FzHit::FzTelescope telid = (DAQ::FzHit::FzTelescope) (event[event_index] & TELID_MASK);
   unsigned short int detid = (event[event_index] & DETID_MASK) >> 5;
   unsigned short int dtype = (event[event_index] & DTYPE_MASK) >> 8;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   // verify waveform length of event just acquired

   if (rd_wflen == wf->sample_size())
      wf->set_len_error(false);
   else {
        sprintf(logbuf, "(S5->S4) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", blk->blkid(), FzFec_str[hit->feeid()], \
		FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], ev->ec(), ev->ec(), rd_wflen, rd_wflen, wf->sample_size(), \
		wf->sample_size(), event[event_index]);
        log->write(WARN, logbuf);
      wf->set_len_error(true);
      err_in_event = true;
   }

   // verify energy length of event just acquired

   // for Q3 two energy values are required
   if ( (d->type() == DAQ::FzData::Q3) ) {

      if (en->value_size() == 2)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
		blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size());
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }

   } else if ( (d->type() == DAQ::FzData::QH1) || (d->type() == DAQ::FzData::Q2) ) {

   // for QH1, Q2, one energy value is required
      if (en->value_size() == 1)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
		blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size());
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }
   } 

   // verify (DETHDR[FEEID] == fee->feeid() == sm.hit->feeid())
   // verify (DETHDR[TELID] == sm.hit->telid())

   // copy previous hit values
   gttag = hit->gttag();
   dettag = hit->dettag();

   hit = fee->add_hit();
   hit->set_ec(ev->ec());
   hit->set_gttag(gttag);
   hit->set_dettag(dettag);

   // validate range of FEEID
   if( (feeid < DAQ::FzFee::FEC0) || (feeid > DAQ::FzFee::ADCF) )
      feeid = DAQ::FzFee::UNKFEC;	// unknown fec  
   else
      feeid = (DAQ::FzFee::FzFec) feeid;

   // validate range of DETID
   /* if( (detid < DAQ::FzHit::Si1) || (detid > DAQ::FzHit::CsI) )
      detid = DAQ::FzHit::UNKD; 	// unknown detector
   else
      detid = (DAQ::FzHit::FzDetector) detid; */

   // validate range of TELID
   if( (telid < DAQ::FzHit::A) || (telid > DAQ::FzHit::B) )
      telid = DAQ::FzHit::UNKT;	// unknown telescope
   else
      telid = (DAQ::FzHit::FzTelescope) telid;

   // validate range of DTYPE
   //if ( (dtype < DAQ::FzData::QH1) || (dtype > DAQ::FzData::ADC) ) 
   //   dtype = DAQ::FzData::UNKDT;	// unknown datatype
   //else
   // dtype = (DAQ::FzData::FzDataType) dtype;

   if( (detid == 0) && (dtype == 0)) {

      dtype = DAQ::FzData::QH1;

   } else if( (detid == 0) && (dtype == 1)) {

      dtype = DAQ::FzData::I1; 
      
   } else if( (detid == 0) && (dtype == 2)) {

      dtype = DAQ::FzData::QL1;

   } else if( (detid == 1) && (dtype == 0)) {

      dtype = DAQ::FzData::Q2; 
   
   } else if( (detid == 1) && (dtype == 1)) {

      dtype = DAQ::FzData::I2;
   
   } else if( (detid == 2) && (dtype == 0)) {

      dtype = DAQ::FzData::Q3;

   } else {
	
     dtype = DAQ::FzData::UNKDT;
   }

   //hit->set_detid((DAQ::FzHit::FzDetector) detid);      
   hit->set_detid((DAQ::FzHit::FzDetector) detid);      
   hit->set_telid((DAQ::FzHit::FzTelescope) telid);
   hit->set_feeid((DAQ::FzHit::FzFec) feeid);

   d = hit->add_data();

   if (feeid == DAQ::FzFee::ADCF) {       // ADC

      d->set_type(DAQ::FzData::ADC);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::QH1) {

      d->set_type(DAQ::FzData::QH1);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::I1) {

      d->set_type(DAQ::FzData::I1);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::QL1) {

      d->set_type(DAQ::FzData::QL1);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::Q2) {

      d->set_type(DAQ::FzData::Q2);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::I2) {

      d->set_type(DAQ::FzData::I2);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else if(dtype == DAQ::FzData::Q3) {

      d->set_type(DAQ::FzData::Q3);
      en = d->mutable_energy();
      en->set_len_error(true);
      wf = d->mutable_waveform();
      wf->set_len_error(true);

   } else {

      d->set_type(DAQ::FzData::UNKDT);
      wf = d->mutable_waveform();
      wf->set_len_error(true);
      err_in_event = true;
   }
}

void FzFSM::trans10(void) {	// S5      ->      (TELID)         -> S2

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      (TELID)         -> S2 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   DAQ::FzFee::FzFec feeid = (DAQ::FzFee::FzFec) ((event[event_index] & FEEID_MASK) >> 1);
   DAQ::FzHit::FzTelescope telid = (DAQ::FzHit::FzTelescope) (event[event_index] & TELID_MASK);
   //unsigned short int detid = (event[event_index] & DETID_MASK) >> 5;
   //unsigned short int dtype = (event[event_index] & DTYPE_MASK) >> 8;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen); 
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   feelen++;
   feecrc ^= event[event_index];

   // verify waveform length of event just acquired

   if (rd_wflen == wf->sample_size())
      wf->set_len_error(false);
   else {
      sprintf(logbuf, "(S5->S2) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", blk->blkid(), FzFec_str[hit->feeid()], \
         FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], ev->ec(), ev->ec(), rd_wflen, rd_wflen, wf->sample_size(), \
         wf->sample_size(), event[event_index]); 
      log->write(WARN, logbuf);
      wf->set_len_error(true);
      err_in_event = true;
   }

   // verify energy length of event just acquired

   // for Q3 two energy values are required
   if ( (d->type() == DAQ::FzData::Q3) ) {

      if (en->value_size() == 2)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
                blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size()); 
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }

   } else if ( (d->type() == DAQ::FzData::QH1) || (d->type() == DAQ::FzData::Q2) ) {	

   // for QH1, Q2 one energy value is required

      if (en->value_size() == 1)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
                blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size()); 
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }
   }

   // verify (TELHDR[FEEID] = fee->feeid())

   hit = fee->add_hit();
   hit->set_ec(ev->ec());
      
   // validate range of FEEID
   if( (feeid < DAQ::FzFee::FEC0) || (feeid > DAQ::FzFee::ADCF) )
      hit->set_feeid((DAQ::FzHit::FzFec) DAQ::FzFee::UNKFEC); // unknown fec  
   else
      hit->set_feeid((DAQ::FzHit::FzFec) feeid);

   // validate range of TELID
   if( (telid < DAQ::FzHit::A) || (telid > DAQ::FzHit::B) )
      hit->set_telid((DAQ::FzHit::FzTelescope) DAQ::FzHit::UNKT);   // unknown telescope
   else
      hit->set_telid((DAQ::FzHit::FzTelescope) telid);

   //hit->set_telid((DAQ::FzHit::FzTelescope) telid);
   //hit->set_feeid((DAQ::FzHit::FzFec) feeid);
}

void FzFSM::trans11(void) {	// S5      ->      (LENGTH)        -> S6

#ifdef FSM_DEBUG
   sprintf(logbuf, "S5      ->      (LENGTH)        -> S6 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int len = event[event_index] & LENGTH_MASK;

   rd_feelen = len;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   // verify waveform length of event just acquired

   if (rd_wflen == wf->sample_size())
      wf->set_len_error(false);
   else {
      sprintf(logbuf, "(S5->S6) B%d.%s.Tel%s.%s-%s - EC: %d (0x%X) WAVEFORM len error (value read: %d (0x%X) - value calc: %d (0x%X) - current word: 0x%X", blk->blkid(), FzFec_str[hit->feeid()], \
         FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], ev->ec(), ev->ec(), rd_wflen, rd_wflen, wf->sample_size(), \
         wf->sample_size(), event[event_index]);
      log->write(WARN, logbuf);
      wf->set_len_error(true);
      err_in_event = true;
   }

   // verify energy length of event just acquired

   // for Q3 two energy values are required
   if ( (d->type() == DAQ::FzData::Q3) ) {

      if (en->value_size() == 2)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 2", \
                blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size());
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }

   } else if ( (d->type() == DAQ::FzData::QH1) || (d->type() == DAQ::FzData::Q2) ) {

   // for QH1, Q2 one energy value is required

      if (en->value_size() == 1)
         en->set_len_error(false);
      else {
         sprintf(logbuf, "B%d.%s.Tel%s.%s-%s - EC: %d ENERGY len error: value calc: %d and it must be 1", \
                blk->blkid(), FzFec_str[hit->feeid()], FzTelescope_str[hit->telid()], FzDetector_str[hit->detid()], FzDataType_str[d->type()], \
                ev->ec(), en->value_size());
         log->write(WARN, logbuf);
         en->set_len_error(true);
         err_in_event = true;
      }
   }
}

void FzFSM::trans12(void) {	// S6      ->      (CRCFE)         -> S7

#ifdef FSM_DEBUG
   sprintf(logbuf, "S6      ->      (CRCFE)         -> S7 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int crcfe = ((event[event_index] & CRCFE_MASK) >> 4);
   uint8_t crc_lsb, crc_msb, crc_calc;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   rd_feecrc = crcfe;
   rd_feecrc &= 0x00FF;   // convert to 8 bit

   // verify FEE length

   if ( (feelen & 0xFFF) == rd_feelen ) {		// module with 0xFFF due to 12 bit width of LENGTH word

      fee->set_len_error(false);

   } else {

      //
      // FEE FPGA firmware to modify to add this feature	
      // 
      sprintf(logbuf, "EC: %d FEE len error (value read: %d - value calc: %d)", ev->ec(), rd_feelen, feelen);
      log->write(WARN, logbuf);
      fee->set_len_error(true);
      err_in_event = true;
   } 

   // verify FEE CRC

   crc_lsb = (feecrc & 0x00FF);
   crc_msb = (feecrc & 0xFF00) >> 8;
   crc_calc = (crc_lsb ^ crc_msb) & 0x00FF;

   if( rd_feecrc == crc_calc ) {

      fee->set_crc_error(false);

   } else {

      //
      // FEE FPGA firmware to modify to add this feature     
      // 
      sprintf(logbuf, "EC: %d FEE crc error (value read: %d - value calc: %d)", ev->ec(), rd_feecrc, crc_calc);
      log->write(WARN, logbuf);
      fee->set_crc_error(true);
      err_in_event = true;
   } 
}

void FzFSM::trans13(void) {	// S7      ->      (EC)            -> S1

#ifdef FSM_DEBUG
   sprintf(logbuf, "S7      ->      (EC)            -> S1 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int ec = event[event_index] & EC_MASK;

   tmp_ec = ec;
   tmp_rawec = event[event_index];

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   // verify (tmp_ec == ev->ec())
}

void FzFSM::trans14(void) {	// S1      ->      (BLKID)         -> S8

#ifdef FSM_DEBUG
   sprintf(logbuf, "S1      ->      (BLKID)         -> S8 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int blkid = event[event_index] & BLKID_MASK;

   blklen++;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc ^= event[event_index];

   blk->set_blkid(blkid);
}

void FzFSM::trans15(void) {	// S8      ->      (LENGTH)        -> S9

#ifdef FSM_DEBUG
   sprintf(logbuf, "S8      ->      (LENGTH)        -> S9 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif
 
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int len = event[event_index] & LENGTH_MASK; 

   rd_blklen = len;

   blkcrc ^= event[event_index];
      
   // block length validation is moved on CRCBL state transition
}

void FzFSM::trans16(void) {	// S9      ->      (CRCBL)         -> S10

#ifdef FSM_DEBUG
   sprintf(logbuf, "S9      ->      (CRCBL)         -> S10 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int crcbl = ((event[event_index] & CRCBL_MASK) >> 4);
   uint8_t crc_lsb, crc_msb, crc_calc;

   rd_blkcrc = crcbl;
   rd_blkcrc &= 0x00FF;	// convert to 8 bit

   // verify block length

   if ( ((blklen & 0x0FFF) + 2 ) == rd_blklen ) {	// module with 0xFFF due to 12 bit width of LENGTH word

      blk->set_len_error(false);

   } else {

      sprintf(logbuf, "B%d - EC: %d BLOCK len error (value read: %X (%d) - value calc: %X (%d))", blk->blkid(), ev->ec(), rd_blklen, rd_blklen, (blklen & 0x0FFF), (blklen & 0x0FFF));
      log->write(WARN, logbuf);
      blk->set_len_error(true);
      err_in_event = true;
   }

   // verify block CRC
         
   crc_lsb = (blkcrc & 0x00FF);
   crc_msb = (blkcrc & 0xFF00) >> 8;
   crc_calc = (crc_lsb ^ crc_msb) & 0x00FF;

   if( rd_blkcrc == crc_calc ) {		
 
     blk->set_crc_error(false);
    
     // check if this block is a 'fake block' containing trigger info
     if(blk->blkid() == 0x7FF) {

        if(blk->fee_size() > 0)
           read_triggerinfo(blk, ev);

        ev->mutable_block()->RemoveLast(); 
     }

   } else {

      sprintf(logbuf, "B%d - EC: %d BLOCK crc error (value read: %X - value calc: %X)", blk->blkid(), ev->ec(), rd_blkcrc, crc_calc);
      log->write(WARN, logbuf);
      blk->set_crc_error(true);
      err_in_event = true;
   }
}

void FzFSM::trans17(void) {	// S10     ->      (EC)            -> S1

#ifdef FSM_DEBUG
   sprintf(logbuf, "S10     ->      (EC)            -> S1 - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int ec = event[event_index] & EC_MASK;

   blk = ev->add_block();
   blk->set_len_error(true);
   blk->set_crc_error(true);

   blklen = 1;
#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif
   blkcrc = event[event_index];

   tmp_rawec = event[event_index];
   tmp_ec = ec;
   ev->set_ec(ec);	// useful for empty events
}

void FzFSM::trans18(void) {	// S10     ->      (REGID)         -> idle

#ifdef FSM_DEBUG
   sprintf(logbuf, "S10     ->      (REGID)         -> idle - word: %4.4X", event[event_index]);
   log->write(DEBUG, logbuf);
#endif

#ifdef FSM_DEBUG
   sprintf(logbuf, "blklen: %d", blklen);
   log->write(DEBUG, logbuf);
#endif

   unsigned short int regid = event[event_index] & REGID_MASK;

   event_is_empty = true;

   ev->set_regid(regid);

   if(ev->trinfo_size() > 0) {		// an event that includes only trigger info is not empty...

      event_is_empty = false;

   } else {

      for(int i=0; i<ev->block_size(); i++) {		// for each block

         if(ev->block(i).fee_size() > 0) {

            event_is_empty = false;
            break; 
         }
      }	// end for

   }

   if(event_is_empty) {

      fsm_report.set_events_empty( fsm_report.events_empty() + 1 );
      sprintf(logbuf, "EC: %d - got empty event\n", ev->ec());
      log->write(DEBUG, logbuf);

   } else {

      if(err_in_event) 
         fsm_report.set_events_bad( fsm_report.events_bad() + 1 );   
      else fsm_report.set_events_good( fsm_report.events_good() + 1 );   

      err_in_event = false;
   }      
}

Report::FzFSM FzFSM::get_report(void) {
   return(fsm_report);
}

void FzFSM::read_triggerinfo(DAQ::FzBlock *blk, DAQ::FzEvent *ev) {

   const DAQ::FzFee& fee = blk->fee(0); 
   const DAQ::FzHit& hit = fee.hit(0);
   const DAQ::FzData& data = hit.data(0);
   const DAQ::Waveform wf = data.waveform();
   DAQ::FzTrigInfo *tri = NULL;

   uint32_t supp;
   uint8_t idx;

   for(int i=0; i<wf.sample_size(); i=i+3) {	

      supp = 0;
 
      tri = ev->add_trinfo();
      tri->set_id(wf.sample(i));
      idx = wf.sample(i) - 0x100;

      if( (idx >= 0) && (idx <= 15) )
         tri->set_attr(FzTriggerInfo_str[idx]);
      else
         tri->set_attr("unknown");
      
      supp = wf.sample(i+1) << 15;
      supp += wf.sample(i+2);

      if(wf.sample(i) == 0x10C) {

         supp = (supp * 4) / 150E6;

      } else if(wf.sample(i) == 0x10D) {

         supp = (supp << 15) + hit.gttag();
 
      } else if(wf.sample(i) == 0x10E) {

         supp = (supp << 12) + hit.ec();

      } else if(wf.sample(i) == 0x10F) {

         supp = supp & 0x00FF;
      }

      tri->set_value(supp);
   }     

/*
   std::string text_str;
   for(int i=0; i<ev->trinfo_size(); i++) {
      google::protobuf::TextFormat::PrintToString(ev->trinfo(i), &text_str);
      std::cout << text_str << std::endl;
   }
*/
}

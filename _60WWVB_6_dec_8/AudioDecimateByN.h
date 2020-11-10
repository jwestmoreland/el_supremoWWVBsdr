//------------------------------------------------------------------------------
//-- AudioDecimateByN.h - A variable integer factor down-sampler for use with the
//                      Teensy Audio Library
//
//-- Notes: This is a strict downsampler and DOES NOT include a pre-decimation
//        LP anti-aliasing filter.  It is suggested that this be preceded by a
//        4-stage biquad filter (IIR).
//
//-- Author:  Derek Rowell   8/29/2017
//------------------------------------------------------------------------------
#ifndef audioddecimatebyn_h_
#define audioddecimatebyn_h_
#include "AudioStream.h"
//--
class AudioDecimateByN : public AudioStream
  {
  public:
    AudioDecimateByN() : AudioStream(1, inputQueueArray) {outblock = allocate();}
// --
   virtual void update(void);
//--
   void factor(uint16_t down_factor){          
      decimate_factor = down_factor;
      return;
   }
//--
  private:
    audio_block_t  *inputQueueArray[1];
    audio_block_t  *outblock;
    int16_t        in_ptr  =  0;
    int16_t        out_ptr =  0;
    uint16_t       decimate_factor = 2;
};
#endif

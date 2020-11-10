//------------------------------------------------------------------------------
//-- AudioDecimateByN.cpp - A variable integer factor down-sampler for use with the
//                        Teensy Audio Library
//
//-- Notes: This is a strict downsampler and DOES NOT include a pre-decimation
//        LP anti-aliasing filter.  It is suggested that this be preceded by a
//        4-stage biquad filter (IIR).
//
//-- Author:  Derek Rowell   8/29/2017
//------------------------------------------------------------------------------
//*** In normal use, comment out the following line to prevent the serial prints
//#define DEBUG
//***
#include <Arduino.h>
#include "AudioDecimateByN.h"
//------------------------------------------------------------------------------
void AudioDecimateByN::update(void) { 
  audio_block_t *inblock;
  inblock  = receiveReadOnly(0);
  if (!inblock) return;
// --
     while (in_ptr<AUDIO_BLOCK_SAMPLES) { 
       if (out_ptr == 0)  outblock = allocate();                 // Get a new output block
       outblock->data[out_ptr] = inblock->data[in_ptr];         // Move a sample to the output block
       out_ptr++;
       if (out_ptr == AUDIO_BLOCK_SAMPLES){                     // The output buffer is full;
          transmit(outblock, 0);                                // Transmit the full block 
          release(outblock);
          out_ptr = 0;                                          // Start a new output block...
#ifdef DEBUG   // Suggestion: For debugging, use the Serial Plotter to look at the output data
          for (int i=0; i<128; i++) Serial.println(outblock->data[i]);
#endif
       }
       in_ptr += decimate_factor;
     }
     in_ptr -= AUDIO_BLOCK_SAMPLES;                           // Set pointer to the first valid sample in next block
     release(inblock);
     return;
   }

     
       
           

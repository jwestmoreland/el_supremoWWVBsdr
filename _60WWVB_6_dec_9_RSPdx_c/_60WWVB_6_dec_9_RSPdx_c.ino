#define VERSION     "v0.6_dec_9_RSPdx_c"
/*
201103
_9_RSPdx_c
  - Remove extraneous code from _8 (e.g. the decimator code)

201102
_8_RSPdx_c
YES! Now records at 8kHz.
Now to add the SD card.
  Would it be possible to run this at a sample rate of 8kHz
  and record the audio to the SD card?
Hmmm. It records at 48kHz and decimates by 6 so it's already
doing the decoding at 8kHz anyway. Can the samples be written
to the card?

200219
_7_RSPdx_c
  - remove all extraneous code from the original version

_7_RSPdx_b
  turn debug output off but print the value of each sample
IT WORKS!!
RSPdx WWVB CW 775Hz
CW with 500Hz filter
RSPdx Volume about 0.1
PC volume 23
(use level 30 when recording with the Tascam DR-40)
AGC off
Uses LINE-IN via the 1:1 transformer cable



_7_RSPdx_a
  - try to get the audio from the RSPdx through
    the 1:1 transformer into the T3.6 and decode
    WWVB
Originally this sampled at 192kHz. Decimate sampling frequency
of 48kHz down to 8kHz.
  It appears to be hearing the signal and trying to decode
  but so far not succeeding


191101
- Using the T3.6 which has audio board and the Hauptwerk/MIDI
  stuff on it.
Getting the hardware ready to use this version for
  the WWVB DST indicators change tomorrow (2nd at 6pmCST)
  and Sunday (3rd at 6pm CST)
- *MUST* use the 12v 9A battery to power the
  Teensy
- the female DC adapter doesn't work so the Banggood DC-DC
  converter can't be used. Instead I'm using my
  5V regulator board.
- to handle the SD card, see R:\WWVB_Audio\wwvb_procedure.txt




181101
  This is the most recent version. It has my own version of AGC
  and decodes better than 6_dec_2_rel (the version originally
  intended for release)
  
https://forum.pjrc.com/threads/40102
  Converted from DCF77 to WWVB 60kHz
  by Pete (El_Supremo)

  _6_dec_6
    - user can type comment which will be printed to monitor

  _6_dec_5
    - switch the order of the x6 and x4 decimators so that
      the first one produces a sample rate of 48kHz which
      might come in useful since the Tascam DR40 can produce
      audio files at 48kHz.
 
  _6_dec_4
    - add AGC and determine decoding level from that

  _6_dec_3
    - add a lopass biquad filter in front of each
      stage of decimation.

  _6_dec_2
    - reworked the test for 64 chars on a line.

  _6_dec_1 This decodes WWVB with mag_limit=100000
          See info.h for the first output I got from it.
          This is using the two antennas - one active, one passive.
    - remove FFT and add processing of samples
      at 8kHz
    - add call to iq_init()
    - c_count counts number of chars printed on a line.
      If it reaches 64, start a newline.
      
  _5_dec_2
  - add lineout so that I can try to record
    with Sony minidisc.
  
  _5_dec_1
  _ fork from _5 and add decimation of the
    output from biquad2 by a factor of 24
    to bring the sampling rate down to 8kHz.
    Then the output of the decimator goes to
    a queue which passes the samples to this
    sketch so that it can process them with
    a different decoder.
    (That's the plan anyway)
    
  _5
  - test the WWVB decoder when TEST_WWVB_DECODER
    is defined in wwvb.h - see info.h for example
    of the output
  - Change the decoding to look for the
    WWVB 0, 1 and P pulses

  _4
  - Changed DCF77 to WWVB in the code

  _3
  -
  _2
  - Add 600Hz sidetone which required a mixer and a button
    This allows you to hear what tone should be generated if
    WWVB is being received.
  - changed a few "long" to uint32_t

  _1
  - Removed all the display stuff with #ifdef USE_DISPLAY
*/

/***********************************************************************
   Copyright (c) 2016, Frank Bösing, f.boesing@gmx.de & Frank DD4WH, dd4wh.swl@gmail.com

    Teensy DCF77 Receiver & Real Time Clock

    uses only minimal hardware to receive accurate time signals

    For information on how to setup the antenna, see here:

    https://github.com/DD4WH/Teensy-DCF77/wiki
*/


#include "wwvb.h"
#include "samples.h"
#include "setI2SFreq.h"

/*
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice, development funding notice, and this permission
   notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/

#include "config.h"

#include <TimeLib.h>
#include <Bounce.h>
#include <Audio.h>
#include <SPI.h>
#include <Metro.h>


// When grounded, this adds a SIDETONE_FREQUENCY
// tone to the audio output so that you can hear
// what the WWVB (or DCF77) signal should sound
// like
#define SIDETONE_PIN 29
Bounce sidetone = Bounce(SIDETONE_PIN, 5);
float sidetone_volume = 0.008;

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}


// GUItool: begin automatically generated code
AudioInputI2S            i2s_in;         //xy=459,381
AudioRecordQueue         queue1;         //xy=1437,398
AudioOutputI2S           i2s_out;        //xy=1457,499
// Record at 8kHz and feed straight to the queue
AudioConnection          patchCord8c(i2s_in, 0, queue1, 0);
AudioConnection          patchCord10(i2s_in, 0, i2s_out, 0);
AudioConnection          patchCord11(i2s_in, 0, i2s_out, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=482,252
// GUItool: end automatically generated code

const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

// Metro 1 second
Metro second_timer = Metro(1000);

// Direct from linein at 8kHz
const unsigned int sample_rate = SAMPLE_RATE_8K;


// Used in AGC
float wwvb_med = 0;

//=========================================================================

void setup(void)
{
  Serial.begin(115200);

  uint32_t now = millis();
  while(!Serial) {
    if(millis() - now > 5000)break;    
  }
  
#ifdef TEST_WWVB_DECODER
  while(!Serial);
  test_wwvb_decoder();
  Serial.println("DONE");
  while(1);
#endif

//  while(!Serial);

  Serial.printf("%s\n",VERSION);
  Serial.print(__TIME__);
  Serial.print(" CST ");
  Serial.println(__DATE__);
  

//>>>
  Serial.print("Sidetone = ");
  Serial.print(SIDETONE_FREQUENCY);
  Serial.print(", mag_limit = ");
  Serial.println(mag_limit,0);
  pinMode(LED_PIN,OUTPUT);

  setSyncProvider(getTeensy3Time);

  iq_init();

  // Audio connections require memory.
  AudioMemory(32);

  // Enable the audio shield. select input. and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.95);

  // Pass the signal through to the lineout - don't need this for
  // decoding but it doesn't hurt anything
  sgtl5000_1.unmuteLineout();
  
  // According to info in libraries\Audio\control_sgtl5000.cpp
  // 31 is LOWEST output of 1.16V and 13 is HIGHEST output of 3.16V
  // but this doesn't make a difference
  sgtl5000_1.lineOutLevel(15);
  
  set_sample_rate(sample_rate);

  display_settings();

AudioNoInterrupts();
  queue1.begin();
AudioInterrupts();
} // END SETUP


// serial input
#define IN_LENGTH 100
char instring[IN_LENGTH+2];
int c_idx = 0;

// temorary buffer for samples
int16_t s_buffer[AUDIO_BLOCK_SAMPLES];

void loop(void)
{
  int16_t *sp = NULL;

  static int volume = 0;
  int n = analogRead(15);
  if (n != volume) {
    volume = n;
    sgtl5000_1.volume(n / 1023.);
  }

  // This allows you to add a comment to the Serial Monitor
  // output. Whatever you type in the serial monitor will be
  // printed with a semicolon added on the fron. E.g.
  // ; this is a comment
  // But NOTE that it is printed immediately and will
  // interfere with the decoder output which will still be
  // correct but spread across 2 lines.
  if(Serial.available() > 0) {
    instring[c_idx] = Serial.read();
    if(instring[c_idx] == '\n') {
      instring[c_idx] = 0;
      Serial.print("\n;");
      Serial.println(instring);
      c_idx = 0;
      return;      
    }
    c_idx++;
    if(c_idx >= IN_LENGTH) {
      instring[c_idx] = 0;
      Serial.print("\n;");
      Serial.println(instring);
      c_idx = 0;
    }
  }

  if (queue1.available() >= 1) {
    // Get address of packet, copy the samples to s_buffer
    // then release the audio buffer
    sp = queue1.readBuffer();
    memcpy(s_buffer,sp,AUDIO_BLOCK_SAMPLES*sizeof(int16_t));
    queue1.freeBuffer();
    
    // Process the 128 samples in the block
    for(int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      process_sample(*sp++);
    }
  }
}

void check_processor(void)
{
  if (second_timer.check() == 1) {
    Serial.print("Proc = ");
    Serial.print(AudioProcessorUsage());
    Serial.print(" (");
    Serial.print(AudioProcessorUsageMax());
    Serial.print("),  Mem = ");
    Serial.print(AudioMemoryUsage());
    Serial.print(" (");
    Serial.print(AudioMemoryUsageMax());
    Serial.println(")");
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
  }
} // END function check_processor


void display_settings(void)
{
  Serial.printf("sample_rate: %dkHz\n",sample_rate_real / 1000);
}

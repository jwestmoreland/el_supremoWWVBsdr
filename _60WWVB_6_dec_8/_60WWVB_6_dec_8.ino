/*

191101
- Using the T3.6 which has audio board and the Hauptwerk/MIDI
  stuff on it.
Getting the hardware ready to use this version for
  the WWVB DST indicators change tomorrow (2nd at 6pmCST)
  and Sunday (3rd at 6pm CST)
- *MUST* use the 12v 9A-hr battery to power the
  Teensy
- the female DC adapter doesn't work so the Banggood DC-DC
  converter can't be used. Instead I'm using my
  5V regulator board.
- to handle the Tascam SD card, see R:\WWVB_Audio\wwvb_procedure.txt




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
  >>> This is now a user-selectable frequency SIDETONE_FREQUENCY
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

#ifdef __MK66FX1M0__
// T3.6 code
#else
// T4.1 code
#endif


#define VERSION     " v0.6_dec_7"
#include "AudioDecimateByN.h"
#include "wwvb.h"
#include "samples.h"

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

#define USE_DISPLAY
#define USE_FFT
// #define USE_SIDETONE   // will disable for now --- aj6bc/jcw

#include <TimeLib.h>
#include <Bounce.h>
#include <Audio.h>
#include <SPI.h>
#include <Metro.h>

#ifdef USE_DISPLAY
#include <ILI9341_t3.h>
#include "font_Arial.h"
#endif
#include <utility/imxrt_hw.h>
// When grounded, this adds a 600Hz tone to the
// audio output so that you can hear what the
// WWVB (or DCF77) signal should sound like
#ifdef USE_SIDETONE
#define SIDETONE_PIN 29
#endif

#ifdef USE_SIDETONE
Bounce sidetone = Bounce(SIDETONE_PIN, 5);
float sidetone_volume = 0.008;
#endif

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

//#include <FlexiBoard.h>

#define SAMPLE_RATE_MIN               0
#define SAMPLE_RATE_8K                0
#define SAMPLE_RATE_11K               1
#define SAMPLE_RATE_16K               2
#define SAMPLE_RATE_22K               3
#define SAMPLE_RATE_32K               4
#define SAMPLE_RATE_44K               5
#define SAMPLE_RATE_48K               6
#define SAMPLE_RATE_88K               7
#define SAMPLE_RATE_96K               8
#define SAMPLE_RATE_176K              9
#define SAMPLE_RATE_192K              10
#define SAMPLE_RATE_MAX               10
// GUItool: begin automatically generated code
AudioInputI2S            i2s_in;         //xy=459,381
AudioFilterBiquad        biquad1;        //xy=643,374
AudioSynthWaveformSine   sine1;          //xy=663,471
AudioEffectMultiply      mult1;          //xy=885,462
AudioFilterBiquad        biquad2;        //xy=1087,462
#ifdef USE_FFT
AudioAnalyzeFFT1024      myFFT;          //xy=834,372
#endif
#ifdef USE_SIDETONE
AudioSynthWaveformSine   sine600;        //xy=1086,516

// 23kHz lopass at 192kHz in to x4 decimator
AudioFilterBiquad        biquad3;
// 3800Hz lopass at 48kHz in to x6 decimator
AudioFilterBiquad        biquad4;
#endif

AudioDecimateByN         decimator6;      //xy=1281.88330078125,397.8833312988281
AudioDecimateByN         decimator4;      //xy=1281.88330078125,397.8833312988281

#ifdef USE_SIDETONE
AudioMixer4              mixer_output;   //xy=1284,497
AudioRecordQueue         queue1;         //xy=1437,398
AudioOutputI2S           i2s_out;        //xy=1457,499
#endif

AudioConnection          patchCord1(i2s_in, 0, biquad1, 0);
AudioConnection          patchCord2(biquad1, 0, mult1, 0);
#ifdef USE_FFT
AudioConnection          patchCord3(biquad1, myFFT);
#endif
AudioConnection          patchCord4(sine1, 0, mult1, 1);
AudioConnection          patchCord5a(mult1, biquad2);
#ifdef USE_SIDETONE
AudioConnection          patchCord5b(mult1, biquad3);

AudioConnection          patchCord6(sine600, 0, mixer_output, 1);
AudioConnection          patchCord7(biquad2, 0, mixer_output, 0);
// #endif
AudioConnection          patchCord8a(biquad3, 0, decimator4, 0);
AudioConnection          patchCord8b(decimator4, 0, biquad4, 0);
AudioConnection          patchCord8c(biquad4, 0, decimator6, 0);
AudioConnection          patchCord9(decimator6, 0, queue1, 0);
// #ifdef USE_SIDETONE
AudioConnection          patchCord10(mixer_output, 0, i2s_out, 0);
AudioConnection          patchCord11(mixer_output, 0, i2s_out, 1);
#endif
AudioControlSGTL5000     sgtl5000_1;     //xy=482,252
// GUItool: end automatically generated code
//const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// Metro 1 second
Metro second_timer = Metro(1000);

const uint16_t FFT_points = 1024;
//const uint16_t FFT_points = 256;

// Mic gain from 0 to 63dB
int8_t mic_gain = 60 ;//start detecting with this MIC_GAIN in dB
// originally 40
const float bandpass_q = 70;
// #1 = 59712
// #2 = 58967
const float WWVB_FREQ = 60000.0; //WWVB 60 kHz
// start detecting at this frequency, so that
// you can hear the carrier as an audible tone
// whose frequency is specified as SIDETONE_FREQUENCY
unsigned int freq_real = WWVB_FREQ - SIDETONE_FREQUENCY;

const unsigned int sample_rate = SAMPLE_RATE_192K;
unsigned int sample_rate_real = 192000;

unsigned int freq_LO = 7000;
float wwvb_signal = 0;
float wwvb_threshold = 100;
float wwvb_med = 0;
unsigned int wwvb_bin;// this is the FFT bin where the 60kHz signal is

bool timeflag = 0;
const int8_t pos_x_date = 14;
const int8_t pos_y_date = 68;
const int8_t pos_x_time = 14;
const int8_t pos_y_time = 110;
uint8_t hour10_old;
uint8_t hour1_old;
uint8_t minute10_old;
uint8_t minute1_old;
uint8_t second10_old;
uint8_t second1_old;
uint8_t precision_flag = 0;
int bit;

const float displayscale = 5;

#ifdef USE_DISPLAY
#define BACKLIGHT_PIN 22
#define TFT_DC      5
#define TFT_CS      14
#define TFT_RST     255  // 255 = unused. connect to 3.3V
#define TFT_MOSI     11
#define TFT_SCLK    13
#define TFT_MISO    12

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

typedef struct SR_Descriptor {
  const int SR_n;
  const char* const f1;
  const char* const f2;
  const char* const f3;
  const char* const f4;
  const float32_t x_factor;
} SR_Desc;

// Text and position for the FFT spectrum display scale
const SR_Descriptor SR[SAMPLE_RATE_MAX + 1] = {
  //   SR_n ,  f1, f2, f3, f4, x_factor = pixels per f1 kHz in spectrum display
  {  SAMPLE_RATE_8K,  " 1", " 2", " 3", " 4", 64.0}, // which means 64 pixels per 1 kHz
  {  SAMPLE_RATE_11K,  " 1", " 2", " 3", " 4", 43.1},
  {  SAMPLE_RATE_16K,  " 2", " 4", " 6", " 8", 64.0},
  {  SAMPLE_RATE_22K,  " 2", " 4", " 6", " 8", 43.1},
  {  SAMPLE_RATE_32K,  "5", "10", "15", "20", 80.0},
  {  SAMPLE_RATE_44K,  "5", "10", "15", "20", 58.05},
  {  SAMPLE_RATE_48K,  "5", "10", "15", "20", 53.33},
  {  SAMPLE_RATE_88K,  "10", "20", "30", "40", 58.05},
  {  SAMPLE_RATE_96K,  "10", "20", "30", "40", 53.33},
  {  SAMPLE_RATE_176K,  "20", "40", "60", "80", 58.05},
  {  SAMPLE_RATE_192K,  "20", "40", "60", "80", 53.33} // which means 53.33 pixels per 20kHz
};


//const char* const Days[7] = { "Samstag", "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag"};
const char* const Days[7] = { "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
#endif



//=========================================================================

void setup(void)
{
  Serial.begin(115200);

  uint32_t now = millis();
  while(!Serial) {
    if(millis() - now > 5000)break;    
  }


//  while(!Serial);

  Serial.printf("_60WWVB %s\n",VERSION);
  Serial.print(__TIME__);
  Serial.print(" CST ");
  Serial.println(__DATE__);
#ifdef USE_SIDETONE  
  pinMode(SIDETONE_PIN, INPUT_PULLUP);
#endif  
//>>>
  Serial.print("Carrier = ");
  Serial.print(WWVB_FREQ,2);
  Serial.print(", Q = ");
  Serial.print(bandpass_q,2);
#ifdef USE_SIDETONE  
  Serial.print(", sidetone = ");
  Serial.print(SIDETONE_FREQUENCY);
#endif  
  Serial.print(", mag_limit = ");
  Serial.println(mag_limit,0);
#ifdef USE_SIDETONE  
  pinMode(LED_PIN,OUTPUT);   /// not sure about this rigtht now... --- aj6bc/jcw
#endif
  setSyncProvider(getTeensy3Time);

  iq_init();

  // Audio connections require memory.
  AudioMemory(32);

  // Enable the audio shield. select input. and enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.95);
  sgtl5000_1.micGain (mic_gain);
  sgtl5000_1.adcHighPassFilterDisable(); // does not help too much!

  // I want output on the line out too
  sgtl5000_1.unmuteLineout();
  // According to info in libraries\Audio\control_sgtl5000.cpp
  // 31 is LOWEST output of 1.16V and 13 is HIGHEST output of 3.16V
  // but this doesn't make a difference
  sgtl5000_1.lineOutLevel(15);

#ifdef USE_DISPLAY
  // Init TFT display
  pinMode( BACKLIGHT_PIN, OUTPUT );
  analogWrite( BACKLIGHT_PIN, 1023 );
  tft.begin();
  tft.setRotation( 3 );
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(14, 7);
  tft.setTextColor(ILI9341_ORANGE);
  tft.setFont(Arial_12);
  tft.print("Teensy WWVB Receiver "); tft.print(VERSION);
  tft.setTextColor(ILI9341_WHITE);
#endif

  set_sample_rate (sample_rate);
  set_freq_LO (freq_real);

  display_settings();
  //  decodeTelegram( 0x8b47c0501a821b80ULL );
#ifdef USE_DISPLAY
  displayDate();
  displayClock();
  displayPrecisionMessage();
#endif

#ifdef USE_SIDETONE
AudioNoInterrupts();
  mixer_output.gain(0, 1.0 - sidetone_volume);
  mixer_output.gain(1, sidetone_volume);
  sine600.frequency(AUDIO_SAMPLE_RATE_EXACT / sample_rate_real * SIDETONE_FREQUENCY);
  decimator6.factor(6);
  decimator4.factor(4);
  queue1.begin();
AudioInterrupts();
#endif
} // END SETUP


// serial input
#define IN_LENGTH 100
char instring[IN_LENGTH+2];
int c_idx = 0;

void loop(void)
{
  int16_t *sp = NULL;

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
#ifdef USE_SIDETONE  
  sidetone.update();
  if(sidetone.fallingEdge()) {
    mixer_output.gain(1, sidetone_volume);
  }
  if(sidetone.risingEdge()) {
    mixer_output.gain(1, 0.0);
  }
#endif  

#ifdef USE_FFT
  if (myFFT.available()) {
    agc();
    detectBit();
#ifdef USE_DISPLAY
    spectrum();
    displayClock();
#endif
  }
#endif
#ifdef USE_SIDETONE 
  if (queue1.available() >= 1) {
    // Get address of packet and process the samples
    sp = queue1.readBuffer();
    // Process the 128 samples in the block
    for(int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      process_sample(*sp++);
    }
    queue1.freeBuffer();

#ifdef NOTDEF
static int q_count = 0;
    q_count++;
    if(q_count == 64) {
      q_count = 0;
      Serial.println("q_count");
    }
#endif
  }
#endif  
}

void set_mic_gain(int8_t gain)
{
  // AudioNoInterrupts();
  sgtl5000_1.micGain (mic_gain);
  // AudioInterrupts();
  //  display_settings();
} // end function set_mic_gain

void set_freq_LO(int freq)
{
  // audio lib thinks we are still in 44118sps sample rate
  // therefore we have to scale the frequency of the local oscillator
  // in accordance with the REAL sample rate
  freq_LO = freq * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real);
  // if we switch to LOWER samples rates, make sure the running LO
  // frequency is allowed ( < 22k) ! If not, adjust consequently, so that
  // LO freq never goes up 22k, also adjust the variable freq_real
  if (freq_LO > 22000) {
    freq_LO = 22000;
    freq_real = freq_LO * (sample_rate_real / AUDIO_SAMPLE_RATE_EXACT) + 9;
  }
  AudioNoInterrupts();
  sine1.frequency(freq_LO);
  AudioInterrupts();
  //  display_settings();
} // END of function set_freq_LO


void set_sample_rate(int sr)
{
  switch(sr) {
  case SAMPLE_RATE_8K:
    sample_rate_real = 8000;
    break;
  case SAMPLE_RATE_11K:
    sample_rate_real = 11025;
    break;
  case SAMPLE_RATE_16K:
    sample_rate_real = 16000;
    break;
  case SAMPLE_RATE_22K:
    sample_rate_real = 22050;
    break;
  case SAMPLE_RATE_32K:
    sample_rate_real = 32000;
    break;
  case SAMPLE_RATE_44K:
    sample_rate_real = 44100;
    break;
  case SAMPLE_RATE_48K:
    sample_rate_real = 48000;
    break;
  case SAMPLE_RATE_88K:
    sample_rate_real = 88200;
    break;
  case SAMPLE_RATE_96K:
    sample_rate_real = 96000;
    break;
  case SAMPLE_RATE_176K:
    sample_rate_real = 176400;
    break;
  case SAMPLE_RATE_192K:
    sample_rate_real = 192000;
    break;
  }
  AudioNoInterrupts();
///  sample_rate_real = setI2SFreq(sample_rate_real);
  setI2SFreq(sample_rate_real);   /// returns void now
  if(sample_rate_real == 0) {
    Serial.printf("ERROR: failed to set sampling frequency\n");
    while(1);
  }
  delay(200); // this delay seems to be very essential !
  set_freq_LO(freq_real);

// biquad2 was 5000. Now 3800
  biquad2.setLowpass(0, 3800 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 0.54);
  biquad2.setLowpass(1, 3800 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 1.3);
  biquad2.setLowpass(2, 3800 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 0.54);
  biquad2.setLowpass(3, 3800 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 1.3);

  biquad1.setBandpass(0, WWVB_FREQ * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), bandpass_q);
  biquad1.setBandpass(1, WWVB_FREQ * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), bandpass_q);
  biquad1.setBandpass(2, WWVB_FREQ * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), bandpass_q);
  biquad1.setBandpass(3, WWVB_FREQ * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), bandpass_q);
#ifdef USE_SIDETONE
// 23kHz lopass at 192kHz in to x4 decimator
  biquad3.setLowpass(0, 23000 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 0.54);
  biquad3.setLowpass(1, 23000 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 1.3);
  biquad3.setLowpass(2, 23000 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 0.54);
  biquad3.setLowpass(3, 23000 * (AUDIO_SAMPLE_RATE_EXACT / sample_rate_real), 1.3);

// 3800Hz lopass at 48kHz in to x6 decimator
  biquad4.setLowpass(0, 3800 * (AUDIO_SAMPLE_RATE_EXACT / 48000), 0.54);
  biquad4.setLowpass(1, 3800 * (AUDIO_SAMPLE_RATE_EXACT / 48000), 1.3);
  biquad4.setLowpass(2, 3800 * (AUDIO_SAMPLE_RATE_EXACT / 48000), 0.54);
  biquad4.setLowpass(3, 3800 * (AUDIO_SAMPLE_RATE_EXACT / 48000), 1.3);
#endif
  AudioInterrupts();
  delay(20);
  wwvb_bin = round((WWVB_FREQ / (sample_rate_real / 2.0)) * (FFT_points / 2));

#ifdef USE_DISPLAY
  //  display_settings();
  prepare_spectrum_display();
#endif

} // END function set_sample_rate

//#define FFT_DEBUG
#ifdef FFT_DEBUG
// print the FFT amplitude every 50th FFT output
int fft_print = 50;
#endif
void agc()
{
  // PAH - long should be uint32_t
  static uint32_t tspeed = millis(); //Timer for startup
// Original values:
//  const float speed_agc_start = 0.95;   //initial speed AGC
//  const float speed_agc_run   = 0.9995;

  const float speed_agc_start = 0.95;   //initial speed AGC
  const float speed_agc_run   = 0.9995;
  static float speed_agc = speed_agc_start;
  // PAH - long should be uint32_t
  static uint32_t tagc = millis(); //Timer for AGC

  const float speed_thr = 0.995;

#ifdef USE_DISPLAY
  //  tft.drawFastHLine(14, 220 - wwvb_med, 256, ILI9341_BLACK);
  tft.drawFastHLine(220, 220 - wwvb_med, 46, ILI9341_BLACK);
#endif

#ifdef USE_FFT
  wwvb_signal = myFFT.output[wwvb_bin];
#endif
#ifdef FFT_DEBUG
  fft_print--;
  if(fft_print == 0) {
    Serial.printf("%8.3f\n", wwvb_signal);
    fft_print = 50;
  }
#endif
  wwvb_signal = abs(wwvb_signal) * displayscale;
  if (wwvb_signal > 175) wwvb_signal  = 175;
  else if (wwvb_med == 0) wwvb_med = wwvb_signal;
  wwvb_med = (1 - speed_agc) * wwvb_signal + speed_agc * wwvb_med;
#ifdef USE_DISPLAY
  tft.drawFastHLine(220, 220 - wwvb_med, 46, ILI9341_ORANGE);

  tft.drawFastHLine(220, 220 - wwvb_threshold, 46, ILI9341_BLACK);
#endif

  wwvb_threshold = (1 - speed_thr) * wwvb_signal + speed_thr * wwvb_threshold;

#ifdef USE_DISPLAY
  tft.drawFastHLine(220, 220 - wwvb_threshold, 46, ILI9341_GREEN);
#endif
  // PAH - long should be uint32_t
  uint32_t t = millis();
  //Slow down speed after a while
  if((t - tspeed > 1500) && (t - tspeed < 1530)) {
    if (speed_agc < speed_agc_run) {
      speed_agc = speed_agc_run;
      Serial.printf("Set AGC-Speed %f\n", speed_agc);
    }
  }
//#ifdef NOTDEF
  if (t - tagc > 2221) {
    tagc = t;
    if (wwvb_med > 160 && mic_gain > 30) {
      mic_gain--;
      set_mic_gain(mic_gain);
      Serial.printf("Gain: %d\n", mic_gain);
    } else if (wwvb_med < 100 && mic_gain < 58) {
      mic_gain++;
      set_mic_gain(mic_gain);
      Serial.printf("Gain: %d\n", mic_gain);
    }
  }
//#endif
}

int getParity(uint32_t value)
{
  int par = 0;

  while(value) {
    value = value & (value - 1);
    par = ~par;
  }
  return ~par & 1;
}


int decodeTelegram(uint64_t telegram)
{
  uint8_t minute, hour, day, weekday, month, year;
  int parity;

  //Plausibility checks and decoding telegram
  //Example-Data: 0x8b47c14f468f9ec0ULL : 2016/11/20

  //https://de.wikipedia.org/wiki/DCF77

  //TODO : more plausibility-checks to prevent false positives

  //right shift for convienience
  telegram = telegram >> 5;

  //Check 1-Bit:
  if ((telegram >> 20) == 0) {
    Serial.printf("1-Bit is 0\n");
    return 0;
  }

  //1. decode date & date-parity-bit
  parity = telegram >> 58 & 0x01;
  year = ((telegram >> 54) & 0x0f) * 10 + ((telegram >> 50) & 0x0f);
  month = ((telegram >> 49) & 0x01) * 10 + ((telegram >> 45) & 0x0f);
  weekday = ((telegram >> 42) & 0x07);
  day = ((telegram >> 40) & 0x03) * 10 + ((telegram >> 36) & 0x0f);
  //  Serial.printf( "Date: %s, %d.%d.20%d P:%d %d", Days[weekday+1], day, month, year, parity, getParity( (telegram>>36) & 0x3ffff) );

  //Plausibility
  if ((weekday == 0) || (month == 0) || (month > 12) || (day == 0) || (day > 31) || (year < 16) ) {
    //Todo add check on 29.feb, 30/31 and more...
    Serial.printf(" is NOT plausible.\n");
    return 0;
  }
  Serial.printf(" is plausible.\n");

  //2. decode time & date-parity-bit
  parity = telegram >> 35 & 0x01;
  hour = (telegram >> 33 & 0x03) * 10 + (telegram >> 29 & 0x0f);
  Serial.printf( "Hour: %d P:%d %d\n", hour, parity, getParity( hour) );
  if (hour > 23) {
    Serial.printf(" is NOT plausible.\n");
    return 0;
  }
  Serial.printf(" is plausible.\n");

  parity = telegram >> 28 & 0x01;
  minute = (telegram >> 25 & 0x07) * 10 + (telegram >> 21 & 0x0f);
  Serial.printf( "Minute: %d P:%d %d\n", minute, parity, getParity( minute ) );
  if (minute > 59) {
    Serial.printf(" is NOT plausible.\n");
    return 0;
  }
  Serial.printf(" is plausible.\n");

  //All data seem to be ok.
  //TODO: Set & display Date and Time ...
  setTime (hour, minute, 0, day, month, year);
  Teensy3Clock.set(now());
#ifdef USE_DISPLAY
  displayDate();
#endif
  return 1;
}


// DCF
// 0.1s - 0
// 0.2s - 1
// No position marker.
// WWVB - duration of reduced carrier power
// 0.8s - Position marker
// 0.5s - 1
// 0.2s - 0

void decode(unsigned long t)
{
#ifdef ORIGINAL_DECODER
  static uint64_t data = 0;
  static int sec = 0;
  static unsigned long tlastBit = 0;


  //  int bit;
  Serial.printf("Bit = %ld\n",t);
  if ( millis() - tlastBit > 1600) {
    Serial.printf(" End Of Telegram. Data: 0x%llx %d\n", data, sec);
#ifdef USE_DISPLAY
    tft.fillRect(14, 54, 59 * 5, 3, ILI9341_BLACK);
#endif
    if (sec == 59) {
      precision_flag = decodeTelegram(data);
#ifdef USE_DISPLAY
      displayPrecisionMessage();
#endif
    }

    sec = 0;
    data = 0;
  }
  tlastBit = millis();

  if (t > 150) {
    bit = 1;
  } else bit = 0;
  Serial.print(bit);
  // plot horizontal bar
#ifdef USE_DISPLAY
  tft.fillRect(14 + 5 * sec, 54, 3, 3, ILI9341_YELLOW);
#endif
  data = ( data >> 1) | ((uint64_t)bit << 63);

  sec++;

#else

// WWVB decoder
  static int char_count = 0;
  uint8_t x_pulse = xlate(t);
  Serial.printf("%c",x_pulse);
  char_count++;
  if(char_count >= 64) {
    Serial.println();
    char_count = 0;
  }
  if(x_pulse == '?') {
    // reset the decoder and start looking for
    // the first of two Position pulses
    p_idx = 0;
    return;
  }
  time_char[p_idx] = x_pulse;
  if(p_idx < 2) {

    // The first two pulses must be P
    if(x_pulse != 'P') {
      p_idx = 0;
      return;
    }
    // If P_leap_second is set there will
    // be an extra 'P' pulse caused by
    // a leap-second. Ignore it and reset
    // the flag
    if(P_leap_second) {
      Serial.println("P_leap_second detected - skip this extra 'P'");
      char_count = 0;
      P_leap_second = 0;
      return;
    }
    p_idx++;
    if(p_idx == 2) {
      Serial.println("Got PP");
      char_count = 0;
    }
    return;
  }
  // Every 10th pulse must be a 'P'
  // otherwise it must be 0 or 1
  if((p_idx % 10) == 0) {
    if(x_pulse != 'P') {
      Serial.printf("ERROR: Missing 'P' at p_idx=%d\n",p_idx);
      p_idx = 0;
      char_count = 0;
      return;
    }
    p_idx++;
    return;
  }
  // Must be 0 or 1. We've already removed
  // possibility of '?' so only bad one remaining
  // is 'P'
  if(x_pulse == 'P') {
    p_idx = 0;
    Serial.println("ERROR: received 'P' - should be '0' or '1'");
    char_count = 0;
    return;
  }
  // Everything so far is good.
  // If we have just decoded the last data bit,
  // decode the message
  if(p_idx == 59) {
    p_idx++;
    time_char[p_idx] = 0;
    decode_wwvb(time_char);
    p_idx = 0;
    char_count = 0;
    return;
  }
  p_idx++;
  return;
#endif
}

void detectBit(void)
{
  static float wwvb_threshold_last = 1000;
  //>>> Pete - should be unsigned, not long
  static uint32_t secStart = 0;

  if ( wwvb_threshold <= wwvb_threshold_last) {
    if (secStart == 0) {
      secStart = millis();
      //tft.fillRect(300, 10, 16, 16, ILI9341_RED);
    }
    //>>> Pete
    // This turns the LED on because I want the LED to
    // display the inverted output of the detector which
    // is what the WWV detector board/antenna shows.
    digitalWrite(LED_PIN,1);
  } else {
    digitalWrite(LED_PIN,0);
    unsigned long t = millis() - secStart;
    if ((secStart > 0) && (t > 90)) {
      decode(t);
#ifdef USE_DISPLAY
      tft.fillRect(291, 5, 18, 18, ILI9341_BLACK);
      tft.setFont(Arial_12);
      tft.setTextColor(ILI9341_WHITE);
      tft.setCursor(295, 8);
      tft.print(bit);
#endif
    }
    secStart = 0;
#ifdef USE_DISPLAY
    //    tft.fillRect(300, 10, 16, 16, ILI9341_BLACK);
#endif
  }
  wwvb_threshold_last = wwvb_threshold;
}


#if 0
int setI2SFreq(int freq)
{
  typedef struct {
    uint8_t mult;
    uint16_t div;
  } tmclk;

  const int numfreqs = 14;
  const int samplefreqs[numfreqs] = { 8000, 11025, 16000, 22050, 32000, 44100, (int)44117.64706 , 48000, 88200, (int)44117.64706 * 2, 96000, 176400, (int)44117.64706 * 4, 192000};

#if (F_PLL==16000000)
  const tmclk clkArr[numfreqs] = {{16, 125}, {148, 839}, {32, 125}, {145, 411}, {64, 125}, {151, 214}, {12, 17}, {96, 125}, {151, 107}, {24, 17}, {192, 125}, {127, 45}, {48, 17}, {255, 83} };
#elif (F_PLL==72000000)
  const tmclk clkArr[numfreqs] = {{32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {128, 1125}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375}, {249, 397}, {32, 51}, {185, 271} };
#elif (F_PLL==96000000)
  const tmclk clkArr[numfreqs] = {{8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {32, 375}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125}, {151, 321}, {8, 17}, {64, 125} };
#elif (F_PLL==120000000)
  const tmclk clkArr[numfreqs] = {{32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {128, 1875}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625}, {178, 473}, {32, 85}, {145, 354} };
#elif (F_PLL==144000000)
  const tmclk clkArr[numfreqs] = {{16, 1125}, {49, 2500}, {32, 1125}, {49, 1250}, {64, 1125}, {49, 625}, {4, 51}, {32, 375}, {98, 625}, {8, 51}, {64, 375}, {196, 625}, {16, 51}, {128, 375} };
#elif (F_PLL==168000000)
  const tmclk clkArr[numfreqs] = {{32, 2625}, {21, 1250}, {64, 2625}, {21, 625}, {128, 2625}, {42, 625}, {8, 119}, {64, 875}, {84, 625}, {16, 119}, {128, 875}, {168, 625}, {32, 119}, {189, 646} };
#elif (F_PLL==180000000)
  const tmclk clkArr[numfreqs] = {{46, 4043}, {49, 3125}, {73, 3208}, {98, 3125}, {183, 4021}, {196, 3125}, {16, 255}, {128, 1875}, {107, 853}, {32, 255}, {219, 1604}, {214, 853}, {64, 255}, {219, 802} };
#elif (F_PLL==192000000)
  const tmclk clkArr[numfreqs] = {{4, 375}, {37, 2517}, {8, 375}, {73, 2483}, {16, 375}, {147, 2500}, {1, 17}, {8, 125}, {147, 1250}, {2, 17}, {16, 125}, {147, 625}, {4, 17}, {32, 125} };
#elif (F_PLL==216000000)
  const tmclk clkArr[numfreqs] = {{32, 3375}, {49, 3750}, {64, 3375}, {49, 1875}, {128, 3375}, {98, 1875}, {8, 153}, {64, 1125}, {196, 1875}, {16, 153}, {128, 1125}, {226, 1081}, {32, 153}, {147, 646} };
#elif (F_PLL==240000000)
  const tmclk clkArr[numfreqs] = {{16, 1875}, {29, 2466}, {32, 1875}, {89, 3784}, {64, 1875}, {147, 3125}, {4, 85}, {32, 625}, {205, 2179}, {8, 85}, {64, 625}, {89, 473}, {16, 85}, {128, 625} };
#endif

  for (int f = 0; f < numfreqs; f++) {
    if ( freq == samplefreqs[f] ) {
      while (I2S0_MCR & I2S_MCR_DUF) ;
      I2S0_MDR = I2S_MDR_FRACT((clkArr[f].mult - 1)) | I2S_MDR_DIVIDE((clkArr[f].div - 1));
      return round(((float)F_PLL / 256.0) * clkArr[f].mult / clkArr[f].div); //return real freq
    }
  }
  return 0;
}
#endif

void setI2SFreq(int freq) {

  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (freq * 256 * n1);
  double C = ((double)freq * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2, true);
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
               | CCM_CS1CDR_SAI1_CLK_PRED(n1 - 1) // &0x07
               | CCM_CS1CDR_SAI1_CLK_PODF(n2 - 1); // &0x3f

  //START//Added afterwards to make the SAI2 function at the desired frequency as well.

  CCM_CS2CDR = (CCM_CS2CDR & ~(CCM_CS2CDR_SAI2_CLK_PRED_MASK | CCM_CS2CDR_SAI2_CLK_PODF_MASK))
               | CCM_CS2CDR_SAI2_CLK_PRED(n1 - 1) // &0x07
               | CCM_CS2CDR_SAI2_CLK_PODF(n2 - 1); // &0x3f)

  //END//Added afterwards to make the SAI2 function at the desired frequency as well.
}

void check_processor()
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
    /*      tft.fillRect(100,120,200,80,ILI9341_BLACK);
          tft.setCursor(10, 120);
          tft.setTextSize(2);
          tft.setTextColor(ILI9341_WHITE);
          tft.setFont(Arial_14);
          tft.print ("Proc = ");
          tft.setCursor(100, 120);
          tft.print (AudioProcessorUsage());
          tft.setCursor(180, 120);
          tft.print (AudioProcessorUsageMax());
          tft.setCursor(10, 150);
          tft.print ("Mem  = ");
          tft.setCursor(100, 150);
          tft.print (AudioMemoryUsage());
          tft.setCursor(180, 150);
          tft.print (AudioMemoryUsageMax());
    */
    AudioProcessorUsageMaxReset();
    AudioMemoryUsageMaxReset();
  }
} // END function check_processor


void display_settings()
{
#ifdef USE_DISPLAY
  tft.fillRect(14, 32, 200, 17, ILI9341_BLACK);
  tft.setCursor(14, 32);
  tft.setFont(Arial_12);
  tft.print("gain: "); tft.print (mic_gain);
  tft.print("     ");
  tft.print("freq: "); tft.print (freq_real);
  tft.print("    ");
  tft.fillRect(232, 32, 88, 17, ILI9341_BLACK);
  tft.setCursor(232, 32);
  tft.print("       ");
  tft.print(sample_rate_real / 1000); tft.print("k");
#else
  //gain: 60, freq: 59400, sample_rate: 192kHz, FFT bin = 320
  //Set AGC-Speed 0.999500
  Serial.printf("gain: %d, freq: %d, sample_rate: %dkHz, FFT bin = %d\n", mic_gain,
                freq_real, sample_rate_real / 1000, wwvb_bin);
#endif
}

#ifdef USE_DISPLAY

void spectrum()   // spectrum analyser code by rheslip - modified
{

  static int barm [512];

  for (unsigned int x = 2; x < FFT_points / 2; x++) {
#ifdef USE_FFT
    int bar = abs(myFFT.output[x]) * displayscale;
#endif
    if (bar > 175) bar = 175;

    // this is a very simple first order IIR filter to smooth the reaction of the bars
    bar = 0.05 * bar + 0.95 * barm[x];
    tft.drawPixel(x / 2 + 10, 210 - barm[x], ILI9341_BLACK);
    tft.drawPixel(x / 2 + 10, 210 - bar, ILI9341_WHITE);
    barm[x] = bar;
  }
} // end void spectrum


void prepare_spectrum_display()
{
  int base_y = 211;
  int b_x = 10;
  int x_f = SR[sample_rate].x_factor;
  tft.fillRect(0, base_y, 320, 240 - base_y, ILI9341_BLACK);
  //    tft.drawFastHLine(b_x, base_y + 2, 256, ILI9341_PURPLE);
  //    tft.drawFastHLine(b_x, base_y + 3, 256, ILI9341_PURPLE);
  tft.drawFastHLine(b_x, base_y + 2, 256, ILI9341_MAROON);
  tft.drawFastHLine(b_x, base_y + 3, 256, ILI9341_MAROON);
  // vertical lines
  tft.drawFastVLine(b_x - 4, base_y + 1, 10, ILI9341_YELLOW);
  tft.drawFastVLine(b_x - 3, base_y + 1, 10, ILI9341_YELLOW);
  tft.drawFastVLine( x_f + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  tft.drawFastVLine( x_f + 1 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  tft.drawFastVLine( x_f * 2 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  tft.drawFastVLine( x_f * 2 + 1 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  if (x_f * 3 + b_x < 256 + b_x) {
    tft.drawFastVLine( x_f * 3 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
    tft.drawFastVLine( x_f * 3 + 1 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  }
  if (x_f * 4 + b_x < 256 + b_x) {
    tft.drawFastVLine( x_f * 4 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
    tft.drawFastVLine( x_f * 4 + 1 + b_x,  base_y + 1, 10, ILI9341_YELLOW);
  }
  tft.drawFastVLine( x_f * 0.5 + b_x,  base_y + 1, 6, ILI9341_YELLOW);
  tft.drawFastVLine( x_f * 1.5 + b_x,  base_y + 1, 6, ILI9341_YELLOW);
  tft.drawFastVLine( x_f * 2.5 + b_x,  base_y + 1, 6, ILI9341_YELLOW);
  if (x_f * 3.5 + b_x < 256 + b_x) {
    tft.drawFastVLine( x_f * 3.5 + b_x,  base_y + 1, 6, ILI9341_YELLOW);
  }
  if (x_f * 4.5 + b_x < 256 + b_x) {
    tft.drawFastVLine( x_f * 4.5 + b_x,  base_y + 1, 6, ILI9341_YELLOW);
  }
  // text
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(Arial_9);
  int text_y_offset = 16;
  int text_x_offset = - 5;
  // zero
  tft.setCursor (b_x + text_x_offset, base_y + text_y_offset);
  tft.print(0);
  tft.setCursor (b_x + x_f + text_x_offset, base_y + text_y_offset);
  tft.print(SR[sample_rate].f1);
  tft.setCursor (b_x + x_f * 2 + text_x_offset, base_y + text_y_offset);
  tft.print(SR[sample_rate].f2);
  tft.setCursor (b_x + x_f * 3 + text_x_offset, base_y + text_y_offset);
  tft.print(SR[sample_rate].f3);
  tft.setCursor (b_x + x_f * 4 + text_x_offset, base_y + text_y_offset);
  tft.print(SR[sample_rate].f4);
  //    tft.setCursor (b_x + text_x_offset + 256, base_y + text_y_offset);
  tft.print(" kHz");

  tft.setFont(Arial_14);
} // END prepare_spectrum_display

void displayPrecisionMessage()
{
  if (precision_flag) {
    tft.fillRect(14, 32, 300, 18, ILI9341_BLACK);
    tft.setCursor(14, 32);
    tft.setFont(Arial_11);
    tft.setTextColor(ILI9341_GREEN);
    tft.print("Full precision of time and date");
    tft.drawRect(290, 4, 20, 20, ILI9341_GREEN);
  } else {
    tft.fillRect(14, 32, 300, 18, ILI9341_BLACK);
    tft.setCursor(14, 32);
    tft.setFont(Arial_11);
    tft.setTextColor(ILI9341_RED);
    tft.print("Unprecise, trying to collect data");
    tft.drawRect(290, 4, 20, 20, ILI9341_RED);
  }
} // end function displayPrecisionMessage

void displayClock()
{

  uint8_t hour10 = hour () / 10 % 10;
  uint8_t hour1 = hour() % 10;
  uint8_t minute10 = minute() / 10 % 10;
  uint8_t minute1 = minute() % 10;
  uint8_t second10 = second() / 10 % 10;
  uint8_t second1 = second() % 10;
  uint8_t time_pos_shift = 26;
  tft.setFont(Arial_28);
  tft.setTextColor(ILI9341_WHITE);
  uint8_t dp = 14;

  // set up ":" for time display
  if (!timeflag) {
    tft.setCursor(pos_x_time + 2 * time_pos_shift, pos_y_time);
    tft.print(":");
    tft.setCursor(pos_x_time + 4 * time_pos_shift + dp, pos_y_time);
    tft.print(":");
    tft.setCursor(pos_x_time + 7 * time_pos_shift + 2 * dp, pos_y_time);
    //      tft.print("UTC");
  }

  if (hour10 != hour10_old || !timeflag) {
    tft.setCursor(pos_x_time, pos_y_time);
    tft.fillRect(pos_x_time, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    if (hour10) tft.print(hour10);  // do not display, if zero
  }
  if (hour1 != hour1_old || !timeflag) {
    tft.setCursor(pos_x_time + time_pos_shift, pos_y_time);
    tft.fillRect(pos_x_time  + time_pos_shift, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    tft.print(hour1);  // always display
  }
  if (minute1 != minute1_old || !timeflag) {
    tft.setCursor(pos_x_time + 3 * time_pos_shift + dp, pos_y_time);
    tft.fillRect(pos_x_time  + 3 * time_pos_shift + dp, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    tft.print(minute1);  // always display
  }
  if (minute10 != minute10_old || !timeflag) {
    tft.setCursor(pos_x_time + 2 * time_pos_shift + dp, pos_y_time);
    tft.fillRect(pos_x_time  + 2 * time_pos_shift + dp, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    tft.print(minute10);  // always display
  }
  if (second10 != second10_old || !timeflag) {
    tft.setCursor(pos_x_time + 4 * time_pos_shift + 2 * dp, pos_y_time);
    tft.fillRect(pos_x_time  + 4 * time_pos_shift + 2 * dp, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    tft.print(second10);  // always display
  }
  if (second1 != second1_old || !timeflag) {
    tft.setCursor(pos_x_time + 5 * time_pos_shift + 2 * dp, pos_y_time);
    tft.fillRect(pos_x_time  + 5 * time_pos_shift + 2 * dp, pos_y_time, time_pos_shift, time_pos_shift + 2, ILI9341_BLACK);
    tft.print(second1);  // always display
  }

  hour1_old = hour1;
  hour10_old = hour10;
  minute1_old = minute1;
  minute10_old = minute10;
  second1_old = second1;
  second10_old = second10;
  timeflag = 1;

} // end function displayTime

void displayDate()
{
  char string99 [20];
  tft.fillRect(pos_x_date, pos_y_date, 320 - pos_x_date, 20, ILI9341_BLACK); // erase old string
  tft.setTextColor(ILI9341_ORANGE);
  tft.setFont(Arial_16);
  tft.setCursor(pos_x_date, pos_y_date);
  //  Date: %s, %d.%d.20%d P:%d %d", Days[weekday-1], day, month, year
  sprintf(string99, "%s, %02d.%02d.%04d", Days[weekday()], day(), month(), year());
  tft.print(string99);
} // end function displayDate
#endif

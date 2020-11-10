#include <Arduino.h>
#include "wwvb.h"
#include "config.h"

// in the .ino file
void decode(unsigned long t);

// The original sampling frequency of
// 192000/sec has been decimated by a
// factor of 24 down to 8000/sec
#define SAMPLING_FREQ 8000

#define BAUD_RATE 100

// The sidetone
#define MARK_FREQ 775

/*
  There are 40 samples per bit. This specifies how many samples
  to skip. e.g. 10 will mean that there will be 4 samples per
  printed bit and 8 would mean there will be 5 samples per printed
  bit
*/
#define NUM_SKIP 400

extern uint8_t debug_flag;

// For now, try this with float
#define double float
double i_mark[SAMPLING_FREQ/BAUD_RATE];
double q_mark[SAMPLING_FREQ/BAUD_RATE];

uint32_t n_sample = 0;

float im_sum = 0, qm_sum = 0;
int cb_idx = 0;


int o_count = 0;
int p_count = 0;
int c_count = 0;
char decoded[64];
int dec_idx = 0;

// AGC
extern float wwvb_med;
double mag_limit = 100000000L;
double mag_fraction = 0.18;
float speed_agc = 0.75;
double max_agc = 0;
int max_agc_count = 0;
double agc_val;


void process_sample(int16_t s)
{
  int cb_j;
  float sample;

#ifdef BLOCK_COUNT_TIME
  static int b_count = 0;
  static uint32_t b_time;
  if(b_count == 0) {
    b_time = millis();
  }
  b_count++;
  if(b_count == 8000) {
    b_count = 0;
    Serial.println(millis() - b_time);
  }
#endif

  if(cb_idx < SAMPLING_FREQ/BAUD_RATE) {
    im_sum += s*i_mark[cb_idx];
    qm_sum += s*q_mark[cb_idx];
    cb_idx++;
    return;
  }
  if(cb_idx == SAMPLING_FREQ/BAUD_RATE) {
    sample = im_sum*im_sum + qm_sum*qm_sum;
// Process the sample
//    Serial.println(sample,1);
    if(wwvb_med == 0) wwvb_med = sample;
    wwvb_med = (1 - speed_agc) * sample + speed_agc * wwvb_med;
  // Find the highest value of wwvb_med over the last second
  if(wwvb_med > max_agc)max_agc = wwvb_med;
  max_agc_count++;
  if(max_agc_count >= 20) {
    max_agc_count = 0;
    // Set the decision limit for decoding to an empirically
    // determined fraction of the highest wwvb_med.
    // 0.2 seems to be a good value.
    mag_limit = max_agc*mag_fraction;
//printf("mag_limit = %14.1f\n",mag_limit);
    max_agc = 0;
  }
      
    if(sample > mag_limit) {
      o_count++;
      digitalWrite(LED_PIN,1);
    } else {
      digitalWrite(LED_PIN,0);
// DON'T FORGET that this is INVERTED data. i.e. the tone is
// when the carrier is at full power. WWVB encodes the bits
// by dropping the power. A Position marker is 0.8secs of
// LOW power. Therefore in that one-second period, there is
// a period of 0.8 secs LOW power and 0.2 secs HIGH power.
// This is why the 'P' is detected here using a range of
// from 3 to 6 samples which includes the nominal
// 4 sample width of 0.2 seconds HIGH power.
      if(o_count) {
//        Serial.printf("   %3d",o_count);
        // Try to decode it
        if(o_count >= 3 && o_count <= 6) {
          if(debug_flag)Serial.print("P");
          decoded[dec_idx++] = 'P';
          decoded[dec_idx] = 0;
          c_count++;
          if(++p_count == 2) {
            p_count = 0;
            decoded[dec_idx] = 0;
            decode_wwvb(decoded);
            //decode-wwvb prints a line feed so reset the counter
            dec_idx = 0;
//          Serial.print(" **\n");
          }
        } else if(o_count >= 9 && o_count <= 12) {
          if(debug_flag)Serial.print("1");
          decoded[dec_idx++] = '1';
          decoded[dec_idx] = 0;
          p_count = 0;
          c_count++;
        } else if(o_count >= 14 && o_count <= 18) {
          if(debug_flag)Serial.print("0");
          decoded[dec_idx++] = '0';
          decoded[dec_idx] = 0;
          p_count = 0;
          c_count++;
        }

        if(c_count >= 64) {
          Serial.println();
          c_count = 0;
        }
        if(dec_idx > 62) {
          dec_idx = 0;
          Serial.println();
        }
      }
      o_count = 0;
    }

    im_sum = 0;
    qm_sum = 0;
    cb_idx++;
    return;
  }
  if(cb_idx >= NUM_SKIP-1) {
    cb_idx = 0;
  } else {
    cb_idx++;
  }

}

// Initialize the I and Q correlation samples
void iq_init(void)
{
  int i;

  for(i = 0; i < SAMPLING_FREQ/BAUD_RATE; i++) {
    i_mark[i] = cos(2*M_PI*i*MARK_FREQ/SAMPLING_FREQ);
    q_mark[i] = sin(2*M_PI*i*MARK_FREQ/SAMPLING_FREQ);
  }
}

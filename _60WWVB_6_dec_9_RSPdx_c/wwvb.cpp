#include <Arduino.h>
#include "wwvb.h"

// Room for 60 characters plus the null at the end
char time_char[61];
int p_idx = 0;

// Indicates that this is the last minute of last day of Jun or Dec
// and the leap second flag is on. This will cause a P sync failure
// at 00:00 (the next minute) because of the extra P that is sent
// when the leap second occurs.
char P_leap_second = 0;

// temporary for sprintf
char s_tmp[64];

// something's wrong with the Arduino abs() or I'm using it
// incorrectly somehow. Roll my own for now.
uint32_t iabs(int32_t a)
{
  if(a >= 0)return(a);
  return(-a);
}

// Translate a WWVB pulse length to a character
char xlate(int plen)
{
  if(iabs(plen - 800) <= PULSE_DT)return('P');
  if(iabs(plen - 500) <= PULSE_DT)return('1');
  if(iabs(plen - 200) <= PULSE_DT)return('0');
  return('?');
}

// Leap year calculator and days of month from the Time library
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0

// Convert the day of the year to month and day
void get_month_day(int c_jdays, int c_year, int *c_month, int *c_day)
{
  for(int i = 0; i < 12; i++) {
    if(c_jdays <= monthDays[i]) {
      *c_month = i + 1;
      *c_day = c_jdays;
      return;
    }
    c_jdays -= monthDays[i];
    if((i == 1) && LEAP_YEAR(c_year - 1970))c_jdays -= 1;
  }
}

/*
Example input string - see also t_vector in the
  wwvb decoder test below:
          11111111112222222222333333333344444444445555555555
012345678901234567890123456789012345678901234567890123456789
PP10101000P001000011P001100110P011000000P000000001P011001000
^--- Note that this first P marker in the string occurs
      at mm:59 of the minute. The next 'P' at index 1
      occurs at mm:00
*/
uint8_t debug_flag = 1;
#ifdef OLD_VERSION
// The decoder assumes that the caller has
// already verified that the string is
// 60 characters long and contains 'P'
// pulses in the correct positions and
// '0' or '1' everywhere else
void decode_wwvb(char *dt)
{
  int days,mins,hours,wyear;
  int t_month = 0,t_day = 0;

  Serial.printf("> decode '%s'\n",dt);
  days = (dt[23] - '0') * 200;
  days += (dt[24] - '0') * 100;
  days += (dt[26] - '0') * 80;
  days += (dt[27] - '0') * 40;
  days += (dt[28] - '0') * 20;
  days += (dt[29] - '0') * 10;
  days += (dt[31] - '0') * 8;
  days += (dt[32] - '0') * 4;
  days += (dt[33] - '0') * 2;
  days += (dt[34] - '0');

  mins = (dt[2] - '0') * 40;
  mins += (dt[3] - '0') * 20;
  mins += (dt[4] - '0') * 10;
  mins += (dt[6] - '0') * 8;
  mins += (dt[7] - '0') * 4;
  mins += (dt[8] - '0') * 2;
  mins += (dt[9] - '0');

  hours = (dt[13] - '0') * 20;
  hours += (dt[14] - '0') * 10;
  hours += (dt[16] - '0') * 8;
  hours += (dt[17] - '0') * 4;
  hours += (dt[18] - '0') * 2;
  hours += (dt[19] - '0');

  wyear = (dt[46] - '0') * 80;
  wyear += (dt[47] - '0') * 40;
  wyear += (dt[48] - '0') * 20;
  wyear += (dt[49] - '0') * 10;
  wyear += (dt[51] - '0') * 8;
  wyear += (dt[52] - '0') * 4;
  wyear += (dt[53] - '0') * 2;
  wyear += (dt[54] - '0');

// Some sanity checks. Noise can change
// the value of a bit - usually from 1 to zero
// but might change 0 to 1.

#ifdef TEST_WWVB_DECODER
  if(wyear < 16 || wyear > 99) {
    Serial.printf("* ERROR year <2016 or >2099 (%d) - '%s'\n",wyear,dt);
    return;
  }
#else
  if(wyear < 18 || wyear > 99) {
    Serial.printf("* ERROR year <2018 or >2099 (%d) - '%s'\n",wyear,dt);
    return;
  }
#endif

// Really ought to check for leap year here
  if(days > 366) {
    Serial.printf("* ERROR days > 366 (%d) - '%s'\n",days,dt);
    return;
  }

  if(mins > 59) {
    Serial.printf("* ERROR mins > 59 (%d) - '%s'\n",mins,dt);
    return;
  }

  if(hours > 23) {
    Serial.printf("* ERROR hours > 23 (%d) - '%s'\n",hours,dt);
    return;
  }

  get_month_day(days,wyear,&t_month,&t_day);
  P_leap_second = 0;

  // FYI: Leap YEAR indicator is dt[56] == '1'
  //      Leap SECOND is dt[57]
  //      DST bits are dt[58] and dt[59]

  // If leap SECOND bit is set
  // This indicates that a leap second will
  // occur at the end of THIS month which can
  // be June or December.
  // i.e. this indicator will be on all month,
  // not just on the last day.
  // Therefore we must test for 23:59 in
  // the last day of the month
  if(dt[57] == '1') {
    // If this is 23:59
    if((hours == 23) && (mins == 59)) {
      if( ((t_month == 6) && (t_day == 30)) ||
          ((t_month == 12) && (t_day == 31))) {
        // This is the last minute of the last day of Jun or Dec
        // signal the pulse reading code to ignore the next pulse
        // which will be the 'P' from the leap second if it decodes
        // properly. If it isn't decoding properly it won't matter.
        P_leap_second = 1;
      }
    }
  }

  sprintf(s_tmp,"%2d",wyear);

  if(strlen(s_tmp) != 2) {
    sprintf(s_tmp,"> 20%2d/%02d/%02d %02d:%02d (days=%3d)\n",wyear,t_month,t_day,
            hours,mins,days);
    s_tmp[0] = '*';
    Serial.print(s_tmp);
    // Also print the decoded data.
    Serial.print("* ");
    Serial.println(dt);
  } else {
    sprintf(s_tmp,"> 20%2d/%02d/%02d %02d:%02d (days=%3d)\n",wyear,t_month,t_day,
            hours,mins,days);
    Serial.print(s_tmp);
  }
}

#else

// in samples.cpp. Need to clear it here when a linefeed is printed
extern int c_count;
void decode_wwvb(char *dt)
{
  int days,mins,hours,wyear,i;
  int t_month = 0,t_day = 0;
  char s_tmp[10];

// Need to do a wee bit of validation before accepting
// that the string is OK to decode.

// Ignore "PP" - it is usually caused by two bursts
// of noise close together and occurs rather frequently
// when the signal has faded
  if(strcmp(dt,"PP") == 0)return;

  if(strlen(dt) != 60) {
    if(debug_flag) {
      Serial.print("* ERROR length\n");
      c_count = 0;
    }
    return;
  }
// The last char should be a P
  if(dt[59] != 'P') {
    if(debug_flag) {
      Serial.printf("* ERROR 60th char is not P '%s'\n",dt);
      c_count = 0;
    }
    return;
  }

// There should be a P at positions 8, 18, 28, 38, 48, 58
  for(i = 8; i < 60; i += 10) {
    if(dt[i] != 'P') {
      if(debug_flag) {
        Serial.print("* ERROR missing P\n");
        c_count = 0;
      }
      return;
    }
  }

// There should NOT be a P anywhere else
// and the remaining chars must be 0 or 1

  for(i = 0; i < 59; i++) {
    // Skip the Position markers - they've been checked
    if((i % 10) == 8)continue;
    if(dt[i] == 'P') {
      if(debug_flag) {
        Serial.print("* ERROR P out of place\n");
        c_count = 0;
      }
      return;
    }
    if((dt[i] != '0') && (dt[i] != '1')) {
      if(debug_flag) {
        Serial.print("* ERROR not 0 or 1\n");
        c_count = 0;
      }
      return;
    }
  }

// OK. There's a good chance that this is a valid string.
  days = (dt[21] - '0') * 200;
  days += (dt[22] - '0') * 100;
  days += (dt[24] - '0') * 80;
  days += (dt[25] - '0') * 40;
  days += (dt[26] - '0') * 20;
  days += (dt[27] - '0') * 10;
  days += (dt[29] - '0') * 8;
  days += (dt[30] - '0') * 4;
  days += (dt[31] - '0') * 2;
  days += (dt[32] - '0');

  mins = (dt[0] - '0') * 40;
  mins += (dt[1] - '0') * 20;
  mins += (dt[2] - '0') * 10;
  mins += (dt[4] - '0') * 8;
  mins += (dt[5] - '0') * 4;
  mins += (dt[6] - '0') * 2;
  mins += (dt[7] - '0');

  hours = (dt[11] - '0') * 20;
  hours += (dt[12] - '0') * 10;
  hours += (dt[14] - '0') * 8;
  hours += (dt[15] - '0') * 4;
  hours += (dt[16] - '0') * 2;
  hours += (dt[17] - '0');

  wyear = (dt[44] - '0') * 80;
  wyear += (dt[45] - '0') * 40;
  wyear += (dt[46] - '0') * 20;
  wyear += (dt[47] - '0') * 10;
  wyear += (dt[49] - '0') * 8;
  wyear += (dt[50] - '0') * 4;
  wyear += (dt[51] - '0') * 2;
  wyear += (dt[52] - '0');

  get_month_day(days,wyear,&t_month,&t_day);

#ifdef NOTDEF
// Used in the Arduino NANO version
  // If leap second bit is set
  if(dt[56] == '1') {
    // If this is 23:59
    if((hours == 23) && (mins == 59)) {
      if( ((t_month == 6) && (t_day == 30)) ||
          ((t_month == 12) && (t_day == 31))) {
        // This is the last minute of the last day of Jun or Dec
        // signal the pulse reading code to ignore the next pulse
        // which will be the 'P' from the leap second if it decodes
        // properly. If it isn't decoding properly it won't matter.
        P_sync_flag = 1;
      }
    }
  }
#endif
  if(wyear < 16 || wyear > 99) {
    if(debug_flag)Serial.printf("* ERROR year <2018 or >2099\n");
    return;
  }
  // Check that the year is two digits
  sprintf(s_tmp,"%d",wyear);
  if(strlen(s_tmp) != 2) {
    Serial.printf("> 20%2d/%02d/%02d %02d:%02d (days=%3d)\n",wyear,t_month,t_day,
                  hours,mins,days);
    // Also print the decoded data.
    Serial.printf("* %s\n",dt);
  } else {
    Serial.printf("> 20%2d/%02d/%02d %02d:%02d (days=%3d)\n",wyear,t_month,t_day,
                  hours,mins,days);
  }
  c_count = 0;
}

#endif


#ifdef TEST_WWVB_DECODER

const char *t_vector[] = {
  "10101000P001000011P001100110P011000000P000000001P011001000PP",
  "10101001P001000011P001100110P011000000P000000001P011001000PP",
  "00000000P000000000P000000000P000100000P000000001P011100000PP",
  "00000001P000000000P000000000P000100000P000000001P011100000PP",
  "00000010P000000000P000000000P000100000P000000001P011100000PP"
};
/*
  Output from above 5 vectors should be:
> 2016/12/31 23:58 (days=366)
> 2016/12/31 23:59 (days=366)
> 2017/01/01 00:00 (days=  1)
> 2017/01/01 00:01 (days=  1)
> 2017/01/01 00:02 (days=  1)
*/

// This has a leap second - actual data recorded at end of 2016
const char *wwvb_leap_second = "0PP10101000P001000011P001100110P011000010P010000001P011001100PP10101001P001000011P001100110P011000010P010000001P011001100PPP00000000P000000000P000000000P000100101P011000001P011100000PP00000001P000000000P000000000P000100101P011000001P011100000PP00000010P000000000P000000000P000100101P011000001P011100000PP0";

// WWVB - duration of reduced carrier power
// 0.8s - Position marker
// 0.5s - 1
// 0.2s - 0

void decode(unsigned long t)
{
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
  // Have we just decoded the last data bit?
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
}

void send_wwvb_pulse(const char *tv)
{
  while(*tv) {
    switch(*tv) {
    case 'P':
      decode(800);
      break;

    case '1':
      decode(500);
      break;

    case '0':
      decode(200);
      break;
    }
    tv++;
  }
}

/*
This was using an earlier version of the decoder
*/
void test_wwvb_decoder(void)
{
  // Simple test of the current decoder
  for(uint32_t i = 0; i < sizeof(t_vector)/sizeof(const char *); i++) {
    Serial.printf("%s",t_vector[i]);
    decode_wwvb((char *)t_vector[i]);
  }
/*
// The current decoder can't be called with one long
// string.
  Serial.println("\nLeap Second Test\n");
// Test the leap second
  send_wwvb_pulse(wwvb_leap_second);
*/
}

/*
  Sample output from testing the NEW decoder:
10101000P001000011P001100110P011000000P000000001P011001000PP> 2016/12/31 23:58 (days=366)
10101001P001000011P001100110P011000000P000000001P011001000PP> 2016/12/31 23:59 (days=366)
00000000P000000000P000000000P000100000P000000001P011100000PP> 2017/01/01 00:00 (days=  1)
00000001P000000000P000000000P000100000P000000001P011100000PP> 2017/01/01 00:01 (days=  1)
00000010P000000000P000000000P000100000P000000001P011100000PP> 2017/01/01 00:02 (days=  1)
DONE


  Sample output from testing the OLD decoder:
Got PP
> decode 'PP10101000P001000011P001100110P011000000P000000001P011001000'
> 2016/12/31 23:58 (days=366)
Got PP
> decode 'PP10101001P001000011P001100110P011000000P000000001P011001000'
> 2016/12/31 23:59 (days=366)
Got PP
> decode 'PP00000000P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:00 (days=  1)
Got PP
> decode 'PP00000001P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:01 (days=  1)
Got PP
> decode 'PP00000010P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:02 (days=  1)

Leap Second Test

Got PP
> decode 'PP10101000P001000011P001100110P011000010P010000001P011001100'
> 2016/12/31 23:58 (days=366)
Got PP
> decode 'PP10101001P001000011P001100110P011000010P010000001P011001100'
> 2016/12/31 23:59 (days=366)
P_leap_second detected - skip this extra 'P'
Got PP
> decode 'PP00000000P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:00 (days=  1)
Got PP
> decode 'PP00000001P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:01 (days=  1)
Got PP
> decode 'PP00000010P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:02 (days=  1)
Got PP
DONE

*/
#endif

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


  if(wyear < 18 || wyear > 99) {
    Serial.printf("* ERROR year <2016 or >2099 (%d) - '%s'\n",wyear,dt);
    return;
  }


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
    Serial.print("* ERROR 1st not P\n");
    c_count = 0;
  }
  return;
}

// There should be a P at positions 8, 18, 28, 38, 48, 58
for(i = 8; i < 60;i += 10) {
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

for(i = 0; i < 59;i++) {
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
  if(wyear < 18 || wyear > 99) {
    if(debug_flag)printf("* ERROR year <2018 or >2099\n");
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

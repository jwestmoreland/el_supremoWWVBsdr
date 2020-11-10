#ifndef WWVB_H
#define WWVB_H

// Specify the required tone that the decoded carrier will
// have. Pick a comfortable frequency. For me it is 775Hz.
// Instead of specifying freq_real such that it gives a
// "sidetone" (as in the original DCF77 code), specify the
// sidetone itself and the declarations will sort out the
// offset required to get that tone. See freq_real below.
// Specified in Hz and should be an audible frequency
// within a range of about 500Hz to 1000Hz.
#define SIDETONE_FREQUENCY 775

// variation allowed in a pulse width before it is deemed to
// be invalid (in milliseconds)
#define PULSE_DT 100

char xlate(int plen);
void decode_wwvb(char *dt);

extern char time_char[];
extern int p_idx;
extern char P_leap_second;

#endif

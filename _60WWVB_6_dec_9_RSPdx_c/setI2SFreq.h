#ifndef SETI2SFREQ_H
#define SETI2SFREQ_H


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

void set_sample_rate(int sr);
/// int setI2SFreq(int freq);
void setI2SFreq(int freq);

// Set by set_sample_rate
extern unsigned int sample_rate_real;
#endif

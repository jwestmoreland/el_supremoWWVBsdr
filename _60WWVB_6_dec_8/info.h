/*
// This will show a diagram in the audio design tool.
// GUItool: begin automatically generated code
AudioInputI2S            i2s_in;         //xy=259.8833312988281,433
AudioFilterBiquad        biquad1;        //xy=443.8833312988281,426
AudioSynthWaveformSine   sine1;          //xy=463.8833312988281,523
AudioEffectMultiply      mult1;          //xy=685.8833312988281,514
AudioFilterBiquad        biquad3;        //xy=850.8833312988281,307
AudioSynthWaveformSine   sine600;        //xy=886.8833312988281,568
AudioFilterBiquad        biquad2;        //xy=887.8833312988281,514
AudioFilterBiquad        decimate_x6;    //xy=1027.88330078125,307
AudioMixer4              mixer_output;   //xy=1084.8833312988281,549
AudioFilterBiquad        biquad4;        //xy=1192.88330078125,307
AudioOutputI2S           i2s_out;        //xy=1257.8833312988281,551
AudioFilterBiquad        decimate_x4;    //xy=1352.88330078125,307
AudioRecordQueue         queue1;         //xy=1515.88330078125,307
AudioConnection          patchCord1(i2s_in, 0, biquad1, 0);
AudioConnection          patchCord2(biquad1, 0, mult1, 0);
AudioConnection          patchCord3(sine1, 0, mult1, 1);
AudioConnection          patchCord4(mult1, biquad2);
AudioConnection          patchCord5(mult1, biquad3);
AudioConnection          patchCord6(biquad3, decimate_x6);
AudioConnection          patchCord7(sine600, 0, mixer_output, 1);
AudioConnection          patchCord8(biquad2, 0, mixer_output, 0);
AudioConnection          patchCord9(decimate_x6, biquad4);
AudioConnection          patchCord10(mixer_output, 0, i2s_out, 0);
AudioConnection          patchCord11(mixer_output, 0, i2s_out, 1);
AudioConnection          patchCord12(biquad4, decimate_x4);
AudioConnection          patchCord13(decimate_x4, queue1);
AudioControlSGTL5000     sgtl5000_1;     //xy=282.8833312988281,304
// GUItool: end automatically generated code






First good decodes:
_60WWVB  v0.6_dec_1
Carrier = 60000.00, Q = 70.00, sidetone = 775, mag_limit = 100000
gain: 60, freq: 59225, sample_rate: 192kHz, FFT bin = 320
0010100PP* ERROR length
P00001P0010P001P1001P11P00010011P1P000100001P100000P111PP* ERROR length
010000P10P000000010P001000011P000100101P000100001P100000011PP* ERROR length
0100001P000001P010P001P100011P000100101P000100001P100000011PP* ERROR length
01000010P000000010P001000011P000100101P000100001P10000011PP* ERROR length
01000011P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:23 (days=231)
01000100P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:24 (days=231)
01000101P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:25 (days=231)
01000110P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:26 (days=231)
01000111P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:27 (days=231)
01001000P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:28 (days=231)
01001001P000000010P001000011P000100101P000100001P100000011PP> 2018/08/19 02:29 (days=231)
0110000




>>> Example of output when TEST_WWVB_DECODER is defined in wwvb.h

PPGot PP
10101000P001000011P001100110P011000000P000000001P011001000> decode 'PP10101000P001000011P001100110P011000000P000000001P011001000'
> 2016/12/31 23:58 (days=366)
PPGot PP
10101001P001000011P001100110P011000000P000000001P011001000> decode 'PP10101001P001000011P001100110P011000000P000000001P011001000'
> 2016/12/31 23:59 (days=366)
PPGot PP
00000000P000000000P000000000P000100000P000000001P011100000> decode 'PP00000000P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:00 (days=  1)
PPGot PP
00000001P000000000P000000000P000100000P000000001P011100000> decode 'PP00000001P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:01 (days=  1)
PPGot PP
00000010P000000000P000000000P000100000P000000001P011100000> decode 'PP00000010P000000000P000000000P000100000P000000001P011100000'
> 2017/01/01 00:02 (days=  1)

Leap Second Test

0PPGot PP
10101000P001000011P001100110P011000010P010000001P011001100> decode 'PP10101000P001000011P001100110P011000010P010000001P011001100'
> 2016/12/31 23:58 (days=366)
PPGot PP
10101001P001000011P001100110P011000010P010000001P011001100> decode 'PP10101001P001000011P001100110P011000010P010000001P011001100'
> 2016/12/31 23:59 (days=366)
PP_leap_second detected - skip this extra 'P'
PPGot PP
00000000P000000000P000000000P000100101P011000001P011100000> decode 'PP00000000P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:00 (days=  1)
PPGot PP
00000001P000000000P000000000P000100101P011000001P011100000> decode 'PP00000001P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:01 (days=  1)
PPGot PP
00000010P000000000P000000000P000100101P011000001P011100000> decode 'PP00000010P000000000P000000000P000100101P011000001P011100000'
> 2017/01/01 00:02 (days=  1)
PPGot PP
0DONE



A leap year indicator is transmitted at second 55. If it is set to 1, the current year is a
leap year. The bit is set to 1 during each leap year after January 1 but before February 29.
It is set back to 0 on January 1 of the year following the leap year.
A leap second indicator is transmitted at second 56. If this bit is high, it indicates that
a leap second will be added to UTC at the end of the current month. The bit is set to 
1 near the start of the month in which a leap second is added. It is set to 0 immediately
after the leap second insertion

Daylight saving time (DST) and standard time (ST) information is transmitted at seconds
57 and 58. When ST is in effect, bits 57 and 58 are set to 0. When DST is in effect, bits
57 and 58 are set to 1. On the day of a change from ST to DST bit 57 changes from 0 to
1 at 0000 UTC, and bit 58 changes from 0 to 1 exactly 24 hours later. On the day of a
change from DST back to ST bit 57 changes from 1 to 0 at 0000 UTC, and bit 58 changes
from 1 to 0 exactly 24 hours later.
*/

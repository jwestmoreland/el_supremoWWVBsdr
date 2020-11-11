This repository is based on this thread and post:
https://forum.pjrc.com/threads/40102-Time-signal-LF-Receiver-Teensy-DCF77-with-minimal-hardware-effort?p=258978&viewfull=1#post258978

More details will be added as time permits.
Note - this is to be considered in debug/test and no performance claims are made.

Here's a comment from the original code:

191101
- Using the T3.6 which has audio board and the Hauptwerk/MIDI
  stuff on it.

+=+=+=

Added check of mic control register:

Note (Audio Lib):
In control_sgtl5000.h:

unsigned int read(unsigned int reg); // removed from protected:

In control_sgtl5000.cpp:

return write(CHIP_MIC_CTRL, 0x0000 | preamp_gain)                          // attempt to disable mic bias block --- aj6bc/jcw

Example:

In the sketch:

// check value of mic control register:

mic_ctl_regval = sgtl5000_1.read(CHIP_MIC_CTRL);

Serial.printf("\r\nMic Control Register (ADDR 0x002A) = 0x%x \r\n", mic_ctl_regval);

And in the serial terminal window:

Mic Control Register (ADDR 0x002A) = 0x3  <- example output w/mic bias block disabled (register bits 9:8)
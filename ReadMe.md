This repository is based on this thread and post:
https://forum.pjrc.com/threads/40102-Time-signal-LF-Receiver-Teensy-DCF77-with-minimal-hardware-effort?p=258978&viewfull=1#post258978

More details will be added as time permits.
Note - this is to be considered in debug/test and no performance claims are made.

Here's a comment from the original code:

191101
- Using the T3.6 which has audio board and the Hauptwerk/MIDI
  stuff on it.

I'm going to 'stub' that with #define 'wrappers' for now - once I've looked
up the pin differences for what's being used with the two processors - I'll 
try to re-enable that - or just uncomment 
// #define USING_SIDETONE

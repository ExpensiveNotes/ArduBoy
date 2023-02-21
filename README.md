# ArduBoy
ArduBoy Programs
This program plays 2 independent 16 step sequences via the beep class and outputs a sync pulse for syncing to Volca Synths, Pocket Operators etc. This requires modification of the Arduboy or in my case the clone I used.

This is a clone of a clone of a Gameboy which I have modified to have stereo audio out and a sync pulse so it can sync Pocket Operators, Volcas, etc...
The Arduboy system is awesome and makes it relatively easy to program the Clone (DuinoBoy) to play music. Unfortunately the Clone only has one pin connected the buzzer so I added two line outs.

Details:
Audio:
I am using the BeepPin1 and BeepPin2 class to make sounds (square Waves) which go separately to the left and right channels of the output. Unfortunately, the clone has no pads to make hacking it easy, I had to solder one wire directly to the chip, which was a bit nerve-racking for me and my dodgy soldering skills. The pins for the speaker are PC6 and PC7 on the ATMega32U4.
Sync:
I send a 15ms pulse at the start of each note to pin PD2 (digital pin 0) which is connected to the sync out socket. (Tip not ring to match the Pocket Operators spec)
Inuse on Youtube:
https://youtu.be/GA5VriTmi_w

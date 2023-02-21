/*
  2 Voice ChipTuneSequencer for Arduboy (DuinoBoy clone too)
  By John Melki-Wegner aka "Expensive Notes"

  Assumes: pin D0 (PD2 on ATMEGA32U4) is connected to sync out socket. And, pins PC6 and PC7 are speaker outs are also connected to a stereo out socket

  Buttons
  A = Choose Mode
  B = Interface Swap between voices

  D Buttons:
  Mode          Left    Up    Down    Right

  Cursor Mode
  0             Left    Up    Down    Right

  Set Value
  1   Note     -12   +1    -1      +12
    Length     -10   +1    -1      +10

  Set BPM     x1/2    +1    -1      x2
  2

  Length Octave
  Voice dependent
  3           Oc-12 Seq+1 Seq-1   Oct+12

  Control
  4          OnOff1  Rand1 Rand2   OnOff2

*/

#include <Arduboy2.h>
Arduboy2 arduboy;

//================================================================================= Variables

BeepPin1 beep; // Create a class instance for speaker pin 1 - PC6 pin on ATMEGA32U4
BeepPin2 beep2; // Create a class instance for speaker pin 2 - PC7 pin on ATMEGA32U4

int StepIndex = 0;    //Voice 1
int StepIndex2 = 0;   //Voice 2

const float noteFreqs[128] = {8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 652709.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85};
byte notes[16] = {38, 0, 41, 0, 40, 0, 38, 0, 40, 43, 45, 48, 45, 45, 43, 43}; //MIDI Notes for Voice 1
byte noteLength[16] = {2, 2, 5, 0, 2, 3, 5, 2, 5, 4, 3, 10, 20, 5, 20, 5};     //Note length for Voice 1
byte notes2[16] = {38, 0, 41, 0, 40, 0, 38, 0, 38, 0, 41, 0, 43, 41, 40, 40};  //MIDI Notes for Voice 2
byte note2Length[16] = {2, 2, 5, 0, 2, 3, 5, 2, 5, 4, 3, 10, 5, 5, 20, 5};     //Note length for Voice 2
byte sequenceLength = 16;
byte sequenceLength2 = 16;
bool stopSequence = false;      //Sequence 1
bool stopSequence2 = false;
long t1 = 0, t2 = 0;            //times for note playing
int selectX = 0, selectY = 0;   //Cursor position for selecting notes
byte modeIndex = 0;             //0 == move cursor, 1 == change value, 2 == BPM, 3 == Octave and sequence Length, 4 == On/Off Random On/Off
bool showVoice1 = true;         //Is voice1 or 2 active for display or editing
int waitTime = 198;             //Time between sequence steps
int BPM;
int octave = 0;                 //Jump up or down an octave for voice 1
int octave2 = 0;
bool randomize = false;         //should the sequence randomly mutate?
bool randomize2 = false;

//pin for sync out pulse
#define syncOut1 0              //PD2 pin on ATMEGA32U4
long syncStart = 0;             //When last pulse fired

//==================================================================================== Setup and Loop

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(25);
  beep.begin();
  beep2.begin();
  BPM = 30000 / waitTime;
  pinMode(syncOut1, OUTPUT);
  randomSeed(analogRead(0));    //Generate Random Seed for Random numbers from floating voltage of A0 pin
}

void loop() {
  checkEndOfPulse();            //Check sync pulse freely without arduboy framework
  if (!arduboy.nextFrame()) {
    return;
  }

  beep.timer();                     // handle tone duration 40ms per frame
  beep2.timer();                    // handle tone duration 40ms per frame
  arduboy.pollButtons();            //Check for button actions
  arduboy.clear();
  buttonCheck();                    //Process Buttons
  if (!stopSequence || !stopSequence2) PlayNote();    //Play notes if running
  randomNotes();
  showScreen();
}

//====================================================================================== Music

void PlayNote() {
  //Check time for note
  t2 = millis();
  if (t2 - t1 < waitTime) return;
  //Sync Pulse on
  startPulse();

  //Play notes if legal MIDI values
  t1 = t2;
  if (!stopSequence && notes[StepIndex] + octave > 0 && notes[StepIndex] + octave < 128) beep.tone(beep.freq(noteFreqs[notes[StepIndex] + octave]), noteLength[StepIndex]);
  if (!stopSequence2 && notes2[StepIndex2] + octave2 > 0 && notes2[StepIndex2] + octave2 < 128) beep2.tone(beep2.freq(noteFreqs[notes2[StepIndex2] + octave2]), note2Length[StepIndex2]);

  //Next step
  StepIndex++;
  if (StepIndex > sequenceLength - 1) StepIndex = 0;
  StepIndex2++;
  if (StepIndex2 > sequenceLength2 - 1) StepIndex2 = 0;
}

void randomNotes() {
  //voice 1
  if (randomize) {
    if (random(5) == 0) {
      int randStep = random(16);
      notes[randStep] = notes[randStep] + random(5) - 2;
      if (notes[randStep] > 128) notes[randStep] = 0; //because byte values subtract to numbers > 200 ish.
    }
  }
  if (randomize2) {
    if (random(5) == 0) {
      int randStep = random(16);
      notes2[randStep] = notes2[randStep] + random(5) -  2 ;
      if (notes2[randStep] > 128) notes2[randStep] = 0;
    }
  }
}

//===================================================================================== Screen

void showScreen() {
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  for (int i = 0; i < 16; i++) {
    if (i < 8) arduboy.setCursor(2 + i * 16, 2);
    else arduboy.setCursor(2 + (i - 8) * 16, 26);
    if (showVoice1) arduboy.print(notes[i] + octave);
    else arduboy.print(notes2[i] + octave2);
    if (i < 8) arduboy.setCursor(2 + i * 16, 14);
    else arduboy.setCursor(2 + (i - 8) * 16, 38);
    if (showVoice1) arduboy.print(noteLength[i]);
    else arduboy.print(note2Length[i]);
  }
  if (StepIndex < 8) arduboy.drawLine(16 * StepIndex + 2, 11, 16 * StepIndex + 12 , 11);
  else arduboy.drawLine(16 * (StepIndex - 8) + 2, 35, 16 * (StepIndex - 8) + 12 , 35);
  arduboy.drawRect(16 * selectX, 12 * selectY, 16, 12);
  arduboy.drawRect(0, 48, WIDTH, 15);
  arduboy.setCursor(16, 52);
  if (showVoice1) {
    arduboy.println(sequenceLength);
  } else {
    arduboy.println(sequenceLength2);
  }
  arduboy.setCursor(31, 52);
  arduboy.println(BPM);
  showMode();
  if (!showVoice1) {
    arduboy.setTextColor(BLACK);
    arduboy.setTextBackground(WHITE);
  }
  arduboy.setCursor(2, 52);
  arduboy.println(StepIndex);
  arduboy.display();
}

//============================================================================ Modes

//What mode are the D buttons in
void showMode() {
  arduboy.setCursor(55, 52);
  switch (modeIndex) {
    case 0:
      arduboy.print("Cursor Mode");
      break;
    case 1:
      arduboy.print("Set Value");
      break;
    case 2:
      arduboy.print("Set BPM");
      break;
    case 3:
      arduboy.print("Oct. Length");
      break;
    case 4:
      arduboy.print("On/Off Rand");
      break;
    default:
      // statements
      break;
  }
}

//============================================================================ Buttons

void buttonCheck() {

  //Mode Change for D buttons
  if (arduboy.justPressed(A_BUTTON)) {
    modeIndex = modeIndex + 1;
    if (modeIndex > 4) modeIndex = 0;
  }


  //Change View to the other voice
  if (arduboy.justPressed(B_BUTTON)) {
    showVoice1 = !showVoice1;
  }

  //move cursor if mode 0
  if (modeIndex == 0) {
    if (arduboy.justPressed(LEFT_BUTTON)) {
      selectX--;
      if (selectX < 0) selectX = 7;
    }
    if (arduboy.justPressed(RIGHT_BUTTON)) {
      selectX++;
      if (selectX > 7) selectX = 0;
    }
    if (arduboy.justPressed(UP_BUTTON)) {
      selectY--;
      if (selectY < 0) selectY = 3;
    }
    if (arduboy.justPressed(DOWN_BUTTON)) {
      selectY++;
      if (selectY > 3) selectY = 0;
    }
  }

  //Other modes depend on current Voice
  if (showVoice1) {
    //Voice 1
    //alter note pitches
    if (modeIndex == 1) {
      //Top Row
      if (selectY == 0) {
        if (arduboy.justPressed(LEFT_BUTTON)) notes[selectX] = notes[selectX] - 12;
        if (arduboy.justPressed(RIGHT_BUTTON)) notes[selectX] = notes[selectX] + 12;
        if (arduboy.justPressed(UP_BUTTON)) notes[selectX] = notes[selectX] + 1;
        if (arduboy.justPressed(DOWN_BUTTON)) notes[selectX] = notes[selectX] - 1;
      }
      //Second Row
      //alter note lengths
      if (selectY == 1) {
        if (arduboy.justPressed(LEFT_BUTTON)) noteLength[selectX] = noteLength[selectX] - 10;
        if (arduboy.justPressed(RIGHT_BUTTON)) noteLength[selectX] = noteLength[selectX] + 10;
        if (arduboy.justPressed(UP_BUTTON)) noteLength[selectX] = noteLength[selectX] + 1;
        if (arduboy.justPressed(DOWN_BUTTON)) noteLength[selectX] = noteLength[selectX] - 1;
      }
      //3rd Row
      //alter note pitches
      if (selectY == 2) {
        if (arduboy.justPressed(LEFT_BUTTON)) notes[selectX + 8] = notes[selectX + 8] - 12;
        if (arduboy.justPressed(RIGHT_BUTTON)) notes[selectX + 8] = notes[selectX + 8] + 12;
        if (arduboy.justPressed(UP_BUTTON)) notes[selectX + 8] = notes[selectX + 8] + 1;
        if (arduboy.justPressed(DOWN_BUTTON)) notes[selectX + 8] = notes[selectX + 8] - 1;
      }
      //4th Row
      //alter note lengths
      if (selectY == 3) {
        if (arduboy.justPressed(LEFT_BUTTON)) noteLength[selectX + 8] = noteLength[selectX + 8] - 10;
        if (arduboy.justPressed(RIGHT_BUTTON)) noteLength[selectX + 8] = noteLength[selectX + 8] + 10;
        if (arduboy.justPressed(UP_BUTTON)) noteLength[selectX + 8] = noteLength[selectX + 8] + 1;
        if (arduboy.justPressed(DOWN_BUTTON)) noteLength[selectX + 8] = noteLength[selectX + 8] - 1;
      }
    }
  } else {
    //Voice 2
    //alter values for voice 2
    if (modeIndex == 1) {
      //Top Row
      if (selectY == 0) {
        if (arduboy.justPressed(LEFT_BUTTON)) notes2[selectX] = notes2[selectX] - 12;
        if (arduboy.justPressed(RIGHT_BUTTON)) notes2[selectX] = notes2[selectX] + 12;
        if (arduboy.justPressed(UP_BUTTON)) notes2[selectX] = notes2[selectX] - 1;
        if (arduboy.justPressed(DOWN_BUTTON)) notes2[selectX] = notes2[selectX] + 1;
      }
      //Second Row
      if (selectY == 1) {
        if (arduboy.justPressed(LEFT_BUTTON)) note2Length[selectX] = note2Length[selectX] - 10;
        if (arduboy.justPressed(RIGHT_BUTTON)) note2Length[selectX] = note2Length[selectX] + 10;
        if (arduboy.justPressed(UP_BUTTON)) note2Length[selectX] = note2Length[selectX] - 1;
        if (arduboy.justPressed(DOWN_BUTTON)) note2Length[selectX] = note2Length[selectX] + 1;
      }
      //3rd Row
      if (selectY == 2) {
        if (arduboy.justPressed(LEFT_BUTTON)) notes2[selectX + 8] = notes2[selectX + 8] - 12;
        if (arduboy.justPressed(RIGHT_BUTTON)) notes2[selectX + 8] = notes2[selectX + 8] + 12;
        if (arduboy.justPressed(UP_BUTTON)) notes2[selectX + 8] = notes2[selectX + 8] - 1;
        if (arduboy.justPressed(DOWN_BUTTON)) notes2[selectX + 8] = notes2[selectX + 8] + 1;
      }
      //4th Row
      if (selectY == 3) {
        if (arduboy.justPressed(LEFT_BUTTON)) note2Length[selectX + 8] = note2Length[selectX + 8] - 10;
        if (arduboy.justPressed(RIGHT_BUTTON)) note2Length[selectX + 8] = note2Length[selectX + 8] + 10;
        if (arduboy.justPressed(UP_BUTTON)) note2Length[selectX + 8] = note2Length[selectX + 8] - 1;
        if (arduboy.justPressed(DOWN_BUTTON)) note2Length[selectX + 8] = note2Length[selectX + 8] + 1;
      }
    }
  }
  //Alter BPM
  if (modeIndex == 2) {
    if (arduboy.justPressed(LEFT_BUTTON)) BPM = BPM / 2;
    if (arduboy.justPressed(RIGHT_BUTTON)) BPM = BPM * 2;
    if (arduboy.justPressed(UP_BUTTON)) BPM++;
    if (arduboy.justPressed(DOWN_BUTTON)) BPM--;
    if (BPM < 30) BPM = 30;
    if (BPM > 300) BPM = 300;
    waitTime = 30000 / BPM;
  }
  //Alter Sequence Length or Octave for current voice
  if (modeIndex == 3) {
    if (showVoice1) {
      if (arduboy.justPressed(LEFT_BUTTON)) octave = octave - 12;
      if (arduboy.justPressed(RIGHT_BUTTON)) octave = octave + 12;
      if (arduboy.justPressed(UP_BUTTON)) sequenceLength++;
      if (arduboy.justPressed(DOWN_BUTTON)) sequenceLength--;
      if (sequenceLength < 1) sequenceLength = 1;
      if (sequenceLength > 16) sequenceLength = 16;
    } else {
      if (arduboy.justPressed(LEFT_BUTTON)) octave2 = octave2 - 12;
      if (arduboy.justPressed(RIGHT_BUTTON)) octave2 = octave2 + 12;
      if (arduboy.justPressed(UP_BUTTON)) sequenceLength2++;
      if (arduboy.justPressed(DOWN_BUTTON)) sequenceLength2--;
      if (sequenceLength2 < 1) sequenceLength2 = 1;
      if (sequenceLength2 > 16) sequenceLength2 = 16;
    }
  }
  //On/off for sequence and on/off for Randomizing
  if (modeIndex == 4) {
    if (arduboy.justPressed(LEFT_BUTTON)) onOff();
    if (arduboy.justPressed(RIGHT_BUTTON)) onOff2();
    if (arduboy.justPressed(UP_BUTTON)) randomize = !randomize;
    if (arduboy.justPressed(DOWN_BUTTON)) randomize2 = !randomize2;

  }
}

void onOff() {
  beep.noTone(); // Stop the tone if one is playing
  stopSequence = !stopSequence;
}
void onOff2() {
  beep2.noTone(); // Stop the tone if one is playing
  stopSequence2 = !stopSequence2;
}

//================================================================================================== Sync Out

void startPulse() {
  syncStart = millis();    //When did it start
  digitalWrite(syncOut1, HIGH);
}

void checkEndOfPulse() {
  if (millis() > syncStart + 15) digitalWrite(syncOut1, LOW); //sync pulse off after 15ms
}

/*
  Sync Master for Arduboy (DuinoBoy clone too)
  By John Melki-Wegner aka "Expensive Notes"

  Assumes: pin D0 (PD2 on ATMEGA32U4) is connected to sync out socket. And, pins PC6 and PC7 are speaker outs are also connected to a stereo out socket

*/

#include <Arduboy2.h>
Arduboy2 arduboy;

//================================================================================= Variables

BeepPin1 beep; // Create a class instance for speaker pin 1 - PC6 pin on ATMEGA32U4

int StepIndex = 0;    //Voice 1
bool syncEnabled = true;      //Sequence 1
byte sequenceLength = 16;
long t1 = 0, t2 = 0;                 //times for note playing
int stepX = 0; //Cursor position for selecting notes
byte modeIndex = 0;             //0 == move cursor, 1 == change value, 2 == BPM, 3 == Octave and sequence Length, 4 == On/Off Random On/Off
bool showVoice1 = true;         //Is voice1 or 2 active for display or editing
int waitTimes[16] = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200}; //Time between sequence steps
int waitTimesFlat[16] = {200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200}; //Time between sequence steps
int waitTimesSwing[16] = {100, 300, 100, 300, 100, 300, 100, 300, 100, 300, 100, 300, 100, 300, 100, 300}; //Time between sequence steps
int waitTimesSwing2[16] = {100, 100, 300, 300, 100, 100, 300, 300, 100, 100, 300, 300, 100, 100, 300, 300}; //Time between sequence steps
int waitTimesSine[16] = {200, 238, 271, 292, 300, 292, 271, 238, 200, 162, 129, 108, 100, 108, 129, 162}; //Time between sequence steps
int waitTimesRamp[16] = {100, 114, 128, 142, 156, 170, 184, 198, 212, 226, 240, 254, 268, 282, 296, 310}; //Time between sequence steps
int waitTimesMixedSwing[16] = {100, 300, 100, 300, 100, 300, 100, 300,200, 200, 200, 200, 200, 200, 200, 200 }; //Time between sequence steps
int globalOffset = 0;
//waitTimesFlat
int BPM;
int randomAmount = 0;         //How much should the sequence randomly mutate?

//pin for sync out pulse
#define syncOut1 0              //PD2 pin on ATMEGA32U4
long syncStart = 0;             //When last pulse fired

//==================================================================================== Setup and Loop

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(25);
  beep.begin();
  BPM = 30000 / waitTimes[StepIndex];
  pinMode(syncOut1, OUTPUT);
  randomSeed(analogRead(0));    //Generate Random Seed for Random numbers from floating voltage of A0 pin
}

void loop() {
  checkEndOfPulse();            //Check sync pulse freely without arduboy framework
  if (!arduboy.nextFrame()) {
    return;
  }

  beep.timer();                     // handle tone duration 40ms per frame
  //beep2.timer();                    // handle tone duration 40ms per frame
  arduboy.pollButtons();            //Check for button actions
  arduboy.clear();
  buttonCheck();                    //Process Buttons
  if (syncEnabled) playSync();    //Play notes if running
  showScreen();
}

//====================================================================================== Music

void playSync() {
  //Check time for sync Pulse
  t2 = millis();
  if (t2 - t1 < waitTimes[StepIndex] + globalOffset) return;

  //Sync Pulse on
  startPulse();

  t1 = t2;

  //Next step
  StepIndex++;
  if (StepIndex > sequenceLength - 1) StepIndex = 0;
  mutateRandomAmount();
}

void mutateRandomAmount() {
  if (randomAmount == 0) return;
  int mutationStrength = 7 - randomAmount;
  for (int i = 0; i < 16; i++) {
    if (random(mutationStrength) == 0) waitTimes[i] = waitTimes[i] + random(randomAmount) - randomAmount / 2;
  }
}

//===================================================================================== Screen

void showScreen() {
  arduboy.setTextColor(WHITE);
  arduboy.setTextBackground(BLACK);
  //randomAmount
  arduboy.setCursor(2, 52);
  arduboy.println(randomAmount);
  //globalOffset
  arduboy.setCursor(22, 52);
  arduboy.println(globalOffset);
  arduboy.fillCircle(stepX * 8 + 2, 3, 2, WHITE);  //Individual Changes
  arduboy.fillRect(StepIndex * 8 + 2, 6, 2, 2, WHITE); //Step
  for (int i = 0; i < sequenceLength; i++) {
    arduboy.fillRect(i * 8 + 2, 10, 2, waitTimes[i] / 10, WHITE);
  }
  showMode();
  arduboy.display();
}

//============================================================================ Modes

//What mode are the D buttons in
void showMode() {
  arduboy.setCursor(45, 52);
  switch (modeIndex) {
    case 0:
      arduboy.print("Step Change");
      break;
    case 1:
      arduboy.print("Global Change");
      break;
    case 2:
      arduboy.print("Swing Patterns");
      break;
    case 3:
      arduboy.print("Odd Patterns");
      break;
    case 4:
      arduboy.print("Mutations");
      break;
    case 5:
      arduboy.print("Seq. Length");
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
    if (modeIndex > 5) modeIndex = 0;
  }

  //Change View to the other voice
  if (arduboy.justPressed(B_BUTTON)) {
    beep.noTone();                      // Stop the tone if one is playing
    syncEnabled = !syncEnabled;
    if (!syncEnabled) StepIndex = 0;    // Back to the start
  }
  //globalOffset
  //Individual Changes
  if (modeIndex == 0) {
    if (arduboy.justPressed(LEFT_BUTTON)) stepX--;
    if (arduboy.justPressed(RIGHT_BUTTON))  stepX++;
    if (arduboy.justPressed(UP_BUTTON)) waitTimes[stepX] = waitTimes[stepX] + 10;
    if (arduboy.justPressed(DOWN_BUTTON)) waitTimes[stepX] = waitTimes[stepX] - 10;
    if (stepX > 15) stepX = 0;
    if (stepX < 0) stepX = 15;
  }
  //globalOffset
  if (modeIndex == 1) {
    if (arduboy.justPressed(LEFT_BUTTON)) globalOffset--;
    if (arduboy.justPressed(RIGHT_BUTTON)) globalOffset++;
    if (arduboy.justPressed(UP_BUTTON)) globalOffset = globalOffset + 10;
    if (arduboy.justPressed(DOWN_BUTTON)) globalOffset = globalOffset - 10;
  }

  //Mutation
  if (modeIndex == 4) { //Clockwise Increase
    if (arduboy.justPressed(UP_BUTTON)) randomAmount = 0;
    if (arduboy.justPressed(RIGHT_BUTTON))  randomAmount = 1;
    if (arduboy.justPressed(DOWN_BUTTON)) randomAmount = 2;
    if (arduboy.justPressed(LEFT_BUTTON)) randomAmount = 3;
  }
  //Sequence Length
  if (modeIndex == 5) {
    if (arduboy.justPressed(UP_BUTTON)) sequenceLength = 16;
    if (arduboy.justPressed(RIGHT_BUTTON))  sequenceLength = sequenceLength + 1;
    if (arduboy.justPressed(DOWN_BUTTON)) sequenceLength = 8;
    if (arduboy.justPressed(LEFT_BUTTON)) sequenceLength = sequenceLength - 1;
    if (sequenceLength > 16) sequenceLength = 16;
    if (sequenceLength < 1) sequenceLength = 1;
  }
  
  //Patterns
  if (modeIndex == 2) {
    if (arduboy.justPressed(LEFT_BUTTON)) choosePattern(0);     //swing
    if (arduboy.justPressed(RIGHT_BUTTON)) choosePattern(5);    //Mixed Swing
    if (arduboy.justPressed(UP_BUTTON)) choosePattern(3);      //flat
    if (arduboy.justPressed(DOWN_BUTTON)) choosePattern(1);
  }

  //Patterns 2
  if (modeIndex == 3) {
    if (arduboy.justPressed(LEFT_BUTTON)) choosePattern(4);
    if (arduboy.justPressed(RIGHT_BUTTON))  choosePattern(2);//sine
    if (arduboy.justPressed(UP_BUTTON)) choosePattern(3);     //flat
    if (arduboy.justPressed(DOWN_BUTTON)) choosePattern(6);
  }
}

void choosePattern(byte choice) {
  for (int i = 0; i < 16; i++) {
    switch (choice) {
      case 0:
        waitTimes[i] = waitTimesSwing[i];
        break;
      case 1:
        waitTimes[i] = waitTimesSwing2[i];//waitTimesSine
        break;
      case 2:
        waitTimes[i] = waitTimesSine[i];  //waitTimesSine
        break;
      case 3:
        waitTimes[i] = waitTimesFlat[i];  //Default
        break;
      case 4:
        waitTimes[i] = waitTimesRamp[i];
        break;
      case 5:
        waitTimes[i] = waitTimesMixedSwing[i];//waitTimesSine
        break;
      case 6:
        randomWaitTimes();
        break;
      default:
        // statements
        break;
    }
  }
}

void randomWaitTimes() {
  for (int i = 0; i < 16; i++) {
    waitTimes[i] = 100 + random(200);
  }
}


//================================================================================================== Sync Out

void startPulse() {
  syncStart = millis();    //When did it start
  digitalWrite(syncOut1, HIGH);
  if (syncEnabled) {
    if (StepIndex == 0) beep.tone(beep.freq(400));   //start metronome
    else beep.tone(beep.freq(100));
  }
}

void checkEndOfPulse() {
  if (millis() > syncStart + 15) {
    digitalWrite(syncOut1, LOW);                  //sync pulse off after 15ms
    beep.noTone();                                // Stop the metronome if one is playing
  }
}

/*
* BitLooper
* version 1.3 (March 27, 2019)
* 
* A handheld square wave synthesis sequencer built on the TWSU DIY Gamer Kit.
* 2-64 note loops with 40 note range (B2 through D6)
* Requires speaker connected to digital pin 13 and ground.
* 
* NOTE: Requires modified version of the Gamer library.
* 
* Programmed by Matthew Petry (fireTwoOneNine) <firetwoonenine@gmail.com>
* Licensed under the GNU GPLv3 (See LICENSE file)
* 
* Uses the TimerFreeTone library: 
* Copyright 2016 Tim Eckel <teckel@leethost.com>
* Licensed under GNU GPLv3 (See LICENSE file)
* 
* Uses the Flash library (available in Arduino Library Manager):
* Copyright Mikal Hart
* Maintained by Christopher Schirner and contributors
*/

#include <Gamer.h>
#include <TimerFreeTone.h>
#include <EEPROM.h>
#include <Flash.h>

Gamer gamer;

FLASH_ARRAY(int, noteFreq, 0,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175);
const String noteText[41] = {"No","B2","C3","CS3","D3","DS3","E3","F3","FS3","G3","GS3","A3","AS3","B3","C4","CS4","D4","DS4","E4","F4","FS4","G4","GS4","A4","AS4","B4","C5","CS5","D5","DS5","E5","F5","FS5","G5","GS5","A5","AS5","B5","C6","CS6","D6"};
int noteIndex = 0;
float freqDelay = 0;
int currStep = 1;
int STEPS = 8;
int sequence[32] = {0};

const int MAX_STEPS = 64;

// For SerialMusic playback
byte inNote1;
byte inNote2;
int freq;
String note;
bool serialMode;
float volume;
int timeout;

// === Images for screen ===

byte playButton[8] = {B00000000, //Play button/icon (right-facing triangle)
                      B01100000,
                      B01111000,
                      B01111110,
                      B01111110,
                      B01111000,
                      B01100000,
                      B00000000};

byte recButton[8] = {B00000000, //Record button/icon (circle)
                     B00011000,
                     B00111100,
                     B01111110,
                     B01111110,
                     B00111100,
                     B00011000,
                     B00000000};

byte musicNote[8] = {B00000000, //Music note
                     B00111100,
                     B00100010,
                     B00100010,
                     B01100010,
                     B01100110,
                     B00000110};

byte computer1[8] = {B00111100, //Computer, blinking cursor part 1
                     B01000010,
                     B01000010,
                     B01000010,
                     B00111100,
                     B00000000,
                     B01111010,
                     B01111010};

byte computer2[8] = {B00111100, //Computer, blinking cursor part 2
                     B01000010,
                     B01110010,
                     B01000010,
                     B00111100,
                     B00000000,
                     B01111010,
                     B01111010};

void playSynth(int freq, int sustain = 1) { // Square wave synthesizer with basic decay
  TimerFreeTone(13,freq,100*sustain);
  for (int i = 10; i > 0; i--) {
    TimerFreeTone(13,freq,10, i);
  }  
}

int MultiplicationCombine(unsigned int x_high, unsigned int x_low) // Combines a high byte and a low byte into a single 16 bit (2 byte) integer
{
  int combined;
  combined = x_high;
  combined = combined*256; 
  combined |= x_low;
  return combined;
}

void serialMusic() { // Modified version of SerialMusic (https://github.com/fire219/SerialMusic) for use within BitLooper -- uses TimerFreeTone instead of Volume3
  Serial.begin(38400);
//  TimerFreeTone(13,131,200);
  TimerFreeTone(13,175,300);
  while(true && !serialMode) {   //
    gamer.printImage(computer1); // We're waiting to be rebooted by the incoming serial connection.
    delay(500);                  // Show the blinking cursor computer animation until then.
    gamer.printImage(computer2); //
    delay(500);                  //
  }
  EEPROM.write(0,0); // We have now been rebooted, so let's get rid of that bit. Else, we would never be able to get back to normal!
  gamer.printImage(musicNote);
  while(true) {
  if (Serial.available() == 2) { // Wait for two bytes to be in the serial buffer
    inNote1 = Serial.read();
    inNote2 = Serial.read();
    freq = MultiplicationCombine(inNote1, inNote2); // Turn those two bytes into a single usable int
    volume = 10.0;
    timeout = 3000;
    TimerFreeTone(13, freq,10,volume); // Play the note!
  }
  else {
    if (freq != 0) {                            //
      TimerFreeTone(13, freq, 10, (int)volume); // SerialMusic needs a more responsive synth to keep on the pace it is getting fed bytes!
      volume = volume * 0.9;                    // So instead of using the normal playSynth() function used by the rest of BitLooper, this is here instead. 
      timeout--;                                // 
    }                                           // It only plays the note for 10 milliseconds at a time before playing again. This means that we can check to see if we have new bytes coming
    if (timeout <= 0) {                         // in from serial in between. If there is no new bytes, it will play that note again at a slightly reduced volume.
      TimerFreeTone(13,0,10);                   //
      timeout = 0;                              // This results in what sounds like a continuous decaying note to the human ear, albeit slightly dirty.
    }
  }
   delayMicroseconds(100);
  }  
}

void setup() { // We are starting up now! Yay!
  
  serialMode = EEPROM.read(0); // Check the EEPROM bit for if we should be going into SerialMusic mode or not
  gamer.begin();
  if(serialMode) { // Should we be going into SerialMusic?
    serialMusic(); // If so, jump to it!
  }
  if(!digitalRead(A4)) { // This reads the START button in a more immediate fashion than the Gamer library's isPressed() function. Is the START button pressed when we are starting up?
    gamer.allOn();               // Let's do a test mode! Light up all the pixels on the screen to make sure all of them are working.
    TimerFreeTone(13,262,1000);  // Beep the speaker to make sure it is working too!
    gamer.printString("v1.3");   // Print the current version of this sketch on the screen.
  }
  while(!gamer.isPressed(START)) { // Do all this while waiting for the user to press the START button.
    gamer.showScore(STEPS); // We are on the loop length select screen, so print the current length selected.
    if(gamer.isPressed(LEFT)) { //
      STEPS--;                  // If the left button is pressed, decrease the number of steps by one.
      if (STEPS < 2) {          // ... unless we're now under two loops, then let's cycle up to the maximum number of steps.
        STEPS = MAX_STEPS;      //
      } 
    }
    if(gamer.isPressed(RIGHT)) { //
      STEPS++;                   // If the right button is pressed, increase the number of steps by one.
      if (STEPS > MAX_STEPS) {   // .. unless we're now over the maximum number of loops, then let's cycle down to the lower number of steps (2).
        STEPS = 2;               //
      }
    }
    if(gamer.isPressed(DOWN)) {      // Pressing the down button on the loop select screen is how to get ready to switch BitLooper into SerialMusic mode.
      gamer.printImage(musicNote);   // Let's show a cute music note on screen to tell the user that we are ready to go into SerialMusic mode.
      while(!gamer.isPressed(UP)) {  // The user can cancel this by pressing up. Until then, let's wait for them.
        if(gamer.isPressed(START)) { // Once on this screen, the way to start SerialMusic mode is to press START. Has the user pressed start?
          EEPROM.write(0,1);         // We're going to have to reboot once we're in SerialMusic mode, so let's set a bit in permanent memory (EEPROM) to remind us where to go!
          serialMusic();             // Start SerialMusic mode!
        }                            //
        delay(100);                  //
      }                              //
      gamer.printImage(recButton);   // The user pressed up to cancel SerialMusic mode. Lets show the record icon to show that they are back in normal BitLooper mode.
      delay(1000);                   //
    }
    if(gamer.isPressed(UP)) {        //
      gamer.printImage(recButton);   // Show the user that they are in normal BitLooper mode.
      delay(1000);                   //
    }
    delay(100);
  }
}

void loop() { // We're now in the main BitLooper.... loop! :D

  if(gamer.isPressed(START)) {              // Pressing START in the main BitLooper mode is how to play our recorded loop.
    gamer.printImage(playButton);           // Show a cute play button for play mode!
    while(!gamer.isPressed(START)) {        // If START is pressed again, we go back to recording mode. Until that happens...
      for (int i = 0; i < STEPS; i++) {     // loop through each of our steps...
        playSynth(noteFreq[sequence[i]],2); // and send the note on that step to playSynth()!
      }
    }
    gamer.printImage(recButton);            // The user has pressed START again, so we're going back to recording mode. Show the record button.
    delay(300);
  }
  if(gamer.isPressed(LEFT)) {            // Pressing left in recording mode goes to the previous step in the loop.
    currStep--;                          // Decrease our step number by one.
    if (currStep < 1) {                  // ...unless that would put us at the zero step! That doesn't make sense for humans!
      currStep = STEPS;                  // If it does, let's cycle to the last step in the loop instead.
    }                                    //
    noteIndex = sequence[currStep-1];    // Save the current note into our sequence array so we can remember it later.
    gamer.showScore(currStep);           // Show that we are now on a new step.
    TimerFreeTone(13,131,200);           // Play a sound to give an auditory cue that we are on a new step.
    TimerFreeTone(13,175,300);           // Why not two sounds?
  }
  if(gamer.isPressed(RIGHT)) {           // Pressing right in recording mode goes to the next step in the loop.
    currStep++;                          // Increase our step number by one.
    if (currStep > STEPS) {              // ...unless that would put us at a step number outside the loop!
      currStep = 1;                      // If it does, lets cycle to the first step in the loop instead.
    }                                    //
    noteIndex = sequence[currStep-1];    // Save the current note into our sequence array so we can remember it later.
    gamer.showScore(currStep);           // Show that we are now on a new step.
    TimerFreeTone(13,175,200);           // Play a sound to give an auditory cue that we are on a new step.
    TimerFreeTone(13,131,300);           // Why not two sounds?
  }
  if(gamer.isPressed(UP)) {  // Pressing up in recording mode increases the note by one.
    noteIndex++;             // Increase the note by one.
    if (noteIndex > 40) {    // ..unless that puts us at a note that doesn't exist!
      noteIndex = 0;         // If it does, go to the lowest note we have (which is actually silent, but this code doesn't know that)
    }
  }
   if(gamer.isPressed(DOWN)) {              // Pressing down in recording mode decreases the note by one.
    noteIndex--;                            // Decrease the note by one.
    if (noteIndex > 40 or noteIndex < 0) {  // ...unless that puts us at a note outside our range! (may not actually be negative, because computers can be weird)
      noteIndex = 40;                       // If it does, go to our highest note instead.
    }
  }
  gamer.printString(noteText[noteIndex], 20); // Phew, we're done checking the buttons! So lets show the pretty name of our note to the user.
  gamer.showScore(currStep); // Show what step we are on.
  sequence[currStep-1] = noteIndex; // Save the note to the sequence so we can remember it later.
  playSynth(noteFreq[noteIndex]); // Play the note to the user so they can hear it. 
  
} // We reached the end of the program loop. But since it is a loop, we're going back to the beginning to start all over!

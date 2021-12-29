
#include "pitches.h"
// this files holds the tempo's and melodies of music to be played,
// with the Slot Car Race Controller


const int testMelodyNotes[] PROGMEM = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
const int testMelodyTempo[] PROGMEM = {
  4, 8, 8, 4, 4, 4, 4, 4
};
const int testMelodyLength = sizeof(testMelodyNotes)/sizeof(int); 



//Mario main theme melody
const int marioMainThemeNotes[] PROGMEM = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7,
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,

  
  NOTE_G6, 0, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,

  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,

  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0
};
//Mario main them tempo
const int marioMainThemeTempo[] PROGMEM = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
};
const int marioMainThemeLength = sizeof(marioMainThemeNotes)/sizeof(int); 


//Underworld melody
const int marioUnderworldNotes[] PROGMEM = {
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_C4, NOTE_C5, NOTE_A3, NOTE_A4,
  NOTE_AS3, NOTE_AS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0,
  NOTE_F3, NOTE_F4, NOTE_D3, NOTE_D4,
  NOTE_DS3, NOTE_DS4, 0,
  0, NOTE_DS4, NOTE_CS4, NOTE_D4,
  NOTE_CS4, NOTE_DS4,
  NOTE_DS4, NOTE_GS3,
  NOTE_G3, NOTE_CS4,
  NOTE_C4, NOTE_FS4, NOTE_F4, NOTE_E3, NOTE_AS4, NOTE_A4,
  NOTE_GS4, NOTE_DS4, NOTE_B3,
  NOTE_AS3, NOTE_A3, NOTE_GS3,
  0, 0, 0
};
//Underwolrd tempo
const int marioUnderworldTempo[] PROGMEM = {
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  3,
  12, 12, 12, 12,
  12, 12, 6,
  6, 18, 18, 18,
  6, 6,
  6, 6,
  6, 6,
  18, 18, 18, 18, 18, 18,
  10, 10, 10,
  10, 10, 10,
  3, 3, 3
};
const int marioUnderworldLength = sizeof(marioUnderworldNotes)/sizeof(int); 

// The melody array 
const int takeOnMeNotes[] PROGMEM = {
  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, NOTE_B4, NOTE_E5, 
  NOTE_E5, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, 
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, NOTE_D5, NOTE_FS5, 
  NOTE_FS5, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5
};
// The note duration, 8 = 8th note, 4 = quarter note, etc.
const int takeOnMeTempo[] PROGMEM = {
  8, 8, 8, 4, 4, 4, 
  4, 5, 8, 8, 8, 8, 
  8, 8, 8, 4, 4, 4, 
  4, 5, 8, 8, 8, 8
};
const int takeOnMeLength = sizeof(takeOnMeNotes)/sizeof(int);



const int knightRiderNotes[] PROGMEM = {
  // 1
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  // 2
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  // 3
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_A4,
  NOTE_AS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_GS4, NOTE_A4, NOTE_A4, NOTE_A4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4,
  NOTE_G4, NOTE_GS4, NOTE_G4, NOTE_G4,
  NOTE_GS4, NOTE_G4, NOTE_G4, NOTE_G4,
  NOTE_FS4, NOTE_G4, NOTE_G4, NOTE_G4,
  // solo
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_E5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_E5,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_E5, NOTE_A5, NOTE_G5,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_E5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_E5,
  NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_E5, NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_A5
};

const int knightRiderTempo[] PROGMEM = {
  // 1
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  // 2
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  // 3
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  250, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  125, 125, 125, 125,
  // solo
  250, 125, 125, 1500,
  250, 125, 125, 1500,
  250, 125, 125, 250, 250, 2000,
  250, 125, 125, 1500,
  250, 125, 125, 1500,
  250, 125, 125, 250, 250, 40, 250, 500
};

const int knightRiderLength = sizeof(knightRiderNotes)/sizeof(int);


// File of songs defined using pitch Notes[] and Lengths[] arrays.
// This is an alternative method to using Rttl style songs and players.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!
// The 'pitches.h' file is required to use this song data
// It should be included by the main sketch and is commented out here.
// #include "pitches.h"

// ****** MELODY STRUCTURE **************
// Each melody should be made up of 4 parts.
//    mySongNameNotes[] is an array of frequencies in Hz, using pitch aliases
//    mySongNameLengths[] is the corresponding note's length
//      1 = whole note
//      2 = half note
//      4 = quarter note
//      8 = eighth note
//      16 = sixteenth note, etc.
//    mySongNameTempo is a constant 0-255 of the beats per minute to play melody
//    mySongNameSize holds the number of notes in the melody for easy reference

// *** TEMPLATE for MELODY VARIABLES, COPY - PASTE - EDIT ***
// const int zzzNotes[] PROGMEM = {

// };
// const int zzzLengths[] PROGMEM = {
  
// };
// const int zzzSize = sizeof(zzzNotes)/sizeof(int);
// const int zzzTempo = 135;



const int testMelodyNotes[] PROGMEM = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
const int testMelodyLengths[] PROGMEM = {
  4, 8, 8, 4, 4, 4, 4, 4
};
const int testMelodySize = sizeof(testMelodyNotes)/sizeof(int);
const int testMelodyTempo = 85;



const int imperialMarchNotes[] PROGMEM = {
  NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, 0,
  NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, 0,
  NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_C5,

  NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4,//4
  NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_C5,
  NOTE_A4, NOTE_F4, NOTE_C5, NOTE_A4,
};
const int imperialMarchLengths[] PROGMEM = {
  -4, -4, 16, 16, 16, 16, 8, 8,
  -4, -4, 16, 16, 16, 16, 8, 8,
  4, 4, 4, -8, 16,

  4, -8, 16, 2,//4
  4, 4, 4, -8, 16,
  4, -8, 16, 2,
};
const int imperialMarchSize = sizeof(imperialMarchNotes)/sizeof(int);
const int imperialMarchTempo = 135;



const int cScaleNotes[] PROGMEM = {
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4,NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5
};
const int cScaleLengths[] PROGMEM = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};
const int cScaleSize = sizeof(cScaleNotes)/sizeof(int);
const int cScaleTempo = 85;



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
const int marioMainThemeLengths[] PROGMEM = {
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
const int marioMainThemeSize = sizeof(marioMainThemeNotes)/sizeof(int);
const int marioMainThemeTempo = 130;



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
const int marioUnderworldLengths[] PROGMEM = {
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
const int marioUnderworldSize = sizeof(marioUnderworldNotes)/sizeof(int);
const int marioUnderworldTempo = 130;



const int takeOnMeNotes[] PROGMEM = {
  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, 0, NOTE_B4, 0, NOTE_E5,
  0, NOTE_E5, 0, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, 
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, 0, NOTE_D5, 0, NOTE_FS5, 
  0, NOTE_FS5, 0, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5,

  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, 0, NOTE_B4, 0, NOTE_E5,
  0, NOTE_E5, 0, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, 
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, 0, NOTE_D5, 0, NOTE_FS5, 
  0, NOTE_FS5, 0, NOTE_FS5, NOTE_E5, NOTE_E5, NOTE_FS5, NOTE_E5,

  NOTE_FS5, NOTE_FS5, NOTE_D5, NOTE_B4, 0, NOTE_B4, 0, NOTE_E5,
  0, NOTE_E5, 0, NOTE_E5, NOTE_GS5, NOTE_GS5, NOTE_A5, NOTE_B5, 
  NOTE_A5, NOTE_A5, NOTE_A5, NOTE_E5, 0, NOTE_D5, 0, NOTE_FS5, 
  0, NOTE_FS5, 0, NOTE_FS5, 0
};
const int takeOnMeLengths[] PROGMEM = {
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,

  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,

  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 2
};
const int takeOnMeSize = sizeof(takeOnMeNotes)/sizeof(int);
const int takeOnMeTempo = 160;



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
const int knightRiderLengths[] PROGMEM = {
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
const int knightRiderSize = sizeof(knightRiderNotes)/sizeof(int);
const int knightRiderTempo = 0;



const int gameOfThronesNotes[] PROGMEM = {
  NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, //1
  NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4,
  NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_E4, NOTE_F4,
  NOTE_G4, NOTE_C4,//5

  NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, //6
  NOTE_D4, //7 and 8
  NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_C4, //11 and 12

  NOTE_G4, NOTE_C4,//5
  
  NOTE_DS4, NOTE_F4, NOTE_G4, NOTE_C4, NOTE_DS4, NOTE_F4, //6
  NOTE_D4, //7 and 8
  NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_F4, NOTE_AS3,
  NOTE_DS4, NOTE_D4, NOTE_C4, //11 and 12
  NOTE_G4, NOTE_C4,
  NOTE_DS4, NOTE_F4, NOTE_G4,  NOTE_C4, NOTE_DS4, NOTE_F4,
};
const int gameOfThronesLengths[] PROGMEM = {
  8, 8, 16, 16, 8, 8, 16, 16, //1
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  8, 8, 16, 16, 8, 8, 16, 16,
  -4, -4,//5

  16, 16, 4, 4, 16, 16, //6
  -1, //7 and 8
  -4, -4,
  16, 16, 4, -4,
  16, 16, -1, //11 and 12
  
  -4, -4,//5
  
  16, 16, 4, 4, 16, 16, //6
  -1, //7 and 8
  -4, -4,
  16, 16, 4, -4,
  16, 16, -1, //11 and 12
  -4, -4,
  16, 16, 4,  4, 16, 16,
};
const int gameOfThronesSize = sizeof(gameOfThronesNotes)/sizeof(int);
const int gameOfThronesTempo = 110;










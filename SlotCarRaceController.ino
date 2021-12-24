// Wire library is for I2C communication
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						            // main hd44780 header file
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i/o class header for i2c expander backpack 
// libraries to support 4 x 4 keypad
#include <Keypad.h>
// library for 7-seg LED Bars
#include <LedControl.h>
// library of sounds
#include "pitches.h"


// ********* AUDIO HARDWARE *********
const byte buzzPin = A3;

int melody[] = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};


//***** Setting Up Lap Triggers and Pause-Stop button *********
const byte lane1Pin = PIN_A0;
const byte lane2Pin = PIN_A1;
const byte pauseStopPin = PIN_A2;
const byte ledPIN = 13;
int ledState = HIGH;

//***** Variables for LCD 4x20 Display **********
// Pin A4 is used for SDA
// Pin A5 used for SCL
// the hd44780 library I think hardcodes or auto-detects these pins.
// declare lcd object: auto locate & config exapander chip
hd44780_I2Cexp lcd;
// Set display size
// const int LCD_COLS = 20;
// const int LCD_ROWS = 2;
const byte LCD_COLS = 20;
const byte LCD_ROWS = 4;
const byte RACE_CLK_POS = 13;
const byte PRESTART_CLK_POS = RACE_CLK_POS - 2;


// ***** 7-Seg 8-digit LED Bars *****
// LedControl(DataIn, CLK, CS/LOAD, Number of Max chips (ie 8-digit bars))
const byte PIN_TO_LED_DIN = 2;
const byte PIN_TO_LED_CS = 3;
const byte PIN_TO_LED_CLK = 4;
const byte LED_BAR_COUNT = 2; // # of attached max7219 controlled LED bars
const byte LED_DIGITS = 8;    // # of digits on each LED bar
LedControl lc = LedControl(PIN_TO_LED_DIN, PIN_TO_LED_CLK, PIN_TO_LED_CS, LED_BAR_COUNT);


//***** Declare KeyPad Variables *****
// set keypad size
const int KP_COLS = 4;
const int KP_ROWS = 4;
// Layout KeyMap
char keys[KP_ROWS][KP_COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// Establish the row pinouts, {Row1,Row2,Row3,Row4} => Arduino pins 5,6,7,8
byte pin_rows[KP_ROWS] = {5,6,7,8};
// Establish the column pinouts, {Col1,Col2,Col3,Col4} => Arduino pins 7,8,9,10
byte pin_column[KP_COLS] = {9,10,11,12}; 
// Declare keypad object
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, KP_ROWS, KP_COLS );

// using an enum to define reference id for attached diplays.
enum displays {
  lcdDisp,
  led1Disp,
  led2Disp
};
// create enum to hold possible state values
enum states {
  Menu,
  Race,
  Paused
};
// state that a lane can be in during a race
// enum defaults to these values, but they are written in order to indicate it's important to preserve
enum laneState {
  Off = 0,      // lane is not being used
  Active = 1,   // lane is being used and live in a race
  StandBy = 2   // lane is being used in a race, but currently not active
};
// racetypes (standard is 1st to reach lap count)
enum races {
  Standard,  // First to finish the set number of laps
  Timed,     // Finish the most laps before time runs out
  Pole       // 1-10 lap practice time trials
};
// used to set clock time width;
enum clockWidth {
  H,    // 00:00:00.0
  M,    // 00:00.0
  Sd,   // 00.0
  Sc,   // 00.00
  Sm    // 00.000
};

//*** Variables and defaults for USER SET, RACE PROPERTIES
races raceType = Standard;
// raceSetTime[0] holds Min, raceSetTime[1] holds Sec
const byte defMin = 2;
const byte defSec = 0;
int raceSetTime[2] = {2, 0};
// manually set to milliseconds = to raceSetTime default, must update if that changes
unsigned long raceSetTimeMs = 120000;
int raceLaps = 8;
// pre-race countdown length in seconds
byte preStartCountDown = 5;
// flag to tell race timer if race time is going up or going down as in a time limit race.
bool countingDown = false;

// bitwise flag mask for enabling/disabling lanes
const byte lane1Enabled = (1 << 0);
const byte lane2Enabled = (1 << 1);

// racer#'s value is the index of Racers[] identifying the racer name
int racer1 = 0;
int racer2 = 1;
// Variable to track current lap being timed.
// Note this may often be 1 greater than the lap of interest
int r1LapCount = 0;
int r2LapCount = 0;
// for fastest laps table col0 = lap#, col1 = laptime in ms
byte const fastLapsLimit = 10;
unsigned long r1FastestLaps[ fastLapsLimit ][2] = {};
unsigned long r2FastestLaps[ fastLapsLimit ][2] = {};
unsigned long topFastestLaps[ 2 * fastLapsLimit ][2] = {};
// There is not room to store all laps, so we only track the last X lap timeStamps
// The modulus of the lap count by the lapMillisQSize, will set the index.
// (lapCount % lapMillisQSize) = idx will always be 0 <= idx < lapMillisQSize
// DO NOT SET lapMillisQSize < 3
byte const lapMillisQSize = 5;
unsigned long r1LastXMillis [ lapMillisQSize ] = {};
unsigned long r2LastXMillis [ lapMillisQSize ] = {};

// Array variable to hold lane state and property indexes
// col1 is the idx of the text array for the display, col2 is its enabled bit flag value
byte lanesEnabled[3][2] = {
  {0, 3},  // All           0x00000011
  {1, 1},  // Lane 1 Only   0x00000001
  {2, 2}   // Land 2 Only   0x00000010
};
// The millis() timestamp at start of each timed loop and race
unsigned long curMillis;
unsigned long raceStartMillis;
// Live elapsed race time in ms, or remaining time in preStart or timed race
unsigned long raceCurTime;
// The millis() timestamp of last displayed timepoint
unsigned long lastTickMillis;
// Live lap elapsed time in ms of current lap for each racer
unsigned long r1CurLapTime;
unsigned long r2CurLapTime;
// used to store millis() timestamp at start of current lap
unsigned long r1LapStartMillis;
unsigned long r2LapStartMillis;
// Interval in milliseconds that the clock displays are updated.
// this value does not affect lap time precision, it's only for display updating
int displayTick = 100;
// timestamp to be used for debouncing the pause/stop button
unsigned long pauseDebounceMillis = 0;
// flag indicating if race state is in preStart countdown or active race
bool preStart = false;

// The # of physical lanes that will have a counter sensor
byte const laneCount = 2;
// we add one to the lane count to account for 'All'
const char* laneText[laneCount + 1] = {"All", "1 Only", "2 Only"};
// lanes enabledLanes = [0, 6] is lanes 1 and 2;
byte lanesEnabledIdx = 0;
// initialize lane state to be off which will be 0 and evaluate as false
// if enabled, the state will be made truthy (>0).
laneState lane1State = Off;
laneState lane2State = Off;

// flags to indicate that the previous lap should be displayed
// 0 = don't show lap flash, 1 = write last lap time to dispaly, 2 = show lap flash
byte lane1LapFlash = 0;
byte lane2LapFlash = 0;
const int flashDisplayTime = 1250;
unsigned long flash1StartMillis;
unsigned long flash2StartMillis;
int lane1FlashRemaining = 0;
int lane2FlashRemaining = 0;

byte resultsMenuIdx = 0;
unsigned long lane2Time = 0;
// this flag is used to indicate the first entry into a state so
// that the state can do one time only prep for its first loop
bool entryFlag = true;

// variable to hold the last digit displayed so it does not rewrite every program loop
byte ledCountdownTemp = 0;

enum Menus {
  MainMenu,
    SettingsMenu,
      SetRaceTimeMenu,
      SetRaceLapsMenu,
      SetLanesMenu,
    SelectRacersMenu,
    SelectRaceMenu,
    ResultsMenu,

};

// create variale to hold current 'state'
states state = Menu;
Menus currentMenu;

//****** Menu String Arrays **********
// Racer list
// need to keep a count of the number of names in list becauase
// because this is an array of different length character arrays
// there is not easy way to determine the number of elements
// So we use a constant to set the length.
byte const racerCount = 7;
// For 7-seg, there are no W's, M's, X's, K's, or V's
const char* Racers[racerCount] = {
  "Lucien", "ZOE", "Elise", "John", "Angie", "Uncle BadAss", "5318008"
};
const char* Start = {"Start"};
const char* MainText[4] = {
  "A| Select Racers",
  "B| Change Settings",
  "C| Select a Race",
  "D| See Results"
};
// Main Menu's sub-Menus
const char* SettingsText[4] = {
  " A|Lap  B|Min  C|Sec",
  " D|Lanes:",
  "Num Laps:",
  "Racetime:   :"
};
const char* SelectRacersText[4] = {
  "<--A            B-->",
  "Racer1:",
  "<--C            D-->",
  "Racer2:"
};
const char* SelectRaceText[4] = {
  "A|First to     Laps",
  "B|Most Laps in   :",
  "C|Start Pol Trials",
  "D|Countdown:    Sec"
};
const char* ResultsText[4] = {
  "RESULTS: C|Overall",
  "",
  "",
  ""
};



// Passing in variables byRef (using &) so we can update the input variables directly.
void SplitTime(unsigned long curr, unsigned long &ulHour, unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulDec, unsigned long &ulCent, unsigned long &ulMill) {
  // Calculate HH:MM:SS from millisecond count
  ulMill = curr % 1000;
  ulCent = curr % 1000 / 10;
  ulDec = curr % 1000 / 100;
  ulSec = curr / 1000;
  ulMin = ulSec / 60;
  ulHour = ulMin / 60;
  ulMin -= ulHour * 60;
  ulSec = ulSec - ulMin * 60 - ulHour * 3600;
}

// Converts clock time into milliseconds
unsigned long ClockToMillis(const byte ulHour, const byte ulMin, const byte ulSec) {
  // Sets the number of milliseconds from midnight to current time
  return (ulHour * 60 * 60 * 1000) +
         (ulMin * 60 * 1000) +
         (ulSec * 1000);
}


void Beep() {
  tone(buzzPin, 4000, 200);
}

// Used to set fastest lap array to high numbers that will be replaced
// A lap number of 0 indicates it is a dummy lap
void InitializeRacerArrays(){
  for (byte i = 0; i < fastLapsLimit; i++) {
    r1FastestLaps[i][0] = 0;
    r1FastestLaps[i][1] = 999999;
    r2FastestLaps[i][0] = 0;
    r2FastestLaps[i][1] = 999999;
  }
  for (byte i = 0; i < lapMillisQSize; i++) {
    r1LastXMillis[i] = 0;
    r2LastXMillis[i] = 0;
  }
}

// Used to initialize fastest lap arrays to high numbers that will be replaced.
// A lap number of 0 marks it is a dummy lap.
void InitializeResultsArrays(){
  for (byte i = 0; i < fastLapsLimit * 2; i++) {
    topFastestLaps[i][0] = 0;
    topFastestLaps[i][1] = 999999;
  }
}

// This function sets up change interrupt for pins among A0-A6
void pciSetup(byte pin) {
  // Enable interrupts on pin
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));
  // Clear any outstanding interrupt
  PCIFR  |= bit (digitalPinToPCICRbit(pin));
  // Enable interrupt for the group
  PCICR  |= bit (digitalPinToPCICRbit(pin));
}
void clearPCI(byte pin) {
  // Clear any outstanding interrupt
  PCIFR  |= bit (digitalPinToPCICRbit(pin));
  // Disable interrupts on pin
  *digitalPinToPCMSK(pin) &= bit (digitalPinToPCMSKbit(pin));
}

// debounceTime (ms), time within which not to accept additional signal input
const int debounceTime = 1000;
// ISR is a special Arduino Macro or routine that handles interrupts ISR(vector, attributes)
// PCINT1_vect handles pin change interrupt for the pin block A0-A5, represented in bit0-bit5
ISR (PCINT1_vect) {
  unsigned long logMillis = millis();
  // Serial.println("LED Interrupt log Millis");
  // Serial.println(logMillis);
  // Serial.println("PINC");
  // Serial.println(PINC);

  // We have setup the lap triggers and Pause button as inputs.
  // This means the pins have been set to HIGH, indicated by a 1 on its register bit.
  // When a button is pressed, or sensor triggered, it should bring the pin LOW.
  // A LOW pin is indicated by a 0 on its port register.

  // Lane 1 positive trigger PINC = 0xXXXXXXX0
  // Lane 1 is on pin A0 which uses the first bit of the registry, idx 0, of PINC Byte
  if (!IsBitSet(PINC, 0)) {
    // if it's the first log, it's the intial cross of start line
    if (r1LapCount == 0) {
      lane1State = Active;
      r1LastXMillis [ r1LapCount ] = logMillis;
      r1LapStartMillis = logMillis;
      // Serial.println("R1 Lap Count 0");
      // Serial.println(r1LapCount);
      r1LapCount++;
      Beep();
    } else if ((logMillis - r1LastXMillis [(r1LapCount-1) % lapMillisQSize] ) > debounceTime){
      lane1LapFlash = 1;
      r1LastXMillis [r1LapCount % lapMillisQSize] = logMillis;
      r1LapStartMillis = logMillis;
      // Serial.println("R1 Lap Count 1++");
      // Serial.println(r1LapCount);
      r1LapCount++;
      Beep();
    }
    // if not the first log or beyond debounce time then don't log the trigger
    // Serial.println("R1 r1LastXMillis [0]");
    // Serial.println(r1LastXMillis [0]); 
    // Serial.println("R1 r1LastXMillis [1]"); 
    // Serial.println(r1LastXMillis [1]); 
    // Serial.println("A0");
    // digitalWrite(ledPIN, HIGH);
  }
  // Lane 2 positive trigger PINC = 0xXXXXXX0X
  // Lane 2 is on pin A1 which uses the 2nd bit of the registry, idx 1, of PINC Byte
  if (!IsBitSet(PINC, 1)) {
    if (r2LapCount == 0) {
      lane2State = Active;
      r2LastXMillis [ r2LapCount ] = logMillis;
      r2LapStartMillis = logMillis;
      // Serial.println("R2 Lap Count 0");
      // Serial.println(r2LapCount);
      r2LapCount++;
      Beep();
    } else if ((logMillis - r2LastXMillis [(r2LapCount-1) % lapMillisQSize] ) > debounceTime){
      lane2LapFlash = 1;
      r2LastXMillis [r2LapCount % lapMillisQSize] = logMillis;
      r2LapStartMillis = logMillis;
      // Serial.println("R2 Lap Count 1++");
      // Serial.println(r2LapCount);
      r2LapCount++;
      Beep();
    }
  } // END of PINC, 1 aka lane/racer2 pin detect

  // Puase button positive trigger PINC = 0xXXXXX0XX
  // Pause is on pin A2 which uses the 3rd bit of the registry, idx 2, of PINC Byte
  if (!IsBitSet(PINC, 2)) {
    // If not within debounce time of previous trigger process
    if (logMillis - pauseDebounceMillis > debounceTime){
      pauseDebounceMillis = logMillis;
      switch(state){
        // if currently in a race, then put in Paused state
        case Race:
          state = Paused;
          entryFlag = true;
        break;
        // if already paused then return to race
        case Paused:
          state = Race;
          // make sure entry flag is turned back off before re-entering race state
          entryFlag = false;
        break;
        // otherwise ignore the button
        default:
        break;
      } // END switch
    } // END debounce if
  } // END of Pause button PINC2 detect
} // END of ISR()

// Function returns true if the bit at the, 'pos', postion of a byte is 1,
// otherwise it returns false.
// In a Byte, position is from right to left and starts with 0.
// EX. If function parameters are pos = 3 and b = 0x11111000
//     the bit 1 = 0x00000001, if we left shift 3 places we get 0x00001000
//     thus 0x00001000 & (0x11111000) = 0x00001000
// which is not zero, so this returns true, that bit of the input Byte is set.
bool IsBitSet(byte b, byte pos) {
   return (b & (1 << pos)) != 0;
}

// Update menu on main LCD with static base text for given menu screen
void lcdUpdateMenu(const char *curMenu[]){
  // For each string in the menu array, print it to the screen
  // Clear screen first, in case new text doesn't cover all old text
  lcd.clear();
  for (int i=0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(curMenu[i]);
  }
}


// Provides, looping, up or down indexing of items in list
// This function assumes the start of the list is at zero index
int IndexList(int curIdx, int listLength, bool cycleUp = true) {
  int newIndex = curIdx;
  if (cycleUp) {
    // Cycling up we need to reset to zero if we reach end of list.
    // The modulus (%) of a numerator smaller than its denominator is equal to itself,
    // while the modulus of an integer with itself is 0.
    newIndex = (newIndex + 1) % (listLength);
  } else {
    // if cycling down list we need to reset to end of list if we reach the beginning
    if (curIdx == 0){
      newIndex = listLength - 1;
    } else {
      newIndex--;
    }
  }
  return newIndex;
}


// numberIn   --Any positive number to be printed to display
// width      --The number of places/digits to be printed
// endPosIdx  --The index of the character position that contains the final digit
// display    --The display enum that determines where to print number
// leadingZs  --Flag, if = true, if number actual width is less than 'width' fill with '0'
// line       --For display's that have more than 1 line, line on which to print number
// endWithDecimal   --Flag, if true print decimal at end of number
// NOTES:  this function doesn't error check, if the number is larger than the width
// the digits outside of the width (the largest place digits) will be cut off.
void PrintNumbers(const unsigned long numberIN, const byte width, const byte endPosIdx, displays display, bool leadingZs = true, const byte line = 0, bool endWithDecimal = false){
  // we take 1 away from width because the endposition is inclusive in the width count
  byte cursorStartPos = endPosIdx - (width - 1);
  byte cursorEndPos = endPosIdx;
  // variable to track present digit by its place value
  byte placeValue = width - 1;
  switch (display) {
    // for printing to the main LCD
    case lcdDisp: {
      // written to screen from max place value to min defined by width and end position
      for (byte displayIdx = cursorStartPos; displayIdx <= cursorEndPos; displayIdx++) {
        lcd.setCursor(displayIdx, line);
        // print current place value's digit
        lcd.print(numberIN / ipow(10, placeValue) % 10);
        placeValue--;
      }
      if (endWithDecimal) lcd.print(".");
    }
    break;
    // for printing to LED bar displays
    case led1Disp: case led2Disp: {
      for (byte displayIdx = cursorStartPos; displayIdx <= cursorEndPos; displayIdx++) {
        // print current place value's digit
        lc.setDigit(
          display - 1, 
          (LED_DIGITS - 1) - displayIdx, 
          numberIN / ipow(10, placeValue) % 10,
          endWithDecimal && displayIdx == cursorEndPos);
        placeValue--;
      }
    }
    default:
    break;
  }
}


// input time in ms; clockEndPos is the index of where the last digit of the clock should go.
// this index is always considered left to right, and starting with 0.
// The function will flip index internally if needed for right to left indexed display
// The MAX7219 LED bars are such displays where, internally, digit idx 0 is the far right digit
// Using this function with LED bar, to have clock end at far right input clockEndPos 7
void PrintClock(ulong timeMillis, byte clockEndPos, clockWidth printWidth, displays display, byte line = 0) {
  unsigned long ulHour;
  unsigned long ulMin;
  unsigned long ulSec;
  unsigned long ulDec;
  unsigned long ulCent;
  unsigned long ulMill;
  SplitTime(timeMillis, ulHour, ulMin, ulSec, ulDec, ulCent, ulMill);
  // calculate and adjust start position affected by number of decimal second digits requested
  // the Hours position sets the start, we use the endPos of the hours as the start endPos
  unsigned long decimalSec;
  byte decimalWidth;
  byte hourEndPos;
  switch(printWidth){
    break;
    case Sc:
      decimalSec = ulCent;
      decimalWidth = 2;
      hourEndPos = clockEndPos - 9;
    break;
    case Sm:
      decimalSec = ulMill;
      decimalWidth = 3;
      hourEndPos = clockEndPos - 10;
    break;
    default:
      // if not cent or mill, all clock prints with seconds includes .0 by default
      decimalSec = ulDec;
      decimalWidth = 1;
      hourEndPos = clockEndPos - 8;
    break;
  }

  // Using a series of if statements instead of a switch because of how the
  // the cascading width variable falls through with options it works better with ifs
  if (printWidth == H) {
    // H 00:00:00.0
    switch (display){
      case lcdDisp: {
        PrintNumbers(ulHour, 2, hourEndPos, display, true, line);
        lcd.print(":");
      }
      break;
      case led1Disp: led2Disp: {
        PrintNumbers(ulHour, 2, clockEndPos - 5, display, true, 0, true);
      }
      break;
      default:
      break;
    } // END of display switch
    printWidth = M;
  }

  if (printWidth == M) {
    // M 00:00.0
    switch (display){
      case lcdDisp: {
        PrintNumbers(ulMin, 2, hourEndPos + 3, display, true, line);
        lcd.print(":");
      }
      break;
      case led1Disp: case led2Disp: {
        PrintNumbers(ulMin, 2, clockEndPos - 3, display, true, 0, true);
      }
      break;
      default:
      break;
    } // END of display switch
    // the default decimal is deci-seconds ie 00.0
    printWidth = Sd;
  }
  // This final if is probably not necessary if all clocks end in S as we already
  // evaluate the same options at the beginning to set the value for decimalSec.
  // Any adjustments to code that changes seconds, like adding a no decimal option
  // should be made at that point.
  // However, keeping the if sets up for the preferred next update which adds
  // the option of no decimal & decimal with M & H, ie: S, Md, Mc, Mm, Hd, Hc, Hm
  // 
  // If width starts with Seconds then there is the option to display .0d, .00c, or .000m
  if (printWidth == Sd || printWidth == Sc || printWidth == Sm) {
    // Sd 00.0,   Sc 00.00,   Sm 00.000
    switch (display){
      case lcdDisp: {
        PrintNumbers(ulSec, 2, hourEndPos + 6, display, true, line, true);
        lcd.print(decimalSec);
      } // END lcdDisp case
      break;
      case led1Disp: case led2Disp: {
        PrintNumbers(ulSec, 2, clockEndPos - decimalWidth, display, true, 0, true);
        PrintNumbers(decimalSec, decimalWidth, clockEndPos, display, true);
        // lc.setDigit(display - 1, (LED_DIGITS-1) - clockEndPos, decimalSec, false);
      } // END of ledDips cases
      break;
      default:
      break;
    } // END of display switch
  }
  // Serial.println("ulMin");
  // Serial.println(ulMin);
  // Serial.println("ulSec");
  // Serial.println(ulSec);
  // Serial.println("ulcent");
  // Serial.println(ulcent);
} // END PrintClock()


int calcDigits(int number){
  int digitCount = 0;
  while (number != 0) {
    // integer division will drop decimal
    number = number/10;
    digitCount++;
  }
  return digitCount;
}


// clear screen on given line from start to end position, inclusive
// the defalut is to clear the whole line
void lcdClearLine(byte lineNumber, byte posStart = 0, byte posEnd = LCD_COLS) {
  lcd.setCursor(posStart, lineNumber);
  for(byte n = posStart; n < posEnd; n++){
    lcd.print(" ");
  }
}

void ledWriteDigits(byte digit) {
  for (int j=0; j < LED_BAR_COUNT; j++){
    for (int i=0; i < LED_DIGITS; i++){
      lc.setDigit(j, i, digit, false);
    }
  }
}

void PrintText(const char textToWrite[], const displays display, const byte writeSpaceEndPos, const byte width = LED_DIGITS, bool rightJust = false, const byte line = 0, bool clear = true) {
  // Even though we say right justified we still truncate the name from the left.
  // need to track the character index seperately from display digit position
  byte const strLength = strlen(textToWrite);
  // we take 1 away from width because the endposition is inclusive in the width count
  byte cursorStartPos = writeSpaceEndPos - (width - 1);
  byte cursorEndPos = writeSpaceEndPos;
  // adjust start and end positions based on name length and available space
  if (strLength < width && rightJust) cursorStartPos = cursorEndPos - (strLength - 1);
  if (strLength < width && !rightJust) cursorEndPos = cursorEndPos - (width - strLength);
  // because the writing position index may not match the name char index
  // we nust track them seperately.
  byte nameIndex = 0;
  switch (display) {
    case lcdDisp: {
      if (clear) lcdClearLine(line, writeSpaceEndPos - width, writeSpaceEndPos);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        lcd.setCursor(i, line);
        lcd.print(textToWrite[nameIndex]);
        nameIndex++;
      }
    }
    break;
    case led1Disp: case led2Disp: {
      if (clear) lc.clearDisplay(display - 1);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        // The digit position of the LED bars is right to left so we convert
        lc.setChar(display - 1, (LED_DIGITS-1) - i, textToWrite[nameIndex], false);
        nameIndex++;
      }
    } // END LED cases
    break;
    default:
    break;
  } // END display switch
}


void UpdateFastestLap(unsigned long fastestArray[fastLapsLimit][2], const int lap, const unsigned long newLapTime){
  Serial.println("ENTERED FASTEST");
  for (byte i = 0; i < fastLapsLimit; i++){
        Serial.print("Fastest For i = ");
        Serial.println(i);
        Serial.println(fastestArray[i][1]);
        Serial.println(newLapTime);
    // Starting from beginning of list, if new lap time is faster
    // then replace and shift remaining rows down an index
    if (fastestArray[i][1] > newLapTime) {
      // starting from the end, replace rows with previous row until back to newly added lap
      for (byte j = fastLapsLimit-1; j > i; j--){
        fastestArray[j][0] = fastestArray[j-1][0];
        fastestArray[j][1] = fastestArray[j-1][1];
      }
      // then replace old i lap time wiht new, faster lap and time
      fastestArray[i][0] = lap;
      fastestArray[i][1] = newLapTime;
      for (int k = 0; k < fastLapsLimit; k++){
        Serial.println("UpdateFastest End");
        Serial.print(fastestArray[k][0]);
        Serial.print(" : ");
        Serial.println(fastestArray[k][1]);
      }
      // exit function, no need to continue once found
      return;
    } // END if (fastestArray) block
  } // END fastest lap array for loop
}

void BuildTopFastest(unsigned long racerFastest[ fastLapsLimit * 2 ][2], const byte racer, const int lapNumber, const unsigned long lapTime) {

}


bool melodyPlaying = false;
// Holds the time of last tone played so timing of next note in melody can be determined
unsigned long lastNoteMillis;

// *********************************************
// ********** SETUP ****************************
// Initialize hardware and establish software initial state
void setup(){
  // SETUP LCD DIPSLAY
	// initialize LCD with begin() which will return
  // a non-zero error code int if it fails, or zero on success 
	// the actual status codes are defined in <hd44780.h>
	int status = lcd.begin(LCD_COLS, LCD_ROWS);
  // if display initialization fails, trigger onboard error LED if exists
	if(status) hd44780::fatalError(status);
  // clear the display of any existing content
  lcd.clear();

  // SETUP LED 7-SEG, 8-DIGIT MAX7219 LED BARS
  //we have already set the number of devices when we created the LedControl
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  for(int deviceID = 0; deviceID < devices; deviceID++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(deviceID, false);
    // This sets the brightness, but doesn't work with this LCD, use pot on back
    // lc.setIntensity(deviceID, 12);
    /* and clear the display */
    lc.clearDisplay(deviceID);
  }

  // SETUP LAP TRIGGERS AND BUTTONS
  // roughly equivalent to digitalWrite(lane1Pin, HIGH)
  pinMode(lane1Pin, INPUT_PULLUP);
  pinMode(lane2Pin, INPUT_PULLUP);
  pinMode(pauseStopPin, INPUT_PULLUP);
  // Setup an indicator LED
  pinMode(ledPIN, OUTPUT); 
  digitalWrite(ledPIN, ledState);

  // we don't really need to setup lap time arrays here they but we
  // do it here just to put everything in a usable default state at startup
  InitializeRacerArrays();
  InitializeResultsArrays();

  // Setup initial program variables
  currentMenu = MainMenu;
  entryFlag = true;
  
  // open connection on serial port
  Serial.begin(9600);
  // Serial.println(currentMenu);
}


void loop(){
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
  // Serial.println("R2Lap count MAIN");
  // Serial.println(r2LapCount);
  switch (state) {
    // Serial.println("Entered Stat Switch");
    // In the 'Menu' state the program is focused on looking for keypad input
    // and using that keypad input to navigate the menu tree
    case Menu:{
      // Serial.println("entering Menu STATE");
      char key = keypad.getKey();
      // only if a press is detected or it is the first loop of a new state
      // do we bother to evaluate anything
      if (key || entryFlag) {
        Beep();
        // clear the lap log interrupts on initial entry into menu state just to make sure
        clearPCI(lane1Pin);
        clearPCI(lane2Pin);
        // All of the menus and sub-menus have been flattened to a single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree
          // Within each menu case is another switch for keys with a response in that menu
          case MainMenu:
            if (entryFlag) {
              lcdUpdateMenu(MainText);
              entryFlag = false;
            }
            switch (key) {
              case 'A':
                currentMenu = SelectRacersMenu;
                entryFlag = true;
                break;
              case 'B':
                currentMenu = SettingsMenu;
                entryFlag = true;
                break;
              case 'C':
                currentMenu = SelectRaceMenu;
                entryFlag = true;
                break;
              case 'D':
                currentMenu = ResultsMenu;
                entryFlag = true;
              default:
                break;
            }
            break;

          case SettingsMenu:
            if (entryFlag) {
              //draw non-editable text
              lcdUpdateMenu(SettingsText);
              // draw current minute setting
              PrintNumbers(raceSetTime[0], 2, 11, lcdDisp, true, 3);
              // draw current seconds setting
              PrintNumbers(raceSetTime[1], 2, 14, lcdDisp, true, 3);
              // draw current lap count
              PrintNumbers(raceLaps, 3, 12, lcdDisp, true, 2);
              //draw current lane settings
              lcd.setCursor(10, 1);
              lcd.print(laneText[lanesEnabled[lanesEnabledIdx][0]]);
              entryFlag = false;
            }
            switch (key) {
              case 'A':
                // change lap count
                raceLaps = lcdEnterNumber(3, 999, 2, 10);
                break;
              case 'B':
                // Change minutes
                raceSetTime[0] = lcdEnterNumber(2, 60, 3, 10);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[0], raceSetTime[1]);
                break;
              case 'C':
                // change seconds
                raceSetTime[1] = lcdEnterNumber(2, 59, 3, 13);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[0], raceSetTime[1]);
                break;
              case 'D':
                // cycles through active lane options
                lanesEnabledIdx = (lanesEnabledIdx + 1) % ((sizeof lanesEnabled/sizeof lanesEnabled[0]));
                lcdClearLine(1, 10);
                lcd.setCursor(10, 1);
                lcd.print(laneText[lanesEnabled[lanesEnabledIdx][0]]);
                break;
              case '*':
                // return to main menu
                currentMenu = MainMenu;
                entryFlag = true;
                break;
              default:
                break;
            }
            break;

          case SelectRacersMenu: {
            // cursor position that the racer names start
            byte nameCursorPos = 8;
            if (entryFlag) {
              // write base, static text to screen
              lcdUpdateMenu(SelectRacersText);
              // write current racer1's name to screen
              lcd.setCursor(nameCursorPos,1);
              lcd.print(Racers[racer1]);
              // write current racer2's name to screen
              lcd.setCursor(nameCursorPos,3);
              lcd.print(Racers[racer2]);
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down racer list and update Racer1 name
              case 'A': case 'B':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(1, nameCursorPos);
                lcd.setCursor(nameCursorPos,1);
                racer1 = IndexList(racer1, racerCount, key == 'A');
                lcd.print(Racers[racer1]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(3, nameCursorPos);
                lcd.setCursor(nameCursorPos,3);
                racer2 = IndexList(racer2, racerCount, key == 'D');
                lcd.print(Racers[racer2]);
                break;
              }
              case '*':{
                // return to MainMenu
                currentMenu = MainMenu;
                entryFlag = true;
                break;
              }
              default:
                break;
            } // END of keypad switch   
            break;
          } // END SelectRacer Menu Case

          case SelectRaceMenu: {
            if (entryFlag) {
              //draw non-editable text
              lcdUpdateMenu(SelectRaceText);
              // add current race lap setting to lap race selection text
              PrintNumbers(raceLaps, 3, 13, lcdDisp, true, 0);
              // add current race minutes setting to timed race screen
              PrintNumbers(raceSetTime[0], 2, 16, lcdDisp, true, 1);
              // add current race seconds setting to timed race screen
              PrintNumbers(raceSetTime[1], 2, 19, lcdDisp, true, 1);
              // add the curent preStartCountDown setting to screen
              PrintNumbers(preStartCountDown, 2, 14, lcdDisp, true, 3);
              entryFlag = false;
            }
            switch (key) {
              case 'A': case 'B': case 'C':
                state = Race;
                preStart = true;
                entryFlag = true;
                // if the lane is enabled in settings then set to truthy default, StandBy
                // if (lane1Enabled & lanesEnabled[lanesEnabledIdx][1] > 0) lane1State = StandBy;
                // if (lane2Enabled & lanesEnabled[lanesEnabledIdx][1] > 0) lane2State = StandBy;
                if (key == 'A') {raceType = Standard; countingDown = false;}
                else if (key == 'B') {raceType = Timed; countingDown = true;}
                else {raceType = Pole; countingDown = false;};
                break;
              case 'D':
                // change how long the preStartCountDown before the race start lasts
                preStartCountDown = lcdEnterNumber(2, 30, 3, 13);
                break;
              case '*':
                // return to main menu
                currentMenu = MainMenu;
                entryFlag = true;
                break;
              default:
                break;
            } // END of key switch
            break;
          } // END of SelectRace Menu Case
          case ResultsMenu:{
            if (entryFlag) {
              lcdUpdateMenu(ResultsText);
              r1LapCount = 0;
              r2LapCount = 0;
              lcdClearLine(1);
              lcd.setCursor(5, 1);;
              // lcd.print( r1LastXMillis [ r1LapCount ] );
              lcdClearLine(3);
              lcd.setCursor(5, 3);
              // lcd.print( r2LastXMillis [ r2LapCount ]);
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down the lap list
              case 'A': case 'B':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(1);
                lcd.setCursor(5, 1);
                if (key == 'A') r1LapCount--;
                if (key == 'B') r1LapCount++;
                lcd.print(r1LastXMillis[r1LapCount]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(3);
                lcd.setCursor(5, 3);
                if (key == 'C') r2LapCount--;
                if (key == 'D') r2LapCount++;
                lcd.print(r2LastXMillis[r2LapCount]);
              }
              break;
              case '*':{
                // return to MainMenu
                currentMenu = MainMenu;
                entryFlag = true;
              }
              break;
              default:
              break;
            } // END of keypad switch   
            break;
          } // END of ResultsMenu Case
          default:
          break;
        }; // end of Menu switch
      }; // end of if key pressed wrap
      break; 
    } // END of Menu State 

    case Race:{
      curMillis = millis();
      // initialize variables for preStart timing loop first cycle only
      if (entryFlag & preStart) {
        // initialize race lap time arrays to [0, 999999]
        InitializeRacerArrays();
        // preStartCountDown is in seconds so we convert to millis
        raceCurTime = preStartCountDown * 1000;
        lastTickMillis = curMillis;
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Your Race Starts in:");
        PrintClock(raceCurTime, PRESTART_CLK_POS, Sd, lcdDisp, 2);
        PrintText(Racers[racer1], led1Disp, 7, 8, true);
        PrintText(Racers[racer2], led2Disp, 7, 8, false);
        entryFlag = false;
        r1LapCount = 0;
        r2LapCount = 0;
      }
      // initialize variables for race timing loop first cycle only
      if (entryFlag & !preStart) {
        raceCurTime = 0;
        lastTickMillis = curMillis;
        raceStartMillis = curMillis;
        entryFlag = false;
        // Write Live Race menu and racer names to displays
        lcd.clear();
        lcd.setCursor(9,1);
        lcd.print("Lap | Best");
        PrintText(Racers[racer1], lcdDisp, 7, 8, false, 2);
        lcd.print(":");
        lcd.setCursor(10, 2);
        lcd.print("0");
        lcd.setCursor(14, 2);
        lcd.print("00.000");
        PrintText(Racers[racer2], lcdDisp, 7, 8, false, 3);
        lcd.print(":");
        lcd.setCursor(10, 3);
        lcd.print("0");
        lcd.setCursor(14, 3);
        lcd.print("00.000");
        // Indicate race start for each racer
        PrintText(Start, led1Disp, 7, 8, false);
        PrintText(Start, led2Disp, 7, 8, false);
        // put any enabled lanes into an active state ready to log 
        // if (lane1State) lane1State = Active;
        // if (lane2State) lane2State = Active;
        // clear led of name text
        // lc.clearDisplay(led1Disp-1);
        // this act enables the racer to trigger first lap
        pciSetup(A0);
        pciSetup(A1);
        // enable race pause button interrupt
        pciSetup(A2);
      }

      if (preStart) {
        if (raceCurTime > 0){
          // update LCD on each tick
          if (curMillis - lastTickMillis > displayTick){
            raceCurTime = raceCurTime - displayTick;
            PrintClock(raceCurTime, PRESTART_CLK_POS, Sc, lcdDisp, 2);
            lastTickMillis = curMillis;
          }
          // In last 3 seconds send countdown to LEDs
          if (raceCurTime < 2999 & (raceCurTime/1000 + 1) != ledCountdownTemp) {
            // we add 1 because it should change on start of the digit not end
            ledCountdownTemp = raceCurTime/1000 + 1;
            ledWriteDigits(ledCountdownTemp);
          }
        } else {
          // ++++ Set Flags for RACE START +++++++
          preStart = false;
          // reset ledCountdownTemp to default for next race
          ledCountdownTemp = 0;
          // reset entry flag to true so race timing variables can be initialized
          entryFlag = true;
        }
      } else { // ********* LIVE RACE **********
        // Regardless of race type, we do these things
        if (curMillis - lastTickMillis >  displayTick){
          // update current racetime
          raceCurTime = curMillis - raceStartMillis;
          // If racetype is timed then displayTick will be a negative number
          if (countingDown) raceCurTime = ClockToMillis(0, raceSetTime[0], raceSetTime[1]) - raceSetTimeMs;
          // If lanestate is non-zero (ie active or standby) then update current lap time
          if (lane1State) r1CurLapTime = curMillis - r1LapStartMillis;
          if (lane2State) r2CurLapTime = curMillis - r2LapStartMillis;
          // Serial.println("lane1state");
          // Serial.println(lane1State);
          // Serial.println("r1CurLapTime");
          // Serial.println(r1CurLapTime);
          // Serial.println("lane1LapFlash");
          // Serial.println(lane1LapFlash);
          // Serial.println("R1 index");
          // Serial.println(r1LapCount);
          // A lap Flash is triggered by the completion of a lap, it's 'resting' value is 0
          if (lane1LapFlash > 0) {
            if (lane1LapFlash == 1) {
              unsigned long r1LapTimeToLog;
              r1LapTimeToLog = r1LastXMillis [(r1LapCount-1) % lapMillisQSize] - r1LastXMillis [(r1LapCount-2) % lapMillisQSize];
              // during the intro to the lap flash cycle we also update all the racer lap records
              // we do this here to keep it interruptable and out of the lap trigger intrrupt funct.
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(r1FastestLaps, r1LapCount - 1, r1LapTimeToLog);
              lc.clearDisplay( led1Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r1LapCount - 1, 3, 2, led1Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r1LapTimeToLog, 7, Sc, led1Disp);
              // update racer's current lap total on lcd
              PrintNumbers(r1LapCount - 1, 3, 11, lcdDisp, true, 2);
              // update racer's current best lap time to lcd
              PrintClock(r1FastestLaps[0][1], 19, Sm, lcdDisp, 2);
              // record the timestamp at which this flash period is starting at
              flash1StartMillis = curMillis;
              // set the flash state to 2 = hold until flash period time is over
              lane1LapFlash = 2;
            } else { // flash status = 2 which means it's been written and still active
              // if flash time is up, set flag to zero ending display of last lap
              if (curMillis - flash1StartMillis > flashDisplayTime) lane1LapFlash = 0;
            }
          } else { // lane flash status = 0, or inactive
            // if the lane is in use then start displaying live lap time again
            if (lane1State) {
              lc.clearDisplay(led1Disp - 1);
              PrintNumbers(r1LapCount, 3, 2, led1Disp);
              PrintClock(r1CurLapTime, 7, Sc, led1Disp);
            }
          }

          if (lane2LapFlash > 0) {
            if (lane2LapFlash == 1) {
              unsigned long r2LapTimeToLog;
              r2LapTimeToLog = r2LastXMillis [(r2LapCount-1) % lapMillisQSize] - r2LastXMillis [(r2LapCount-2) % lapMillisQSize];
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(r2FastestLaps, r2LapCount - 1, r2LapTimeToLog);
              lc.clearDisplay( led2Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r2LapCount - 1, 3, 2, led2Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r2LapTimeToLog, 7, Sc, led2Disp);
              // update racer's current lap total on lcd
              PrintNumbers(r2LapCount - 1, 3, 11, lcdDisp, true, 3);
              // update racer's current best lap time to lcd
              PrintClock(r2FastestLaps[0][1], 19, Sm, lcdDisp, 3);
              // record the timestamp at which this flash period is starting at
              flash2StartMillis = curMillis;
              // set the flash state to 2 = hold until flash period time is over
              lane2LapFlash = 2;
            } else { // flash status = 2 which means it's been written and still active
              // if flash time is up, set flag to zero ending display of last lap
              if (curMillis - flash2StartMillis > flashDisplayTime) lane2LapFlash = 0;
            }
          } else { // lane flash status = 0, or inactive
            // if the lane is in use then start displaying live lap time again
            if (lane2State) {
              lc.clearDisplay(led2Disp - 1);
              PrintNumbers(r2LapCount, 3, 2, led2Disp);
              PrintClock(r2CurLapTime, 7, Sc, led2Disp);
            }
          }
          // Update LCD with absolute race time and racer lap logs
          PrintClock(raceCurTime, RACE_CLK_POS, M, lcdDisp, 0);
          lastTickMillis = curMillis;
        } // END of if(displayTick) block

        // check for the race end condition and finishing places
        switch (raceType) {
          case Standard: {
            // ****** STANDARD FINSIH *******************
            // check for a winner
            // After crossing finish of last lap, lap Count will be equal to raceLaps + 1
            // which will trigger the finish of a standard race
            if (r1LapCount > raceLaps || r2LapCount > raceLaps) {
              // turn off lap trigger interrupts to prevent any additional lap triggers
              clearPCI(lane1Pin);
              clearPCI(lane2Pin);
              // create temp variable that will hold the determined 1st and 2nd place racers
              byte first = 255;
              byte second = 255;
              // adjust final lap count by 1 because current, unfinished, lap is not to be included
              // this ternary operator notatation states that if lapCount is 0 already,
              // then just leave is zero, otherwise subtract 1
              // we do this because '##LapCount' is used as an index so we need to protect against negatives
              r1LapCount = r1LapCount == 0 ? r1LapCount : r1LapCount - 1;
              r2LapCount = r2LapCount == 0 ? r2LapCount : r2LapCount - 1;
              Serial.println("racer1 laps final");
              Serial.println(r1LapCount);
              Serial.println("racer2 laps final");
              Serial.println(r2LapCount);
              // verify winner if one lap count is higher than the other it is clear
              if (r1LapCount > r2LapCount) {first = racer1; second = racer2;}
              else if (r2LapCount > r1LapCount) {first = racer2; second = racer1;}
              // if timing is such that 2nd place has crossed while processing then the
              // final lap timestamp will show who was first
              else if (r1LastXMillis[ r1LapCount % lapMillisQSize ] < r2LastXMillis[ r2LapCount % lapMillisQSize ]) {first = racer1; second = racer2;}
              else if (r2LastXMillis[r2LapCount % lapMillisQSize] < r1LastXMillis[r1LapCount % lapMillisQSize]) {first = racer2; second = racer1;}
              // else if lapcount and last lap timestamps are the same it's a tie
              else if(r2LapCount == r1LapCount && r1LastXMillis[r1LapCount] == r2LastXMillis[r2LapCount]){
                // we use 254 as a flag for tie, since it is larger than the racer name list size will ever be
                first = 254;
                second = 254;
              }
              if (first == 254 && second == 254) {
                PrintText("A TIE", led1Disp, 7, 8, true);
                PrintText("A TIE", led2Disp, 7, 8, true);
              } else if (first == 255) {
                PrintText("ERROR", led1Disp, 7, 8);
                PrintText("ERROR", led2Disp, 7, 8);
              } else {
                PrintText("1st", first == racer1 ? led1Disp:led2Disp, 2, 3, false);
                PrintText(Racers[first], first == racer1 ? led1Disp:led2Disp, 7, 4, true, 0, false);
                PrintText("2nd", second == racer1 ? led1Disp:led2Disp, 2, 3, false);
                PrintText(Racers[second], second == racer1 ? led1Disp:led2Disp, 7, 4, true, 0, false);
              }
              state = Menu;
              currentMenu = ResultsMenu;
              entryFlag = true;
            }
            break;
          } // END standard case
          case Timed:{

            break;
          }
          case Pole:{

            break;
          }
          default:
            break;
        } // END RaceType Switch

      } // END PreStart if-then-else


      break;
    } // END of Race state
    case Paused:{
      // Serial.println("Entered Paused state");      
      char key = keypad.getKey();
      // only if a press is detected or it is the first loop of a new state
      // do we bother to evaluate anything
      if (key == '*') {
        state = Menu;
        currentMenu = ResultsMenu;
        entryFlag = true;
      }
      break;
    } // END of Paused state
    default:{
      // if the state becomes unknown then default back to 'Menu'
      Serial.println("Entered default state");
      state = Menu;
      break;
    }
  } // END of States Switch

} // END of MAIN LOOP



// Integer power function as pow() is only good with floats
int ipow(int base, int exp) {
  int result = 1;
  for (;;) {
    if (exp & 1)
      result *= base;
    exp >>= 1;
    if (!exp)
      break;
    base *= base;
  }
  return result;
}


// This function handles user input of values on the lcd. Once this function is entered, 
// it continues to monitor keypad until the user enters enough digits.
// This funcion writes the input directly to the lcd then returns the entered number
int lcdEnterNumber(int digits, int maxValue, int line, int cursorPos){
  byte digitsOG = digits;
  // char inputNumber[digits];
  int inputNumber[digits];
  bool done = false;
  char keyIN;
  const int startCursorPos = cursorPos;
  // turn on the cursor at point of data entry
  lcd.setCursor(cursorPos, line);
  lcd.cursor();
  // clear number entry space then reset cursor to beginning of entry.
  for (int i=0; i<digits; i++){
    lcd.print(' ');
  };
  // need to reset cursor back to start of number before switch which waits for key
  lcd.setCursor(cursorPos, line);

  while (!done){
    keyIN = keypad.getKey();
    switch (keyIN) {
      case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '0': {
        Beep();
        // set cursor to current position for next digit
        lcd.setCursor(cursorPos, line);
        lcd.cursor();
        lcd.print(keyIN);
        cursorPos++;
        digits--;
        inputNumber[digits] = keyIN - '0';
        Serial.println("inputNumber digit");
        Serial.println(inputNumber[digits]);
        break;
      }
      default:
        break;
    }
    if (digits == 0){
      done = true;
      lcd.noCursor();
    }
  }

  int returnNumber = 0;
  // rebuild integer from array. The array index matches it's power of 10
  for (int i = 0; i < digitsOG; i++) {
    // can't use the built in pow() because it is only accurate with floats.
    // instead we need to use our own custom integer power funtion, ipow().
    returnNumber = (inputNumber[i]) * ipow(10, i) + returnNumber;
  }
  if (returnNumber > maxValue) {
    returnNumber = maxValue;
    lcd.setCursor(startCursorPos, line);
    if (maxValue < 10) lcd.print("0");
    lcd.print(maxValue);
  }
  // Serial.println("returnNumber");
  // Serial.println(returnNumber);
  return returnNumber;
}

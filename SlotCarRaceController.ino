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
// Establish the column pinouts, {Col1,Col2,Col3,Col4} => Arduino pins 9,10,11,12
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
// enum defaults to these values, but they are written in explicitly
// in order to indicate it's important to preserve those values.
enum laneState {
  Off = 0,      // lane is not being used
  Active = 1,   // lane is being used and live in a race
  StandBy = 2   // lane is being used in a race, but currently not active
};
// Racetype options
enum races {
  Standard,  // First to finish the set number of laps
  Timed,     // Finish the most laps before time runs out
  Pole       // TODO - NOT IMPLEMENTED
};
// used to define desired clock time width when written to display;
enum clockWidth {
  H,    // 00:00:00.0
  M,    // 00:00.0
  Sd,   // 00.0
  Sc,   // 00.00
  Sm    // 00.000
};

//*** Variables and defaults for USER SET, RACE PROPERTIES
races raceType = Standard;
// Variable for the number of laps to win standard race type
int raceLaps = 8;
// Race time for a Timed race type, most laps before time runs out wins.
// We create a clock time array to hold converted values for easy display update.
// raceSetTime[1] holds Min, raceSetTime[0] holds Sec, settable from menu.
const byte defMin = 2;
const byte defSec = 0;
byte raceSetTime[2] = {defSec, defMin};
// raceSetTime in milliseconds, will be initialized in setup().
unsigned long raceSetTimeMs;
// pre-race countdown length in seconds, settable from menu
byte preStartCountDown = 5;
// Flag to tell race timer if race time is going up or going down as in a time limit race.
// Since the default race type is standard the default countingDown is false.
bool countingDown = false;

// racer#'s value is the index of Racers[] identifying the racer name
byte racer1 = 0;
byte racer2 = 1;
// Variable to track current lap being timed.
// Note this may often be 1 greater than the lap of interest
int r1LapCount = 0;
int r2LapCount = 0;
// Fastest laps table col0 = lap#, col1 = laptime in ms
byte const fastLapsQSize = 10;
// We use two arrays, kept in sync to track fastest laps.
// one array of longs to hold the time and an int array of the lap# and racer id.
// This is to save on memory over a single 2D long array.
unsigned int r1FastestLaps[ fastLapsQSize ][2] = {};
unsigned long r1FastestTimes[ fastLapsQSize ] = {};
unsigned int r2FastestLaps[ fastLapsQSize ][2] = {};
unsigned long r2FastestTimes[ fastLapsQSize ] = {};
unsigned int topFastestLaps[ 2 * fastLapsQSize ][2] = {};
unsigned long topFastestTimes[ 2 * fastLapsQSize ] = {};
// Due to memory limits, we only track the last X lap millis() timestamps.
// The modulus of the lap count, by the lapMillisQSize, will set the looping index.
// (lapCount % lapMillisQSize) = idx, will always be 0 <= idx < lapMillisQSize
// DO NOT SET lapMillisQSize < 3
byte const lapMillisQSize = 5;
unsigned long r1LastXMillis [ lapMillisQSize ] = {};
unsigned long r2LastXMillis [ lapMillisQSize ] = {};

// bitwise mask for enabling/disabling lanes
const byte lane1Enabled = (1 << 0);
const byte lane2Enabled = (1 << 1);
// Array variable to hold lane state and property indexes
// col0 is the idx of the text array for the display, col1 is its enabled bit flag value
byte lanesEnabled[3][2] = {
  {0, 3},  // All           0x00000011
  {1, 1},  // Lane 1 Only   0x00000001
  {2, 2}   // Land 2 Only   0x00000010
};
// The millis() timestamp of the start of current program loop.
unsigned long curMillis;
// The millis() timestamp of the start of active race.
unsigned long raceStartMillis;
// Live elapsed race time in ms, or remaining time in preStart, or timed race.
unsigned long raceCurTime;
// Live lap elapsed time in ms of current lap for each racer
unsigned long r1CurLapTime;
unsigned long r2CurLapTime;
// used to store millis() timestamp at start of current lap for each racer
unsigned long r1LapStartMillis;
unsigned long r2LapStartMillis;
// The millis() timestamp of last display update.
unsigned long lastTickMillis;
// Interval in milliseconds that the clock displays are updated.
// this value does not affect lap time precision, it only sets min display update rate.
int displayTick = 100;
// timestamp to be used for debouncing the pause/stop button
unsigned long pauseDebounceMillis = 0;
// flag indicating if race state is in preStart countdown or active race
bool preStart = false;

// The # of physical lanes that will have a counter sensor
byte const laneCount = 2;
// Variable to map lane selection to menu text
// We add one to the lane count to account for 'All'
const char* laneText[laneCount + 1] = {"All", "1 Only", "2 Only"};
// lanes enabledLanes = [0, 6] is lanes 1 and 2;
byte lanesEnabledIdx = 0;
// Initialize lane state to be off, which is 0, and evaluates as false.
// if enabled, the state will be made truthy (>0).
laneState lane1State = Off;
laneState lane2State = Off;

// After a racer finishes a lap the logged time will be 'flashed' up
// to that racer's LED. The period that this lap time stays displayed
// before the display returns to logging current lap time is the 'flash' period.
// The lane Lap Flash variables indicate the state of this flash period.
// 0 = update display with racer's current lap and running time.
// 1 = write last finished lap, and its logged time, to racer's LED.
// 2 = hold the last finished lap info on display until flash time is up.
byte lane1LapFlash = 0;
byte lane2LapFlash = 0;
const int flashDisplayTime = 1500;
// millis() timestamp at start of current flash period
unsigned long flash1StartMillis;
unsigned long flash2StartMillis;

// index of top lap time to display in results window
byte resultsMenuIdx = 0;
enum Results {
  TopResults = 0,
  Racer1Results = 1,
  Racer2Results = 2
};
Results resultsSubMenu = TopResults;

// Variable to hold the last digit displayed to LEDs during last 3 sec
// of the pre-start countdown, so it does not rewrite every program loop.
byte ledCountdownTemp = 0;

// enum to name menu screens for easier reference
enum Menus {
  MainMenu,
    SettingsMenu,
    SelectRacersMenu,
    SelectRaceMenu,
    ResultsMenu
};
// create variale to hold current 'state'
states state = Menu;
Menus currentMenu;
// This flag is used to indicate first entry into a state so
// that the state can do one time only prep for its first loop.
bool entryFlag = true;

//****** Menu String Arrays **********
// Racer list
// Because this is an array of different length character arrays
// there is not easy way to determine the number of elements,
// so we use a constant to set the length.
// This must be maintained manually to match Racers[] actual content below
byte const racerCount = 7;
// For 7-seg displays there are no W's, M's, X's, K's, or V's
const char* Racers[racerCount] = {
  "Lucien", "ZOE", "Elise", "John", "Angie", "Uncle BadAss", "5318008"
};
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
  "",
  "D|Countdown:    Sec"
};
const char* ResultsText[4] = {
  "RESULTS: C|Overall",
  "",
  "",
  ""
};
// additional text strings used in multiple places
const char* Start = {"Start"};


// This function accepts in a millisecond value (msIN) and converts it into clock time.
// Because we have so many variables and want to save memory,
// we pass the clock time variables in by reference (using &).
// This means this function will be updating those variables from the higher scope directly.
void SplitTime(unsigned long msIN, unsigned long &ulHour, unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulDec, unsigned long &ulCent, unsigned long &ulMill) {
  // Calculate HH:MM:SS from millisecond count
  ulMill = msIN % 1000;
  ulCent = msIN % 1000 / 10;
  ulDec = msIN % 1000 / 100;
  ulSec = msIN / 1000;
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
  for (byte i = 0; i < fastLapsQSize; i++) {
    r1FastestLaps[i][0] = 0;
    r1FastestLaps[i][1] = 255;
    r1FastestTimes[i] = 999999;
    r2FastestLaps[i][0] = 0;
    r2FastestLaps[i][1] = 255;
    r2FastestTimes[i] = 999999;
  }
  for (byte i = 0; i < lapMillisQSize; i++) {
    r1LastXMillis[i] = 0;
    r2LastXMillis[i] = 0;
  }
}

// Used to initialize results, top fastest, lap array to high numbers.
// A lap number of 0 marks it is a dummy lap.
void InitializeResultsArrays(){
  for (byte i = 0; i < fastLapsQSize * 2; i++) {
    topFastestLaps[i][0] = 0;
    topFastestLaps[i][1] = 255;
    topFastestTimes[i] = 999999;
  }
}

// This function enables port register change interrupts on given pin
// can be used for pins among A0-A6
void pciSetup(byte pin) {
  // Enable interrupts on pin
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));
  // Clear any outstanding interrupt
  PCIFR  |= bit (digitalPinToPCICRbit(pin));
  // Enable interrupt for the group
  PCICR  |= bit (digitalPinToPCICRbit(pin));
}
// Used to disable port register interrupt on given pin
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
  // Lane 1 is on pin A0 which uses the first bit of the register, idx 0, of PINC Byte
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
  // Lane 2 is on pin A1 which uses the 2nd bit of the register, idx 1, of PINC Byte
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
  // Pause is on pin A2 which uses the 3rd bit of the register, idx 2, of PINC Byte
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
//     the integer 1 = 0x00000001, if we left shift 3 places we get 0x00001000
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
byte IndexList(int curIdx, int listLength, bool cycleUp = true) {
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
// the digits outside of the width (the largest place digits) will not be printed.
void PrintNumbers(const unsigned long numberIN, const byte width, const byte endPosIdx, displays display, bool leadingZs = true, const byte line = 0, bool endWithDecimal = false){
  // we take 1 away from width because the endposition is inclusive in the width count
  byte cursorStartPos = endPosIdx - (width - 1);
  byte cursorEndPos = endPosIdx;
  byte digitValue;
  // variable to track present digit by its place value
  byte placeValue = width - 1;
  switch (display) {
    // for printing to the main LCD
    case lcdDisp: {
      // written to screen from max place value to min defined by width and end position
      for (byte displayIdx = cursorStartPos; displayIdx <= cursorEndPos; displayIdx++) {
        lcd.setCursor(displayIdx, line);
        // print current place value's digit
        digitValue = numberIN / ipow(10, placeValue) % 10;
        if (!leadingZs && digitValue == 0)
          lcd.print(" ");
        else {
          lcd.print(digitValue);
            // once we get non-zero digit then we want all zeros
          leadingZs = true; 
        }
        placeValue--;
      }
      if (endWithDecimal) lcd.print(".");
    }
    break;
    // for printing to LED bar displays
    case led1Disp: case led2Disp: {
      for (byte displayIdx = cursorStartPos; displayIdx <= cursorEndPos; displayIdx++) {
        // print current place value's digit
        digitValue = numberIN / ipow(10, placeValue) % 10;
        if(!leadingZs && digitValue == 0){
          // skip this digit
        }
        else {
          lc.setDigit(
            display - 1, 
            (LED_DIGITS - 1) - displayIdx, 
            digitValue,
            endWithDecimal && displayIdx == cursorEndPos);
            // once we get non-zero digit then we want all zeros
            leadingZs = true;
        }
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
void PrintClock(ulong timeMillis, byte clockEndPos, clockWidth printWidth, displays display, byte line = 0, bool leadingZs = false) {
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
        PrintNumbers(ulHour, 2, hourEndPos, display, leadingZs, line);
        lcd.print(":");
      }
      break;
      case led1Disp: led2Disp: {
        PrintNumbers(ulHour, 2, clockEndPos - 5, display, leadingZs, 0, true);
      }
      break;
      default:
      break;
    } // END of display switch
    // All non-leading parts of the clock time should have leading zeros
    leadingZs = true;
    printWidth = M;
  }

  if (printWidth == M) {
    // M 00:00.0
    switch (display){
      case lcdDisp: {
        PrintNumbers(ulMin, 2, hourEndPos + 3, display, leadingZs, line);
        lcd.print(":");
      }
      break;
      case led1Disp: case led2Disp: {
        PrintNumbers(ulMin, 2, clockEndPos - 3, display, leadingZs, 0, true);
      }
      break;
      default:
      break;
    } // END of display switch
    // All non-leading parts of the clock time should have leading zeros
    leadingZs = true;
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
        PrintNumbers(ulSec, 2, hourEndPos + 6, display, leadingZs, line, true);
        lcd.print(decimalSec);
      } // END lcdDisp case
      break;
      case led1Disp: case led2Disp: {
        PrintNumbers(ulSec, 2, clockEndPos - decimalWidth, display, leadingZs, 0, true);
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
  // Even if text is right justified, available space filled starting with 1st character.
  // need to track the character index seperately from display digit position
  byte const textLength = strlen(textToWrite);
  // we take 1 away from width because the endposition is inclusive in the width count
  byte cursorStartPos = writeSpaceEndPos - (width - 1);
  byte cursorEndPos = writeSpaceEndPos;
  // adjust start and end positions based on text length and available space
  if (textLength < width && rightJust) cursorStartPos = cursorEndPos - (textLength - 1);
  if (textLength < width && !rightJust) cursorEndPos = cursorEndPos - (width - textLength);
  // because the writing position index may not match the text char index
  // we nust track them seperately.
  byte textIndex = 0;
  switch (display) {
    // For writing to LCD display
    case lcdDisp: {
      if (clear) lcdClearLine(line, writeSpaceEndPos - width, writeSpaceEndPos);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        lcd.setCursor(i, line);
        lcd.print(textToWrite[textIndex]);
        textIndex++;
      }
    } // END LCD case
    break;
    // For writing to LED display
    // Note that 7-seg cannot display 'M's, 'X's, 'W'x, or 'V's
    case led1Disp: case led2Disp: {
      if (clear) lc.clearDisplay(display - 1);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        // The digit position of the LED bars is right to left so we flip the requested index
        lc.setChar(display - 1, (LED_DIGITS-1) - i, textToWrite[textIndex], false);
        textIndex++;
      }
    } // END LED cases
    break;
    default:
    break;
  } // END display switch
}


// This function compares the input race time with current fastest list.
// If the new lap time is faster than any existing time, it takes its place,
// pushing the subsequent times down by 1, dropping the last time off the list.
void UpdateFastestLap(unsigned long fastestTimes[], unsigned int fastestLaps[][2], const int lap, const unsigned long newLapTime, const byte racer, const byte arrayLength){
  Serial.println("ENTERED FASTEST");
  for (byte i = 0; i < arrayLength; i++){
        // Serial.print("Fastest For i = ");
        // Serial.println(i);
        // Serial.println(fastestTimes[i][1]);
        // Serial.println(newLapTime);
    // Starting from beginning of list, compare new time with existing times.
    // If new lap time is faster, hold its place, and shift displaced, and remaining times down.
    if (fastestTimes[i] > newLapTime) {
      // Starting from the end of list, replace rows with previous row,
      // until we reach row of the bested time to be replaced.
      for (byte j = arrayLength - 1; j > i; j--){
        fastestTimes[j] = fastestTimes[j-1];
        fastestTimes[j] = fastestTimes[j-1];
        fastestLaps[j][0] = fastestLaps[j-1][0];
        fastestLaps[j][1] = fastestLaps[j-1][1];
      }
      // Then replace old, bested lap time, with new, faster lap and time.
      fastestTimes[i] = newLapTime;
      fastestLaps[i][0] = lap;
      fastestLaps[i][1] = racer;
      for (int k = 0; k < arrayLength; k++){
        Serial.println("UpdateFastest End");
        Serial.print(fastestTimes[k]);
        Serial.print(" : ");
        Serial.println(fastestLaps[k][0]);
      }
      // exit function, no need to continue once found and shift made
      return;
    } // END if (fastestTimes) block
  } // END fastest lap array for loop
}


void UpdateResults(unsigned long fastestTimes[], unsigned int fastestLaps[][2], int arraySize) {
  for (byte i = 0; i < 3; i++){
    // ignore if lap = 0 which means it's a dummy lap
    if (fastestLaps[resultsMenuIdx + i][0] > 0) {
      // print rank of time, which is index + 1
      PrintNumbers(resultsMenuIdx + i + 1, 2, 1, lcdDisp, false, i + 1, false);
      // then print lap
      PrintNumbers(fastestLaps[resultsMenuIdx + i][0], 3, 5, lcdDisp, true, i + 1, false);
      // Print the lap time
      // if the laptime exceeds 60 then just print '60+'
      if (fastestTimes[resultsMenuIdx + i] > 60000) {
        PrintText("60+", lcdDisp, 10, 3, false, i + 1, false);
      } else {
        PrintClock(fastestTimes[resultsMenuIdx + i], 12, Sm, lcdDisp, i + 1);
      }
      // clear racer name space of any previous characters
      lcdClearLine(1 + i, 13);
      // then print racer
      PrintText(Racers[fastestLaps[resultsMenuIdx + i][1]], lcdDisp, 19, 7, true, i + 1, false);
    } else {
      // we want to set the index back one so it doesn't keep scrolling in empty space
      resultsMenuIdx = resultsMenuIdx -1;
    }
  }
}

void UpdateResultsMenu() {
  switch(resultsSubMenu){
    case TopResults:  // update the results menu title
      lcdClearLine(0);
      PrintText("C | TOP RESULTS", lcdDisp, 14, 15, false, 0);
      UpdateResults(topFastestTimes, topFastestLaps, fastLapsQSize * 2);
    break;
    case Racer1Results:
      lcdClearLine(0);
      PrintText("C | RACER 1 RESULTS", lcdDisp, 18, 19, false, 0);
      UpdateResults(r1FastestTimes, r1FastestLaps, fastLapsQSize);
    break;
    case Racer2Results:
      lcdClearLine(0);
      PrintText("C | RACER 2 RESULTS", lcdDisp, 18, 19, false, 0);
      UpdateResults(r2FastestTimes, r2FastestLaps, fastLapsQSize);
    break;
  }
}


void ResetRace(){
  r1LapCount = 0;
  r2LapCount = 0;
  lane1LapFlash = 0;
  lane2LapFlash = 0;
  lane1State = Off;
  lane2State = Off;
  InitializeRacerArrays();
  InitializeResultsArrays();
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

  // make sure all race variables are in initial condition
  ResetRace();

  // Initialize racetime millisecond to default raceSetTime
  raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
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
    // and using that keypad input to navigate the menu tree and adjust settings.
    case Menu:{
      // Serial.println("entering Menu STATE");
      char key = keypad.getKey();
      // Only if a press is detected or it is the first loop of a new state
      // do we bother to evaluate anything.
      if (key || entryFlag) {
        Beep();
        // Clear the lap log interrupts on initial entry into menu state just to make sure,
        // but DON'T clear entry flag here, it is used at the menu level in the menu state.
        // This means these interrupts get disabled on every new menu, but keeps things simpler.
        if (entryFlag) {
          clearPCI(lane1Pin);
          clearPCI(lane2Pin);
        }
        // All of the menus and sub-menus should been flattened to this single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree.
          // Within each menu case is another switch for each available key input.
          case MainMenu:
            if (entryFlag) {
              lcdUpdateMenu(MainText);
              entryFlag = false;
            }
            // depending on seleted option, change menu state accordingly and set entry flag
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
              PrintNumbers(raceSetTime[1], 2, 11, lcdDisp, true, 3);
              // draw current seconds setting
              PrintNumbers(raceSetTime[0], 2, 14, lcdDisp, true, 3);
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
                raceSetTime[1] = lcdEnterNumber(2, 60, 3, 10);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
                break;
              case 'C':
                // change seconds
                raceSetTime[0] = lcdEnterNumber(2, 59, 3, 13);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
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
              lcd.setCursor(nameCursorPos, 1);
              lcd.print(Racers[racer1]);
              // write current racer2's name to screen
              lcd.setCursor(nameCursorPos, 3);
              lcd.print(Racers[racer2]);
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down racer list and update Racer1 name
              case 'A': case 'B':{
                // clear line to remove extra ch from long names replaced by shorter ones
                lcdClearLine(1, nameCursorPos);
                lcd.setCursor(nameCursorPos, 1);
                racer1 = IndexList(racer1, racerCount, key == 'A');
                lcd.print(Racers[racer1]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replaced by shorter ones
                lcdClearLine(3, nameCursorPos);
                lcd.setCursor(nameCursorPos, 3);
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
              PrintNumbers(raceSetTime[1], 2, 16, lcdDisp, true, 1);
              // add current race seconds setting to timed race screen
              PrintNumbers(raceSetTime[0], 2, 19, lcdDisp, true, 1);
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
                // else {raceType = Pole; countingDown = false;};
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
              UpdateResultsMenu();
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down the lap list
              case 'A': case 'B':{
                byte arrayMax;
                // if resultsSubMenu is 'Top', then it equals 0 and is false
                if (resultsSubMenu) arrayMax = fastLapsQSize;
                else arrayMax = fastLapsQSize * 2;
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(1);
                lcdClearLine(2);
                lcdClearLine(3);
                if (key == 'A' && resultsMenuIdx > 0) resultsMenuIdx--;
                // we subtract 3 because there are two rows printed after tracked index
                if (key == 'B' && resultsMenuIdx < arrayMax-3) resultsMenuIdx++;
                UpdateResultsMenu();
                Serial.println("result idx");
                Serial.println(resultsMenuIdx);
              }
              break;
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                if (resultsSubMenu == TopResults) resultsSubMenu = Racer1Results;
                else if (resultsSubMenu == Racer1Results) resultsSubMenu = Racer2Results;
                else if (resultsSubMenu == Racer2Results) resultsSubMenu = TopResults;
                // reset the result index to 0
                resultsMenuIdx = 0;
                UpdateResultsMenu();
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
        // preStartCountDown is in seconds so we convert to millis
        raceCurTime = preStartCountDown * 1000;
        lastTickMillis = curMillis;
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Your Race Starts in:");
        PrintClock(raceCurTime, PRESTART_CLK_POS, Sd, lcdDisp, 2);
        PrintText(Racers[racer1], led1Disp, 7, 8, true);
        PrintText(Racers[racer2], led2Disp, 7, 8, false);
        // reset any race variables to initial values
        ResetRace();
        entryFlag = false;
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
          if (countingDown) raceCurTime = ClockToMillis(0, raceSetTime[1], raceSetTime[0]) - raceSetTimeMs;
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
              UpdateFastestLap(r1FastestTimes, r1FastestLaps, r1LapCount - 1, r1LapTimeToLog, racer1, fastLapsQSize);
              UpdateFastestLap(topFastestTimes, topFastestLaps, r1LapCount - 1, r1LapTimeToLog, racer1, 2 * fastLapsQSize);
              // UpdateFastestLap(topFastestLaps, r1LapCount - 1, r1LapTimeToLog);
              lc.clearDisplay( led1Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r1LapCount - 1, 3, 2, led1Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r1LapTimeToLog, 7, Sc, led1Disp);
              // update racer's current lap total on lcd
              PrintNumbers(r1LapCount - 1, 3, 11, lcdDisp, true, 2);
              // update racer's current best lap time to lcd
              PrintClock(r1FastestTimes[0], 19, Sm, lcdDisp, 2);
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
              // if over 60 seconds use minutes format
              if (r1CurLapTime > 60000){
                PrintClock(r1CurLapTime, 7, M, led1Disp);
              } else {
                PrintClock(r1CurLapTime, 7, Sc, led1Disp, 0, true);
              }
            }
          }

          if (lane2LapFlash > 0) {
            if (lane2LapFlash == 1) {
              unsigned long r2LapTimeToLog;
              r2LapTimeToLog = r2LastXMillis [(r2LapCount-1) % lapMillisQSize] - r2LastXMillis [(r2LapCount-2) % lapMillisQSize];
              // the current lap is the live lap so we need to subtract 1 from lapcount
              // UpdateFastestLap(r2FastestLaps, r2LapCount - 1, r2LapTimeToLog);
              // UpdateFastestLap(topFastestLaps, r2LapCount - 1, r2LapTimeToLog);
              UpdateFastestLap(r2FastestTimes, r2FastestLaps, r2LapCount - 1, r2LapTimeToLog, racer2, fastLapsQSize);
              UpdateFastestLap(topFastestTimes, topFastestLaps, r2LapCount - 1, r2LapTimeToLog, racer2, 2 * fastLapsQSize);
              lc.clearDisplay( led2Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r2LapCount - 1, 3, 2, led2Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r2LapTimeToLog, 7, Sc, led2Disp);
              // update racer's current lap total on lcd
              PrintNumbers(r2LapCount - 1, 3, 11, lcdDisp, true, 3);
              // update racer's current best lap time to lcd
              PrintClock(r2FastestTimes[0], 19, Sm, lcdDisp, 3);
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
              // if over 60 seconds use minutes format
              if (r2CurLapTime > 60000){
                PrintClock(r2CurLapTime, 7, M, led2Disp);
              } else {
                PrintClock(r2CurLapTime, 7, Sc, led2Disp, 0, true);
              }
            }
          }
          // Update LCD with absolute race time and racer lap logs
          PrintClock(raceCurTime, RACE_CLK_POS, M, lcdDisp, 0, true);
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

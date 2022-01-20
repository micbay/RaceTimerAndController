
// Enable if using RTTL type song/melody data for playing sounds
//-------------------
// library for playing RTTTL song types
#include <PlayRtttl.h>
// file of RTTTL song definition strings.
// Because these strings are stored in PROGMEM we must also include 'avr/pgmspace.h' to access them.
#include "RTTTL_songs.h"
//-------------------

// Enable if using Note-Lengths Array method of playing Arduino sounds
//-------------------
// Defines the note constants that make up a melodie's Notes[] array.
#include <pitches.h>
// File of songs/melodies defined using Note & Lengths array.
// Because these arrays are stored in PROGMEM we must also include 'avr/pgmspace.h' to access them.
#include "melodies_prog.h"
//-------------------

// Library to support storing/accessing constant variables in PROGMEM
#include <avr/pgmspace.h>

// The 'Wire' library is for I2C, and is included in the Arduino installation.
// Specific implemntation is determined by the board selected in Arduino IDE.
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						            // main hd44780 header file
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i/o class header for i2c expander backpack
// library for 7-seg LED Bars
#include <LedControl.h>

// Library to support 4 x 4 keypad
#include <Keypad.h>

// ********* AUDIO HARDWARE *********
// A3 is a built in Arduino identifier that is replaced by the preprocessor,
// with the physical pin number, of the Arduino hardware selected in the IDE.
const byte buzzPin1 = A3;


//***** Setting Up Lap Triggers and Pause-Stop button *********
const byte lane1Pin = PIN_A0;
const byte lane2Pin = PIN_A1;
const byte pauseStopPin = PIN_A2;
const byte ledPIN = 13;
int ledState = HIGH;

//***** Variables for LCD 4x20 Display **********
// This display communicates using I2C via the SCL and SDA pins,
// which are dedicated by the hardware and cannot be changed by software.
// For the Arduino Nano, pin A4 is used for SDA, pin A5 is used for SCL.
// Declare 'lcd' object representing display:
// Use class 'hd44780_I2Cexp' to control LCD using i2c i/o expander backpack (PCF8574 or MCP23008)
hd44780_I2Cexp lcd;
// Constants to set display size
const byte LCD_COLS = 20;
const byte LCD_ROWS = 4;
const byte RACE_CLK_POS = 13;
const byte PRESTART_CLK_POS = RACE_CLK_POS - 2;


// ***** 7-Seg 8-digit LED Bars *****
const byte PIN_TO_LED_DIN = 2;
const byte PIN_TO_LED_CS = 3;
const byte PIN_TO_LED_CLK = 4;
const byte LED_BAR_COUNT = 2; // # of attached max7219 controlled LED bars
const byte LED_DIGITS = 8;    // # of digits on each LED bar
// LedControl parameters (DataIn, CLK, CS/LOAD, Number of Max chips (ie 8-digit bars))
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
const byte defMin = 0;
const byte defSec = 30;
byte raceSetTime[2] = {defSec, defMin};
// raceSetTime in milliseconds, will be initialized in setup().
unsigned long raceSetTimeMs;
// pre-race countdown length in seconds, settable from menu
byte preStartCountDown = 5;
// Flag to tell race timer if race time is going up or going down as in a time limit race.
// Since the default race type is standard the default countingDown is false.
bool countingDown = false;

// If program is updated to accomodate more than 2 racers,
// these racer variables should become an array to accomodate any number 2+.
// For now, with just 2, it's easier to handle them as individual variables.
// racer#'s value is the index of Racers[] identifying the racer name.
// The value of each racer must be unique.
// Do not allow user to select same racer id for more than 1 lane.
byte racer1 = 0;
byte racer2 = 1;
// Variable to track current lap being timed.
// Note that the current lap count may often be 1 greater than the lap of interest.
int r1LapCount = 0;
int r2LapCount = 0;
// Fastest laps table col0 = lap#, col1 = laptime in ms
const byte fastLapsQSize = 10;
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
const byte lapMillisQSize = 5;
unsigned long r1LastXMillis [ lapMillisQSize ] = {};
unsigned long r2LastXMillis [ lapMillisQSize ] = {};
// flag to alert results menu whether the race data is garbage or not
bool raceDataExists = false;

// The millis() timestamp of the start of current program loop.
unsigned long curMillis;
// The millis() timestamp of the start of active race.
unsigned long raceStartMillis;
// Live elapsed race time in ms, or remaining time in preStart, or timed race.
unsigned long curRaceTime;
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
const byte laneCount = 2;
// Variable to map lane selection to menu text
// We add one to the lane count to account for 'All'
const char* laneText[laneCount + 1] = {"All", "1 Only", "2 Only"};
// lanes enabledLanes = [0, 6] is lanes 1 and 2;
// default of zero has the mask enabling all lanes
byte lanesEnabledIdx = 0;
// Array variable to hold lane state and property indexes
// col0 is the idx of the text array for the display, col1 is its enabled bit flag value
const byte lanesEnabled[3][2] = {
  {0, 3},  // All           0x11111111
  {1, 1},  // Lane 1 Only   0x00000001
  {2, 2}   // Land 2 Only   0x00000010
};
// we skip index 0 so that the lane # matches its laneEnabledMask index value
// col idx 0 is the byte mask representing the lane number.
// col idx 1 is the present state of that lane #
byte lanes[laneCount + 1][3] = {
  {0, Off, 255},          // Not used
  {1 << 0, Off, lane1Pin},  // Lane 1 properties (bit mask, lane state, pin #)
  {1 << 1, Off, lane2Pin}   // Lane 2 properties (bit mask, lane state, pin #)
};

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
// keep this an int so it can be signed. A negative is the trigger to loop index
int resultsMenuIdx = 0;
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
// Create variale to hold current game 'state' and menu page.
states state;
Menus currentMenu;
// This flag is used to indicate first entry into a state or menu so
// that the state can do one time only prep for its first loop.
bool entryFlag;

//****** Menu String Arrays **********
// Racer list
// Because this is an array of different length character arrays
// there is not easy way to determine the number of elements,
// so we use a constant to set the length.
// This must be maintained manually to match Racers[] actual content below
byte const racerCount = 9;
// For 7-seg displays there are no W's, M's, X's, K's, or V's
const char* Racers[racerCount] = {
  "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle 1", "Rat2020", "The OG", "5318008"
};
const char* victorySong[racerCount] = {
  starWarsImperialMarch, TakeOnMe, airWolfTheme, tmnt1, gameOfThrones, galaga, outrun, starWarsEnd, spyHunter
};
// byte nameCursorPos = 8;
// sets screen cursor position names on the racer select menu
byte nameEndPos = 19;

// *** STRING PROGMEM *************
// in this section we define our menu string constants to use program memory
// this frees up significant RAM. In this case, using progmem to replace
// these few const char* arrays, reduced RAM used by globals, by 10%.
// The character buffer needs to be as large as largest string to be used.
// Since our longest row of characters is on the LCD we use its row length.
char buffer[LCD_COLS];

const char Main0[] PROGMEM = "A| Select Racers";
const char Main1[] PROGMEM = "B| Change Settings";
const char Main2[] PROGMEM = "C| Select a Race";
const char Main3[] PROGMEM = "D| See Results";
const char* const MainText[4] PROGMEM = {
  Main0,
  Main1,
  Main2,
  Main3
};
// const char* MainText[4] = {
//   "A| Select Racers",
//   "B| Change Settings",
//   "C| Select a Race",
//   "D| See Results"
// };

const char Settings0[] PROGMEM = " A|Lap  B|Min  C|Sec";
const char Settings1[] PROGMEM = " D|Lanes:";
const char Settings2[] PROGMEM = "Num Laps:";
const char Settings3[] PROGMEM = "Racetime:   :";
const char* const SettingsText[4] PROGMEM = {
  Settings0,
  Settings1,
  Settings2,
  Settings3
};
// Main Menu's sub-Menus
// const char* SettingsText[4] = {
//   " A|Lap  B|Min  C|Sec",
//   " D|Lanes:",
//   "Num Laps:",
//   "Racetime:   :"
// };

const char SelectRacers0[] PROGMEM = "<--A            B-->";
const char SelectRacers1[] PROGMEM = "Racer1:";
const char SelectRacers2[] PROGMEM = "<--C            D-->";
const char SelectRacers3[] PROGMEM = "Racer2:";
const char* const SelectRacersText[4] PROGMEM = {
  SelectRacers0,
  SelectRacers1,
  SelectRacers2,
  SelectRacers3
};
// const char* SelectRacersText[4] = {
//   "<--A            B-->",
//   "Racer1:",
//   "<--C            D-->",
//   "Racer2:"
// };

const char SelectRace0[] PROGMEM = "A|First to     Laps";
const char SelectRace1[] PROGMEM = "B|Most Laps in   :";
const char SelectRace2[] PROGMEM = "";
const char SelectRace3[] PROGMEM = "D|Countdown:    Sec";
const char* const SelectRaceText[4] PROGMEM = {
  SelectRace0,
  SelectRace1,
  SelectRace2,
  SelectRace3
};
// const char* SelectRaceText[4] = {
//   "A|First to     Laps",
//   "B|Most Laps in   :",
//   "",
//   "D|Countdown:    Sec"
// };

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
  // NOTE: we must us the 'UL' designation or otherwise cast to unsigned long
  //       if we don't, then the multiplication will not be the exptected value.
  //       This is because the default type of the number is a signed int which
  //       in this case, results in 60 * 1000 is out of range for int.
  //       Even though the variable it is going into is a long, the right side
  //       remains an int until after the evaluation so we would get an overflow.       
  return (ulHour * 60UL * 60UL * 1000UL) +
         (ulMin * 60UL * 1000UL) +
         (ulSec * 1000UL);
}


void Beep() {
  tone(buzzPin1, 4000, 200);
}

// Used to set fastest lap array to high numbers that will be replaced on comparison.
// A lap number of 0, and racer id of 255, marks these as dummy laps.
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
// A lap number of 0, and racer id of 255, marks these as dummy laps.
void InitializeTopFastest(){
  for (byte i = 0; i < fastLapsQSize * 2; i++) {
    topFastestLaps[i][0] = 0;
    topFastestLaps[i][1] = 255;
    topFastestTimes[i] = 999999;
  }
}


// Update menu on main LCD with static base text for given menu screen
void lcdUpdateMenu(const char *curMenu[]){
  // For each string in the menu array, print it to the screen
  // Clear screen first, in case new text doesn't cover all old text
  lcd.clear();
  // Code for using menus in form of global variable, const char* arrays.
  // This is left as a comparison in syntax with the PROGMEM approach used
  // for (int i=0; i<4; i++){
  //   lcd.setCursor(0,i);
  //   lcd.print(curMenu[i]);
  // }
  // Code for using menus built from strings saved in PROGMEM.
  for (int i=0; i<4; i++){
    strcpy_P(buffer, (char*)pgm_read_word(&(curMenu[i])));
    lcd.setCursor(0,i);
    lcd.print(buffer);
  }
}


// Provides, looping, up or down indexing of items in list
// This function assumes the start of the list is at zero index
byte IndexList(byte curIdx, byte listLength, bool cycleUp, byte otherRacer) {
  byte newIndex = curIdx;
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
  // We can't allow two racers to have the same id,
  // so if the result is the other selected racer then index again.
  if (newIndex == otherRacer) {
    newIndex = IndexList(newIndex, listLength, cycleUp, otherRacer);
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


// Input time in ms; clockEndPos is the index of where the last digit of the clock should go.
// This index is always considered left to right, and starting with 0.
// The function will flip index internally if needed for a right to left indexed display.
// The MAX7219 LED bars are such displays where, internally, digit idx 0 is the far right digit.
// Using this function with LED bar, to have clock end at far right input clockEndPos 7, not 0.
void PrintClock(ulong timeMillis, byte clockEndPos, clockWidth printWidth, displays display, byte line = 0, bool leadingZs = false) {
  unsigned long ulHour;
  unsigned long ulMin;
  unsigned long ulSec;
  unsigned long ulDec;
  unsigned long ulCent;
  unsigned long ulMill;
  SplitTime(timeMillis, ulHour, ulMin, ulSec, ulDec, ulCent, ulMill);
  //Calculate and adjust start position affected by number of decimal seconds digits requested
  // We use the endPos of the hours as the start endPos of the whole clock print.
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

  // Using a series of if statements for printWidth instead of a switch because of how the
  // the cascading width variable falls through with options, it works better with ifs.
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
  /* This final if is probably not necessary if all clocks end in S as we already evaluate the same options at the beginning to set the value for decimalSec.
  Any adjustments to code that changes seconds, like adding a no decimal option should be made at that point.
  However, keeping the if sets up for the preferred next update which adds the option of no decimal S & decimal with M & H, ie: S, Md, Mc, Mm, Hd, Hc, Hm.
  If width starts with Seconds then there is the option to display .0d, .00c, or .000m
  */
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


// Clear screen on given line from start to end position, inclusive.
// The defalut is to clear the whole line.
void lcdClearLine(byte lineNumber, byte posStart = 0, byte posEnd = LCD_COLS - 1) {
  lcd.setCursor(posStart, lineNumber);
  for(byte n = posStart; n <= posEnd; n++){
    lcd.print(" ");
  }
}


// function to write pre-start final countdown digits to LEDs
void ledWriteDigits(byte digit) {
  for (int j=0; j < LED_BAR_COUNT; j++){
    for (int i=0; i < LED_DIGITS; i++){
      lc.setDigit(j, i, digit, false);
    }
  }
}


// This function prints the input text, to the indicated display, at the indicated position.
void PrintText(const char textToWrite[], const displays display, const byte writeSpaceEndPos, const byte width = LED_DIGITS, bool rightJust = false, const byte line = 0, bool clear = true) {
  // Even if text is right justified, available space is filled starting with 1st character.
  // Need to track the character index seperately from display digit position.
  byte const textLength = strlen(textToWrite);
  // We take 1 away from width because the endposition is inclusive in the width count.
  byte cursorStartPos = writeSpaceEndPos - (width - 1);
  byte cursorEndPos = writeSpaceEndPos;
  // Adjust start and end positions based on text length and available space.
  if (textLength < width && rightJust) cursorStartPos = cursorEndPos - (textLength - 1);
  if (textLength < width && !rightJust) cursorEndPos = cursorEndPos - (width - textLength);
  // Because the writing position index may not match the text char index
  // we nust track them seperately.
  byte textIndex = 0;
  switch (display) {
    // For writing to LCD display
    case lcdDisp: {
      // for clearing we want to clear the whole space so we don't use adjusted endPos.
      if (clear) lcdClearLine(line, cursorStartPos, writeSpaceEndPos);
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
        // The digit position of the LED bars is right to left so we flip the requested index.
        lc.setChar(display - 1, (LED_DIGITS-1) - i, textToWrite[textIndex], false);
        textIndex++;
      }
    } // END LED cases
    break;
    default:
    break;
  } // END display switch
}

void DrawPreStartScreen(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Your Race Starts in:");
  PrintClock(curRaceTime, PRESTART_CLK_POS, Sd, lcdDisp, 2);
  // if lanes are not off, then update screen
  if(lanes[1][1] > 0) PrintText(Racers[racer1], led1Disp, 7, 8, false);
  if(lanes[2][1] > 0) PrintText(Racers[racer2], led2Disp, 7, 8, false);
}


// This function sets the lane state set by the user menu
void UpdateLaneState(byte enabledIndex, bool updateDisplay = false) {
  // Check all the physical lanes available established by the lanes[].
  // We start at 1 because the mask array skips the 1st index to match lane# with idx
  for (byte i = 1; i <= laneCount; i++){
    // Idx1 of lanesEnabled is the byte mask representing the enabled lanes.
    // Idx0 of a lanes[i] element is its byte mask, index 1 is its current enabled state. 
    if((lanes[i][0] & lanesEnabled[enabledIndex][1]) > 0) lanes[i][1] = StandBy;
    else lanes[i][1] = Off;
    if (updateDisplay){
      if (i == 1){
        PrintText(lanes[1][1] == 0 ? "DISABLED" : Racers[racer1], led1Disp, 7, 8);
      }
      if (i == 2){
        PrintText(lanes[2][1] == 0 ? "DISABLED" : Racers[racer2], led2Disp, 7, 8);
      }
    }
    // Serial.println("Lane State");
    // Serial.println(lanes[i][0]);
    // Serial.println(lanes[i][1]);
    // Serial.println(lanesEnabled[enabledIndex][1]);
  }
}

// This function goes through the lanes enabled by the settings
// and turns the interrupt pins on or off.
void SetLanePins(bool EnableInterrupt){
  // if the enabled state is truthy (Standby or Active) then turn pin on
  for (byte i = 1; i <= laneCount; i++){
    if(lanes[i][1] && EnableInterrupt) {
      pciSetup(lanes[i][2]);
      // Serial.println("pin enabled");
    }
    else if(lanes[i][1] && !EnableInterrupt){
      clearPCI(lanes[i][2]);
      // Serial.println("pin disabled");
    }
    // Serial.println("Set i, State then Pin");
    // Serial.println(i);
    // Serial.println(lanes[i][1]);
    // Serial.println(lanes[i][2]);
  }
}

// Because the software is single threaded any poling method used
// to detect lap triggers can only check one pin at a time.
// This creates a potential that simultaneous triggers would cause
// one racer's lap to be skipped.
// With the Arduino, however, we can use its port register interrupts
// to read the state of an entire block of pins simultaneously.
// This function enables port register change interrupts on given pin
// can be used for pins among A0-A6.
// As long as the sensor positive trigger duration is longer than the
// exectuion of the ISR(), then we should never miss a trigger.
// We'll use this to enable interrupts 
void pciSetup(byte pin) {
  // Enable interrupts on pin
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));
  // Clear any outstanding interrupt
  PCIFR  |= bit (digitalPinToPCICRbit(pin));
  // Enables the port for interrupts interrupt for the group
  PCICR  |= bit (digitalPinToPCICRbit(pin));
  // Serial.println("setPCI");
  // Serial.println(*digitalPinToPCMSK(pin), BIN);
  // Serial.println(bit (digitalPinToPCMSKbit(pin)), BIN);
}
// Used to disable port register interrupt on given pin
// when for periods we don't want to have lap sensor input.
void clearPCI(byte pin) {
  // Clear any outstanding interrupt
  PCIFR  |= bit (digitalPinToPCICRbit(pin));
  // Disable interrupts on pin,
  // using a logical & with the bitwise not (~) of the bitmask for the pin
  *digitalPinToPCMSK(pin) &= ~bit (digitalPinToPCMSKbit(pin));
  // Serial.println("clearPCI");
  // Serial.println(*digitalPinToPCMSK(pin), BIN);
  // Serial.println(bit (digitalPinToPCMSKbit(pin)), BIN);
  // Serial.println("Not");
  // Serial.println(~bit (digitalPinToPCMSKbit(pin)), BIN);
}

// debounceTime (ms), time within which not to accept additional signal input
const int debounceTime = 1000;

// ISR is a special Arduino Macro or routine that handles interrupts ISR(vector, attributes)
// PCINT1_vect handles pin change interrupt for the pin block A0-A5, represented in bit0-bit5
// The execution time of this function should be as fast as possible as
// interrupts are disabled while inside it.
ISR (PCINT1_vect) {
  unsigned long logMillis = millis();
  // We have setup the lap triggers and Pause button as inputs.
  // This means the pins have been set to HIGH, indicated by a 1 on its register bit.
  // When a button is pressed, or sensor triggered, it should bring the pin LOW.
  // A LOW pin is indicated by a 0 on its port register.

  // Lane 1 positive trigger PINC = 0xXXXXXXX0
  // Lane 1 is on pin A0 which uses the first bit of the register, idx 0, of PINC Byte
  if (!IsBitSet(PINC, 0)) {
    // if it's the first log, it's the intial cross of start line
    // if (r1LapCount == 0) {
    // Lanes are in standby before initial start or when returning from Pause.
    // The first trigger from standby should only set the lap start time
    if (lanes[1][1] == StandBy) {
      lanes[1][1] = Active;
      // If the very first lap, then log and index
      if(r1LapCount == 0) {
        r1LastXMillis [ r1LapCount ] = logMillis;
        r1LapStartMillis = logMillis;
        // only index the lapcount if initial start, not on pause restart
        r1LapCount++;
      } else {
        // If returning from Pause, we need to feed the new start time,
        // into the pevious lap index spot, and not index the current lapcount.
        r1LastXMillis [(r1LapCount - 1) % lapMillisQSize] = logMillis;
        r1LapStartMillis = logMillis;
      }
      Beep();
    } else if(lanes[1][1] == Active) {
      // Check that current sense time is outside of active debounce period, otherwise ignore it.
      if((logMillis - r1LastXMillis [(r1LapCount-1) % lapMillisQSize] ) > debounceTime){
        lane1LapFlash = 1;
        r1LastXMillis [r1LapCount % lapMillisQSize] = logMillis;
        r1LapStartMillis = logMillis;
        // Serial.println("R1 Lap Count 1++");
        // Serial.println(r1LapCount);
        r1LapCount++;
        Beep();
      }
    }
    // if not the first log or beyond debounce time then don't log the trigger
  }
  // Lane 2 positive trigger PINC = 0xXXXXXX0X
  // Lane 2 is on pin A1 which uses the 2nd bit of the register, idx 1, of PINC Byte
  if (!IsBitSet(PINC, 1)) {
    if (lanes[2][1] == StandBy) {
      lanes[2][1] = Active;
      // If the very first lap, then log and index
      if(r2LapCount == 0){
        r2LastXMillis [ r2LapCount ] = logMillis;
        r2LapStartMillis = logMillis;
        // only index the lapcount if initial start, not on pause restart
        r2LapCount++;
      } else {
        // If returning from Pause, we need to feed the new start time,
        // into the pevious lap index spot, and not index the current lapcount.
        r2LastXMillis [(r2LapCount - 1) % lapMillisQSize] = logMillis;
        r2LapStartMillis = logMillis;
      }
      Beep();
    } else if (lanes[2][1] == Active) {
      if((logMillis - r2LastXMillis [(r2LapCount-1) % lapMillisQSize] ) > debounceTime){
        lane2LapFlash = 1;
        r2LastXMillis [r2LapCount % lapMillisQSize] = logMillis;
        r2LapStartMillis = logMillis;
        // Serial.println("R2 Lap Count 1++");
        // Serial.println(r2LapCount);
        r2LapCount++;
        Beep();
      }
    }
  } // END of PINC, 1 aka lane/racer2 pin detect

  // Puase button positive trigger PINC = 0xXXXXX0XX
  // Pause is on pin A2 which uses the 3rd bit of the register, idx 2, of PINC Byte
  if (!IsBitSet(PINC, 2)) {
    Beep();
    // Because this is the paused state, it's not as important how long it takes to execute
    // If not within debounce time of previous trigger process
    if (logMillis - pauseDebounceMillis > debounceTime){
      pauseDebounceMillis = logMillis;
      switch(state){
        // if currently in a race, then put in Paused state
        case Race: {
          // immediately turn off the interrupts on the lap sensing pins
          state = Paused;
          entryFlag = true;
          // Turn off the lane interrupts
          SetLanePins(false);
          // Reset lanes to the default Enabled laneState, 'StandBy'.
          UpdateLaneState(lanesEnabledIdx);
        }
        break;
        // if already paused then return to race
        case Paused: {
          state = Race;
          // make sure entry flag is turned back off before re-entering race state
          entryFlag = true;
          r1CurLapTime = 0;
          r2CurLapTime = 0;
          r1LapStartMillis = logMillis;
          r2LapStartMillis = logMillis;
          // Renable pins on active lanes
          SetLanePins(true);
          // pciSetup(lane1Pin);
          // pciSetup(lane2Pin);
        }
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
// In a Byte, position is from right to left, the far right bit is considered bit 1 at idx0.
// EX. If function parameters are pos = 3 and b = 0x11111000
//     the integer 1 = 0x00000001, if we left shift pos=3 places we get 0x00001000
//     thus 0x11111000 & (0x00001000) = 0x00001000
// Which is not zero, so the ex returns true, the indicated bit of the input Byte is set.
bool IsBitSet(byte b, byte pos) {
   return (b & (1 << pos)) != 0;
}


// This function compares the input race time with current fastest list.
// If the new lap time is faster than any existing time, it takes its place,
// pushing the subsequent times down by 1, dropping the last time off the list.
void UpdateFastestLap(unsigned long fastestTimes[], unsigned int fastestLaps[][2], const int lap, const unsigned long newLapTime, const byte racer, const byte arrayLength){
  Serial.println("ENTERED FASTEST");
  for (byte i = 0; i < arrayLength; i++){
    // Starting from beginning of list, compare new time with existing times.
    // If new lap time is faster, hold its place, and shift bested, and remaining times down.
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
      // for (int k = 0; k < arrayLength; k++){
      //   Serial.println("UpdateFastest End");
      //   Serial.print(fastestTimes[k]);
      //   Serial.print(" : ");
      //   Serial.println(fastestLaps[k][0]);
      // }
      // exit function, no need to continue once found and shift made
      return;
    } // END if (fastestTimes) block
  } // END fastest lap array for loop
}


// This function combines all the racer's fastest laps and list the top fastest overall.
void CompileTopFastest(){
  // first we need to re-initialize or else we will be duplicating previously added laps
  InitializeTopFastest();
  // add racer 1 fastest laps to top lap list
  for (byte i = 0; i < fastLapsQSize; i++) {
    UpdateFastestLap(topFastestTimes, topFastestLaps, r1FastestLaps[i][0], r1FastestTimes[i], racer1, 2 * fastLapsQSize);
  }
  // add racer 2 fastest laps to top lap list
  for (byte j = 0; j < fastLapsQSize; j++) {
    UpdateFastestLap(topFastestTimes, topFastestLaps, r2FastestLaps[j][0], r2FastestTimes[j], racer2, 2 * fastLapsQSize);
  }
}

// Prints given results array to lcd display
void lcdPrintResults(unsigned long fastestTimes[], unsigned int fastestLaps[][2], int lapCount) {
  for (byte i = 0; i < 3; i++){
    // ignore if lap = 0 which means it's a dummy lap or if index greater than lap count
    if (fastestLaps[resultsMenuIdx + i][0] > 0 && i < lapCount) {
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
      resultsMenuIdx = (resultsMenuIdx -1) >= 0 ? resultsMenuIdx - 1 : 0;
      // if this condition exists, we want to end the loop to avoid doubles
      break;
    }
  }
}

void UpdateResultsMenu() {
  switch(resultsSubMenu){
    case TopResults: {
      lcd.clear();
      PrintText("C | TOP RESULTS", lcdDisp, 14, 15, false, 0);
      lcdPrintResults(topFastestTimes, topFastestLaps, r1LapCount + r2LapCount);
    }
    break;
    case Racer1Results: {
      lcd.clear();
      PrintText("C | RACER 1 RESULTS", lcdDisp, 18, 19, false, 0);
      lcdPrintResults(r1FastestTimes, r1FastestLaps, r1LapCount);
    }
    break;
    case Racer2Results: {
      lcd.clear();
      PrintText("C | RACER 2 RESULTS", lcdDisp, 18, 19, false, 0);
      lcdPrintResults(r2FastestTimes, r2FastestLaps, r2LapCount);
    }
    break;
  }
}


void ResetRace(){
  r1LapCount = 0;
  r2LapCount = 0;
  lane1LapFlash = 0;
  lane2LapFlash = 0;
  UpdateLaneState(lanesEnabledIdx);
  InitializeRacerArrays();
}


// *** This section for using Note and Lengths arrays for songs
// Globals for holding the current melody data references.
int *playingNotes;
int *playingLengths;                                                            
byte playingTempoBPM = 135;
int playingCount = 0;
// flag to indicate to the main program loop whether a melody is in process
// so it should execute the 'PlayNote()' function with the current melody parameters.
bool melodyPlaying = false;
// Holds the timestamp of last tone played so timing of next note in melody can be determined
unsigned long lastNoteMillis = 0;
// index of the current note to play of 'playing...' song.
int melodyIndex = 0;
// time in ms between beginning of last note and when next note should be played.
int noteDelay = 0;

// Function to play the current note index of a melody using 'tone()'.
// We want to pass all the variables instead of depending on their globality.
int PlayNote(int *songNotes, int *songLengths, int curNoteIdx, byte tempoBPM){
  int noteDuration;
  int noteLength = pgm_read_word(&songLengths[curNoteIdx]);
  // If tempo = 0 then use note length directly as ms duration
  if(tempoBPM == 0){
    noteDuration = noteLength;
  } else {
    // Otherwise calculate duration in ms from bpm:
    // (60,000ms/min)/Xbpm * 4beats/note * 1/notelength
    // Make sure equation has a decimal or result will be incorrect integer math.
    if (noteLength > 0){
      noteDuration = (60000 / tempoBPM) * 4 * (1.0 / noteLength);
    } else {
      // If note length is negative, then it's dotted so add extra half length.
      noteDuration = 1.5 * (60000 / tempoBPM) * 4 * (1 / abs(noteLength));
    }
  }
  // Record millisecond timestamp at start of new note.
  lastNoteMillis = millis();
  // Play note
  tone(buzzPin1, pgm_read_word(&songNotes[curNoteIdx]), noteDuration);
  melodyIndex++;
  // If we have reached the end of the melody array then
  // flip playing flag off and reset tracking variables for next melody.
  if(curNoteIdx == playingCount - 1){
    melodyPlaying = false;
    melodyIndex = 0;
    noteDelay = 0;
    playingCount = 0;
    noTone(buzzPin1);
  }
  // Buzzer notes have no transition time or strike impulse.
  // So when played as written, each note sounds unaturally run together.
  // Making the time between notes slightly longer than the note will
  // create a small break, giving the melody a more natural sound.
  // Factoring + 5-10% seems to be enough.
  return noteDuration * 1.1;
  //***this will make the true tempo slightly slower.
  // So increase the song's set tempo from the music sheet to accomodate.
}


// *********************************************
// ***************** SETUP *********************
// Initialize hardware and establish software initial state
void setup(){
  // --- SETUP SERIAL ------------------------
  // NOTE: Serial port is only used in debugging at the moment.
  // The hile loop waits for the serial connection to be established
  // before moving on so we don't miss anything. If the Arduion seems to
  // 'hang', this may be the culprit if there is a connection issue.
  // Open port and wait for connection before proceeding.
  Serial.begin(9600);
  while(!Serial);
  // --- SETUP LCD DIPSLAY -----------------------------
  // Initialize LCD with begin() which will return zero on success.
  // Non-zero failure status codes are defined in <hd44780.h>
	int status = lcd.begin(LCD_COLS, LCD_ROWS);
  // If display initialization fails, trigger onboard error LED if exists.
	if(status) hd44780::fatalError(status);
  // Make sure display has no residual data and starts in a blank state
  lcd.clear();

  // --- SETUP LED 7-SEG, 8-DIGIT MAX7219 LED BARS ------
  //we have already set the number of devices when we created the 'lc' object
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  for(int deviceID = 0; deviceID < devices; deviceID++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(deviceID, false);
    // intensity range from 0-15, higher = brighter
    lc.setIntensity(deviceID, 8);
    // Blank the LED digits
    lc.clearDisplay(deviceID);
  }

  // --- SETUP LAP TRIGGERS AND BUTTONS ----------------
  // roughly equivalent to digitalWrite(lane1Pin, HIGH)
  pinMode(lane1Pin, INPUT_PULLUP);
  pinMode(lane2Pin, INPUT_PULLUP);
  pinMode(pauseStopPin, INPUT_PULLUP);
  // Setup an indicator LED
  pinMode(ledPIN, OUTPUT); 
  digitalWrite(ledPIN, ledState);
  // Reset all race variables to initial condition.
  ResetRace();
  // Initialize racetime millisecond to default raceSetTime.
  raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
  // Set initial state to Menu and initial menu to MainMenu, turn initial entry flag on.
  state = Menu;
  currentMenu = MainMenu;
  entryFlag = true;
  // Set lanes to their default parameters stored in the array, lanes[].
  // This will set any enabled lanes to 'StandBy' state, and any disabled lanes to 'Off'
  UpdateLaneState(lanesEnabledIdx);
  // Update LEDs with the active default racer names.
  PrintText(Racers[racer1], led1Disp, 7, 8);
  PrintText(Racers[racer2], led2Disp, 7, 8);
  // Serial.println(currentMenu);
  // melodyPlaying = true;

  // playingNotes = takeOnMeNotes;
  // playingLengths = takeOnMeLengths;
  // playingCount = takeOnMeCount;
  // playingTempoBPM = takeOnMeTempo;

  playingNotes = marioUnderworldNotes;
  playingLengths = marioUnderworldLengths;
  playingCount = marioUnderworldCount;
  playingTempoBPM = marioUnderworldTempo;
  
  // Initiates a non-blocking play of melody using 'PlayRtttl' library
  // startPlayRtttlPGM(buzzPin1, spyHunter);
  // startPlayRtttlPGM(buzzPin1, reveille);
  // startPlayRtttlPGM(buzzPin1, gameOfThrones);
}

void loop(){
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
  // Serial.println("R2Lap count MAIN");
  // Serial.println(r2LapCount);
  updatePlayRtttl();
  if(melodyPlaying){
    if(millis() - lastNoteMillis >= noteDelay){
    // stop generation of square wave triggered by 'tone()'.
    // This is not required as all tones should stop at end of duration.
    // However, we do it just to make sure it 
      // noTone(buzzPin1);
      noteDelay = PlayNote(playingNotes, playingLengths, melodyIndex, playingTempoBPM);
    }
  } else {
    // noTone(buzzPin1);
  }
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
          SetLanePins(false);
          clearPCI(pauseStopPin);
        }
        // All of the menus and sub-menus should been flattened to this single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree.
          // Within each menu case is another switch for each available key input.
          case MainMenu:{
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
                resultsSubMenu = TopResults;
                entryFlag = true;
              default:
                break;
            } // END key switch
            break;
          } // END MainMenu case

          case SettingsMenu: {
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
              lcd.print(laneText[ lanesEnabled [ lanesEnabledIdx ][0]]);
              entryFlag = false;
            }
            switch (key) {
              case 'A':{
                // change lap count
                raceLaps = lcdEnterNumber(3, 999, 2, 10);
              }
              break;
              case 'B':{
                // Change minutes
                raceSetTime[1] = lcdEnterNumber(2, 60, 3, 10);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
              }
              break;
              case 'C':{
                // change seconds
                raceSetTime[0] = lcdEnterNumber(2, 59, 3, 13);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
              }
              break;
              case 'D':{
                // cycles through active lane options
                // The lanes enabled array holds the bit mask of enabled pins in col idx 1
                lanesEnabledIdx = (lanesEnabledIdx + 1) % ((sizeof lanesEnabled/sizeof lanesEnabled[0]));
                PrintText(laneText[lanesEnabled[lanesEnabledIdx][0]], lcdDisp, 19, 10, false, 1);
                // Update lane parameters per new setting
                UpdateLaneState(lanesEnabledIdx, true);
              }
              break;
              case '*':{
                // return to main menu
                currentMenu = MainMenu;
                entryFlag = true;
              }
              break;
              default:
              break;
            } // END key switch
            break;
          } // END Settings Menu case

          case SelectRacersMenu: {
            if (entryFlag) {
              // write base, static text to screen
              lcdUpdateMenu(SelectRacersText);
              // Write current racer's names to screen.
              // Check if lane is enabled first.
              PrintText(lanes[1][1] == 0 ? "DISABLED" : Racers[racer1], lcdDisp, nameEndPos, 12, false, 1);
              PrintText(lanes[2][1] == 0 ? "DISABLED" : Racers[racer2], lcdDisp, nameEndPos, 12, false, 3);
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down racer list and update Racer1 name
              case 'A': case 'B':{
                // if lane is enabled it wil be > 0
                if(lanes[1][1]){
                  // clear line to remove extra ch from long names replaced by shorter ones
                  racer1 = IndexList(racer1, racerCount, key == 'A', racer2);
                  PrintText(Racers[racer1], lcdDisp, nameEndPos, 12, false, 1);
                  PrintText(Racers[racer1], led1Disp, 7, 8);
                  // Play racers victory song
                  startPlayRtttlPGM(buzzPin1, victorySong[racer1]);
                }
              }
              break;
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // if lane is enabled it wil be > 0
                if(lanes[2][1]){
                  // clear line to remove extra ch from long names replaced by shorter ones
                  racer2 = IndexList(racer2, racerCount, key == 'D', racer1);
                  PrintText(Racers[racer2], lcdDisp, nameEndPos, 12, false, 3);
                  PrintText(Racers[racer2], led2Disp, 7, 8);
                  // Play racers victory song
                  startPlayRtttlPGM(buzzPin1, victorySong[racer2]);
                }
              }
              break;
              case '*':{
                // return to MainMenu
                currentMenu = MainMenu;
                entryFlag = true;
                stopPlayRtttl();
              }
              break;
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
              case 'A': case 'B': {
                if (key == 'A') {raceType = Standard; countingDown = false;}
                else if (key == 'B') {raceType = Timed; countingDown = true;}
                // else {raceType = Pole; countingDown = false;};
                state = Race;
                preStart = true;
                entryFlag = true;
              }
              break;
              case 'D': {
                // change how long the preStartCountDown before the race start lasts
                preStartCountDown = lcdEnterNumber(2, 30, 3, 13);
              }
              break;
              case '*': {
                // return to main menu
                currentMenu = MainMenu;
                entryFlag = true;
              }
              break;
              default:
              break;
            } // END of key switch
            break;
          } // END of SelectRace Menu Case

          case ResultsMenu: {
            if (entryFlag) {
              lcd.clear();
              if(raceDataExists){
                resultsSubMenu = TopResults;
                resultsMenuIdx = 0;
                lcd.print("-Compiling Results-");
                CompileTopFastest();
                UpdateResultsMenu();
              } else {
                lcd.print("-NO RACE DATA-");
              }
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down the lap list
              case 'A': case 'B':{
                // only deal with data if it exists,
                // otherwise garbage will be displayed on screen
                if(raceDataExists) {
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
                  if (key == 'B' && resultsMenuIdx < arrayMax-2) resultsMenuIdx++;
                  UpdateResultsMenu();
                }
              }
              break;
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // only deal with data if it exists,
                // otherwise garbage will be displayed on screen
                if(raceDataExists) {
                  // when changing fastest list, reset resultsMenuIdx to 0
                  resultsMenuIdx = 0;
                  // clear line to remove extra ch from long names replace by short ones
                  if (resultsSubMenu == TopResults) resultsSubMenu = Racer1Results;
                  else if (resultsSubMenu == Racer1Results) resultsSubMenu = Racer2Results;
                  else if (resultsSubMenu == Racer2Results) resultsSubMenu = TopResults;
                  UpdateResultsMenu();
                  // Serial.println("result idx");
                  // Serial.println(resultsMenuIdx);
                  // Serial.println(key);
                }
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
        curRaceTime = preStartCountDown * 1000;
        lastTickMillis = curMillis;
        DrawPreStartScreen();
        // reset any race variables to initial values
        ResetRace();
        entryFlag = false;
      }
      // initialize variables for race timing loop first cycle only
      if (entryFlag & !preStart) {
        curRaceTime = 0;
        lastTickMillis = curMillis;
        raceStartMillis = curMillis;
        entryFlag = false;
        // Write Live Race menu and racer names to displays
        lcd.clear();
        lcd.setCursor(8,1);
        lcd.print("Laps   Best");
        if(lanes[1][1] > 0){
          // Print racer's name to LCD
          PrintText(Racers[racer1], lcdDisp, 7, 8, false, 2);
          // update racer's current lap total on LCD
          PrintNumbers(r1LapCount == 0 ? 0 : r1LapCount - 1, 3, 11, lcdDisp, true, 2);
          // update racer's current best lap time to LCD
          if(r1LapCount > 0) PrintClock(r1FastestTimes[0], 19, Sm, lcdDisp, 2);
          PrintText(Start, led1Disp, 7, 8, true, 0, false);
        }
        if(lanes[2][1] > 0){
          // Print racer's name to LCD
          PrintText(Racers[racer2], lcdDisp, 7, 8, false, 3);
          // update racer's current lap total on lcd (0 if start, current if restart)
          PrintNumbers(r2LapCount == 0 ? 0 : r2LapCount - 1, 3, 11, lcdDisp, true, 3);
          // update racer's current best lap time to lcd
          if(r2LapCount > 0) PrintClock(r2FastestTimes[0], 19, Sm, lcdDisp, 3);
          PrintText(Start, led2Disp, 7, 8, true, 0, false);
        }
        // this act enables the racer to trigger first lap
        // Turn on interrupts for enabled lane pins
        SetLanePins(true);
        // enable race pause button interrupt
        pciSetup(pauseStopPin);
      }

      if (preStart) {
        if (curRaceTime > 0){
          // update LCD on each tick
          if (curMillis - lastTickMillis > displayTick){
            curRaceTime = curRaceTime - displayTick;
            PrintClock(curRaceTime, PRESTART_CLK_POS, Sc, lcdDisp, 2);
            lastTickMillis = curMillis;
          }
          // In last 3 seconds send countdown to LEDs
          if (curRaceTime < 2999 & (curRaceTime/1000 + 1) != ledCountdownTemp) {
            // we add 1 because it should change on start of the digit not end
            ledCountdownTemp = curRaceTime/1000 + 1;
            ledWriteDigits(ledCountdownTemp);
          }
        } else {
          // ++++ Set Flags for RACE START +++++++
          preStart = false;
          // reset ledCountdownTemp to default for next race
          ledCountdownTemp = 0;
          // reset entry flag to true so race timing variables can be initialized
          entryFlag = true;
          // Clear LEDs because we don't clear before drawing start
          // so the same function can be used to draw the restart after pause.
          lc.clearDisplay(0);
          lc.clearDisplay(1);
        }
      } else { // ********* LIVE RACE **********
        // Regardless of race type, we do these things
        // update current racetime
        // If racetype is timed then displayTick will be a negative number
        if (countingDown) {
          curRaceTime = raceSetTimeMs < (curMillis - raceStartMillis) ? 0 : raceSetTimeMs - (curMillis - raceStartMillis);
        } else {
          curRaceTime = curMillis - raceStartMillis;
        }
        // If lanestate is active then update current lap time
        if (lanes[1][1] == Active) r1CurLapTime = curMillis - r1LapStartMillis;
          else r1CurLapTime = 0;
        if (lanes[2][1] == Active) r2CurLapTime = curMillis - r2LapStartMillis;
          else r2CurLapTime = 0;
        // if tick has passed, then update displays
        if (curMillis - lastTickMillis >  displayTick){
          // A lap Flash is triggered by the completion of a lap, it's 'resting' value is 0
          if (lane1LapFlash > 0) {
            if (lane1LapFlash == 1) {
              raceDataExists = true;
              unsigned long r1LapTimeToLog;
              r1LapTimeToLog = r1LastXMillis [(r1LapCount-1) % lapMillisQSize] - r1LastXMillis [(r1LapCount-2) % lapMillisQSize];
              // during the intro to the lap flash cycle we also update all the racer lap records
              // we do this here to keep it interruptable and out of the lap trigger intrrupt funct.
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(r1FastestTimes, r1FastestLaps, r1LapCount - 1, r1LapTimeToLog, racer1, fastLapsQSize);
              // UpdateFastestLap(topFastestTimes, topFastestLaps, r1LapCount - 1, r1LapTimeToLog, racer1, 2 * fastLapsQSize);
              lc.clearDisplay( led1Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r1LapCount - 1, 3, 2, led1Disp);
              // print the lap time of just completed lap to right side of racer's LED
              if (r1LapTimeToLog > 60000){
                PrintClock(r1LapTimeToLog, 7, M, led1Disp);
              } else {
                PrintClock(r1LapTimeToLog, 7, Sm, led1Disp);
              }
              // update racer's current lap total on LCD
              PrintNumbers(r1LapCount - 1, 3, 11, lcdDisp, true, 2);
              // update racer's current best lap time to LCD
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
            if (lanes[1][1] == Active) {
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
              raceDataExists = true;
              unsigned long r2LapTimeToLog;
              r2LapTimeToLog = r2LastXMillis [(r2LapCount-1) % lapMillisQSize] - r2LastXMillis [(r2LapCount-2) % lapMillisQSize];
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(r2FastestTimes, r2FastestLaps, r2LapCount - 1, r2LapTimeToLog, racer2, fastLapsQSize);
              // UpdateFastestLap(topFastestTimes, topFastestLaps, r2LapCount - 1, r2LapTimeToLog, racer2, 2 * fastLapsQSize);
              lc.clearDisplay( led2Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(r2LapCount - 1, 3, 2, led2Disp);
              // print the lap time of just completed lap to right side of racer's LED
              if (r2LapTimeToLog > 60000){
                PrintClock(r2LapTimeToLog, 7, M, led2Disp);
              } else {
                PrintClock(r2LapTimeToLog, 7, Sm, led2Disp);
              }
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
            if (lanes[2][1] == Active) {
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
          PrintClock(curRaceTime, RACE_CLK_POS, M, lcdDisp, 0, true);
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
              SetLanePins(false);
              DetermineWinner();
              state = Menu;
              currentMenu = ResultsMenu;
              entryFlag = true;
              resultsMenuIdx = TopResults;
            }
            break;
          } // END standard race case
          case Timed:{
            // if time <= 0 then race is over and the most laps wins
            if (curRaceTime <= 0) {
              // turn off lap trigger interrupts to prevent any additional lap triggers
              SetLanePins(false);
              DetermineWinner();
              state = Menu;
              currentMenu = ResultsMenu;
              entryFlag = true;
              resultsMenuIdx = TopResults;
            }
          } // END Timed race case
          break;
          case Pole:{

            break;
          }
          default:
            break;
        } // END RaceType Switch

      } // END PreStart if-then-else
      break;
    } // END of Race state switch
    case Paused:{
      if(entryFlag){
        if(lanes[1][1]) PrintText("PAUSE", led1Disp, 7, 5, true, 0, false);
        if(lanes[2][1]) PrintText("PAUSE", led2Disp, 7, 5, true, 0, false);
      }
      // Serial.println("Entered Paused state");      
      char key = keypad.getKey();
      // only if a press is detected or it is the first loop of a new state
      // do we bother to evaluate anything
      if (key == '*') {
        state = Menu;
        currentMenu = ResultsMenu;
        entryFlag = true;
        resultsMenuIdx = TopResults;
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

// Returns racer index of winner.
// This function assumes only 2 racers, needs to be reworked if more racers added.
byte DetermineWinner() {
  byte first = 255;
  byte second = 255;
  // Adjust final lap count by 1 because current, unfinished, lap is not to be included.
  // This conditional ternary operator notatation states that if lapCount is 0 already,
  // then leave it zero, otherwise subtract 1.
  // Because '##LapCount' is used as an index we need to protect against negatives.
  r1LapCount = r1LapCount == 0 ? 0 : r1LapCount - 1;
  r2LapCount = r2LapCount == 0 ? 0 : r2LapCount - 1;
  // Serial.println("racer1 laps final");
  // Serial.println(r1LapCount);
  // Serial.println("racer2 laps final");
  // Serial.println(r2LapCount);
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
    if(lanes[1][1]) PrintText("A TIE", led1Disp, 7, 8, true);
    if(lanes[2][1]) PrintText("A TIE", led2Disp, 7, 8, true);
  } else if (first == 255) {
    // If first is never changed from default 255 then somehow an error occurred.
    if(lanes[1][1]) PrintText("ERROR", led1Disp, 7, 8);
    if(lanes[2][1]) PrintText("ERROR", led2Disp, 7, 8);
  } else {
    PrintText("1st", first == racer1 ? led1Disp:led2Disp, 2, 3, false);
    PrintText(Racers[first], first == racer1 ? led1Disp:led2Disp, 7, 4, true, 0, false);
    // Only if 'All' lanes are enabled, aka =3, aka 0x00000011, is there a 2nd place.
    if(lanesEnabled[lanesEnabledIdx][1] == 3) {
      PrintText("2nd", second == racer1 ? led1Disp:led2Disp, 2, 3, false);
      PrintText(Racers[second], second == racer1 ? led1Disp:led2Disp, 7, 4, true, 0, false);
    }
  }
  // Play theme song from winner's victorySong[] index
  startPlayRtttlPGM(buzzPin1, victorySong[first]);
  return first;
}


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

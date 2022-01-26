
// Enable if using RTTL type song/melody data for playing sounds
//-------------------
// library for playing RTTTL song types
#include <PlayRtttl.h>
// file of RTTTL song definition strings.
// Because these strings are stored in PROGMEM we must also include 'avr/pgmspace.h' to access them.
#include "RTTTL_songs.h"
//-------------------

// Enable if using Note-Lengths Array method of playing Arduino sounds
// //-------------------
// // Defines the note constants that make up a melodie's Notes[] array.
// #include <pitches.h>
// // File of songs/melodies defined using Note & Lengths array.
// // Because these arrays are stored in PROGMEM we must also include 'avr/pgmspace.h' to access them.
// #include "melodies_prog.h"
// //-------------------

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


// The # of physical lanes that will have a counter sensor
const byte laneCount = 4;

// ********* AUDIO HARDWARE *********
// A3 is a built in Arduino identifier that is replaced by the preprocessor,
// with the physical pin number, of the Arduino hardware selected in the IDE.
const byte buzzPin1 = 13;

// indicator LED, disable buzzer to use
// const byte ledPIN = 13;
// int ledState = HIGH;

//***** Setting Up Lap Triggers and Pause-Stop button *********
const byte lane1Pin = PIN_A0;
const byte lane2Pin = PIN_A1;
const byte lane3Pin = PIN_A2;
const byte lane4Pin = PIN_A3;

// A6 is an analog input only pin it must be read as analog.
// An external pullup resistor must be added to button wiring.
// This input is expected to be HIGH and go LOW when pressed.
const byte pauseStopPin = PIN_A6;

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
// When more than 2 MAX7219s are chained each additional chip needs direct power supply.
const byte LED_BAR_COUNT = 4; // # of attached max7219 controlled LED bars
const byte LED_DIGITS = 8;            // # of digits on each LED bar
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
// Main LCD display should have index 0
// Each LED dispaly should have a value equal to its lane #.
enum displays {
  lcdDisp = 0,
  led1Disp,
  led2Disp,
  led3Disp,
  led4Disp
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
  S = 2,    // 00
  M = 4,    // 00:00
  H = 6,    // 00:00:00
};

//*** Variables and defaults for USER SET, RACE PROPERTIES
races raceType = Standard;
// Variable for the number of laps to win standard race type
int raceLaps = 5;
// Race time for a Timed race type, most laps before time runs out wins.
// Create a clock time array to hold converted values for easy display update.
// raceSetTime[1] holds Min, raceSetTime[0] holds Sec, settable from menu.
const byte defMin = 0;
const byte defSec = 30;
byte raceSetTime[2] = {defSec, defMin};
// raceSetTime in milliseconds, will be initialized in setup().
unsigned long raceSetTimeMs;
// pre-race countdown length in seconds, settable from menu
byte preStartCountDown = 2;
// Flag to tell race timer if race time is going up or going down as in a time limit race.
// Since the default race type is standard the default countingDown is false.
bool countingDown = false;

// Note that the durint a race, current lap count may often be 1 greater than the lap of interest.
volatile int lapCount[laneCount + 1] = {
  0, 0, 0, 0, 0
};


const byte fastestQSize = 10;
// Using two, 2D arrays, whose rows are kept in sync, to track fastest laps.
// One array of longs to hold the time, and one of int for the lap#.
// This is to save on memory over a single 2D long array.
// The row index indicates the associated lane/racer #.
// Row idx = 0 is reserved, and currently not used.
unsigned int fastestLaps[laneCount + 1][fastestQSize] = {};
unsigned long fastestTimes[laneCount + 1][fastestQSize] = {};

// Another pair of lap and time arrays holds the fastest overall laps
unsigned int topFastestLaps[ fastestQSize ] = {};
unsigned long topFastestTimes[ fastestQSize ] = {};
// With the overall lap times we also need to track the associated lane/racer.
byte topFastestRacers[ fastestQSize ] = {};


// During the actual race, due to memory limits, we only track the
// last X lap millis() timestamps.
// The modulus of the lap count, by the lapMillisQSize, will set the looping index.
// (lapCount % lapMillisQSize) = idx, will always be 0 <= idx < lapMillisQSize
// DO NOT SET lapMillisQSize < 3
const byte lapMillisQSize = 5;
// The row index indicates the lane/racer associated with it's row array timestamps.
volatile unsigned long lastXMillis[laneCount + 1] [ lapMillisQSize ] = {};


// idx = 0 logs millis() timestamp at start of race.
// idx > 0 logs millis() timestamp at the start of the current lap on matching lane #.
volatile unsigned long startMillis[laneCount + 1];
// idx = 0 logs current race time in ms.
// idx > 1 logs current lap time in ms of matching lane #.
volatile unsigned long currentTime[laneCount + 1] = {
  0, 0, 0, 0, 0
};


// flag to alert results menu whether the race data is garbage or not
bool raceDataExists = false;
// The millis() timestamp of the start of current program loop.
unsigned long curMillis;

// The millis() timestamp of last display update.
unsigned long lastTickMillis;
// Interval in milliseconds that the clock displays are updated.
// this value does not affect lap time precision, it only sets min display update rate.
int displayTick = 100;
// timestamp marking new press of pause button, used to set start of debounce period.
unsigned long pauseDebounceMillis = 0;
// flag indicating if race state is in preStart countdown or active race
bool preStart = false;


// ******* LANE/RACER VARIABLES ******************
// Throughout this code the terms 'lane', 'racer', and 'sensor' all reference,
// detection and timing related to the triggering of a given hardware sensor.
// In all arrays relating to this data the array index will equal the associate lane/racer/sensor #.
// Index 0 will be reserved for race level times and data or may not be used at present.
//
// The bit mask representing the active lanes, defaults lanes 1 & 2 enabled by default.
byte enabledLanes = 0b00000011;
// Bit masks for each lane that are used to toggle enable/disable.
byte laneMask[ laneCount + 1 ] = {
  0b00000000, 0b00000001, 0b00000010, 0b00000100, 0b00001000
};
// Logs current lane state (see enum for details)
byte laneEnableStatus[ laneCount + 1 ] = {
  Off, StandBy, StandBy, Off, Off
};
// Logs hardware pin related to given lane/racer/sensor
byte laneSensorPin[ laneCount + 1 ] = {
  255, lane1Pin, lane2Pin, lane3Pin, lane4Pin
};
// Holds index of Racers[] array that indicates the name of racer associated with lane.
// set racer name to '0' (aka 'DISABLED') when status is Off.
// Other than 0, the same Racer[idx] cannot be used for more than 1 racer.
// During use, the code will prevent this, but it will not correct duplicates made here.
byte laneRacer[ laneCount + 1 ] = {
  0, 1, 2, 0, 0
};

// After a racer finishes a lap the logged time will be 'flashed' up
// to that racer's LED. The period that this lap time stays displayed
// before the display returns to logging current lap time is the 'flash' period.
// The lane Lap Flash variables indicate the state of this flash period.
// 0 = update display with racer's current lap and running time.
// 1 = write last finished lap, and its logged time, to racer's LED.
// 2 = hold the last finished lap info on display until flash time is up.
// lap flash status idx corresponds to status of lane #, idx0 reserved.
volatile byte flashStatus[ laneCount + 1 ] = {
  0, 0, 0, 0, 0
};
const int flashDisplayTime = 1500;
// millis() timestamp at start of current flash period for each racer.
unsigned long flashStartMillis[ laneCount + 1 ];

// Index of lap time to display in first row of results window.
// Keep this an int so it can be signed. A negative is the trigger to loop index
int resultsRowIdx = 0;
// Tracks which racer's results to show in Results Menu, 0 = show top results overall.
byte resultsMenuIdx = 0;

// Variable to hold the last digit displayed to LEDs during last 3 sec
// of the pre-start countdown, so it does not rewrite every program loop.
byte ledCountdownTemp = 0;

// enum to name menu screens for easier reference
enum Menus {
  MainMenu,
    SettingsMenu,
    SelectRacersMenu,
    StartRaceMenu,
    ResultsMenu
};

// Create variale to hold current game 'state' and menu page.
states state;
Menus currentMenu;
// This flag is used to indicate first entry into a state or menu so
// that the state can do one time only prep for its first loop.
bool entryFlag;


// Racer Names list.
// Because this is an array of different length character arrays
// there is not easy way to determine the number of elements,
// so we use a constant to set the length.
// This must be maintained manually to match Racers[] actual content below
byte const racerListSize = 10;
// 7-seg digits cannot display W's, M's, X's, K's, or V's
const char* Racers[racerListSize] = {
  "DISABLED", "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle 1", "Rat2020_longer", "The OG", "5318008"
};
// Racer's victory song, matched by index of racer.
const char* victorySong[racerListSize] = {
  disabledTone, starWarsImperialMarch, takeOnMeMB, airWolfTheme, tmnt1, gameOfThrones, galaga, outrun, starWarsEnd, spyHunter
};

// sets screen cursor position names on the racer select menu
byte nameEndPos = 19;

// *** STRING PROGMEM *************
// in this section we define our menu string constants to use program memory
// this frees up significant RAM. In this case, using progmem to replace
// these few const char* arrays, reduced RAM used by globals, by 10%.
// The character buffer needs to be as large as largest string to be used.
// Since our longest row of characters is on the LCD we use its column count.
char buffer[LCD_COLS];

const char Main0[] PROGMEM = "A| Select Racers";
const char Main1[] PROGMEM = "B| Change Settings";
const char Main2[] PROGMEM = "C| Start a Race";
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

const char Settings0[] PROGMEM = "Settings     mm:ss";
const char Settings1[] PROGMEM = " A |Time       :";
const char Settings2[] PROGMEM = " B |Laps";
const char Settings3[] PROGMEM = "0-4|Lanes";
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

const char SelectRacers0[] PROGMEM = "A|Racer1";
const char SelectRacers1[] PROGMEM = "B|Racer2";
const char SelectRacers2[] PROGMEM = "C|Racer3";
const char SelectRacers3[] PROGMEM = "D|Racer4";
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
const char* const StartRaceText[4] PROGMEM = {
  SelectRace0,
  SelectRace1,
  SelectRace2,
  SelectRace3
};
// const char* StartRaceText[4] = {
//   "A|First to     Laps",
//   "B|Most Laps in   :",
//   "",
//   "D|Countdown:    Sec"
// };

// additional text strings used in multiple places
const char* Start = {"Start"};


// Reviews lane status array and writes to lcd a number for each enabled lane
// and an 'X' if the lane is disabled. Run after settings change.
void lcdPrintLaneSettings(){
  for(int i = 1; i <= laneCount; i++){
    lcd.setCursor(10+ 2 * i, 3);
    // Something with the lcd.print() doesn't work to use a ternery to assess this.
    if (laneEnableStatus[i] == 0){
      lcd.print('X');
    } else {
      lcd.print(i);
    }
  }
}


// This function accepts in a millisecond value (msIN) and converts it into clock time.
// Because we have so many variables and want to save memory,
// we pass the clock time variables in by reference (using &).
// This means this function will be updating those variables from the higher scope directly.
void SplitTime(unsigned long msIN, unsigned long &ulHour, unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulDec, unsigned long &ulCent, unsigned long &ulMill) {
  // Calculate HH:MM:SS from millisecond count
  // HH:MM:SS.000 --> ulHour:ulMin:ulSec.(0 = ulDec, 00 = ulCent, 000 = ulMill)
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
  for (byte i = 0; i <= laneCount; i++) {
    for (byte j = 0; j < fastestQSize; j++) {
      fastestLaps[i][j] = 0;
      fastestTimes[i][j] = 999999;
    }
  }
  for (byte i = 0; i <= laneCount; i++) {
    for (byte j = 0; j < lapMillisQSize; j++) {
      lastXMillis[i][j] = 0;
    }
  }
}

// Used to initialize results, top fastest, lap array to high numbers.
// A lap number of 0, and racer id of 255, marks these as dummy laps.
void InitializeTopFastest(){
  for (byte i = 0; i < fastestQSize; i++) {
    topFastestLaps[i] = 0;
    topFastestRacers[i] = 255;
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


// This function provides cycling of racer name selection without,
// allowing two racer's to choose the same name.
// laneID is the laneNumber whose racer name should be indexed.
byte IndexRacer(byte laneID) {
  byte newRacerNameIndex = laneRacer[laneID];
  bool notUnique = true;
  // The modulus (%) of a numerator smaller than its denominator is equal to itself,
  // while the modulus of an integer with itself is 0. Add 1 to restart a idx 1, not 0.
  while(notUnique){
    newRacerNameIndex = ((newRacerNameIndex + 1)%racerListSize == 0) ? 1 : (newRacerNameIndex + 1)%racerListSize;
    notUnique = false;
    for(byte i = 1; i <= laneCount; i++){
      if((i != laneID) && (laneRacer[i] == newRacerNameIndex)){
        notUnique = true;
      }
    }
  }
  return newRacerNameIndex;
}


// // Provides, looping, up or down indexing of items in list
// // This function assumes the start of the list is at zero index
// byte IndexList(byte curIdx, byte listLength, bool cycleUp, byte otherRacer) {
//   byte newIndex = curIdx;
//   if (cycleUp) {
//     // Cycling up we need to reset to zero if we reach end of list.
//     // The modulus (%) of a numerator smaller than its denominator is equal to itself,
//     // while the modulus of an integer with itself is 0.
//     newIndex = (newIndex + 1) % (listLength);
//   } else {
//     // if cycling down list we need to reset to end of list if we reach the beginning
//     if (curIdx == 0){
//       newIndex = listLength - 1;
//     } else {
//       newIndex--;
//     }
//   }
//   // We can't allow two racers to have the same id,
//   // so if the result is the other selected racer then index again.
//   if (newIndex == otherRacer) {
//     newIndex = IndexList(newIndex, listLength, cycleUp, otherRacer);
//   }
//   return newIndex;
// }


// Returns the number of digits in an integer.
int calcDigits(int number){
  int digitCount = 0;
  while (number != 0) {
    // integer division will drop decimal
    number = number/10;
    digitCount++;
  }
  return digitCount;
}


// Used to write same character repeatedly from start to end position, inclusive.
// This is primarily used to clear lines and space before writing an update to display.
void writeSpanOfChars(displays disp, byte lineNumber = 0, byte posStart = 0, byte posEnd = LCD_COLS - 1, char printChar = ' ') {
  switch(disp){
    case lcdDisp:{
      lcd.setCursor(posStart, lineNumber);
      for(byte n = posStart; n <= posEnd; n++){
        lcd.print(printChar);
      }
    }
    break;
    // Assumes that Racer LED bars are the only other display.
    default: {
      for(byte i = posStart; i <= posEnd; i++){
        lc.setChar(disp-1, (LED_DIGITS - 1) - i, printChar, false);
      }
    }
    break;
  }
}

// function to write pre-start final countdown digits to LEDs
void ledWriteDigits(byte digit) {
  for (int j=0; j < LED_BAR_COUNT; j++){
    writeSpanOfChars(displays(j+1), 0, 0, 7, char(digit));
  }
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
    default:{
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
    break;
  }
}


// This function prints the input text, to the indicated display, at the indicated position.
void PrintText(const char textToWrite[LCD_COLS], const displays display, const byte writeSpaceEndPos, const byte width = LED_DIGITS, bool rightJust = false, const byte line = 0, bool clear = true) {
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
      if (clear) writeSpanOfChars(lcdDisp, line, cursorStartPos, writeSpaceEndPos);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        lcd.setCursor(i, line);
        lcd.print(textToWrite[textIndex]);
        textIndex++;
      }
    } // END LCD case
    break;
    // For writing to LED display
    // Note that 7-seg cannot display 'K', 'W', 'M', 'X's, 'W'x, or 'V's
    default:{
      if (clear) lc.clearDisplay(display - 1);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        // The digit position of the LED bars is right to left so we flip the requested index.
        lc.setChar(display - 1, (LED_DIGITS-1) - i, textToWrite[textIndex], false);
        textIndex++;
      }
    } // END LED cases
    break;
  } // END display switch
}


// timeMillis
//    - Time in ms to be printed as a clock time to given display.
// clockEndPos
//    - The index of display row, where the last digit of the clock should go.
//    This index is always considered left to right, and starting with 0.
//    The function will flip index internally if needed for a right to left indexed display.
//    For the MAX7219 LED bars, natively, the digit idx 0 is the far right digit.
//    However, when using this function with a MAX7219 LED bar,
//    to have clock end at far right, input clockEndPos 7, not 0.
// printWidth
//    - The number of character spaces available to print time, including ':' and '.'
// precision
//    - The number of decimal places desired. (allowed: 1 = 0.0, 2 = 0.00, or 3 = 0.000)
// display
//    - display enum indicating display to write clock time to.
// line
//    - The line on the display to write to, if display only has 1 line, use any#.
// leadingZs
//    - Indicates if leading time block should have a leading zero.
//    If true, 3,903,000 = 1hr 5min 3sec = 01:05:03.precision
//
// Precision will automatically be dropped to make room until a whole digit
// is dropped, at which point the time exceeds the width available,
// and an 'E' will be written to notify view width has been exceeded.
// NOTE: An 'E' does not affect that actual timing which can go on for about 49 days.
void PrintClock(ulong timeMillis, byte clockEndPos, byte printWidth, byte precision, displays display, byte line = 0, bool leadingZs = false) {

  // Clear clock space on display
  // Must account for end position being included in the printWidth total.
  writeSpanOfChars(display, line, clockEndPos - printWidth + 1, clockEndPos);

  clockWidth nextTimeBlock = H;
  int maxPrecision = 0;
  byte neededWidth = 0;

  // Based on the time value, and width of display area (printWidth),
  // calculate how many decimal places can be accomodated.
  // Because the LCD requires a character space for ':' and '.',
  // we must adjust the required width accordingly.
  //
  // if t < 10sec
  // 0.0
  if(timeMillis < 10000){
    nextTimeBlock = S;
    maxPrecision = (display == lcdDisp ? printWidth-2 : printWidth-1);
    neededWidth = 1;
  } else
  // if 10sec <= t < 1min
  // 00
  if (timeMillis < 60000){
    nextTimeBlock = S;
    maxPrecision = (display == lcdDisp ? printWidth-3 : printWidth-2);
    neededWidth = 2;
  } else
  // if 1min <= t < 10min
  // 0:00
  if (timeMillis < 600000){
    nextTimeBlock = M;
    maxPrecision = (display == lcdDisp ? printWidth-5 : printWidth-3);
    neededWidth = (display == lcdDisp ? 4 : 3);
  } else
  // if 10min <= t < 1hr
  // 00:00
  if (timeMillis < 3600000){
    nextTimeBlock = M;
    maxPrecision = (display == lcdDisp ? printWidth-6 : printWidth-4);
    neededWidth = (display == lcdDisp ? 5 : 4);
  } else
  // if 1hr <= t < 10hr
  // 0:00:00
  if (timeMillis < 30600000){
    nextTimeBlock = H;
    maxPrecision = (display == lcdDisp ? printWidth-8 : printWidth-5);
    neededWidth = (display == lcdDisp ? 7 : 5);
  } else
  // if 10hr <= t < 24hr
  // 00:00:00
  if (timeMillis < 86,400,000){
    nextTimeBlock = H;
    maxPrecision = (display == lcdDisp ? printWidth-9 : printWidth-6);
    neededWidth = (display == lcdDisp ? 8 : 6);
  } else {
  // if 24hr <= t
  // over max clock limit of 24 hrs.
  if (timeMillis >= 86,400,000)
    maxPrecision = -1;
  }

  // A negative maxPrecision means we don't have space to print the time.
  if (maxPrecision < 0){
    // Print an 'E' to alert current time is over display width limit.
    PrintText("E", display, clockEndPos, printWidth, true, line, display == lcdDisp ? true : false);

  } else {
    unsigned long ulHour;
    unsigned long ulMin;
    unsigned long ulSec;
    unsigned long ulDec;
    unsigned long ulCent;
    unsigned long ulMill;
    // Convert millisecond time into its individual time blocks.
    SplitTime(timeMillis, ulHour, ulMin, ulSec, ulDec, ulCent, ulMill);

    unsigned long decimalSec;
    byte hourEndPos;

    // if requested precision is higher than available max, use max.
    precision = maxPrecision < precision ? maxPrecision : precision;
    // LCD must add character space for colons and decimal place if it has precision.
    hourEndPos = (display == lcdDisp ?
                  clockEndPos - (6 + precision + (precision > 0 ? 1 : 0)) :
                  clockEndPos - (4 + precision)
                  );

    // If leading zeros are requested but there is no space turn them off.
    // If LCD, with precision, subtract additional 1 for the decimal position.
    if( (printWidth - precision - neededWidth - ((display==lcdDisp && precision>0)?1:0)) <= 0 ){
      leadingZs = false;
    }
    // Serial.println("leading Zs");
    // Serial.println(leadingZs);
    // Serial.println(printWidth - precision - neededWidth);

    // Set which precision time block to use based on available precision. 
    switch(precision){
      case 1:
        decimalSec = ulDec;
      break;
      case 2:
        decimalSec = ulCent;
      break;
      case 3:
        decimalSec = ulMill;
      break;
      default:
        decimalSec = NULL;
      break;
    }

    if (nextTimeBlock == H) {
      // H 00:00:00
      switch (display){
        case lcdDisp: {
          PrintNumbers(ulHour, 2, hourEndPos, display, leadingZs, line);
          lcd.print(":");
        }
        break;
        default:{
          PrintNumbers(ulHour, 2, hourEndPos, display, leadingZs, 0, true);
        }
        break;
      } // END of display switch
      // All non-leading parts of the clock time should have leading zeros
      leadingZs = true;
      // Move on to print Minutes.
      nextTimeBlock = M;
    }

    if (nextTimeBlock == M) {
      // M 00:00
      switch (display){
        case lcdDisp: {
          PrintNumbers(ulMin, 2, hourEndPos + 3, display, leadingZs, line);
          lcd.print(":");
        }
        break;
        default:{
          PrintNumbers(ulMin, 2, hourEndPos + 2, display, leadingZs, 0, true);
        }
        break;
      } // END of display switch
      // All non-leading parts of the clock time should have leading zeros
      leadingZs = true;
      // Move on to print seconds
      nextTimeBlock = S;
    }

    if (nextTimeBlock == S) {
      // S 00.(000)
      switch (display){
        case lcdDisp: {
          PrintNumbers(ulSec, 2, hourEndPos + 6, display, leadingZs, line, precision > 0);
          if(precision > 0) lcd.print(decimalSec);
        }
        break;
        default:{
          PrintNumbers(ulSec, 2, hourEndPos + 4, display, leadingZs, 0, precision > 0);
          if(precision > 0) PrintNumbers(decimalSec, precision, clockEndPos, display);
        }
        break;
      } // END of display switch
    } // END of if 'S'

  } // END if 'X' Else

} // END PrintClock()


void DrawPreStartScreen(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Your Race Starts in:");
  PrintClock(currentTime[0], PRESTART_CLK_POS, 4, 1, lcdDisp, 2);
  // if lanes are not off, then update screen
  if(laneEnableStatus[1] > 0) PrintText(Racers[laneRacer[1]], led1Disp, 7, 8, false);
  if(laneEnableStatus[2] > 0) PrintText(Racers[laneRacer[2]], led2Disp, 7, 8, false);
}


// Toggle status of given lane mask and update status array.
// This also then must update the racer name between 'DISABLED' and an initial racer.
void ToggleLaneEnable(byte laneNumber){
  // Serial.println("number pressed");
  // Serial.println(laneNumber);
  // First update the Enabled Lanes Mask
  // if lane pressed is 0, disable all the lanes
  bool wasSet = enabledLanes & laneMask[laneNumber];
  if(laneNumber == 0){
    enabledLanes = 0b00000000;
    // set all racer names to idx 0, aka 'DISABLED'
    for (byte i = 1; i <= laneCount; i++){
      laneRacer[i] = 0;
    }
    wasSet = true;
  } else {
    // Toggle enabled lanes bit for indicated lane.
    enabledLanes = enabledLanes xor laneMask[laneNumber];
  }

  // Update the status array
  for (byte i = 1; i <= laneCount; i++){
    // if parentheses are not around the bitwise & expression the ternery operate incorrectly.
    laneEnableStatus[i] = ((enabledLanes & laneMask[i]) > 0) ? StandBy : Off;
  }
  if(wasSet){
    // If lane had been on then, it's off now and the racer name should be to 0, aka 'DISABLED'
    laneRacer[laneNumber] = 0;
  } else {
    // otherwise it was off, and is now on and must get an initial racer name.
    laneRacer[laneNumber] = IndexRacer(1);
  }
}


// If lane enabled, writes the current racer name to display.
// Otherwise writes 'DISABLED' to the display.
void UpdateNamesOnLEDs(){
  for (byte i = 1; i <= laneCount; i++){
    PrintText((laneEnableStatus[i] == 0 ? "DISABLED" : Racers[laneRacer[i]]), displays(i), 7, 8);
  }
}


// This function goes through the lanes enabled by the settings
// and turns the interrupt pins on or off.
void EnablePinInterrupts(bool Enable){
  for (byte i = 1; i <= laneCount; i++){
    if(laneEnableStatus[i] > 0 && Enable) {
      pciSetup(laneSensorPin[i]);
      // Serial.println("pin enabled");
    }
    else if(laneEnableStatus[i] > 0 && !Enable){
      clearPCI(laneSensorPin[i]);
      // Serial.println("pin disabled");
    }
  }
}

// unsigned long micros() {
//     unsigned long m;
//     extern volatile unsigned long timer0_overflow_count;
//     uint8_t oldSREG = SREG, t;
//     cli();
//     m = timer0_overflow_count;
//     t = TCNT0;
//     if ((TIFR0 & _BV(TOV0)) && (t < 255))
//         m++;
//     SREG = oldSREG;
//     return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
// }

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
  // using a logical AND (&) with the bitwise NOT (~) of the bitmask for the pin
  *digitalPinToPCMSK(pin) &= ~bit (digitalPinToPCMSKbit(pin));
  // Serial.println("clearPCI");
  // Serial.println(*digitalPinToPCMSK(pin), BIN);
  // Serial.println(bit (digitalPinToPCMSKbit(pin)), BIN);
  // Serial.println("Not");
  // Serial.println(~bit (digitalPinToPCMSKbit(pin)), BIN);
}

// debounceTime (ms), time within which not to accept additional signal input
const int debounceTime = 1000;
// volatile long timeTest[15];
// volatile int timeTestCount = 0;
// ISR is a special Arduino Macro or routine that handles interrupts ISR(vector, attributes)
// PCINT1_vect handles pin change interrupt for the pin block A0-A5, represented in bit0-bit5
// The execution time of this function should be as fast as possible as
// interrupts are disabled while inside it.
ISR (PCINT1_vect) {
  // Note that millis() does not execute inside the ISR().
  // It can be called and used, but it is not continuing to increment.
  unsigned long logMillis = millis();
  // micros() is used for assessing the ISR execution time.
  // unsigned long logMicros = micros();

  // We have setup the lap triggers as inputs.
  // This means the pins have been set to HIGH, indicated by a 1 on its register bit.
  // When a button is pressed, or sensor triggered, it should bring the pin LOW.
  // A LOW pin is indicated by a 0 on its port register.

  // Lane 1 positive trigger PINC = 0xXXXXXXX0
  // Lane 1 is on pin A0 which uses the 1st bit of the register, idx 0, of PINC Byte
  // Lane 2 positive trigger PINC = 0xXXXXXX0X
  // Lane 2 is on pin A1 which uses the 2nd bit of the register, idx 1, of PINC Byte
  // Lane 3 positive trigger PINC = 0xXXXXX0XX
  // Lane 3 is on pin A2 which uses the 3rd bit of the register, idx 2, of PINC Byte
  // Lane 4 positive trigger PINC = 0xXXXX0XXX
  // Lane 4 is on pin A3 which uses the 4th bit of the register, idx 3, of PINC Byte
  
  // For this function it will work better to have our triggered bits as 1s.
  // To convert the zero based triggers above into 1s, we can simply flip each bit.
  // Since we only need to check the 1st 4 bits, we'll also turn the last 4 to 0.
  // Flip every bit by using 'BitsToFlip' xor 0b11111111
  // then trim off the high bits with 'ByteToTrim' & 0b00001111
  byte triggeredPins = ((PINC xor 0b11111111) & 0b00001111);

  // While the triggeredPins byte is > 0, one of the digits is a 1.
  // If after a shift it's triggerPins = 0, then there is no need to keep checking.
  // As a failsafe, after 8 cycles the byte will always be zero.
  byte i = 0;
  while(triggeredPins > 0){
    // If digit is a 1, then process it as a trigger on lane 'i + 1'
    if(triggeredPins & 1){
      // Depending on the status of this lane we process the trigger differently.
      switch (laneEnableStatus[ i + 1 ]) {

        case StandBy:{
          // If the very first lap or back from a pause, no need for debounce
          // Change lane status from 'StandBy' to 'Active'
          laneEnableStatus[ i + 1 ] = Active;
          // Log timestamp into start log and lap timestamp tracking array (lastXMillis)
          if(lapCount[ i + 1 ] == 0) {
            // If The first lap in the race.
            lastXMillis[ i + 1 ] [0] = logMillis;
            startMillis[ i + 1 ] = logMillis;
            lapCount[ i + 1 ]++;
          } else {
            // Else, if returning from Pause, we need to feed the new start time,
            // into the pevious lap index spot, and not index the current lapcount.
            lastXMillis [ i + 1 ][(lapCount[ i + 1 ] - 1) % lapMillisQSize] = logMillis;
            startMillis[ i + 1 ] = logMillis;
            // DON'T index lapcount, we're restarting the current lap
          }
          Beep();
        }
        break;

        case Active:{
          // If lane is 'Active' then check that it has not been previously triggerd within debounce period.
          if( ( logMillis - lastXMillis [i + 1] [(lapCount[i+1]-1)%lapMillisQSize] ) > debounceTime ){
            // Set lap display flash status to 1, indicating entry into the flash state for the lane.
            flashStatus[i + 1] = 1;
            lastXMillis [i + 1][lapCount[i + 1] % lapMillisQSize] = logMillis;
            startMillis[i + 1] = logMillis;
            lapCount[i + 1]++;
            Beep();
          }
        }
        break;

        default:{
          // If lane is 'Off' then ignore it. It should not have been possible to trigger.
          // The interrupt hardware should not be active on 'Off' lanes. 
        }
        break;
      } // END of lane status switch
    } // END if triggeredPin & 1

    // shift to the next bit
    triggeredPins = triggeredPins >> 1;
    i++;

  } // END of, for loop, checking each digit, loop
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


// Generic function to check if a button is pressed
int buttonPressed(uint8_t analogPin) {
  if (analogRead(analogPin) < 100){
    return true;
  }
  // Serial.println("button pressed");
  // Serial.println(analogRead(analogPin));
  return false;
}

void PauseRace(){
  // Puase button positive trigger PINC = 0xXXXXX0XX
  // Pause is on pin A2 which uses the 3rd bit of the register, idx 2, of PINC Byte
  Beep();
  int logMillis = millis();
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
        EnablePinInterrupts(false);
        // Reset lanes to the default Enabled laneState, 'StandBy'.
        // UpdateLaneState(lanesEnabledIdx);
        for (byte i = 1; i <= laneCount; i++){
          if(laneEnableStatus[i] == Active) laneEnableStatus[i] = StandBy;
        } 
      }
      break;
      // if already paused then return to race
      case Paused: {
        state = Race;
        // make sure entry flag is turned back off before re-entering race state
        entryFlag = true;
        currentTime[1] = 0;
        currentTime[2] = 0;
        for(byte i = 1; i <= laneCount; i++){
          currentTime[i] = 0;
        }
        startMillis[1] = logMillis;
        startMillis[2] = logMillis;
        // Renable pins on active lanes
        EnablePinInterrupts(true);
      }
      break;
      // otherwise ignore the button
      default:
      break;
    } // END switch
  } // END debounce if

}


// This function compares the input race time with current fastest list.
// If the new lap time is faster than any existing time, it takes its place,
// pushing the subsequent times down by 1, dropping the last time off the list.
void UpdateFastestLap(unsigned long timesArray[], unsigned int lapsArray[], const int lap, const unsigned long newLapTime, const byte racer, const byte arrayLength, bool topTimes = false){
  // Serial.println("ENTERED FASTEST");
  for (byte i = 0; i < arrayLength; i++){
    // Starting from beginning of list, compare new time with existing times.
    // If new lap time is faster, hold its place, and shift bested, and remaining times down.
    if (timesArray[i] > newLapTime) {
      // Starting from the end of list, replace rows with previous row,
      // until we reach row of the bested time to be replaced.
      for (byte j = arrayLength - 1; j > i; j--){
        timesArray[j] = timesArray[j-1];
        lapsArray[j] = lapsArray[j-1];
        if(topTimes) topFastestRacers[j] = topFastestRacers[j-1];
      }
      // Then replace old, bested lap time, with new, faster lap and time.
      timesArray[i] = newLapTime;
      lapsArray[i] = lap;
      // It's not being consistent to reference this one array globally.
      // but in this case it cheaper to do this and use a 'topTimes' flag
      // than carry an extra array argument for the corner case.
      if(topTimes) topFastestRacers[i] = racer;
      // for (int k = 0; k < arrayLength; k++){
      //   Serial.println("UpdateFastest End");
      //   Serial.print(timesArray[k]);
      //   Serial.print(" : ");
      //   Serial.println(lapsArray[k][0]);
      // }
      // exit function, no need to continue once found and shift made
      return;
    } // END if (timesArray) block
  } // END fastest lap array for loop
}


// This function combines all the racer's fastest laps and list the top fastest overall.
void CompileTopFastest(){
  // first we need to re-initialize or else we will be duplicating previously added laps
  InitializeTopFastest();
  // for each racer/lane, compare their top laps to over all top laps and insert as appropriate.
  for (byte i = 1; i <= laneCount; i++) {
    for (byte j = 0; j < fastestQSize; j++) {
      UpdateFastestLap(topFastestTimes, topFastestLaps, fastestLaps[i][j], fastestTimes[i][j], laneRacer[i],  fastestQSize, true);
    }
  }
}

// Prints given results array to lcd display
// resultsRowIdx = 0 should be all fastest, = 1, 2, 3, or 4 indicates that lane's fastest.
void lcdPrintResults(unsigned long fastestTimes[], unsigned int fastestLaps[], int listSize) {
  for (byte i = 0; i < 3; i++){
    // ignore if lap = 0 which means it's a dummy lap or if index greater than lap count
    if (fastestLaps[resultsRowIdx + i] > 0 && i < listSize) {
      // print rank of time, which is index + 1
      PrintNumbers(resultsRowIdx + i + 1, 2, 1, lcdDisp, false, i + 1, false);
      // then print LAP
      PrintNumbers(fastestLaps[resultsRowIdx + i], 3, 5, lcdDisp, true, i + 1, false);
      // Print the lap TIME
      PrintClock(fastestTimes[resultsRowIdx + i], 12, 6, 3, lcdDisp, i + 1);
      // clear racer name space of any previous characters
      writeSpanOfChars(lcdDisp, 1 + i, 13);
      // then print RACER NAME
      // if idx = 0 then it's the top results and has racer names with the lap times.
      if(resultsMenuIdx == 0){
        PrintText(Racers[topFastestRacers[resultsRowIdx + i]], lcdDisp, 19, 6, true, i + 1, false);
      }
    } else {
      // we want to set the index back one so it doesn't keep scrolling in empty space
      resultsRowIdx = (resultsRowIdx -1) >= 0 ? resultsRowIdx - 1 : 0;
      // if this condition exists, we want to end the loop to avoid doubles
      break;
    }
  }
}

void UpdateResultsMenu() {
  switch(resultsMenuIdx){
    // Idx = 0 is the top results menu
    case 0: {
      lcd.clear();
      PrintText("C | TOP RESULTS", lcdDisp, 14, 15, false, 0);
      lcdPrintResults(topFastestTimes, topFastestLaps, fastestQSize);
    }
    break;
    // All idx > 0 indicate results related to matching lane number.
    case 1 ... laneCount: {
      lcd.clear();
      PrintText("C | RACER ", lcdDisp, 19, 20, false, 0);
      // Print Racer Lane #
      PrintNumbers(resultsMenuIdx, 1, 10, lcdDisp, false, 0);
      // Print Racer Name
      PrintText(Racers[laneRacer[resultsMenuIdx]], lcdDisp, 19, 9, true);
      // Print results, if number of recorded laps is smaller than Q size, use lap count for list size.
      lcdPrintResults(fastestTimes[resultsMenuIdx], fastestLaps[resultsMenuIdx], lapCount[resultsMenuIdx] < fastestQSize ? lapCount[resultsMenuIdx] : fastestQSize);
    }
    break;
    default:
    break;
  }
}


void ResetRace(){
  // Reset lap count and lap flash status for all lanes/racers to 0.
  for (byte i = 1; i <= laneCount; i++) {
    lapCount[i] = 0;
    flashStatus[i] = 0;
  }
  InitializeRacerArrays();
}

// ------------BELOW NOT USED-------------
// // *** This section for using Note and Lengths arrays for songs
// // Globals for holding the current melody data references.
// int *playingNotes;
// int *playingLengths;                                                            
// int playingTempoBPM = 135;
// int playingMelodySize = 0;
// // flag to indicate to the main program loop whether a melody is in process
// // so it should execute the 'PlayNote()' function with the current melody parameters.
// bool melodyPlaying = false;
// // Holds the timestamp of last tone played so timing of next note in melody can be determined
// unsigned long lastNoteMillis = 0;
// // index of the current note to play of 'playing...' song.
// int melodyIndex = 0;
// // time in ms between beginning of last note and when next note should be played.
// int noteDelay = 0;

// // Function to play the current note index of a melody using 'tone()'.
// // We want to pass all the variables instead of depending on their globality.
// // This function returns, in ms, how long to wait before playing following note.
// int PlayNote(int *songNotes, int *songLengths, int curNoteIdx, byte tempoBPM){

//   int noteDuration;
//   int noteLength = pgm_read_word(&songLengths[curNoteIdx]);

//   // If tempo = 0 then use note length directly as ms duration
//   if(tempoBPM == 0){
//     noteDuration = noteLength;
//   } else {
//     // Otherwise calculate duration in ms from bpm:
//     // (60,000ms/min)/Xbpm * 4beats/note * 1/notelength
//     // Make sure equation has a decimal or result will be incorrect integer math.
//     if (noteLength > 0){
//       noteDuration = (60000 / tempoBPM) * 4 * (1.0 / noteLength);
//     } else {
//       // If note length is negative, then it's dotted so add extra half length.
//       noteDuration = 1.5 * (60000 / tempoBPM) * 4 * (1.0 / abs(noteLength));
//     }
//   }

//   // Record millisecond timestamp at start of new note.
//   lastNoteMillis = millis();
//   // The played notes have no transition time or strike impulse.
//   // Played as written, each note sounds unaturally flat and run together.
//   // Adding a small break between notes makes the melody sound better.
//   // This can be done by slightly shortening the tone played vs the song temp.
//   // or making the gap between notes slightly longer than the note length.
//   // In which case the actual tempo will be slightly slower than the set tempo.
//   // Here we'll factor the played tone down by 10% and keeping the tempo as set.
//   // Play note:
//   tone(buzzPin1, pgm_read_word(&songNotes[curNoteIdx]), .9*noteDuration);
//   melodyIndex++;
//   // If we have reached the end of the melody array then
//   // flip playing flag off and reset tracking variables for next melody.
//   if(melodyIndex == playingMelodySize){
//     melodyPlaying = false;
//     melodyIndex = 0;
//     noteDelay = 0;
//     playingMelodySize = 0;
//   }
//   return noteDuration;
// }
// -------------ABOVE NOT USED-------------------


// *********************************************
// ***************** SETUP *********************
// Initialize hardware and establish software initial state
void setup(){
  // --- SETUP SERIAL ------------------------
  /*
  NOTE: Serial port is only used in debugging at the moment.
  The while loop waits for the serial connection to be established
  before moving on so we don't miss anything. If the Arduion seems to
  'hang', this may be the culprit if there is a connection issue.
  */
  // Open port and wait for connection before proceeding.
  Serial.begin(9600);
  while(!Serial);

  // --- SETUP LCD DIPSLAY -----------------------------
  // Initialize LCD with begin() which will return zero on success.
  // Non-zero failure status codes are defined in <hd44780.h>
	int status = lcd.begin(LCD_COLS, LCD_ROWS);
  // If display initialization fails, trigger onboard error LED if exists.
	if(status) hd44780::fatalError(status);
  // Clear display of any residual data, ensure it starts in a blank state
  lcd.clear();

  // --- SETUP LED 7-SEG, 8-DIGIT MAX7219 LED Globals ------
  // Initialize all the displays
  for(int deviceID = 0; deviceID < LED_BAR_COUNT; deviceID++) {
    // The MAX72XX is in power-saving mode on startup
    lc.shutdown(deviceID, false);
    // intensity range from 0-15, higher = brighter
    lc.setIntensity(deviceID, 1);
    // Blank the LED digits
    lc.clearDisplay(deviceID);
    // writeSpanOfChars(displays(deviceID+1),0, 0, 7, 4);
    // lc.setDigit(3, 6, 3, false);
  }

  // --- SETUP LAP TRIGGERS AND BUTTONS ----------------
  // Roughly equivalent to digitalWrite(lane1Pin, HIGH)
  for (byte i = 1; i <= laneCount; i++){
    pinMode(laneSensorPin[i], INPUT_PULLUP);
  }
  pinMode(pauseStopPin, INPUT);

  // Setup an indicator LED
  // pinMode(ledPIN, OUTPUT); 
  // digitalWrite(ledPIN, ledState);

  // Reset all race variables to initial condition.
  ResetRace();
  // Initialize racetime millisecond to default raceSetTime.
  raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
  // Set initial state to Menu and initial menu to MainMenu, turn initial entry flag on.
  state = Menu;
  currentMenu = MainMenu;
  entryFlag = true;
  
  // Startup test song when using melody arrays.
  // melodyPlaying = true;
  // playingNotes = takeOnMeNotes;
  // playingLengths = takeOnMeLengths;
  // playingMelodySize = takeOnMeSize;
  // playingTempoBPM = takeOnMeTempo;

  // Startup song
  // startPlayRtttlPGM(buzzPin1, reveille);
  Beep();
}

void loop(){
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
  updatePlayRtttl();
  // ----- enable if using melody arrays ----------
  // if(melodyPlaying){
  //   if(millis() - lastNoteMillis >= noteDelay){
  //   // stop generation of square wave triggered by 'tone()'.
  //   // This is not required as all tones should stop at end of duration.
  //   // However, we do it just to make sure it 
  //     // noTone(buzzPin1);
  //     noteDelay = PlayNote(playingNotes, playingLengths, melodyIndex, playingTempoBPM);
  //   }
  // }
  // --------------------------------
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
          EnablePinInterrupts(false);
          // clearPCI(pauseStopPin);
        }
        // All of the menus and sub-menus should been flattened to this single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree.
          // Within each menu case is another switch for each available key input.
          case MainMenu:{
            if (entryFlag) {
              lcdUpdateMenu(MainText);
              // Update Racer LED Displays
              UpdateNamesOnLEDs();
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
                currentMenu = StartRaceMenu;
                entryFlag = true;
                break;
              case 'D':
                currentMenu = ResultsMenu;
                resultsMenuIdx = 0;
                entryFlag = true;
              default:
                break;
            } // END key switch
            break;
          } // END MainMenu Menu case

          case SettingsMenu: {
            if (entryFlag) {
              //draw non-editable text
              lcdUpdateMenu(SettingsText);
              // Minute setting
              PrintNumbers(raceSetTime[1], 2, 14, lcdDisp, true, 1);
              // Seconds setting
              PrintNumbers(raceSetTime[0], 2, 17, lcdDisp, true, 1);
              // Lap count
              PrintNumbers(raceLaps, 3, 16, lcdDisp, true, 2);
              // Lane's Enabled
              lcdPrintLaneSettings();
              // Update Racer LED Displays
              UpdateNamesOnLEDs();
              entryFlag = false;
            }
            switch (key) {
              // TIME
              case 'A':{
                // Change minutes
                raceSetTime[1] = lcdEnterNumber(2, 60, 1, 13);
                // Change seconds
                raceSetTime[0] = lcdEnterNumber(2, 59, 1, 16);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
              }
              break;
              // LAP
              case 'B':{
                // change lap count
                raceLaps = lcdEnterNumber(3, 999, 2, 14);
              }
              break;
              // SECONDS
              case 'C':{
                // change seconds
                raceSetTime[0] = lcdEnterNumber(2, 59, 1, 16);
                // update the race time in ms
                raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
              }
              break;
              // ENABLED LANES
              // If a number is pressed in menu state change enabled lane.
              case '0' ... '4':{
                ToggleLaneEnable(key - '0');
                // Update LCD with change
                lcdPrintLaneSettings();
                // Update racer displays
                UpdateNamesOnLEDs();
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
              // Write all current racer names to LCD
              for(byte i = 1; i <= laneCount; i++){
                PrintText(laneEnableStatus[i] == 0 ? Racers[ laneRacer[0] ] : Racers[ laneRacer[i] ], lcdDisp, nameEndPos, 11, false, i - 1);
              }
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down racer list and update Racer1 name
              case 'A' ... 'D':{
                byte laneNumber = key - 'A' + 1;
                if(laneEnableStatus[laneNumber]){
                  // Cycle to next racer name, if we reach the end of list start back at 1 not 0.
                  // The zero index is reserved for the 'DISABLED' label.
                  laneRacer[laneNumber] = IndexRacer(laneNumber);
                  // Update LCD
                  PrintText(Racers[ laneRacer[laneNumber] ], lcdDisp, nameEndPos, 11, false, laneNumber-1);
                  // Update Racer's LED
                  PrintText(Racers[ laneRacer[laneNumber] ], displays(laneNumber), 7, 8);
                  // Play racers victory song
                  startPlayRtttlPGM(buzzPin1, victorySong[ laneRacer[laneNumber] ]);
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
            } // END of key switch
          } // END SelectRacers Menu Case
          break;

          case StartRaceMenu: {
            if (entryFlag) {
              //draw non-editable text
              lcdUpdateMenu(StartRaceText);
              // Print set LAPS #
              PrintNumbers(raceLaps, 3, 13, lcdDisp, true, 0);
              // Print set race time MINUTES
              PrintNumbers(raceSetTime[1], 2, 16, lcdDisp, true, 1);
              // Print set race time SECONDS
              PrintNumbers(raceSetTime[0], 2, 19, lcdDisp, true, 1);
              // Print sey preStartCountDown
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
          } // END of SelectRace Menu Case
          break;

          case ResultsMenu: {
            if (entryFlag) {
              lcd.clear();
              if(raceDataExists){
                // resultsMenuIdx = TopResults;
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
                  // if resultsMenuIdx is 'Top', then it equals 0 and is false
                  // if (resultsMenuIdx) arrayMax = fastestQSize;
                  // else arrayMax = fastestQSize * 2;
                  arrayMax = fastestQSize;
                  // clear line to remove extra ch from long names replace by short ones
                  writeSpanOfChars(lcdDisp, 1);
                  writeSpanOfChars(lcdDisp, 2);
                  writeSpanOfChars(lcdDisp, 3);
                  if (key == 'A' && resultsRowIdx > 0) resultsRowIdx--;
                  // we subtract 3 because there are two rows printed after tracked index
                  if (key == 'B' && resultsRowIdx < arrayMax-2) resultsRowIdx++;
                  UpdateResultsMenu();
                }
              }
              break;
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // only deal with data if it exists,
                // otherwise garbage will be displayed on screen
                if(raceDataExists) {
                  // Index results menu to next racer.
                  // Currently does not skip 'disabled' data pages.
                  resultsMenuIdx = (resultsMenuIdx + 1) % (laneCount + 1);
                  // Reset row index to 0 so new list starts at the top
                  resultsRowIdx = 0;
                  UpdateResultsMenu();
                  // Serial.println("result idx");
                  // Serial.println(resultsRowIdx);
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
          } // END of ResultsMenu Case
          break;
          default:
          break;
        }; // end of Menu switch
      }; // end of if key pressed wrap
    } // END of Menu State
    break; 

    case Race:{
      curMillis = millis();
      // initialize variables for preStart timing loop first cycle only
      if (entryFlag && preStart) {
        // preStartCountDown is in seconds so we convert to millis
        currentTime[0] = preStartCountDown * 1000;
        lastTickMillis = curMillis;
        DrawPreStartScreen();
        // reset any race variables to initial values
        ResetRace();
        entryFlag = false;
      }
      // initialize variables for race timing loop first cycle only
      if (entryFlag && !preStart) {
        currentTime[0] = 0;
        lastTickMillis = curMillis;
        startMillis[0] = curMillis;
        entryFlag = false;
        // Write Live Race menu and racer names to displays
        lcd.clear();
        lcd.setCursor(8,1);
        lcd.print("Laps   Best");
        if(laneEnableStatus[1] > 0){
          // Print racer's name to LCD
          PrintText(Racers[laneRacer[1]], lcdDisp, 7, 8, false, 2);
          // update racer's current lap total on LCD
          PrintNumbers(lapCount[1] == 0 ? 0 : lapCount[1] - 1, 3, 11, lcdDisp, true, 2);
          // update racer's current best lap time to LCD
          if(lapCount[1] > 0) PrintClock(fastestTimes[1][0], 19, 7, 3, lcdDisp, 2);
          PrintText(Start, led1Disp, 7, 8, true, 0, false);
        }
        if(laneEnableStatus[2] > 0){
          // Print racer's name to LCD
          PrintText(Racers[laneRacer[2]], lcdDisp, 7, 8, false, 3);
          // update racer's current lap total on lcd (0 if start, current if restart)
          PrintNumbers(lapCount[2] == 0 ? 0 : lapCount[2] - 1, 3, 11, lcdDisp, true, 3);
          // update racer's current best lap time to lcd
          if(lapCount[2] > 0) PrintClock(fastestTimes[2][0], 19, 7, 3, lcdDisp, 3);
          PrintText(Start, led2Disp, 7, 8, true, 0, false);
        }
        // this act enables the racer to trigger first lap
        // Turn on interrupts for enabled lane pins
        EnablePinInterrupts(true);
        // enable race pause button interrupt
        pciSetup(pauseStopPin);
      }

      if (preStart) {
        if (currentTime[0] > 0){
          // update LCD on each tick
          if (curMillis - lastTickMillis > displayTick){
            currentTime[0] = currentTime[0] - displayTick;
            PrintClock(currentTime[0], PRESTART_CLK_POS, 4, 2, lcdDisp, 2);
            lastTickMillis = curMillis;
          }
          // In last 3 seconds send countdown to LEDs
          if (currentTime[0] < 2999 && (currentTime[0]/1000 + 1) != ledCountdownTemp) {
            // we add 1 because it should change on start of the digit not end
            ledCountdownTemp = currentTime[0]/1000 + 1;
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
        // Check analog pause button for press
        if(buttonPressed(pauseStopPin)) PauseRace();
        // update current racetime
        // If racetype is timed then displayTick will be a negative number
        if (countingDown) {
          currentTime[0] = raceSetTimeMs < (curMillis - startMillis[0]) ? 0 : raceSetTimeMs - (curMillis - startMillis[0]);
        } else {
          currentTime[0] = curMillis - startMillis[0];
        }
        // If lanestate is active then update current lap time
        if (laneEnableStatus[1] == Active) currentTime[1] = curMillis - startMillis[1];
          else currentTime[1] = 0;
        if (laneEnableStatus[2] == Active) currentTime[2] = curMillis - startMillis[2];
          else currentTime[2] = 0;
        
        for(byte i = 1; i <= laneCount; i++){
          if(laneEnableStatus[i] == Active) {
            currentTime[i] = curMillis - startMillis[i];
          } else {
            currentTime[i] = 0;
          }
        }


        // if tick has passed, then update displays
        if (curMillis - lastTickMillis >  displayTick){
          // A lap Flash is triggered by the completion of a lap, it's 'resting' value is 0
          if (flashStatus[1] > 0) {
            if (flashStatus[1] == 1) {
              raceDataExists = true;
              unsigned long r1LapTimeToLog;
              r1LapTimeToLog = lastXMillis [1] [(lapCount[1]-1) % lapMillisQSize] - lastXMillis [1] [(lapCount[1]-2) % lapMillisQSize];
              // during the intro to the lap flash cycle we also update all the racer lap records
              // we do this here to keep it interruptable and out of the lap trigger intrrupt funct.
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(fastestTimes[1], fastestLaps[1], lapCount[1] - 1, r1LapTimeToLog, laneRacer[1], fastestQSize);
              lc.clearDisplay( led1Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(lapCount[1] - 1, 3, 2, led1Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r1LapTimeToLog, 7, 4, 3, led1Disp);
              // update racer's current lap total on LCD
              PrintNumbers(lapCount[1] - 1, 3, 11, lcdDisp, true, 2);
              // update racer's current best lap time to LCD
              PrintClock(fastestTimes[1][0], 19, 7, 3, lcdDisp, 2);
              // record the timestamp at which this flash period is starting at
              flashStartMillis[1] = curMillis;
              // set the flash state to 2 = hold until flash period time is over
              flashStatus[1] = 2;
            } else { // flash status = 2 which means it's been written and still active
              // if flash time is up, set flag to zero ending display of last lap
              if (curMillis - flashStartMillis[1] > flashDisplayTime) flashStatus[1] = 0;
            }
          } else { // lane flash status = 0, or inactive
            // if the lane is in use then start displaying live lap time again
            if (laneEnableStatus[1] == Active) {
              lc.clearDisplay(led1Disp - 1);
              PrintNumbers(lapCount[1], 3, 2, led1Disp);
              PrintClock(currentTime[1], 7, 4, 1, led1Disp, 0, true);

            }
          }

          if (flashStatus[2] > 0) {
            if (flashStatus[2] == 1) {
              raceDataExists = true;
              unsigned long r2LapTimeToLog;
              r2LapTimeToLog = lastXMillis[2] [(lapCount[2]-1) % lapMillisQSize] - lastXMillis [2][(lapCount[2]-2) % lapMillisQSize];
              // the current lap is the live lap so we need to subtract 1 from lapcount
              UpdateFastestLap(fastestTimes[2], fastestLaps[2], lapCount[2] - 1, r2LapTimeToLog, laneRacer[2], fastestQSize);
              // UpdateFastestLap(topFastestTimes, topFastestLaps, lapCount[2] - 1, r2LapTimeToLog, racer2, 2 * fastestQSize);
              lc.clearDisplay( led2Disp - 1 );
              // print the just completed lap # to left side of racer's  LED
              PrintNumbers(lapCount[2] - 1, 3, 2, led2Disp);
              // print the lap time of just completed lap to right side of racer's LED
              PrintClock(r2LapTimeToLog, 7, 4, 3, led2Disp);
              // update racer's current lap total on lcd
              PrintNumbers(lapCount[2] - 1, 3, 11, lcdDisp, true, 3);
              // update racer's current best lap time to lcd
              PrintClock(fastestTimes[2][0], 19, 7, 3, lcdDisp, 3);
              // record the timestamp at which this flash period is starting at
              flashStartMillis[2] = curMillis;
              // set the flash state to 2 = hold until flash period time is over
              flashStatus[2] = 2;
            } else { // flash status = 2 which means it's been written and still active
              // if flash time is up, set flag to zero ending display of last lap
              if (curMillis - flashStartMillis[2] > flashDisplayTime) flashStatus[2] = 0;
            }
          } else { // lane flash status = 0, or inactive
            // if the lane is in use then start displaying live lap time again
            if (laneEnableStatus[2] == Active) {
              lc.clearDisplay(led2Disp - 1);
              // Print current lap
              PrintNumbers(lapCount[2], 3, 2, led2Disp);
              // Print running lap time
              PrintClock(currentTime[2], 7, 4, 1, led2Disp, 0, true);
            }
          }
          // Update LCD with absolute race time and racer lap logs
          PrintClock(currentTime[0], RACE_CLK_POS, 10, 1, lcdDisp, 0, true);
          lastTickMillis = curMillis;
        } // END of if(displayTick) block

        // check for the race end condition and finishing places
        switch (raceType) {
          case Standard: {
            // ****** STANDARD FINSIH *******************
            // check for a winner
            // After crossing finish of last lap, lap Count will be equal to raceLaps + 1
            // which will trigger the finish of a standard race
            if (lapCount[1] > raceLaps || lapCount[2] > raceLaps) {
              // turn off lap trigger interrupts to prevent any additional lap triggers
              EnablePinInterrupts(false);
              DetermineWinner();
              state = Menu;
              currentMenu = ResultsMenu;
              entryFlag = true;
              // resultsMenuIdx = TopResults;
              resultsMenuIdx = 0;
            }
            break;
          } // END standard race case
          case Timed:{
            // if time <= 0 then race is over and the most laps wins
            if (currentTime[0] <= 0) {
              // turn off lap trigger interrupts to prevent any additional lap triggers
              EnablePinInterrupts(false);
              DetermineWinner();
              state = Menu;
              currentMenu = ResultsMenu;
              entryFlag = true;
              // resultsMenuIdx = TopResults;
              resultsMenuIdx = 0;
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
    } // END of Race state case
    break;

    case Paused:{
      if(buttonPressed(pauseStopPin)) PauseRace();
      if(entryFlag){
        if(laneEnableStatus[1]) PrintText("PAUSE", led1Disp, 7, 5, true, 0, false);
        if(laneEnableStatus[2]) PrintText("PAUSE", led2Disp, 7, 5, true, 0, false);
      }
      // Serial.println("Entered Paused state");      
      char key = keypad.getKey();
      // only if a press is detected or it is the first loop of a new state
      // do we bother to evaluate anything
      if (key == '*') {
        state = Menu;
        currentMenu = ResultsMenu;
        entryFlag = true;
        resultsMenuIdx = 0;
      }
    } // END of Paused state
    break;
    default:{
      // if the state becomes unknown then default back to 'Menu'
      // Serial.println("Entered default state");
      state = Menu;
    }
    break;
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
  lapCount[1] = lapCount[1] == 0 ? 0 : lapCount[1] - 1;
  lapCount[2] = lapCount[2] == 0 ? 0 : lapCount[2] - 1;
  // Serial.println("racer1 laps final");
  // Serial.println(lapCount[1]);
  // Serial.println("racer2 laps final");
  // Serial.println(lapCount[2]);
  // verify winner if one lap count is higher than the other it is clear
  if (lapCount[1] > lapCount[2]) {first = laneRacer[1]; second = laneRacer[2];}
  else if (lapCount[2] > lapCount[1]) {first = laneRacer[2]; second = laneRacer[1];}
  // if timing is such that 2nd place has crossed while processing then the
  // final lap timestamp will show who was first

  else if (lastXMillis[1][ lapCount[1] % lapMillisQSize ] < lastXMillis[2][ lapCount[2] % lapMillisQSize ]) {first = laneRacer[1]; second = laneRacer[2];}

  else if (lastXMillis[2][lapCount[2] % lapMillisQSize] < lastXMillis[1][lapCount[1] % lapMillisQSize]) {first = laneRacer[2]; second = laneRacer[1];}

  else if(lapCount[2] == lapCount[1] && lastXMillis[1][lapCount[1]] == lastXMillis[2][lapCount[2]]){

    // we use 254 as a flag for tie, since it is larger than the racer name list size will ever be
    first = 254;
    second = 254;
  }
  if (first == 254 && second == 254) {
    if(laneEnableStatus[1]) PrintText("A TIE", led1Disp, 7, 8, true);
    if(laneEnableStatus[2]) PrintText("A TIE", led2Disp, 7, 8, true);
  } else if (first == 255) {
    // If first is never changed from default 255 then somehow an error occurred.
    if(laneEnableStatus[1]) PrintText("ERROR", led1Disp, 7, 8);
    if(laneEnableStatus[2]) PrintText("ERROR", led2Disp, 7, 8);
  } else {
    PrintText("1st", first == laneRacer[1] ? led1Disp:led2Disp, 2, 3, false);
    PrintText(Racers[first], first == laneRacer[1] ? led1Disp:led2Disp, 7, 4, true, 0, false);
    // Only if 'All' lanes are enabled, aka =3, aka 0x00000011, is there a 2nd place.
    if(enabledLanes >= 3) {
      PrintText("2nd", second == laneRacer[1] ? led1Disp:led2Disp, 2, 3, false);
      PrintText(Racers[second], second == laneRacer[1] ? led1Disp:led2Disp, 7, 4, true, 0, false);
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
        // Serial.println("inputNumber digit");
        // Serial.println(inputNumber[digits]);
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

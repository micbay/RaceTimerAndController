// Wire library is for I2C communication
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						// main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i2c expander i/o class header
// libraries to support 4 x 4 keypad
#include <Keypad.h>
// library for 7-seg LED Bars
#include <LedControl.h>
// library of sounds
#include "pitches.h"

// note: no '=' or ';' when setting a define
#define DTICK 100
#define NAME_DISPLAY_MAX 8

// ********* AUDIO HARDWARE *********
const byte buzzPin = A3;
int melody[] = {
  NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4
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
// declare lcd object: auto locate & config exapander chip
hd44780_I2Cexp lcd;
// Set display size
// const int LCD_COLS = 20;
// const int LCD_ROWS = 2;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;


// ***** 7-Seg 8-digit LED Bars *****
// LedControl(DataIn, CLK, CS/LOAD, Number of Max chips (ie 8-digit bars))
const byte PIN_TO_LED_DIN = 2;
const byte PIN_TO_LED_CS = 3;
const byte PIN_TO_LED_CLK = 4;
const byte LED_BAR_COUNT = 2; // # of attached max7219 controlled LED bars
const byte LED_DIGITS = 8; // # of digits on each LED bar
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
// establish the row pinouts, {Row1,Row2,Row3,Row4} => Arduino pins 5,6,7,8
byte pin_rows[KP_ROWS] = {5,6,7,8};
// establish the column pinouts, {Col1,Col2,Col3,Col4} => Arduino pins 7,8,9,10
byte pin_column[KP_COLS] = {9,10,11,12}; 
// declare keypad object
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, KP_ROWS, KP_COLS );

// using an enum to make it easier to pass display to write to.
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
// start lanes enum to match value and lane number, All = 0
enum lanes {
  All,
  Lane1,
  Lane2
};
// state that a lane can be in during a race
// enum defaults to these values, but they are written in to avoid editing order error
enum laneState {
  Off = 0,      // lane is not being used
  Active = 1,   // lane is being used and live in a race
  StandBy = 2    // lane is being used in a race, but currently not active
};
// racetypes (standard is 1st to reach lap count)
enum races {
  Standard,  // First to finish the set number of laps
  Timed,     // Finish the most laps before time runs out
  Pole       // 1-10 lap practice time trials
};
// used to set clock time width;
enum clockWidth {
  C,  // .0,
  S,  // 00.0
  M,  // 00:00.0
  H   // 00:00:00.0
};

//*** STORED DEFAULTS and Variables for USER SET GAME PROPERTIES
races raceType = Standard;
int raceTime[2] = {2, 0};
int raceLaps = 5;
// racer#'s value is the index of Racers[] identifying the racer name
int racer1 = 0;
int racer2 = 1;
// create array to hold laps
int R1LapIdx = 0;
int R2LapIdx = 0;
unsigned long R1LapLog[10];
unsigned long R2LapLog[10];
// int currentCursorPos;
lanes enabledLanes = All;
laneState lane1Active;
laneState lane2Active;
// this is # of physical lanes that will have a counter sensor
byte const laneCount = 2;
// pre-race countdown length in seconds
byte preStartCountDown = 8;
// we add one to the lane count to account for 'All'
String laneText[laneCount + 1] = {"All   ", "1 Only", "2 Only"};

int lane1Time = 0;
int lane2Time = 0;
// this flag is used to indicate the first entry into a state so
// that the state can do one time only prep for its first loop
bool entryFlag = true;

// variable to hold the last digit displayed so it does not rewrite every program loop
int ledDigit = 0;

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
//****** Menu String Arrays **********
// String Racers[racerCount] = {
//   "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle BadAss", "Remy", "5318008"
// };
// Racer list
// need to keep a count of the number of names in list becauase
// Strings[] type doesn't have an ability to give an element count.
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
  "Num Laps:",
  "Racetime:   :",
  " D|Lanes:"
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
  "   RACE RESULTS   ",
  "",
  "",
  ""
};
// const char* LiveRaceText[4] = {
//   "",
//   "Racer Fastest Lap",
//   "",
//   ""
// };

// create variale to hold current 'state'
states state = Menu;
Menus currentMenu;


void Beep() {
  tone(buzzPin, 4000, 200);
}


// use this function to set change interrupt for pins among A0-A6
void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
void clearPCI(byte pin) {
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
}

// don't log lap trigger if within debounceTime (ms)
const int debounceTime = 1000;
// ISR is a special Arduino Macro or routine that handles interrupts ISR(vector, attributes)
// PCINT1_vect handles pin change interrupt for the pin block A0-A5, represented in bit0-bit5
ISR (PCINT1_vect) {
  unsigned long logMillis = millis();
  Serial.println("LED Interrupt");
  Serial.println(PINC);
  // Lane 1 is on pin A0 is on bit 0
  if (!IsBitSet(PINC, 0)) {
    if ((logMillis - R1LapLog[R1LapIdx - 1]  > debounceTime) || R1LapIdx == 0){
      R1LapLog[R1LapIdx] = logMillis;
      R1LapIdx++;
      Beep();
      // Serial.println("A0");
      // digitalWrite(ledPIN, HIGH);
    }
  }
  // Lane 2 is on pin A1 is on bit 1
  if (!IsBitSet(PINC, 1)) {
    if ((logMillis - R2LapLog[R1LapIdx - 1]  > debounceTime) || R2LapIdx == 0){
      R2LapLog[R2LapIdx] = logMillis;
      R2LapIdx++;
      Beep();
      // Serial.println("A1");
      // digitalWrite(ledPIN, LOW);
    }
  }
 }  

// left shift the number 1 by the position to check than & with original byte
bool IsBitSet(byte b, int pos) {
   return (b & (1 << pos)) != 0;
}

// update menu with static base text for given menu screen
void UpdateMenu(const char *curMenu[]){
  // for each string in the menu array, print it to the screen
  // clear screen in case new text doesn't cover all old text
  lcd.clear();
  for (int i=0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(curMenu[i]);
  }
}


// Provides, looping, up or down indexing of items in list looping
// This function assumes the start of the list is zero index
int IndexList(int curIdx, int listLength, bool cycleUp = true) {
  int newIndex = curIdx;
  if (cycleUp) {
    // cycling up we need to reset to zero if we reach end of list
    // Modulo (%) of a numerator smaller than its denominator is itself
    // while the modulo of a number with itself is 0
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


// Prints input value to specifiec location on screen
// with leading zeros to fill any gap in the width
void PrintWithLeadingZeros(int integerIN, int width, int cursorPos, displays display, int line){
  int leadingZCount = 0;
  if (integerIN == 0){
    // we subtract 1 because we still print the input integer
    leadingZCount = width - 1;
  } else {
    leadingZCount = width - calcDigits(integerIN);
  }
  if (leadingZCount < 0) leadingZCount = 0;
  lcd.setCursor(cursorPos, line);
  for (int i=0; i < leadingZCount; i++) {
    lcd.print('0');
  }
  lcd.print(integerIN);
  // Serial.println("leading zero count ");
  // Serial.println(leadingZCount);
}

// passing in by Ref (using &) so we can update the input variables directly
void SplitTime(unsigned long curr, unsigned long &ulHour, unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulcent) {
  // Calculate HH:MM:SS from millisecond count
  ulcent = curr % 1000 / 100;
  ulSec = curr / 1000;
  ulMin = ulSec / 60;
  ulHour = ulMin / 60;
  ulMin -= ulHour * 60;
  ulSec = ulSec - ulMin * 60 - ulHour * 3600;
}

// converts clock time into milliseconds
unsigned long SetTime(unsigned long ulHour, unsigned long ulMin,
                      unsigned long ulSec) {
  // Sets the number of milliseconds from midnight to current time
  return (ulHour * 60 * 60 * 1000) +
         (ulMin * 60 * 1000) +
         (ulSec * 1000);
}

// input time in ms, with pos of end of clock placement, and  max clock segment to show
void PrintClock(ulong timeMillis, byte cursorEndPos, clockWidth printWidth, displays display, byte line = 0) {
  unsigned long ulHour;
  unsigned long ulMin;
  unsigned long ulSec;
  unsigned long ulCent;
  SplitTime(timeMillis, ulHour, ulMin, ulSec, ulCent);
  if (printWidth == H) {
    // H 00:00:00.0
    switch (display){
      case lcdDisp: {
        PrintWithLeadingZeros(ulMin, 2, cursorEndPos - 10, display, line);
        lcd.print(":");
        break;
      } // END lcdDisp case
      case led1Disp: led2Disp: {

        break;
      } // END of ledDips cases
      default:
        break;
    } // END of display switch
    printWidth = M;
    } // END printWidth H
  if (printWidth == M) {
    // M 00:00.0
    switch (display){
      case lcdDisp: {
        PrintWithLeadingZeros(ulMin, 2, cursorEndPos - 7, lcdDisp, line);
        lcd.print(":");
        break;
      } // END lcdDisp case
      case led1Disp: case led2Disp: {

        break;
      } // END of ledDips cases
      default:
        break;
    } // END of display switch
    printWidth = S;
  }
  if (printWidth == S) {
    // S 00.0
    switch (display){
      case lcdDisp: {
        PrintWithLeadingZeros(ulSec, 2, cursorEndPos - 4, lcdDisp, line);
        lcd.print(".");
        break;
      } // END lcdDisp case
      case led1Disp: case led2Disp: {

        break;
      } // END of ledDips cases
      default:
        break;
    } // END of display switch
    printWidth = C;
  }
  if (printWidth == C) {
    // C .0
    switch (display){
      case lcdDisp: {
        lcd.setCursor(cursorEndPos - 1, line);
        lcd.print(ulCent);
        break;
      } // END lcdDisp case
      case led1Disp: case led2Disp: {

        break;
      } // END of ledDips cases
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

void WriteName(const byte racer, const displays display, const byte writeSpaceEndPos, const byte width = NAME_DISPLAY_MAX, bool rightJust = false, const byte line = 0, bool clear = true) {
  // Even though we say right justified we still truncate the name from the left.
  // need to track the character index seperately from display digit position
  byte const strLength = strlen(Racers[racer]);
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
        lcd.print(Racers[racer][nameIndex]);
        nameIndex++;
      }
      break;
    }
    case led1Disp: case led2Disp: {
      if (clear) lc.clearDisplay(display - 1);
      for (byte i = cursorStartPos; i <= cursorEndPos; i++){
        // The digit position of the LED bars is right to left so we convert
        lc.setChar(display - 1, (LED_DIGITS-1) - i, Racers[racer][nameIndex], false);
        nameIndex++;
      }
      break;
    } // END LED cases
    default:
      break;
  } // END display switch
}


void ledWriteClockTime(const unsigned long millisTime, const byte ledBarID, const byte endDigitPos = 0) {

}


// The millis() timestamp at start of each timed loop
unsigned long curMillis;
// Live race time in ms, or remaining time in preStart or timed race
unsigned long raceCurTime;
// The millis() timestamp of last displayed timepoint
unsigned long lastTickMillis;
// Live lap time in ms of current lap for each racer
unsigned long r1CurLapTime;
unsigned long r2CurLapTime;
// Interval in milliseconds that the clock displays are updated.
// this value does not affect lap time precision, it's only for display updating
int displayTick = DTICK;
// flag indicating if race state is in preStart countdown or active race
bool preStart = false;

void ledInterrupt() {
  ledState = !ledState;
  digitalWrite(ledPIN, ledState);
  Serial.println("LED Interrupt");
}

bool melodyPlaying = false;
// Holds the time of last tone played so timing of next note in melody can be determined
unsigned long lastNoteMillis;

// Initialize hardware and establish software initial state
void setup(){
  // SETUP LCD DIPSLAY
  int status;
	// initialize (begin) LCD with number of columns and rows: 
	// hd44780 returns a status from begin() that can be used
	// to determine if initalization failed.
	// the actual status codes are defined in <hd44780.h>
	status = lcd.begin(LCD_COLS, LCD_ROWS);
  // non zero status means it was unsuccesful init
  // begin() failed so blink error code using the onboard LED if possible
	if(status) hd44780::fatalError(status);
  // clear the display of any existing content
  lcd.clear();

  // SETUP 7-SEG, 8-DIGIT MAX7219 LED BARS
  //we have already set the number of devices when we created the LedControl
  int devices = lc.getDeviceCount();
  //we have to init all devices in a loop
  for(int deviceID = 0; deviceID < devices; deviceID++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(deviceID, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(deviceID, 8);
    /* and clear the display */
    lc.clearDisplay(deviceID);
  }

  // SETUP LAP TRIGGERS AND BUTTONS
  // roughly equivalent to digitalWrite(lane1Pin, HIGH)
  pinMode(lane1Pin, INPUT_PULLUP);
  pinMode(lane2Pin, INPUT_PULLUP);
  pinMode(pauseStopPin, INPUT_PULLUP);
  // Setup the LED
  pinMode(ledPIN, OUTPUT); 
  digitalWrite(ledPIN, ledState);

  // Setup initial program variables
  currentMenu = MainMenu;
  entryFlag = true;
  
  R1LapLog[0] = 0;
  // open connection on serial port
  Serial.begin(9600);
  Serial.println(currentMenu);
  // Serial.println(SettingsMenu);
}


void loop(){
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
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
        // clear the lap log interrupts
        clearPCI(lane1Pin);
        clearPCI(lane2Pin);
        // All of the menus and sub-menus have been flattened to a single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree
          // Within each menu case is another switch for keys with a response in that menu
          case MainMenu:
            if (entryFlag) {
              UpdateMenu(MainText);
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
              UpdateMenu(SettingsText);
              // draw current minute setting
              lcd.setCursor(10, 2);
              PrintWithLeadingZeros(raceTime[0], 2, 10, lcdDisp, 2);
              // draw current seconds setting
              lcd.setCursor(13, 2);
              PrintWithLeadingZeros(raceTime[1], 2, 13, lcdDisp, 2);
              // draw current lap count
              lcd.setCursor(10, 1);
              PrintWithLeadingZeros(raceLaps, 3, 10, lcdDisp, 1);
              //draw current lane settings
              lcd.setCursor(10,3);
              lcd.print(laneText[enabledLanes]);
              entryFlag = false;
            }
            switch (key) {
              case 'A':
                // change lap count
                raceLaps = enterNumber(3, 999, 1, 10);
                break;
              case 'B':
                // Change minutes
                raceTime[0] = enterNumber(2, 60, 2, 10);
                break;
              case 'C':
                // change seconds
                raceTime[1] = enterNumber(2, 59, 2, 13);
                break;
              case 'D':
                // cycles through active lane options
                enabledLanes = lanes((enabledLanes + 1) % (Lane2 + 1));
                lcd.setCursor(10, 3);
                lcd.print(laneText[enabledLanes]);
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
              UpdateMenu(SelectRacersText);
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
              UpdateMenu(SelectRaceText);
              // add current race lap setting to lap race selection text
              lcd.setCursor(11,0);
              PrintWithLeadingZeros(raceLaps, 3, 11, lcdDisp, 0);
              // add current race minutes setting to timed race screen
              lcd.setCursor(15,1);
              PrintWithLeadingZeros(raceTime[0], 2, 15, lcdDisp, 1);
              // add current race seconds setting to timed race screen
              lcd.setCursor(18,1);
              PrintWithLeadingZeros(raceTime[1], 2, 18, lcdDisp, 1);
              // add the curent preStartCountDown setting to screen
              lcd.setCursor(13,3);
              PrintWithLeadingZeros(preStartCountDown, 2, 13, lcdDisp, 3);
              entryFlag = false;
            }
            switch (key) {
              case 'A': case 'B': case 'C':
                state = Race;
                preStart = true;
                entryFlag = true;
                if ((enabledLanes == All) || (enabledLanes == Lane1)) lane1Active = StandBy;
                if ((enabledLanes == All) || (enabledLanes == Lane2)) lane2Active = StandBy;
                if (key == 'A') {raceType = Standard; displayTick = DTICK;}
                else if (key == 'B') {raceType = Timed; displayTick = -DTICK;}
                else {raceType = Pole; displayTick = DTICK;};
                break;
              case 'D':
                // change how long the preStartCountDown before the race start lasts
                preStartCountDown = enterNumber(2, 30, 3, 13);
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
              UpdateMenu(ResultsText);
              R1LapIdx = 0;
              R2LapIdx = 0;
              lcdClearLine(1);
              lcd.setCursor(5, 1);;
              lcd.print(R1LapLog[R1LapIdx]);
              lcdClearLine(3);
              lcd.setCursor(5, 3);
              lcd.print(R2LapLog[R2LapIdx]);
              entryFlag = false;
            }
            switch (key) {
              // cycle up or down the lap list
              case 'A': case 'B':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(1);
                lcd.setCursor(5, 1);
                if (key == 'A') R1LapIdx--;
                if (key == 'B') R1LapIdx++;
                lcd.print(R1LapLog[R1LapIdx]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(3);
                lcd.setCursor(5, 3);
                if (key == 'C') R2LapIdx--;
                if (key == 'D') R2LapIdx++;
                lcd.print(R2LapLog[R2LapIdx]);
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
          } // END of ResultsMenu Case
          default:
            break;
        }; // end of Menu switch
        // Serial.println(key);
        // Serial.println(currentMenu);
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
        PrintClock(raceCurTime, 12, S, lcdDisp, 2);
        WriteName(racer1, led1Disp, 7, 8, true);
        WriteName(racer2, led2Disp, 7, 8, false);
        // WriteName(racer2, led2Disp, 7, 8);
        entryFlag = false;
        R1LapIdx = 0;
        R2LapIdx = 0;
      }
      // initialize variables for race timing loop first cycle only
      if (entryFlag & !preStart) {
        raceCurTime = 0;
        lastTickMillis = curMillis;
        entryFlag = false;
        lcd.clear();
        lcd.setCursor(8,1);
        lcd.print("Lap  |  Best");
        WriteName(racer1, lcdDisp, 7, 8, false, 2);
        lcd.print(":");
        WriteName(racer2, lcdDisp, 7, 8, false, 3);
        lcd.print(":");
        WriteName(racer1, led1Disp, 7, 8, true);
        WriteName(racer2, led2Disp, 7, 8, false);
        // enable interrupts on lap sensor pins
        // this act enables the racer to trigger first lap
        pciSetup(A0);
        pciSetup(A1);
        // enable race pause button interrupt
        pciSetup(A2);
      }

      if (preStart) {
        if (raceCurTime > 0){
          // update countdown on LCD every 10 millis
          if (curMillis - lastTickMillis > DTICK){
            raceCurTime = raceCurTime - DTICK;
            PrintClock(raceCurTime, 12, S, lcdDisp, 2);
            lastTickMillis = curMillis;
          }
          // In last 3 seconds send countdown to LEDs
          if (raceCurTime < 2999 & (raceCurTime/1000 + 1) != ledDigit) {
            // we add 1 because it should change on start of the digit not end
            ledDigit = raceCurTime/1000 + 1;
            ledWriteDigits(ledDigit);
          }
        } else {
          // ++++ START RACE +++++++
          preStart = false;
          // reset ledDigit to default for next race
          ledDigit = 0;
          // reset entry flag to true so race timing variables can be initialized
          entryFlag = true;
        }

      } else { // ********* LIVE RACE **********
        // Regardless of race type, we do these things
        if (curMillis - lastTickMillis > DTICK){
          // if racetype is timed then displayTick will have been set to negative
          raceCurTime = raceCurTime + displayTick;
          // Update the lap times for active lanes
          if (lane1Active) r1CurLapTime = r1CurLapTime + displayTick;
          if (lane2Active) r2CurLapTime = r2CurLapTime + displayTick;
          // Update LCD with absolute race time and racer lap logs
          PrintClock(raceCurTime, 13, M, lcdDisp, 0);
          // if racer just logged a new lap then display the previous lap time to LEDs
          // update display with current race clock time
          // if (lane1LapFlash) {
          //   ledWriteClockTime();
          // } else {
            
          // }
          lastTickMillis = curMillis;
        }
        switch (raceType) {
          case Standard: {
            if (R1LapIdx >= raceLaps) {
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


// this function is used to monitor user input
// it takes in the current state, number of characters and returns the reulstin int
int enterNumber(int digits, int maxValue, int line, int cursorPos){
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

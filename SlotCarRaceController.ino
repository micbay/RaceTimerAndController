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
byte preStartCountDown = 3;
// we add one to the lane count to account for 'All'
String laneText[laneCount + 1] = {"All   ", "1 Only", "2 Only"};

int lane1Time = 0;
int lane2Time = 0;
// this flag is used to indicate the first entry into a state so
// that the state can do one time only prep for its first loop
bool entryFlag = true;

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

// create variale to hold current 'state'
states state = Menu;
Menus currentMenu;



// use this function to set change interrupt for pins among A0-A6
void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
// handle pin change interrupt for A0-A5 bit0-bit5
ISR (PCINT1_vect) {
  unsigned long logMillis = millis();
  Serial.println("LED Interrupt");
  Serial.println(PINC);
  // Lane 1 is on pin A0 is on bit 0
  if (!IsBitSet(PINC, 0)) {
    R1LapLog[R1LapIdx] = logMillis;
    R1LapIdx++;
    // digitalWrite(ledPIN, HIGH);
    Serial.println("A0");
    // LogLapLane1(logMillis);
  }
  // Lane 2 is on pin A1 is on bit 1
  if (!IsBitSet(PINC, 1)) {
    R2LapLog[R2LapIdx] = logMillis;
    R2LapIdx++;
    // digitalWrite(ledPIN, LOW);
    Serial.println("A1");
    // LogLapLane2(logMillis);
  }
 }  

// left shift the number 1 by the position to check than & with original byte
bool IsBitSet(byte b, int pos) {
   return (b & (1 << pos)) != 0;
}

void LogLapLane1() {

}

void LogLapLane2() {

}



void UpdateMenu(char *curMenu[]){
  // depending on the current menu map key press to next menu
  // for each string in the menu array, print it to the screen
  // clear screen in case new text doesn't cover all old text
  lcd.clear();
  for (int i=0; i<4; i++){
    lcd.setCursor(0,i);
    lcd.print(curMenu[i]);
  }
}


// indexing function to provide loop back to start for cycling
int IndexRacer(int curIdx, int listCount, bool cycleUp = true) {
  int newIndex = curIdx;
  if (cycleUp) {
    if (curIdx == listCount - 1){
      newIndex = 0;
    } else {
      newIndex++;
    }
  } else {
    if (curIdx == 0){
      newIndex = listCount - 1;
    } else {
      newIndex--;
    }
  }
  return newIndex;
}


// Prints input value to specifiec location on screen
// with leading zeros to fill any gap in the width
void PrintWithLeadingZeros(int integerIN, int width, int cursorPos, int line){
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
void SplitTime(unsigned long curr, unsigned long &ulHour,
               unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulcent) {
  // Calculate HH:MM:SS from millisecond count
  ulcent = curr % 1000 / 100;
  ulSec = curr / 1000;
  ulMin = ulSec / 60;
  ulHour = ulMin / 60;
  ulMin -= ulHour * 60;
  ulSec = ulSec - ulMin * 60 - ulHour * 3600;
}

// // converts clock time into milliseconds
// unsigned long SetTime(unsigned long ulHour, unsigned long ulMin,
//                       unsigned long ulSec) {
//   // Sets the number of milliseconds from midnight to current time
//   return (ulHour * 60 * 60 * 1000) +
//          (ulMin * 60 * 1000) +
//          (ulSec * 1000);
// }

// input time in millis, pos of end of clock placement, and digits to show
void lcdPrintClock(ulong timeMillis, byte cursorEndPos, byte line, clockWidth printWidth) {
  unsigned long ulHour;
  unsigned long ulMin;
  unsigned long ulSec;
  unsigned long ulCent;
  SplitTime(timeMillis, ulHour, ulMin, ulSec, ulCent);
  if (printWidth == H) {
    // H 00:00:00.0
    PrintWithLeadingZeros(ulHour, 2, cursorEndPos - 10, line);
    lcd.print(":");
    printWidth = M;
  }
  if (printWidth == M) {
    // M 00:00.0
    PrintWithLeadingZeros(ulMin, 2, cursorEndPos - 7, line);
    lcd.print(":");
    printWidth = S;
  }
  if (printWidth == S) {
    // S 00.0
    PrintWithLeadingZeros(ulSec, 2, cursorEndPos - 4, line);
    lcd.print(".");
    printWidth = C;
  }
  if (printWidth == C) {
    // C .0
    // PrintWithLeadingZeros(ulCent, 1, cursorEndPos - 2, line);
    lcd.setCursor(cursorEndPos-1, line);
    lcd.print(ulCent);
  }
    // Serial.println("ulMin");
    // Serial.println(ulMin);
    // Serial.println("ulSec");
    // Serial.println(ulSec);
    // Serial.println("ulcent");
    // Serial.println(ulcent);
}


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

/* Display a (hexadecimal) digit on a 7-Segment Display
 * Params:
 *  addr   address of the display
 *  digit  the position of the digit on the display (0..7)
 *  value  the value to be displayed. (0x00..0x0F)
 *  dp     sets the decimal point.  
*/ 
// int delaytime = 1000;
// void scrollDigits() {
//   for(int i=0;i<13;i++) {
//     lc.setDigit(0, 3, i, false);
//     lc.setDigit(0, 2, i + 1, false);
//     lc.setDigit(0, 1, i + 2, false);
//     lc.setDigit(0, 0, i + 3, false);
//     delay(delaytime);
//     lc.setChar(0, 0, 'a', false);
//     delay(delaytime);
//   lc.setRow(0, 0, 0x05);
//   }
//   lc.clearDisplay(0);
//   delay(delaytime);
// }

int ledDigit = 0;
void ledWriteDigits(byte digit) {
  for (int j=0; j < LED_BAR_COUNT; j++){
    for (int i=0; i < LED_DIGITS; i++){
      lc.setDigit(j, i, digit, false);
    }
  }
}

void ledWriteName(byte ledBarID, byte racer, bool clear = true) {
  // Serial.println("ledWriteName");
  // if flag true, clear all digits of existing characters
  if (clear) lc.clearDisplay(ledBarID);
  byte digitPos = strlen(Racers[racer])-1;
  for (int i = 0; i < strlen(Racers[racer]); i++){
    lc.setChar(ledBarID, i, Racers[racer][digitPos], false);
    digitPos--;
  }
}


unsigned long curMillis;  // the snapshot of the millis() at entry cycle
unsigned long millisTillStart;
bool preStart;
unsigned long raceCurMillis;
unsigned long lastTickMillis;
unsigned long r1LapMillis;
unsigned long r2LapMillis;
// this is the interval in milliseconds that the clock displays are updated.
// this value does not affect lap record accuracy, it's on for display updating
byte displayTick = 100;

void ledInterrupt() {
  ledState = !ledState;
  digitalWrite(ledPIN, ledState);
  Serial.println("LED Interrupt");
}

bool melodyPlaying = false;

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
  // attach 
  pciSetup(A0);
  pciSetup(A1);
  pciSetup(A2);
  // Setup initial program variables
  currentMenu = MainMenu;
  entryFlag = true;
  
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
        tone(buzzPin, 4000, 200);
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
              PrintWithLeadingZeros(raceTime[0], 2, 10, 2);
              // draw current seconds setting
              lcd.setCursor(13, 2);
              PrintWithLeadingZeros(raceTime[1], 2, 13, 2);
              // draw current lap count
              lcd.setCursor(10, 1);
              PrintWithLeadingZeros(raceLaps, 3, 10, 1);
              //draw current lane settings
              lcd.setCursor(10,3);
              lcd.print(laneText[enabledLanes]);
              entryFlag = false;
            }
            switch (key) {
              case 'B':
                // Change minutes
                raceTime[0] = enterNumber(2, 60, 2, 10);
                break;
              case 'C':
                // change seconds
                raceTime[1] = enterNumber(2, 59, 2, 13);
                break;
              case 'A':
                // change lap count
                raceLaps = enterNumber(3, 999, 1, 10);
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
                racer1 = IndexRacer(racer1, racerCount, key == 'A');
                
                // // cycles through active lane options
                // if(key == 'A') Racers[(racer1 + 1) % (racerCount + 1)];
                // if(key == 'B') Racers[(racer1 - 1) % (racerCount - 1)];

                lcd.print(Racers[racer1]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                lcdClearLine(3, nameCursorPos);
                lcd.setCursor(nameCursorPos,3);
                racer2 = IndexRacer(racer2, racerCount, key == 'D');
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
          // "A|First to     Laps",
          // "B|Most Laps in",
          // "C|Start Pol Trials",
          // "D|Countdown:"
          case SelectRaceMenu: {
            if (entryFlag) {
              //draw non-editable text
              UpdateMenu(SelectRaceText);
              // add current race lap setting to lap race selection text
              lcd.setCursor(11,0);
              PrintWithLeadingZeros(raceLaps, 3, 11, 0);
              // add current race minutes setting to timed race screen
              lcd.setCursor(15,1);
              PrintWithLeadingZeros(raceTime[0], 2, 15, 1);
              // add current race seconds setting to timed race screen
              lcd.setCursor(18,1);
              PrintWithLeadingZeros(raceTime[1], 2, 18, 1);
              // add the curent preStartCountDown setting to screen
              lcd.setCursor(13,3);
              PrintWithLeadingZeros(preStartCountDown, 2, 13, 3);
              entryFlag = false;
            }
            switch (key) {
              case 'A': case 'B': case 'C':
                // set the global raceType to a Standard race
                // Typical to car racing, first to X laps
                state = Race;
                preStart = true;
                entryFlag = true;
                if ((enabledLanes == All) || (enabledLanes == Lane1)) lane1Active = StandBy;
                if ((enabledLanes == All) || (enabledLanes == Lane2)) lane2Active = StandBy;
                if (key == 'A') raceType = Standard;
                else if (key == 'B') raceType = Timed;
                else raceType = Pole;
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
        millisTillStart = preStartCountDown * 1000;
        lastTickMillis = curMillis;
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Your Race Starts in:");
        lcdPrintClock(millisTillStart, 12, 2, S);
        entryFlag = false;
        R1LapIdx = 0;
        R2LapIdx = 0;
      }

      // initialize variables for race timing loop first cycle only
      if (entryFlag & !preStart) {
        raceCurMillis = 0;
        lastTickMillis = curMillis;
        ledWriteName(0, racer1);
        ledWriteName(1, racer2);
        entryFlag = false;
        lcdClearLine(2);
      }

      if (preStart) {
        if (millisTillStart > 0){
          // update countdown on LCD every 10 millis
          if (curMillis - lastTickMillis > 10){
            millisTillStart = millisTillStart - 10;
            lcdPrintClock(millisTillStart, 12, 2, S);
            lastTickMillis = curMillis;
          }
          // 9 and under sec left then send countdown to LEDs
          if (millisTillStart < 8999 & (millisTillStart/1000 + 1) != ledDigit) {
            // we add 1 because it should change on start of the digit not end
            ledDigit = millisTillStart/1000 + 1;
            ledWriteDigits(ledDigit);
          }
        } else {
          // Start race, the next lap count start that racers lap.
          lcd.setCursor(7, 3);
          lcd.print("START!");
          preStart = false;
          // reset ledDigit to default for next race
          ledDigit = 0;
          // reset entry flag to true so race timing variables can be initialized
          entryFlag = true;
        }

      } else {
        switch (raceType) {
          case Standard: {   
            if (curMillis - lastTickMillis > displayTick){
              raceCurMillis = raceCurMillis + displayTick;
              lcdPrintClock(raceCurMillis, 13, 2, M);
              if (lane1Active) r1LapMillis = r1LapMillis + displayTick;
              if (lane2Active) r2LapMillis = r2LapMillis + displayTick; 
              lastTickMillis = curMillis;
            }
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

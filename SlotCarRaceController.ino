// Wire library is for I2C communication
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						// main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i2c expander i/o class header
// libraries to support 4 x 4 keypad
#include <Keypad.h>
// library for 7-seg LED Bars
#include <LedControl.h>
// for creating debounce object for button press detection
#include <Bounce2.h>


//***** Setting Up Lap Triggers and Pause-Stop button *********
const byte lane1Pin = PIN_A1;
const byte lane2Pin = PIN_A3;
const byte pauseStopPin = PIN_A2;
const byte ledPIN = 13;
int ledState = HIGH;
bool statusPauseStop = false;
Bounce lane1 = Bounce();   // Define Bounce to read StartStop switch
Bounce lane2 = Bounce();   // Define Bounce to read Lapse switch
Bounce pauseStop = Bounce();   // Define Bounce to read Lapse switch

// use this function to set change interrupt for pins among A0-A6
void pciSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}
// handle pin change interrupt for A0 to A5 here
ISR (PCINT1_vect) {
  Serial.println("LED Interrupt");
  Serial.println(PINC);
  if (!IsBitSet(PINC, 1)) {
    digitalWrite(ledPIN, HIGH);
    Serial.println("LED A2");
  }
  else if (!IsBitSet(PINC, 3)) {
    digitalWrite(ledPIN, LOW);
    Serial.println("A3");
  }
 }  

// left shift the number 1 by the position to check than & with original byte
bool IsBitSet(byte b, int pos) {
   return (b & (1 << pos)) != 0;
}


//***** Variables for LCD 4x20 Display **********
// declare lcd object: auto locate & config exapander chip
hd44780_I2Cexp lcd;
// Set display size
// const int LCD_COLS = 20;
// const int LCD_ROWS = 2;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

// ***** 7-Seg 8-digit LED Bars *****
//  pin 2 is connected to the DataIn
//  pin 4 is connected to the CLK
//  pin 3 is connected to LOAD
// LedControl(DataIn, CLK, CS/LOAD, Number of Max chips (ie 8-digit bars))
LedControl lc = LedControl(2, 4, 3, 2);



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


// **** Race Variables ****

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
// racetypes (standard is 1st to reach lap count)
enum races {
  Standard,
  Timed,
  Pole
};

// race time will be kept as an integer array of [minutes, seconds]
// race time's default is the starting time of a timed race
// Variables that hold the set values (and default for startup)
//*** STORED DEFAULTS and Variables for USER SET GAME PROPERTIES
// Racer list
// need to keep a count of the number of names in list becauase
// Strings[] type doesn't have an ability to give an element count.
byte const racerCount = 8;
String Racers[racerCount] = {
  "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle BadAss", "Remy", "5318008"
};
races raceType = Standard;
int raceTime[2] = {2, 0};
int raceLaps = 500;
// racer#'s value is the index of Racers[] identifying the racer
int racer1 = 0;
int racer2 = 1;
int currentCursorPos;
int activeLane = All;
// this is # of physical lanes that will have a counter sensor
byte const laneCount = 2;
// pre-race countdown length in seconds
byte preStartCountDown = 5;
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
               unsigned long &ulMin, unsigned long &ulSec, unsigned long &ulhunds) {
  // Calculate HH:MM:SS from millisecond count
  ulhunds = curr % 1000;
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


void PrintClockTime(ulong timeMillis, byte cursorPos, byte line){
  unsigned long ulHour;
  unsigned long ulMin;
  unsigned long ulSec;
  unsigned long ulhunds;
  SplitTime(timeMillis, ulHour, ulMin, ulSec, ulhunds);
  lcd.setCursor(cursorPos, line);
  lcd.print(ulMin);
  lcd.print(":");
  lcd.print(ulSec);
  lcd.print(":");
  lcd.print(ulhunds);
    Serial.println("ulMin");
    Serial.println(ulMin);
    Serial.println("ulSec");
    Serial.println(ulSec);
    Serial.println("ulhunds");
    Serial.println(ulhunds);
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
void clearLine(byte lineNumber, byte posStart = 0, byte posEnd = LCD_COLS) {
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
int delaytime = 1000;
void scrollDigits() {
  for(int i=0;i<13;i++) {
    lc.setDigit(0, 3, i, false);
    lc.setDigit(0, 2, i + 1, false);
    lc.setDigit(0, 1, i + 2, false);
    lc.setDigit(0, 0, i + 3, false);
    delay(delaytime);

    lc.setChar(0, 0, 'a', false);
    delay(delaytime);

  lc.setRow(0, 0, 0x05);
  }
  lc.clearDisplay(0);
  delay(delaytime);
}


unsigned long curMillis;  // the snapshot of the millis() at entry cycle
unsigned long millisTillStart;
bool preStart;
unsigned long preStartMillis;
unsigned long raceStartMillis;
unsigned long raceCurMillis;
int raceCurTime[3];

void ledInterrupt() {
  ledState = !ledState;
  digitalWrite(ledPIN, ledState);
  Serial.println("LED Interrupt");
}


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
  // // Attach the debouncer to a pin with INPUT_PULLUP mode
  // lane1.attach(lane1Pin, INPUT_PULLUP);
  // lane2.attach(lane2Pin, INPUT_PULLUP);
  // pauseStop.attach(pauseStopPin, INPUT_PULLUP);
  // // Use a debounce interval of 25 milliseconds 
  // lane1.interval(25);
  // lane2.interval(25);
  // pauseStop.interval(25);

  pinMode(lane1Pin, INPUT_PULLUP); // roughly equivalent to digitalWrite(lane1Pin, HIGH)
  pinMode(lane2Pin, INPUT_PULLUP);
  pinMode(pauseStopPin, INPUT_PULLUP);
  // test LED on pin13
  pinMode(ledPIN, OUTPUT); // Setup the LED
  digitalWrite(ledPIN, ledState); // Turn on the LED
  attachInterrupt(digitalPinToInterrupt(lane1Pin), ledInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(lane2Pin), ledInterrupt, CHANGE);

  pciSetup(A1);
  pciSetup(A2);
  pciSetup(A3);
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
              lcd.print(laneText[activeLane]);
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
                if (activeLane == laneCount){
                  activeLane = 0;
                } else {
                  activeLane++;
                }
                lcd.setCursor(10, 3);
                lcd.print(laneText[activeLane]);
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
                clearLine(1, nameCursorPos);
                lcd.setCursor(nameCursorPos,1);
                racer1 = IndexRacer(racer1, racerCount, key == 'A');
                lcd.print(Racers[racer1]);
                break;
              }
              // cycle up or down racer list and update Racer2 name
              case 'C': case 'D':{
                // clear line to remove extra ch from long names replace by short ones
                clearLine(3, nameCursorPos);
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
            }          
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
              case 'A':
                // set the global raceType to a Standard race
                // Typical to car racing, first to X laps
                state = Race;
                raceType = Standard;
                preStart = true;
                entryFlag = true;
                break;
              case 'B':
                // set the global raceType to a timed race
                // most laps before time runs out
                state = Race;
                raceType = Timed;
                preStart = true;
                entryFlag = true;
                break;
              case 'C':
                // set the global raceType to Pole position trials
                state = Race;
                raceType = Pole;
                preStart = true;
                entryFlag = true;
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
            break;
          } // END of ResultsMenu Case
          default:
            break;
        }; // end of Menu switch
        // Serial.println(key);
        // Serial.println(currentMenu);
      }; // end of if key pressed wrap
      break; // END of Menu State 
    }
// unsigned long millisTillStart;
// bool preStart;
// long raceStartMillis;
// long raceCurTime;
// long raceLiveTime;
    case Race:{
      curMillis = millis();
      if (entryFlag) {
        millisTillStart = preStartCountDown * 1000;
        preStartMillis = curMillis;
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print("Your Race Starts in:");
        // lcd.setCursor(8,2);
        PrintClockTime(millisTillStart, 8, 2);
        // PrintWithLeadingZeros(millisTillStart, 2, 8, 2);
        preStart = true;
        entryFlag = false;
      }
      // if before race start, run preStart countdown
      if (preStart) {
        if (curMillis - preStartMillis > 0){
          if (curMillis - preStartMillis > 10){
            PrintClockTime(millisTillStart, 8, 2);
            preStartMillis = curMillis;
              Serial.println("curMillis");
              Serial.println(curMillis);
              Serial.println("millistillstart");
              Serial.println(millisTillStart);
              Serial.println("preStartMillis");
              Serial.println(preStartMillis);
          }
        } else {
          // Start race, the next lap count start that racers lap.
          preStart = false;
        }

      } else {
        switch (raceType) {
          case Standard:
            scrollDigits();
            break;
          case Timed:

            break;
          case Pole:

            break;
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

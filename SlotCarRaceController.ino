
// Wire library is for I2C communication
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						// main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i2c expander i/o class header
// libraries to support 4 x 4 keypad
#include <Keypad.h>



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

//***** Declare Display variables *****
// declare lcd object: auto locate & config exapander chip
hd44780_I2Cexp lcd;
// Set display size
// const int LCD_COLS = 20;
// const int LCD_ROWS = 2;
const int LCD_COLS = 20;
const int LCD_ROWS = 4;


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
  Poles
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
int raceLaps = 5;
// racer#'s value is the index of Racers[] identifying the racer
int racer1 = 0;
int racer2 = 1;
int currentCursorPos;
int activeLane = All;
// this is # of physical lanes that will have a counter sensor
byte const laneCount = 2;
byte countdown = 5;
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
  "A Select Racers",
  "B Change Settings",
  "C Select a Race",
  "D See Results"
};
// Main Menu's sub-Menus
const char* SettingsText[4] = {
  " A-Lap  B-Min  C-Sec",
  "Num Laps:",
  "Racetime:   :",
  " D-Lanes:"
};
const char* SelectRacersText[4] = {
  "R1: A Prev, B Next",
  " ",
  "R2: C Prev, D Next",
  " "
};
const char* SelectRaceText[4] = {
  "A Start     Lap Race",
  "B Start Timed Race",
  "C Start Pol Trials",
  "D Countdown:"
};
const char* ResultsText[4] = {
  "     RACE RESULTS",
  "",
  "",
  "*   <-- Return"
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

// Prints input value to specifiec location on screen
// with leading zeros to fill any gap in the width
void PrintWithLeadingZeros(byte integerIN, byte width, byte cursorPos, byte line){
  byte leadingZCount = 0;
  if (integerIN == 0){
    // we subtract 1 because we still print the input integer
    leadingZCount = width - 1;
  } else {
    leadingZCount = width - calcDigits(integerIN);
  }
  if (leadingZCount < 0) leadingZCount = 0;
  lcd.setCursor(cursorPos, line);
  for (byte i=0; i < leadingZCount; i++) {
    lcd.print('0');
  }
  lcd.print(integerIN);
  // Serial.println("leading zero count ");
  // Serial.println(leadingZCount);
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


// Initialize hardware and establish software initial state
void setup(){
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
  currentMenu = MainMenu;
  entryFlag = true;
  
  // open connection on serial port
  Serial.begin(9600);
  Serial.println(currentMenu);
  // Serial.println(SettingsMenu);
}


void loop(){
  // delay(2);
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
  switch (state) {
    // Serial.println("Entered Stat Switch");
    // In the 'Menu' state the program is focused on looking for keypad input
    // then using that keypad input to navigate the menu tree
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

          case SelectRacersMenu:
            if (entryFlag) {
              UpdateMenu(SelectRacersText);
              clearLine(1);
              lcd.setCursor(1,1);
              lcd.print(Racers[racer1]);
              clearLine(3);
              lcd.setCursor(1,3);
              lcd.print(Racers[racer2]);
              entryFlag = false;
            }
            switch (key) {
              case 'A': case 'B':
                clearLine(1);
                lcd.setCursor(1,1);
                racer1 = IndexRacer(racer1, racerCount, key == 'A');
                lcd.print(Racers[racer1]);
                break;
              case 'C': case 'D':
                clearLine(3);
                lcd.setCursor(1,3);
                racer2 = IndexRacer(racer2, racerCount, key == 'D');
                lcd.print(Racers[racer2]);
                break;
              case '*':
                // return to previous screen; MainMenu
                currentMenu = MainMenu;
                entryFlag = true;
                // UpdateMenu(MainText);
                break;
              default:
                break;
            }              
            break;

          // "A Start     Lap Race",
          // "B Start Timed Race",
          // "C Start Pol Trials",
          // "D Countdown:"
          case SelectRaceMenu:
            if (entryFlag) {
              //draw non-editable text
              UpdateMenu(SelectRaceText);
              // add current race lap setting to lap race selection text
              lcd.setCursor(8,0);
              lcd.print(raceLaps);
              // add the curent countdown setting to screen
              lcd.setCursor(13,3);
              PrintWithLeadingZeros(countdown, 2, 13, 3);
              entryFlag = false;
            }
            switch (key) {
              case 'A':
                // set the global raceType to a Standard race, first to X laps
                state = Race;
                raceType = Standard;
                entryFlag = true;
                Serial.println("leaving Menu state");
                // lcd.print(raceType);
                // lcd.cursor();
                break;
              case 'B':

                break;
              case 'C':

                break;
              case 'D':
                // change lap count
                countdown = enterNumber(2, 30, 3, 13);
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

          case ResultsMenu:
            break;

          default:
            break;
        }; // end of Menu switch

        // Serial.println(key);
        // Serial.println(currentMenu);

      }; // end of if key pressed wrap
      break; // case of state = Menu 
    }
    case Race:{
      // Serial.println("Entered Race state");
      // racetypes
      lcd.clear();
      lcd.setCursor(1,1);
      lcd.print(raceType);
      lcd.cursor();
      switch (raceType) {
        case Standard:
          lcd.setCursor(5,2);
          lcd.print("Standard Race");
          break;
        case Timed:

          break;
        case Poles:

          break;
        default:
          break;
      }
      break;
    }
    case Paused:{
      // Serial.println("Entered Paused state");

      break;
    }
    default:{
      // if the state becomes unknown then default back to 'Menu'
      Serial.println("Entered default state");
      state = Menu;
      break;
    }
  } // END of States Switch

} // END of MAIN LOOP

// clear line on display by writing a space for each row
void clearLine(int lineNumber) {
  lcd.setCursor(0,lineNumber);
  for(int n = 0; n < LCD_COLS; n++){
    lcd.print(" ");
  }
}

// this function is used to monitor user input
// it takes in the current state, number of characters and returns the reulstin int
int enterNumber(int digits, int maxValue, int line, int cursorPos){

  char inputNumber[digits];
  bool done = false;
  char keyIN;
  int returnNumber = 0;
  const int startCursorPos = cursorPos;

  lcd.setCursor(cursorPos, line);
  lcd.cursor();
  // clear number entry space then reset cursor to beginning of entry.
  for (int i=0; i<digits; i++){
    lcd.print(' ');
  };
  lcd.setCursor(cursorPos, line);

  while (!done){
    keyIN = keypad.getKey();
    switch (keyIN) {
      case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '0':
        // if number is entered then append it to screen
        lcd.setCursor(cursorPos, line);
        lcd.cursor();
        lcd.print(keyIN);
        cursorPos++;
        digits--;
        inputNumber[digits] = keyIN;
        break;
      default:
        break;
    }
    if (digits == 0){
      done = true;
      lcd.noCursor();
    }
  }

  returnNumber = 0;
  for (int i = 0; i < sizeof(inputNumber); i++) {
   // subtracting a '0' character from another character converts it into an int
    returnNumber = (inputNumber[i]-'0') * pow(10, i) + returnNumber;
  }

  if (returnNumber > maxValue) {
    returnNumber = maxValue;
    lcd.setCursor(startCursorPos, line);
    if (maxValue < 10) lcd.print("0");
    lcd.print(maxValue);
  }
  // lcd.setCursor(0,3);
  // clearLine(3);
  // lcd.print(returnNumber);
  return returnNumber;
}

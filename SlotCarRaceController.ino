
// Wire library is for I2C communication
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						// main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i2c expander i/o class header
// libraries to support 4 x 4 keypad
#include <Keypad.h>


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
// Racer list
String Racers[] = {
  "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle BadAss", "Remy", 
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
// race time will be kept as an integer array of [minutes, seconds]
// race time's default is the starting time of a timed race
int raceTime[2] = {2, 0};
int raceLaps = 5;
int lane1Time = 0;
int lane2Time = 0;
String racer1 = "Racer 1";
String racer2 = "Racer 2";
int currentCursorPos;
int activeLane = All;
byte const laneCount = 2;
String laneText[laneCount + 1] = {"All   ", "1 Only", "2 Only"};

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
  "A Change Settings",
  "B Select Racers",
  "C Select a Race",
  "D See Results"
};
// Main Menu's sub-Menus
const char* SettingsText[4] = {
  " A-Min  B-Sec  C-Lap",
  "Racetime: 00:00",
  "Num Laps: 025",
  " D-Lanes: All"
};
const char* SelectRacersText[4] = {
  "R1: A Prev, B Next",
  " ",
  "R2: C Prev, D Next",
  " "
};
const char* SelectRaceText[4] = {
  "A Start Timed Race",
  "B Start Lap Race",
  "C Start Pol Trials",
  "*   <-- Return"
};
const char* ResultsText[4] = {
  "     RACE RESULTS",
  "",
  "",
  "*   <-- Return"
};

// create variale to hold current 'state'
states state = Menu;


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


Menus currentMenu;

// Initialize hardware and establish software initial state
void setup(){
  int status;
	// initialize (begin) LCD with number of columns and rows: 
	// hd44780 returns a status from begin() that can be used
	// to determine if initalization failed.
	// the actual status codes are defined in <hd44780.h>
	status = lcd.begin(LCD_COLS, LCD_ROWS);
  // non zero status means it was unsuccesful init
	if(status) 
	{
		// begin() failed so blink error code using the onboard LED if possible
		hd44780::fatalError(status); // does not return
	}
  // clear the display of any existing content
  lcd.clear();
  // open connection on serial port
  Serial.begin(9600);
  currentMenu = MainMenu;
  UpdateMenu(MainText);
  
          Serial.println(currentMenu);
          Serial.println(SettingsMenu);
}


void loop(){

  switch (state) {
    // In the 'Menu' state the program is focused on looking for keypad input
    // then using that keypad input to navigate the menu tree
    case Menu:

      char key = keypad.getKey();
      // only if a press is detected do we bother to evaluate anything
      if (key) {
        // All of the menus and sub-menus have been flattened to a single switch
        switch (currentMenu) {
          // The 'MainMenu' is the default and top level menu in the menu tree
          // Within each menu case is another switch for keys with a response in that menu
          case MainMenu:
            switch (key) {
              case 'A':
                currentMenu = SettingsMenu;
                // Serial.println("set" + SettingsMenu);
                UpdateMenu(SettingsText);
                break;
              case 'B':
                currentMenu = SelectRacersMenu;
                UpdateMenu(SelectRacersText);
                break;
              case 'C':
                currentMenu = SelectRaceMenu;
                UpdateMenu(SelectRaceText);
                break;
              case 'D':
                currentMenu = ResultsMenu;
                UpdateMenu(ResultsText);
              default:
                break;
            }
            break;
          case SettingsMenu:
            switch (key) {
              case 'A':
                // Change minutes
                raceTime[0] = enterNumber(2, 60, 1, 10);
                break;
              case 'B':
                // change seconds
                raceTime[1] = enterNumber(2, 59, 1, 13);
                break;
              case 'C':
                // change seconds
                raceLaps = enterNumber(3, 999, 2, 10);
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
                // lcd.print(activeLane);
                break;
              case '*':
                // return to previous screen; SettingsMenu
                currentMenu = MainMenu;
                UpdateMenu(MainText);
                break;
              default:
                break;
            }
            break;
          case SelectRaceMenu:
            break;
          case ResultsMenu:
            break;
          case SetRaceTimeMenu:
            switch (key) {
              case 'A':
                // Change minutes
                raceTime[0] = enterNumber(2, 60, 1, 10);
                break;
              case 'B':
                // change seconds
                raceTime[1] = enterNumber(2, 59, 1, 13);
                break;
              case 'C':
                // change seconds
                raceLaps = enterNumber(3, 999, 2, 10);
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
                // lcd.print(activeLane);
                break;
              case '*':
                // return to previous screen; SettingsMenu
                currentMenu = SettingsMenu;
                UpdateMenu(SettingsText);
                break;
              default:
                break;
            }
            break;
          case SetRaceLapsMenu:
            break;
          case SetLanesMenu:
            break;

          default:
            break;
        }; // end of Menu switch

        Serial.println(key);
        Serial.println(currentMenu);

      }; // end of if key pressed wrap
      break; // case of state = Menu 


    case Race:

      break;

    case Paused:

      break;

    default:
      // if the state becomes unknown then default back to 'Menu'
      state = Menu;
      break;
  }

}

// clear line on display by writing a space for each row
void clearLine(int lineNumber) {
  lcd.setCursor(0,lineNumber);
  for(int n = 0; n < LCD_ROWS; n++){
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
  // because we use digits as a counter it is in the reverse order of it's written value
  // returnNumber = (inputNumber[0]-'0') + (inputNumber[1]-'0') * 10 ;
  returnNumber = 0;
  for (int i = 0; i < sizeof(inputNumber); i++) {
    returnNumber = (inputNumber[i]-'0') * pow(10, i) + returnNumber;
  }

  if (returnNumber > maxValue) {
    returnNumber = maxValue;
    lcd.setCursor(startCursorPos, line);
    if (maxValue < 10) lcd.print("0");
    // subtracting a '0' zero character from another character converts it into an int
    lcd.print(maxValue);
    // lcd.print(printf("%02d", maxValue));
  }
  // lcd.setCursor(0,3);
  // clearLine(3);
  // lcd.print(returnNumber);
  return returnNumber;
}


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


// **** Global Variables ****
// create enum to hold possible state values
enum states {
  Menu,
  Race,
  Paused
};

char MenuKeys[] = {'A', 'B', 'C', 'D'};


//****** Menu String Arrays **********
const char* MainMenuText[4] = {
  "A Change Settings",
  "B Select a Race",
  "C See Results",
  ""
};
const char* SettingsMenuText[4] = {
  "A Set Race Time",
  "B Set Lap Number",
  "C Set Lanes",
  "D   <-- Return"
};
const char* RaceSelectMenuText[4] = {
  "A Start Timed Race",
  "B Start Lap Race",
  "C Start Pol Trials",
  "D   <-- Return"
};
const char* ResultMenuText[4] = {
  "A Set Race Time",
  "B Set Lap Number",
  "C Set Lanes",
  "D   <-- Return"
};

// create variale to hold current 'state'
states state = Menu;


void UpdateMenu(char *curMenu[]){
  // depending on the current menu map key press to next menu
  // for each string in the menu array, print it to the screen
  // clear screen in case new text doesn't cover all old text
  lcd.clear();
  for (int i=0; i<4; i++){
    lcd.setCursor(1,i);
    lcd.print(curMenu[i]);
  }
}

enum Menus {
  MainMenu,
  SettingsMenu,
  RaceSelectMenu,
  ResultMenu,
  SetRaceTime,
  SetRaceLaps,
  SetLanes
};

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
  UpdateMenu(MainMenuText);
  
          Serial.println(currentMenu);
          Serial.println(SettingsMenu);
}


void loop(){

  switch (state) {
    case Menu:
      // monitor keypad for user inputs
      char key = keypad.getKey();
      // if a press is detected then updated the menu
      if (key) {
        switch (currentMenu) {
          case MainMenu:
            switch (key) {
              case 'A':
                currentMenu = SettingsMenu;
                Serial.println("set" + SettingsMenu);
                UpdateMenu(SettingsMenuText);
                break;
              case 'B':
                currentMenu = RaceSelectMenu;
                UpdateMenu(RaceSelectMenuText);
                break;
              case 'C':
                currentMenu = ResultMenu;
                UpdateMenu(ResultMenuText);
                break;
              default:
                break;
            }
            break;
          case SettingsMenu:
            switch (key) {
              case 'A':
                currentMenu = SetRaceTime;
                UpdateMenu(MainMenuText);
                break;
              case 'B':
                currentMenu = SetRaceLaps;
                UpdateMenu(MainMenuText);
                break;
              case 'C':
                currentMenu = SetLanes;
                UpdateMenu(MainMenuText);
                break;
              case 'D':
                currentMenu = MainMenu;
                UpdateMenu(MainMenuText);
              default:
                break;
            }
            break;
          case RaceSelectMenu:
            break;
          case ResultMenu:
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

// void loadMainMenu(){
//   lcd.setCursor(1,0);
//   lcd.print(MainMenu[0]);
//   lcd.setCursor(1,1);
//   lcd.print(MainMenu[1]);
//   lcd.setCursor(1,2);
//   lcd.print(MainMenu[2]);
// }
// Race Timer and Controller
// Version 1.1.0

// include enums from list file
// enums are kept in a seperate file to force them to compile early so they can be used in functions
#include "enum_lists.h"

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
// Specific implementation is determined by the board selected in Arduino IDE.
#include <Wire.h>
// LCD driver libraries
#include <hd44780.h>						            // main hd44780 header file
#include <hd44780ioClass/hd44780_I2Cexp.h>	// i/o class header for i2c expander backpack
// Custom character layout Byte's for LCD
#include "CustomChars.h"
// library for 7-seg LED Bars
#include <LedControl.h>

// Library to support 4 x 4 keypad
#include <Keypad.h>

// load default and local settings that define menu text and default race controller attributes.
#include "defaultSettings.h"


// --------- I2C ADAFRUIT LED BARGRAPH GLOBAL CODE --------------------------
// Adafruit Bar LED libraries
// code taken from built in Adafruit LEDBackpack library 'bargraph24)
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"


// declaring object representing Adafruit LED bar, called 'bar'.
Adafruit_24bargraph bar = Adafruit_24bargraph();

// create a global variable to track the 'on' state of LED bar
// We'll use this flage to make if faster to check status during a race
bool clearStartLight = false;

// This function sets specified block of LEDs on bargraph to the input color.
// 'color' can be 'LED_RED', 'LED_YELLOW', 'LED_GREEN', or 'LED_OFF'
// 'start' is the index reference of 1st LED to change, index 0 is LED 1
// 'end' is index of last LED to change, index 23 is LED 24
// by default, leaving 'start' and 'end' out of function call, will set entire bar
void setBargraph(byte color, byte end = 23, byte start = 0) {
  // cyle through each LED to change, set new color, and update display
  for (uint8_t i=start; i<=end; i++) {
    bar.setBar(i, color);
    bar.writeDisplay();
  }
}
// ------------- END OF BARGRAPH GLOBALS ------------------------


// The # of physical lanes that will have a sensor and lap timer/counter display.
const byte laneCount = LANE_COUNT;

// ********* AUDIO HARDWARE *********
const byte buzzPin1 = BUZZPIN;

// indicator LED, disable buzzer to use
// const byte ledPIN = 13;
// int ledState = HIGH;

//***** Setting Up Lap Triggers and Pause-Stop button *********
// debounceTime (ms), time within which not to accept additional signal input
const int debounceTime = DEBOUNCE;

// LANES DEFININTION
// The following array constant, lanes[], defines the hardware/software
// relationship between the physical arduino lane gates, their interrupts,
// and the associated 'Racer #'.

// The configuration to follow, below, is for the default lane wiring;
// Where, pinA0 is wired to lane1, pinA1-lane2, pinA2-lane3, & pinA3-lane4

// The first term of each row pair, making up lanes[],
// is the hardware pin used by the associated racer/lane# index.
//   ex: lanes[1][0] = PIN_A0;
//       tells controller that PIN_A0 is wired to lane used by racer #1

// The second term of each row pair, making up lanes[],
// is a byte mask, that indicates the bit, on the PCINT1_vect byte,
// that represents an interrupt trigger for that pin.
//   ex: lanes[1][1] = 0b00000001;
//       tells controller that 1st bit of interrupt byte (PCINT1_vec) represents PIN_A0

// Each given pin# and associated byte mask value, must stay together.
// However, pin-mask pairs can be assigned to any racer/lane# index,
// according to the physical wiring.

// The zero row, lanes[0] = {255, 255} is reserved, but not currently used.
// Otherwise, the settings for racerX are held in the array at index lanes[X]

// ---- Default lane configuration
// ------------------------------------
// The following is the default lane wiring, pinA0-lane1, pinA1-lane2, pinA2-lane3, pinA3-lane4
// const byte lanes[5][2] = {
//   {255, 255},
//   {PIN_A0, 0b00000001},
//   {PIN_A1, 0b00000010},
//   {PIN_A2, 0b00000100},
//   {PIN_A3, 0b00001000}
// };
// See the 'defaultSettings.h' file or 'localSettings.h' file,
// for the actual pin-mask byte values of the 'LANE#' tokens below.
// The default values should match those commented out above.
// Lanes greater than 'laneCount' will go unused, but 'lanes[]' is sized for system's max of 4.
const byte lanes[5][2] = {
  {255, 255},
  LANE1,
  LANE2,
  LANE3,
  LANE4
};

// ---- Alternate Configurations
// -----------------------------

// The following example could be used for alternative wiring,
// where pinA3 is connected to lane1, pinA0-lane2, pinA1-lane3, and pinA2-lane4

// const byte lanes[5][2] = {
//   {255, 255},
//   {PIN_A3, 0b00001000},
//   {PIN_A0, 0b00000001},
//   {PIN_A1, 0b00000010},
//   {PIN_A2, 0b00000100}
// };

// ----------------------------
// This is the deafult configuration If using Arduino Mega2560
// Use Port K, analog pins A8-A11
// const byte lanes[5][2] = {
//   {255, 255},
//   {PIN_A8, 0b00000001},
//   {PIN_A9, 0b00000010},
//   {PIN_A10, 0b00000100},
//   {PIN_A11, 0b00001000}
// };
// -------------------------------

// A6 and A7 are analog input only pins and thus, must be read as analog.
// An external pullup resistor must be added to button wiring.
// This input is expected to be HIGH and go LOW when pressed.
const byte pauseStopPin = PAUSEPIN;
const byte startButtonPin = STARTPIN;
// timestamp marking new press of pause button, used to set start of debounce period.
unsigned long pauseDebounceMillis = 0;

//***** Variables for LCD 4x20 Display **********
// This display communicates using I2C via the SCL and SDA pins,
// which are dedicated by the hardware and cannot be changed by software.
// If using Arduino Nano, pin A4 is used for SDA, pin A5 is used for SCL.
// If using Arduino Mega2560, pin D20 can be used for SDA, & pin D21 for SCL.
// Make sure the LCD is wired accordingly.

// Declare 'lcd' object representing display using class 'hd44780_I2Cexp'
// because we are using the i2c i/o expander backpack (PCF8574 or MCP23008)

hd44780_I2Cexp lcd;

// When more than 2 MAX7219s are chained, additional LED bars
// may need direct power supply to avoid intermittent error.
// # of attached max7219 controlled LED bars
const byte LED_BAR_COUNT = LANE_COUNT + 1;
// const byte LED_BAR_COUNT = 4;
// # of digits on each LED bar
const byte LED_DIGITS = 8;
// LedControl parameters (DataIn, CLK, CS/LOAD, Number of Max chips (ie 8-digit bars))
LedControl lc = LedControl(PIN_TO_LED_DIN, PIN_TO_LED_CLK, PIN_TO_LED_CS, LED_BAR_COUNT);


//***** KeyPad Variables *****
// set keypad size
const byte KP_COLS = 4;
const byte KP_ROWS = 4;
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
// Creating keypad object
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, KP_ROWS, KP_COLS );


//*** RACE PROPERTIESS ******
races raceType = Standard;
// variable to hold the 'race to' x lap total setting
int raceLaps = DEFAULT_LAPS;
// variable indicating final lap of race, most often will equal 'raceLaps' except for drag race.
int endLap = DEFAULT_LAPS;
// Race time for a Timed race type, most laps before time runs out wins.
// Create a clock time array to hold converted values for easy display update.
// raceSetTime[1] holds Min, raceSetTime[0] holds Sec, settable from menu.
// const byte raceSetMin = DEFAULT_SET_MIN;
// const byte raceSetSec = DEFAULT_SET_SEC;
// byte raceSetTime[2] = {raceSetSec, raceSetMin};
byte raceSetTime[2] = {DEFAULT_SET_SEC, DEFAULT_SET_MIN};
// raceSetTime in milliseconds, will be initialized in setup().
unsigned long raceSetTimeMs;
// pre-race countdown length in seconds, settable from menu
byte preStartCountDown = DEFAULT_COUNTDOWN;
byte preStartTimerDrag = 3;  //default to 3 sec, but will be randomized when used.
// Flag to tell race timer if race time is going up or going down as in a time limit race.
// Since the default race type is standard the default countingDown is false.
bool countingDown = false;
// flag to discern a new start from a re-start from pause.
bool newRace = true;

// Used to hold the millis() timestamp of the start of current program loop.
// This time is used by all processes in the loop so that everything happens
// psuedo concurrently.
unsigned long curMillis;

// Interval in milliseconds that the clock displays are updated.
// this value does not affect lap time precision, it only sets min display update rate.
// This is done to be more efficient and give's us some control over refresh rate.
int displayTick = DEFAULT_REFRESH_TICKS;
// timing between start light updates during PreStart
int preStartTick = 1000;
int nextStageCountdownTime = 0;
// A millis() timestamp marking last tick completion.
unsigned long lastTickMillis;

// flag indicating if race state is in preStart countdown or active race
// volatile bool preStart = false;


// ***** RACE DATA *********
// Running count of the current lap a racer is on.
// Note that this is 1 greater than the number of completed laps.
// idx of lapCount relates to corresponding lane#, idx = 0 is reserved.
volatile int lapCount[laneCount + 1] = {};
// array to hold the running total time (in ms) of each racer to complete their current lapCount
unsigned long racersTotalTime[ laneCount + 1 ] = {};
// For each lane/racer a list of the fastest X laps will be recorded.
// This will be stored in 2, 2D arrays.
// One array to hold the time (long), and one to hold the corresponding lap# (int).
// The row index indicates the associated lane/racer #.
// Row idx = 0 is used to hold the Top Overall Laps.
// const byte fastestQSize = DEFAULT_MAX_STORED_LAPS;
unsigned int fastestLaps[laneCount + 1][DEFAULT_MAX_STORED_LAPS] = {};
unsigned long fastestTimes[laneCount + 1][DEFAULT_MAX_STORED_LAPS] = {};

// Another pair of lap and time arrays holds the fastest overall laps
// unsigned int topFastestLaps[ DEFAULT_MAX_STORED_LAPS ] = {};
// unsigned long topFastestTimes[ DEFAULT_MAX_STORED_LAPS ] = {};
// With the overall lap times we need to track the associated lane/racer in a seperate array.
byte topFastestRacers[ DEFAULT_MAX_STORED_LAPS ] = {};

// During the actual race, due to memory limits, we only track the
// last few completed lap millis() timestamps.
// These timestamps are used to calculate lap times, which are stored in another array,
// so these timestamps can be discarded once a lap time is logged.
// The modulus of the lap count, by the lapMillisQSize, will set the looping index.
// (lapCount % lapMillisQSize) = idx, will always be 0 <= idx < lapMillisQSize
// DO NOT SET lapMillisQSize < 3
const byte lapMillisQSize = 5;
// The row index indicates the lane/racer associated with it's row array timestamps.
volatile unsigned long lastXMillis[laneCount + 1] [ lapMillisQSize ] = {};

// To keep a running time of the race and each current lap,
// we record a start, millis() timestamp and log elapsed time in ms.
// idx0 used to log overall race time.
// idx > 0, log the current lap time of corresponding lane #.
volatile unsigned long startMillis[laneCount + 1];
volatile unsigned long currentTime[laneCount + 1] = {};

// Flag to alert results menu whether race data has been collected.
// If it tries to print empty tables it will print garbage to the screen.
bool raceDataExists = false;

// Variable to record state of lane seneor pins.
volatile byte triggeredPins = 0;


// ******* LANE/RACER VARIABLES ******************
// In all arrays relating to this data the array index will equal the associate lane/racer#.
// Index 0 will be reserved for race level times and data or may not be used at present.

// Logs current lane state (see enum for details)
volatile byte laneEnableStatus[ laneCount + 1 ] = {};
// These help keep code easier to read instead of calculating them repeatedly from status array.
byte enabledLaneCount = 0;
byte finishedCount = 0;
// Holds index of Racers[] array that indicates the name of racer associated with lane.
// set racer name to '0' when status is Off.
// Other than 0, the same Racer[idx] cannot be used for more than 1 racer.
// During use, the code will prevent this, but it will not correct duplicates made here.
byte laneRacer[ laneCount + 1 ] = {};

// After a racer finishes a lap the logged time will be 'flashed' up
// to that racer's LED. The period that this lap time stays displayed
// before the display returns to logging current lap time is the 'flash' period.
// The lane Lap Flash variables indicate the state of this flash period.
// 0 = update display with racer's current lap and running time.
// 1 = process data for last finished lap, and write result to racer's LED.
// 2 = hold the last finished lap info on display until flash time is up.
// lap flash status idx corresponds to status of lane #, idx0 reserved.
// The flash display period does not affect active timing, or any other functions.
volatile byte flashStatus[ laneCount + 1 ] = {};

const int flashDisplayTime = DEFAULT_FLASH_PERIOD_LENGTH;
// logs millis() timestamp at start of current flash period for each racer.
unsigned long flashStartMillis[ laneCount + 1 ];

// Index of lap time to display in first row of results window.
// Keep this an int so it can be signed. A negative is the trigger to loop index
int resultsRowIdx = 0;
// Tracks which racer's results to show in Results Menu, 0 = show top results overall.
byte resultsMenuIdx = 0;

// Variable to hold the last digit displayed to LEDs during last 3 sec
// of the pre-start countdown, so it does not rewrite every program loop.
byte ledCountdownTemp = 0;

// flag to trigger when 1st racer crosses finish.
bool winner = false;

// Create variables to hold current game state and menu state.
volatile states state;
volatile states prevState;
Menus currentMenu;
// This flag is used to indicate first entry into a state or menu so
// that one time only setup can be performed.
volatile bool entryFlag;

void ChangeStateTo(volatile states newState){
  prevState = state;
  state = newState;
  entryFlag = true;
}


// // enum to use names with context instead of raw numbers when coding audio state
// enum audioModes {
//   AllOn,
//   GameOnly,
//   Mute
// };
// Setup default settings and flags
// audioModes audioState = AllOn;
audioModes audioState = DEFAULT_AUDIO_MODE;
bool gameAudioOn = true;
bool musicAudioOn = true;


// Racer Names list.
// Because this is an array of different length character arrays
// we use an array of pointers.
// The size of 'Racers[]' and 'victorySongs[]' should match if SONGS_BY_PLACE is 'false'.
// byte const racerListSize = RACER_LIST_SIZE;
// Keep in mind that the 7-seg racer lap displays, cannot write W's, M's, X's, K's, or V's
// const char* Racers[racerListSize] = RACER_NAMES_LIST;
const char* Racers[] = RACER_NAMES_LIST;
// Racer's victory song, matched by index of racer.
// const char* victorySong[racerListSize] = RACER_SONGS_LIST;
const char* victorySong[] = RACER_SONGS_LIST;

// sets screen cursor position for the names on the racer select menu
byte nameEndPos = 19;

// *** STRING PROGMEM *************
// in this section we define our menu string constants to use program memory
// this frees up significant RAM. In this case, using progmem to replace
// these few const char* arrays, reduced RAM used by globals, by 10%.
// The character buffer needs to be as large as largest string to be used.
// Since our longest row of characters is on the LCD we use its column count.
char buffer[LCD_COLS];

// const char Main0[] PROGMEM = "A| Select Racers";
// const char Main1[] PROGMEM = "B| Change Settings";
// const char Main2[] PROGMEM = "C| Start a Race";
// const char Main3[] PROGMEM = "D| See Results";
const char Main0[] PROGMEM = A_SELECT_RACER;
const char Main1[] PROGMEM = B_CHANGE_SETTINGS;
const char Main2[] PROGMEM = C_START_RACE;
const char Main3[] PROGMEM = D_SEE_RESULTS;
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


// const char Settings0[] PROGMEM = "Settings     mm:ss";
// const char Settings0[] PROGMEM = " A |Audio";
// const char Settings1[] PROGMEM = " B |Time       :";
// const char Settings2[] PROGMEM = " C |Laps";
// const char Settings3[] PROGMEM = "0-4|Lanes";
const char Settings0[] PROGMEM = A_SETTING_AUDIO;
const char Settings1[] PROGMEM = B_SETTING_TIME;
const char Settings2[] PROGMEM = C_SETTING_LAPS;
const char Settings3[] PROGMEM = D_SETTING_LANES;
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


// const char SelectRacers0[] PROGMEM = "A|Racer1";
// const char SelectRacers1[] PROGMEM = "B|Racer2";
// const char SelectRacers2[] PROGMEM = "C|Racer3";
// const char SelectRacers3[] PROGMEM = "D|Racer4";
const char SelectRacers0[] PROGMEM = A_RACER1;
const char SelectRacers1[] PROGMEM = B_RACER2;
const char SelectRacers2[] PROGMEM = C_RACER3;
const char SelectRacers3[] PROGMEM = D_RACER4;
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

// const char SelectRace0[] PROGMEM = "A|First to     Laps";
// const char SelectRace1[] PROGMEM = "B|Most Laps in   :";
// const char SelectRace2[] PROGMEM = "";
// const char SelectRace3[] PROGMEM = "D|Countdown:    Sec";
const char SelectRace0[] PROGMEM = A_START_RACE_STANDARD;
const char SelectRace1[] PROGMEM = B_START_RACE_IMED;
const char SelectRace2[] PROGMEM = START_RACE_3RD_ROW;
const char SelectRace3[] PROGMEM = D_START_RACE_COUNTDOWN;
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

// flag to identify state of UI text that has an A and B part flashed intermittently.
bool titleA = true;

// additional text strings used in multiple places
const char* Start = {TEXT_START};

// const char DidNotFinish[] PROGMEM = "DNF";
// const char FirstPlace[] PROGMEM = "1st";
// const char SecondPlace[] PROGMEM = "2nd";
// const char ThirdPlace[] PROGMEM = "3rd";
// const char FourthPlace[] PROGMEM = "4th";
const char DidNotFinish[] PROGMEM = FINISH_DNF;
const char FirstPlace[] PROGMEM = FINISH_1ST;
const char SecondPlace[] PROGMEM = FINISH_2ND;
const char ThirdPlace[] PROGMEM = FINISH_3RD;
const char FourthPlace[] PROGMEM = FINISH_4TH;
const char* const FinishPlaceText[5] PROGMEM = {
  DidNotFinish,
  FirstPlace,
  SecondPlace,
  ThirdPlace,
  FourthPlace
};


// Reviews lane status array and updates the settings menu.
// Run after a settings change.
void PrintLaneSettings(){
  for(int i = 1; i <= laneCount; i++){
    lcd.setCursor(10+ 2 * i, 3);
    // Something about the lcd.print() function doesn't work to use a ternery to assess this.
    if (laneEnableStatus[i] == 0){
      // write a skull
      lcd.write(3);
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
  // total number of seconds
  ulSec = msIN / 1000;
  // total minutes
  ulMin = ulSec / 60;
  // total hours
  ulHour = ulMin / 60;
  // subtract out the minutes used by whole hours to get actual minutes digits
  ulMin -= ulHour * 60;
  // subtract seconds used by hour and minutes to get actual seconds digits
  ulSec = ulSec - ulMin * 60 - ulHour * 3600;
  // Debug printout
  // Serial.print("SplitTime ulSec: ");
  // Serial.println(ulSec);
  // Serial.println(msIN);
  // Serial.println("ulMill");
  // Serial.println(ulMill);
}


// Converts clock time into milliseconds
unsigned long ClockToMillis(const byte ulHour, const byte ulMin, const byte ulSec) {
  // Sets the number of milliseconds from midnight to current time
  // NOTE: we must us the 'UL' designation or otherwise cast to unsigned long.
  //       If we don't, then the multiplication will not be the exptected value.
  //       This is because the default type of the number is a signed int which
  //       in this case, results in 60 * 1000, which is out of range for int.
  //       Even though the variable it is going into is a long, the right side
  //       remains an int until after the evaluation so we would get an overflow.       
  return (ulHour * 60UL * 60UL * 1000UL) +
         (ulMin * 60UL * 1000UL) +
         (ulSec * 1000UL);
}


// tone(pin with buzzer, freq in Hz, duration in ms)
void Beep() {
  if (gameAudioOn) tone(buzzPin1, BEEP_FREQ, BEEP_DUR);
}
void Boop() {
  if (gameAudioOn) tone(buzzPin1, BOOP_FREQ, BOOP_DUR);
}
void Bleep() {
  if (gameAudioOn) tone(buzzPin1, BLEEP_FREQ, BLEEP_DUR);
}



// Used to set fastest lap array to high numbers that will be replaced on comparison.
// A lap number of 0, and laptimes of 999999, marks these as dummy laps.
void InitializeRacerArrays(){
  for (byte i = 0; i <= laneCount; i++) {
    for (byte j = 0; j < DEFAULT_MAX_STORED_LAPS; j++) {
      fastestLaps[i][j] = 0;
      fastestTimes[i][j] = 999999;
    }
    for (byte j = 0; j < lapMillisQSize; j++) {
      lastXMillis[i][j] = 0;
    }
    racersTotalTime[i] = 0;
  }
  // for (byte i = 0; i <= laneCount; i++) {
  //   for (byte j = 0; j < lapMillisQSize; j++) {
  //     lastXMillis[i][j] = 0;
  //   }
  // }
}

// Used to initialize results, top fastest, lap array to high numbers.
// A lap number of 0, and racer id of 255, marks these as dummy laps.
void InitializeTopFastest(){
  for (byte i = 0; i < DEFAULT_MAX_STORED_LAPS; i++) {
    // topFastestLaps[i] = 0;
    topFastestRacers[i] = 255;
    // topFastestTimes[i] = 999999;
  }
}


// Update menu on main LCD with static base text for given menu screen
void UpdateLCDMenu(const char *curMenu[]){
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
// The value in laneRacer[laneID] is the index of the racer name array.
byte IndexRacer(byte laneID) {
  byte newRacerNameIndex = laneRacer[laneID];
  bool notUnique = true;
  // The modulus (%) of a numerator smaller than its denominator is equal to itself,
  // while the modulus of an integer with itself is 0. Add 1 to restart at idx 1, not 0.
  byte racerListSize = (sizeof(Racers)/sizeof(char*));
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


// // Returns the number of digits in an integer.
// int calcDigits(int number){
//   int digitCount = 0;
//   while (number != 0) {
//     // integer division will drop decimal
//     number = number/10;
//     digitCount++;
//   }
//   return digitCount;
// }


// Used to write same character repeatedly from start to end position, inclusive.
// This is primarily used to clear lines and space before writing an update to display.
void PrintSpanOfChars(displays disp, byte lineNumber = 0, int posStart = 0, int posEnd = LCD_COLS - 1, char printChar = ' ') {
  switch(disp){
    case lcdDisp:{
      // Serial.println(printChar);
      // Serial.print("StartPos: ");
      // Serial.println(posStart);
      // Serial.print("EndPos: ");
      // Serial.println(posEnd);
      lcd.setCursor(posStart, lineNumber);
      for(int n = posStart; n <= posEnd; n++){
        lcd.print(printChar);
      }
    }
    break;
    // Assumes that Racer LED bars are the only other display.
    default: {
      for(int i = posStart; i <= posEnd; i++){
        lc.setChar(disp-1, (LED_DIGITS - 1) - i, printChar, false);
      }
    }
    break;
  }
}


// function to write pre-start final countdown digits to LEDs
void ledWriteDigits(byte digit) {
  // Use laneCount instead of LED_BAR_COUNT because this is for racer LED only, not start light.
  for (int i = 1; i <= laneCount; i++){
    if(laneEnableStatus[i] > 0) PrintSpanOfChars( displays(i), 0, 0, 7, char(digit) );
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
        if (!leadingZs && digitValue == 0){
          lcd.print(" ");
        }
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
      // For clearing we want to clear the whole space so we don't use adjusted positions.
      if (clear) PrintSpanOfChars(display, line, (writeSpaceEndPos-width+1), writeSpaceEndPos);
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
      // If 'clear' = true then clear the original width from the end position -1.
      if (clear) PrintSpanOfChars(display, line, (writeSpaceEndPos-width+1), writeSpaceEndPos);
      // Serial.print("Print TExt - writeSpacedEnd Pos:  ");
      // Serial.println(writeSpaceEndPos);
      // Serial.print("beginning position:  ");
      // Serial.println(writeSpaceEndPos-width+1);
      for (byte j = cursorStartPos; j <= cursorEndPos; j++){
        // The digit position of the LED bars is right to left so flip the requested index.
        lc.setChar(display - 1, (LED_DIGITS-1) - j, textToWrite[textIndex], false);
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

  clockWidth nextTimeBlock = H;
  int maxPrecision = 0;
  byte baseTimeWidth = 0;

  // Based on the time value, and width of display area (printWidth),
  // calculate how many decimal places can be accomodated.
  // Because the LCD requires a character space for ':' and '.',
  // we must adjust the required width accordingly.
  // 'baseTimeWidth' is min width needed for the whole units HH:MM:SS and colons.
  //
  // if t < 10sec
  // 0.0
  if(timeMillis < 10000){
    nextTimeBlock = S;
    maxPrecision = (display == lcdDisp ? printWidth-2 : printWidth-1);
    baseTimeWidth = 1;
  } else
  // if 10sec <= t < 1min
  // 00
  if (timeMillis < 60000){
    nextTimeBlock = S;
    maxPrecision = (display == lcdDisp ? printWidth-3 : printWidth-2);
    baseTimeWidth = 2;
  } else
  // if 1min <= t < 10min
  // 0:00
  if (timeMillis < 600000){
    nextTimeBlock = M;
    maxPrecision = (display == lcdDisp ? printWidth-5 : printWidth-3);
    baseTimeWidth = (display == lcdDisp ? 4 : 3);
  } else
  // if 10min <= t < 1hr
  // 00:00
  if (timeMillis < 3600000){
    nextTimeBlock = M;
    maxPrecision = (display == lcdDisp ? printWidth-6 : printWidth-4);
    baseTimeWidth = (display == lcdDisp ? 5 : 4);
  } else
  // if 1hr <= t < 10hr
  // 0:00:00
  if (timeMillis < 30600000){
    nextTimeBlock = H;
    maxPrecision = (display == lcdDisp ? printWidth-8 : printWidth-5);
    baseTimeWidth = (display == lcdDisp ? 7 : 5);
  } else
  // if 10hr <= t < 24hr
  // 00:00:00
  if (timeMillis < 86,400,000){
    nextTimeBlock = H;
    maxPrecision = (display == lcdDisp ? printWidth-9 : printWidth-6);
    baseTimeWidth = (display == lcdDisp ? 8 : 6);
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
    if( (printWidth - precision - baseTimeWidth - ((display==lcdDisp && precision>0)?1:0)) <= 0 ){
      leadingZs = false;
    }

    // calculate total width needed to print time string
    // Must account for end position being included in the printWidth total.
    byte totalWidth = baseTimeWidth + (display == lcdDisp ? precision+1: precision);
    // Clear spaces in print width that occur before clock time's first display digit
    PrintSpanOfChars(display, line, clockEndPos - printWidth +1, clockEndPos - totalWidth);

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
      byte pwidth = 2;
      // if the input time is under 1sec then use only 1 digit width leading zero
      if (timeMillis < 1000) {
        leadingZs = true;
        pwidth = 1;
      }
      // S 00.(000)
      switch (display){
        case lcdDisp: {
          PrintNumbers(ulSec, pwidth, hourEndPos + 6, display, leadingZs, line, precision > 0);
          if(precision > 0) PrintNumbers(decimalSec, precision, clockEndPos, display, true, line);
        }
        break;
        default:{
          PrintNumbers(ulSec, pwidth, hourEndPos + 4, display, leadingZs, 0, precision > 0);
          if(precision > 0) PrintNumbers(decimalSec, precision, clockEndPos, display);
        }
        break;
      } // END of display switch
    } // END of if 'S'

  } // END if 'X' Else

} // END PrintClock()


void PreStartDisplaysUpdate(){
  lcd.clear();
  switch (state) {
    case Drag: {
      PrintText("Racers Are Staged", lcdDisp, 19, 20);
    }
    break;
    default: {
      lcd.setCursor(TEXT_PRESTART_STRPOS,1);
      lcd.print(TEXT_PRESTART);
      PrintClock(currentTime[0], PRESTART_CLK_POS, 4, 1, lcdDisp, 2);
      // For active lanes, write racer's name to their corresponding lane LED.
      // If pre-start time is less than 3 seconds, this will be immediately written over.
      for(byte i = 1; i <= laneCount; i++){
        if(laneEnableStatus[i] > 0) PrintText(Racers[laneRacer[i]], displays(i), 7, 8, false);
      }
    }
    break;
  }

}


// 'laneNumber' is a settings menu, keypad entry 0-4.
// '0' means to disable all lanes,
// '1-4' means toggle corresponding lane status between 'StandBy' and 'Off'.
void ToggleLaneEnable(byte laneNumber){
  const bool wasSet = laneEnableStatus[laneNumber] > 0;
  if(laneNumber == 0){
    for (byte i = 1; i <= laneCount; i++){
      laneRacer[i] = 0;
      laneEnableStatus[i] = Off;
    }
    enabledLaneCount = 0;
  } else {
    // check laneNumber is a valid lane, if so then toggle
    if (laneNumber <= laneCount) {
      if(wasSet){
        // If lane had been on then, it's off now and the racer name idx is set to 0.
        laneRacer[laneNumber] = 0;
        laneEnableStatus[laneNumber] = Off;
        enabledLaneCount--;
      } else {
        // otherwise it was off, and is now on and must get an initial, unused, racer name.
        laneRacer[laneNumber] = IndexRacer(laneNumber);
        laneEnableStatus[laneNumber] = StandBy;
        enabledLaneCount++;
      }
    }
    // Update the interrupt trigger ID mask
    setTriggerMask();
  }
  // Serial.print("Toggle() - Lane Enabled count: ");
  // Serial.println(enabledLaneCount);
  // Serial.print("Toggle lane #: ");
  // Serial.println(laneNumber);
}



// This function enable/disables pin change interrupts based on current status of each lane.
void EnablePinInterrupts(bool Enable){
  for (byte i = 1; i <= laneCount; i++){
    // TURN ON PIN
    if(laneEnableStatus[i] > 0 && Enable) {
      // If lane has finished then we don't want to turn it back on.
      // if(laneEnableStatus[i] != 3) pciSetup(laneSensorPin[i]);
      if(laneEnableStatus[i] != 3) pciSetup(lanes[i][0]);
      // Serial.println("pin enabled");
    }
    // TURN OFF PIN
    else if(laneEnableStatus[i] > 0 && !Enable){
      // clearPCI(laneSensorPin[i]);
      clearPCI(lanes[i][0]);
      // Serial.println("pin disabled");
    }
  }
}


// **** Drop-in Replacement method for micros(), shows under the hood
// --------------------------------
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
// --------------------------------


// Using port register pin change interrupts we can
// read the state of an entire block of pins simultaneously.
// As long as the sensor positive trigger duration is longer than the
// exectuion of the ISR(), this controller should never miss a trigger.
//
// This function enables the port register change interrupt on the given pin.
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

// This function will disable the port register interrupt on a given pin.
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


// dec 15 = binary 1111, default with 4 active lanes
// as lanes are enabled/disabled, the triggerClearMask should be updated accordingly.
byte triggerClearMask = 15;

void setTriggerMask () {
  // triggerClearMask = ipow(2, laneCount)-1;
  for (byte i=1; i <= laneCount; i++){
    // if lane is not 'Off', then it is enabled; add its digit to the mask.
    if (laneEnableStatus[i] != Off) {
      // byteDigit = 1 << (i-1);
      triggerClearMask = triggerClearMask | (1 << (i-1));
    }
  }
}

// volatile bool test = false;
volatile byte lastTriggeredPins = 0;

// MICROTIMING code 
// const byte timeTestSize = 20;
// volatile long timeTest[timeTestSize];
// volatile int timeTestCount = 0;

// ISR is a special Arduino Macro or routine that handles interrupts ISR(vector, attributes)
// PCINT1_vect handles pin change interrupt for the pin block A0-A5, represented in bit0-bit5
// The execution time of this function should be as fast as possible as
// interrupts are disabled while inside it.
// This function takes approximately 0.004 - 0.180ms
// Use vector 'PCINT1_vect' for ATmega328 based Arduino (ie Nano)
// Use vector 'PCINT2_vect' for ATmega2560 based Arduino
// 'PCINT_VECT' is defined in '...Settings.h' files
ISR (PCINT_VECT) {
  // MICROTIMING code used for assessing the ISR execution time.
  // unsigned long logMicros = micros();

  // This code expects the lap sensors are setup as inputs.
  // This means the pins have been set to HIGH, indicated by a 1 on its register bit.
  // When a button is pressed, or sensor triggered, it should bring the pin LOW.
  // A LOW pin is indicated by a 0 on its port register.
  // Because all of the lap sensors are on the same port register
  // it will be possible to detect simultaneous triggers.

  // pin A0 positive trigger indicated by zero on byte digit 1, PINC = 0xXXXXXXX0
  // pin A1 positive trigger indicated by zero on byte digit 2, PINC = 0xXXXXXX0X
  // pin A2 positive trigger indicated by zero on byte digit 3, PINC = 0xXXXXX0XX
  // pin A3 positive trigger indicated by zero on byte digit 4, PINC = 0xXXXX0XXX
  
  // For analysis, it will work better to have our triggered bits as 1s.
  // To convert the zero based triggers above into 1s, we can simply flip each bit.
  // Since we only need to check bits for wired lanes, we'll also turn everything elsse to 0.
  // Flip every bit by using (PinPortRegsitryByte xor 0b11111111),
  //     or using bitwise compliment operator (~PinPortRegsitryByte).
  // Then trim off the 4 highest bits using bitwise operator '&',
  // of result with, mask representing available lanes.
  // If 'laneCount = 2', this would result in (~PinPortRegsitryByte & 0b00001111)
  // If 'laneCount = 4', it would be (~PinPortRegsitryByte & 0b00000011)

  // For PinPortRegsitryByte, use pin port C, 'PINC', for ATmega328 based Arduinos (ie Nano)
  // For PinPortRegsitryByte, use pin port K, 'PINK', for ATmega2560 based Arduinos
  // 'INTERRUPT_PORT' sets 'PINC' or 'PINK' per definition in the '...Settings.h' files.
  // 'triggerClearMask' is set on bootup based on `LANE_COUNT` set in '...Settings.h' files.
  // byte triggeredPins = ((INTERRUPT_PORT xor 0b11111111) & 0b00001111);
  // byte triggeredPins = ((INTERRUPT_PORT xor 0b11111111) & triggerClearMask);
  triggeredPins = (~INTERRUPT_PORT & triggerClearMask);
  // If switch voltage drop, on close, is too slight to cause pin to enter LOW state,
  // or controller operation is too slow, the triggering switch may not still be in a LOW state.
  // If this is the case then we just want to ignore the event as we won't know how to attribute it.
  if (triggeredPins == 0) return;
  
  // Note that millis() does not execute inside the ISR().
  // It can be called, and used as the time of entry, but it does not continue to increment.
  unsigned long logMillis = millis();

  // if still in pre-start, declare a fault and return the faulting lane triggers.
  if (state == PreStart) {
    // We need to debounce the fault trigger, like a regular trigger.
    // Store fault trigger timestamp in the 1st element, ie the zero index, of the lastXMillis[] array.
    if( ( logMillis - lastXMillis [0][0] ) > debounceTime ) {
      // prevState = state;
      state = PreFault;
      // state = Fault;
      lastTriggeredPins = triggeredPins;
      lastXMillis [0][0] = logMillis;
    }
    return;
  }

  // While the triggeredPins byte is > 0, one of the digits is a 1.
  // If after a check, triggerPins = 0, then there is no need to keep checking.
  // Since we only have 4 bits that can be a 1, this loop will run a max of 4 times.
  // laneNum is index of lanes[] that defiens the pin and intterupt byte determined by hardware.
  byte laneNum = 1;
  while(triggeredPins > 0){
    // If bit i is a 1, then process it as a trigger on lane 'laneNum'
    // if(triggeredPins & lanes[laneNum][1] && (laneEnableStatus[ laneNum ] != Off)){
    if(triggeredPins & lanes[laneNum][1]){
      // Depending on the status of this lane we process the trigger differently.
        // Serial.print("lanes[laneNum][1]: ");
        // Serial.println(lanes[laneNum][1]);

      switch (laneEnableStatus[ laneNum ]) {

        case StandBy:{
          // If in StandBy, no need for debounce
          // Change lane status from 'StandBy' to 'Active'
          laneEnableStatus[ laneNum] = Active;
          // log current ms timestamp as start time for racer's current lap.
          startMillis[ laneNum ] = logMillis;
          // If the first lap of race
          if(lapCount[ laneNum ] == 0) {
            // Log current ms timestamp to racer's looping, temporary lap time que.
            lastXMillis[ laneNum ][0] = logMillis;
            // Set current lap for triggering racer, to 1.
            lapCount[ laneNum ] = 1;
          } else {
            // Else, if 1st trigger, after a Pause, we need to feed the new start time,
            // into the previous lap index spot, and not index the current lapcount.
            lastXMillis [ laneNum ][(lapCount[ laneNum ] - 1) % lapMillisQSize] = logMillis;
            // DON'T index lapcount, we're restarting the current lap
          }
          Boop();
        }
        break;

        case Active:{
          // If lane is 'Active' then check that it has not been previously triggerd within debounce period.
          if( ( logMillis - lastXMillis [ laneNum ] [(lapCount[ laneNum ]-1)%lapMillisQSize] ) > debounceTime ){
            // Set lap display flash status to 1, indicating that racer's lane data needs to be processed.
            flashStatus[ laneNum ] = 1;
            // Log current ms timestamp to racer's looping, lap time, temporary que.
            lastXMillis [ laneNum ][lapCount[ laneNum ] % lapMillisQSize] = logMillis;
            // log current ms timestamp as start time for racer's new lap.
            startMillis[ laneNum ] = logMillis;
            // increase current lap by one (current lap = completed laps + 1)
            lapCount[ laneNum ] += 1;
            Beep();
          }
        }
        break;

        default:{
          // If lane is 'Off' then ignore it. It should not have been possible to trigger.
          // An interrupt should not be enabled on 'Off' lanes. 
        }
        break;
      } // END of lane status switch

    } // END if triggeredPin & ...

    // Turn checked digit in triggeredPins to zero
    // triggeredPins = triggeredPins & (lanes[laneNum][1] xor 0b11111111);
    triggeredPins = triggeredPins & ~lanes[laneNum][1];
    laneNum++;
    // Serial.print("triggeredPins: ");
    // Serial.println(triggeredPins);
    // Serial.println(INTERRUPT_PORT);

  } // END of While Loop checking each digit

  // MICROTIMING code
  // timeTest[timeTestCount] = micros() - logMicros;
  // timeTestCount++;

} // END of ISR()


// // Function returns true if the bit at the, 'pos', postion of a byte is 1,
// // otherwise it returns false.
// // In a Byte, position is from right to left, the far right bit is considered bit 1 at idx0.
// // EX. If function parameters are pos = 3 and b = 0x11111000
// //     the integer 1 = 0x00000001, if we left shift pos=3 places we get 0x00001000
// //     thus 0x11111000 & (0x00001000) = 0x00001000
// // Which is not zero, so the ex returns true, the indicated bit of the input Byte is set.
// bool IsBitSet(byte b, byte pos) {
//    return (b & (1 << pos)) != 0;
// }


// Generic function to check if an analog button is pressed
bool buttonPressed(uint8_t analogPin) {
  // if below analog trigger threshold AND not within debounce time
  // if (analogRead(analogPin) < 100){
  unsigned long tempTime = millis();
  if ((analogRead(analogPin) < 100) && ((tempTime - pauseDebounceMillis) > debounceTime)){
    // Reset debounce timestatmp
    Beep();
    // Serial.println("pressed");
    // Serial.println(tempTime - pauseDebounceMillis);
    pauseDebounceMillis = tempTime;
    return true;
  }
  // Serial.println("button pressed");
  // Serial.println(analogRead(analogPin));
  return false;
}


// This function compares the input race time with current fastest list.
// If the new lap time is faster than any existing time, it takes its place,
// pushing the subsequent times down by 1, dropping the last time off the list.
void UpdateFastestLap(unsigned long timesArray[], unsigned int lapsArray[], const int lap, const unsigned long newLapTime, const byte racer, const byte arrayLength, bool topTimes = false){
  // Serial.println("ENTERED FASTEST");
  for (byte i = 0; i < arrayLength; i++){
  // for (byte i = 0; i < sizeof(timesArray)/sizeof(long); i++){
    // Starting from beginning of list, compare new time with existing times.
    // If new lap time is faster, hold its place, and shift bested, and remaining times down.
    if (timesArray[i] > newLapTime) {
      // Starting from the end of list, replace rows with previous row,
      // until we reach row of the bested time to be replaced.
      for (byte j = arrayLength - 1; j > i; j--){
      // for (byte j = sizeof(timesArray)/sizeof(long) - 1; j > i; j--){
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
} // END UpdateFastesLaps()


// This function combines all the racer's fastest laps and list the top fastest overall.
void CompileTopFastest(){
  // first we need to re-initialize or else we will be duplicating previously added laps
  InitializeTopFastest();
  // for each racer/lane, compare their top laps to over all top laps and insert as appropriate.
  for (byte i = 1; i <= laneCount; i++) {
    for (byte j = 0; j < DEFAULT_MAX_STORED_LAPS; j++) {
      // UpdateFastestLap(topFastestTimes, topFastestLaps, fastestLaps[i][j], fastestTimes[i][j], laneRacer[i],  DEFAULT_MAX_STORED_LAPS, true);
      UpdateFastestLap(fastestTimes[0], fastestLaps[0], fastestLaps[i][j], fastestTimes[i][j], laneRacer[i],  DEFAULT_MAX_STORED_LAPS, true);
    }
  }
}



void PrintLeaderBoard(bool withLeaders = true){
  // Write static text to main LCD for live race screen
  lcd.clear();
  lcd.setCursor(15, 0);
  lcd.print(RESULTS_TOP_TEXT_BEST);
  // Draw vertical bars seperating leader list from best lap.
  for(byte row = 0; row <= LCD_ROWS; row++){
    lcd.setCursor(12, row);
    lcd.print("|");
  }
  if(withLeaders) UpdateLiveRaceLCD();
}



// Prints given results array to lcd display.
// resultsRowIdx = 0 Top Fastest, = 1, 2, 3, or 4 indicates that lane's fastest.
void PrintResultsList(unsigned long times[], unsigned int laps[], int listSize) {
  for (byte i = 0; i < 3; i++){
    // clear row of previous data (clear all rows, not just those with new data)
    PrintSpanOfChars(lcdDisp, 1 + i);
    // ignore if lap = 0 which means it's a dummy lap, or if index greater than lap count
    if (laps[resultsRowIdx + i] > 0 && ((resultsRowIdx + i) < listSize)) {
      // print rank of time, which is index + 1
      PrintNumbers(resultsRowIdx + i + 1, 2, 1, lcdDisp, false, i + 1, false);
      // then print LAP
      PrintNumbers(laps[resultsRowIdx + i], 3, 5, lcdDisp, true, i + 1, false);
      // Print the lap TIME
      PrintClock(times[resultsRowIdx + i], 12, 6, 3, lcdDisp, i + 1);
      // clear racer name space of any previous characters
      // PrintSpanOfChars(lcdDisp, 1 + i, 13);
      // then print RACER NAME
      // if idx = 0 then it's the top results and has racer names with the lap times.
      if(resultsMenuIdx == 0){
        // if (raceType == Drag){
        //   // for Drag Race, use Lane # instead of the racer names
        //   PrintText("Lane ", lcdDisp, 19, 6, false, i + 1, false);
        //   PrintNumbers(topFastestRacers[resultsRowIdx + i], 1, 19, lcdDisp, true, i + 1);
        // } else {
          PrintText(Racers[topFastestRacers[resultsRowIdx + i]], lcdDisp, 19, 6, true, i + 1, false);
        // }
      }
    } else {
      // we want to set the index back one so it doesn't keep scrolling in empty space
      resultsRowIdx = (resultsRowIdx -1) >= 0 ? resultsRowIdx - 1 : 0;
      // if this condition exists, we want to end the loop to avoid doubles
      // break;
    }
  }
}

void UpdateResultsMenu(bool full = true) {
  // lcd.clear();
  switch(resultsMenuIdx){
    // Idx = 0 is the top results menu
    case 0: {
      lcd.clear();
      PrintText(RESULTS_TOP_LBL, lcdDisp, 14, 15, false, 0);
      // Print custom up down arrow.
      lcd.setCursor(17,0);
      lcd.write('A');
      lcd.write(0);
      lcd.write('B');
      // PrintResultsList(topFastestTimes, topFastestLaps, DEFAULT_MAX_STORED_LAPS);
      PrintResultsList(fastestTimes[0], fastestLaps[0], DEFAULT_MAX_STORED_LAPS);
    }
    break;
    // All idx > 0 indicate results related to matching lane number.
    case 1 ... laneCount: {
      if(laneEnableStatus[resultsMenuIdx] > 0){
        // if (full) lcd.clear();
        if (titleA) {
          // If title-A mode on, print the results racer label
          PrintText(RESULTS_RACER_LBL, lcdDisp, 16, 17, false, 0);
          // Print 'Racer #' of current individual results screen
          PrintNumbers(resultsMenuIdx, 1, RESULTS_RACER_NUM_POS, lcdDisp, false, 0);
          // Print label for and total time
          PrintText(RESULTS_TOTAL_LBLA, lcdDisp, 19, 6, true, 2, false);
          PrintClock(racersTotalTime[resultsMenuIdx], 19, 6, 1, lcdDisp, 3, false);
        }
        // else print the racer name
        else {
          // print total laps completed
          PrintText(Racers[laneRacer[resultsMenuIdx]], lcdDisp, 16, 14, false, 0);
          // Print label for and total # of laps completed
          PrintText(RESULTS_TOTAL_LBLB, lcdDisp, 19, 6, true, 2, false);
          // int lapCountTemp = ((lapCount[resultsMenuIdx] == 0) ? 0 : (lapCount[resultsMenuIdx]-1));
          PrintNumbers(((lapCount[resultsMenuIdx] == 0) ? 0 : (lapCount[resultsMenuIdx]-1)), 6, 19, lcdDisp, false, 3 );
        }
        if (full) {
          // Print custom up down arrow.
          lcd.setCursor(17,0);
          lcd.write('A');
          lcd.write(0);
          lcd.write('B');
          // Print results, if number of recorded laps is smaller than Q size, use lap count for list size.
          PrintResultsList(
            fastestTimes[resultsMenuIdx], 
            fastestLaps[resultsMenuIdx], 
            lapCount[resultsMenuIdx] < DEFAULT_MAX_STORED_LAPS ? lapCount[resultsMenuIdx] : DEFAULT_MAX_STORED_LAPS
          );
          // Print title for blinking lap and time totals
          PrintText(RESULTS_TOTAL_LBL, lcdDisp, 19, 6, true, 1, false);
        }
      } else {
        resultsMenuIdx++;
        UpdateResultsMenu(full);
      }
    }
    break;
    case laneCount + 1:{
      lcd.clear();
      PrintText(RESULTS_FINISH_LBL, lcdDisp, 19, 20, false, 0);
      PrintLeaderBoard(true);
    }
    default:
    break;
  }
} // END UpdateResultsMenu()


// col0 is number of laps, col1 is the lane/racer #
// Even though only 3 places are displayed, the leader board data table,
// should contain all of the lanes in order.
int leaderBoard[laneCount][2] = {};
unsigned long overallFastestTime = 99999999;
byte overallFastestRacer = 5;

// Function to calculate and update to main display with the current leader board.
// This function is determining the current place of all racers.
void UpdateLiveRaceLCD(){
  // Clear table to be rebuilt
  memset(leaderBoard, 0, sizeof(leaderBoard));
  // for (byte a=0; a < laneCount; a++) {
  //   for (byte b=0; b < 2; b++){
  //     leaderBoard[a][b] = 0;
  //   }
  // }
  // For each lane/racer...
  for (byte racerID = 1; racerID <= laneCount; racerID++){
    bool swap = false;
    int racerLapCount = lapCount[racerID] - 1;
    // Iterate through the Leader Board Array.
    // Starting from beginning of list, compare each racer's lap count with already placed racer's
    // Only evaluate lanes that are not disabled.
    if(laneEnableStatus[racerID] > 0){
      // Check who has the overall fastest lap to post
      if(fastestTimes[racerID][0] < overallFastestTime){
        overallFastestTime = fastestTimes[racerID][0];
        overallFastestRacer = racerID;
      }
      // Build Leader Board comparing each racer's last finished lap to each other.
      // The more finished laps, the higher the place.
      // for (byte place = 0; place < laneCount; place++){
      for (byte place = 0; place < enabledLaneCount; place++){
        if (racerLapCount > leaderBoard[place][0]) {
          swap = true;
          // Serial.print(" swap lapCount: ");
          // Serial.print(" current place: ");
          // Serial.println(place);
        } else
        if (racerLapCount == leaderBoard[place][0]) {
          // If two racers have the same lap count,
          // The one with the earliest completion timestamp is in the lead.
          unsigned long racerFinishTimestamp = lastXMillis [racerID] [racerLapCount%lapMillisQSize];
          unsigned long placeFinishTimestamp = lastXMillis [leaderBoard[place][1]] [leaderBoard[place][0]%lapMillisQSize];

          if(racerFinishTimestamp < placeFinishTimestamp){
            swap = true;
            // Serial.print(" swap lapTimes: ");
            // Serial.print(" current place: ");
            // Serial.println(place);
          } else if(racerFinishTimestamp == placeFinishTimestamp){
            // If there was a tie in the completion of the last lap,
            // then keep checking time completion of next earlier lap until a winner is found.
            for(byte n = 1; n < lapMillisQSize; n++){
              racerFinishTimestamp = lastXMillis [racerID] [(racerLapCount-n)%lapMillisQSize];
              placeFinishTimestamp = lastXMillis [leaderBoard[place][1]] [(leaderBoard[place][0]-n)%lapMillisQSize];

              if(racerFinishTimestamp < placeFinishTimestamp){
                swap = true;
                // Serial.println(" true 3");
                break;
              }
            }
          }
        }
        // If new racer is better, then slide everyone down a place.
        // Start at the end of the list and stop when reaching insertion place.
        if(swap){
          for(byte i = enabledLaneCount - 1; i > place; i--){
          // for(byte i = laneCount-1; i > place; i--){
            leaderBoard[i][0] = (leaderBoard[i-1][0]);
            leaderBoard[i][1] = (leaderBoard[i-1][1]);
          }
          // Then insert new racer data into current place.
          leaderBoard[place][0] = racerLapCount;
          leaderBoard[place][1] = racerID;
          // exit loop once a swap is made.
          break;
        }

        // DEBUG PRINTOUT
        // Serial.print("   place: ");
        // Serial.print(place);
        // Serial.print("   racerID: ");
        // Serial.print(racerID);
        // Serial.print("   LapCount: ");
        // Serial.println(racerLapCount);

      } // END end for each PLACE

    } // END if racerID Active

  } // END for each racer

        // DEBUG PRINTOUT
        // Serial.print("   place: ");
        // Serial.print(place);
        // Serial.print("   racerID: ");
        // Serial.println(racerID);
        // for (int k = 0; k < laneCount; k++){
        //   Serial.print("LeaderBoard - place: ");
        //   Serial.print(k + 1);
        //   Serial.print("  Lap:");
        //   Serial.print(leaderBoard[k][0]);
        //   Serial.print("  Racer:");
        //   Serial.println(Racers[leaderBoard[k][1]]);
        // }

  // Even though the places list is as long as there are lanes,
  // we only have 3 lines on the screen, so limit this loop to 3.
  for(byte k = 1; k <= (enabledLaneCount<3?enabledLaneCount:3); k++){
    // Print place #
    lcd.setCursor(0,k);
    lcd.print(k);
    // leaderboard index starts with zero as 1st place
    if((leaderBoard[k-1][0]) == 0){
      // If the lap count is zero then don't print anything.
      PrintSpanOfChars(lcdDisp, k, 0, 11);
    } else {
      // Print lap count.
      PrintNumbers(leaderBoard[k-1][0], 3, 4, lcdDisp, true, k);
      // Print racer's name to LCD
      // PrintText(Racers[leaderBoard[k-1][1]], lcdDisp, 11, 6, false, k);
      PrintText(Racers[laneRacer[leaderBoard[k-1][1]]], lcdDisp, 11, 6, false, k);
    }
  }
  // Update fastest overall lap time, lap, and racer who achieved it
  PrintClock(overallFastestTime, 19, 6, 3, lcdDisp, 1, true);
  // PrintText(Racers[overallFastestRacer], lcdDisp, 19, 6, false, 3);
  PrintText(Racers[laneRacer[overallFastestRacer]], lcdDisp, 19, 6, false, 2);
  PrintText(RESULTS_TOP_TEXT_LAP, lcdDisp, 19, 6, false, 3);
  PrintNumbers(fastestLaps[overallFastestRacer][0], 3, 19, lcdDisp, false, 3);
  
} // END UpdateLiveRaceLCD()



void ResetRaceVars(){
  // Reset lap count and lap flash status for all lanes/racers to 0.
  for (byte i = 1; i <= laneCount; i++) {
    winner = false;
    lapCount[i] = 0;
    flashStatus[i] = 0;
    startMillis[i] = 0;
    currentTime[i] = 0;
    if(laneEnableStatus[i] > 0) laneEnableStatus[i] = StandBy;
  }
  InitializeRacerArrays();
  finishedCount = 0;
  overallFastestTime = 99999;
  overallFastestRacer = 0;
}



// function to iterate through the given audioStates
void toggleAudioMode() {
  switch (audioState) {
    case AllOn:
      audioState = GameOnly;
    break;
    case GameOnly:
      audioState = Mute;
    break;
    case Mute:
      audioState = AllOn;
    break;
    default:
    break;
  }
  UpdateAudioBools();
}

// Set audio boolean flags per audio state
void UpdateAudioBools() {
  switch (audioState) {
    case AllOn: {
      gameAudioOn = true;
      musicAudioOn = true;
    }
    break;
    case GameOnly: {
      gameAudioOn = true;
      musicAudioOn = false;
    }
    break;
    case Mute: {
      gameAudioOn = false;
      musicAudioOn = false;
    }
    break;
    default:
    break;
  } // END of switch block
}


// this function prints the current Audio component status to the settings screen
// This function assumes it is being called from the 'Settings' menu screen state.
void PrintAudioStatus() {
  if (gameAudioOn || musicAudioOn) {
    // clear any residual "-OFF-" text
    PrintSpanOfChars(lcdDisp, 0, 13, 16);
    // print or clear, game audio icon
    lcd.setCursor(14, 0);
    if (gameAudioOn) {
      lcd.write(4);
    } else {
      lcd.write(' ');
    };
    // print or clear, music audio icon
    lcd.setCursor(17, 0);
    if (musicAudioOn) {
      lcd.write(5);
    } else {
      lcd.write(' ');
    };
  } else {
    //else both audio are off, print '-OFF-' indicator
    lcd.setCursor(13, 0);
    lcd.write(TEXT_OFF);
  }
}

// ------------NOTE ARRAY AUDIO CODE-------------
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
// int PlayNote(int *songNotes, int *songLengths, int curNoteIdx, int tempoBPM){

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
//   // This can be done by slightly shortening the tone played vs the song tempo.
//   // or making the gap between notes slightly longer than the note length.
//   // In which case the actual tempo will be slightly slower than the set tempo.
//   // Here we'll factor the played tone down by 10% and keeping the tempo as set.
//   // Play note:
//   // pgm_read_word is used to retrieve the value from PROGMEM
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
// -------------END NOTE ARRAY AUDIO CODE-------------------






// *********************************************
// ***************** SETUP *********************
// Initialize hardware and establish software initial state
void setup(){
  // --- SETUP SERIAL ------------------------
  /*
  NOTE: Serial port is only used in debugging at the moment.
  The while loop waits for the serial connection to be established
  before moving on so we don't miss anything. If the Arduino seems to
  'hang', this may be the culprit if there is a connection issue.
  */
  // !!! Enabling Serial will cause about 4% increase in program memory
  // + memory for each printline used which are very costly, expect about +6% overall needed.
  // Open port and wait for connection before proceeding.
  // Serial.begin(9600);
  // while(!Serial);

  // --- SETUP LCD DIPSLAY -----------------------------
  // Initialize LCD with begin() which will return zero on success.
  // Non-zero failure status codes are defined in <hd44780.h>
	// lcd.begin(LCD_COLS, LCD_ROWS);
	int status = lcd.begin(LCD_COLS, LCD_ROWS);
  // If display initialization fails, trigger onboard error LED if exists.
	if(status) hd44780::fatalError(status);
  // Clear display of any residual data, ensure it starts in a blank state
  lcd.clear();
  // --- CREATE CUSTOM LCD CHARS -------------
  // uncommenting custom characters here may also require uncommenting the corresponding data in 'CustomChars.h'
  lcd.createChar(0, UpDownArrow);
  // lcd.createChar(1, UpArrow);
  // lcd.createChar(2, DownArrow);
  lcd.createChar(3, Skull);
  lcd.createChar(4, GameSound);
  lcd.createChar(5, MusicNote);
  // lcd.createChar(6, Check);

  // --- SETUP LED 7-SEG, 8-DIGIT MAX7219 LED Globals ------
  // Initialize all the displays
  for(int deviceID = 0; deviceID < LED_BAR_COUNT; deviceID++) {
    // The MAX72XX is in power-saving mode on startup
    lc.shutdown(deviceID, false);
    // intensity range from 0-15, higher = brighter
    lc.setIntensity(deviceID, 8);
    // Blank the LED digits
    lc.clearDisplay(deviceID);
  }

  // --- SETUP LAP TRIGGERS AND BUTTONS ----------------
  setTriggerMask();
  for (byte i = 1; i <= laneCount; i++){
    // Equivalent to digitalWrite(lane_Pin, HIGH)
    pinMode(lanes[i][0], INPUT_PULLUP);
    // Set all lanes to default, enabled status, StandBy
    laneEnableStatus[i] = StandBy;
    // Set default racer names for each lane.
    laneRacer[i] = i;
    // maintain a count of the enabled lanes for indexing loops to reference.
    enabledLaneCount++;
  }
  pinMode(pauseStopPin, INPUT);
  pinMode(startButtonPin, INPUT);

  // Setup an indicator LED
  // pinMode(ledPIN, OUTPUT); 
  // digitalWrite(ledPIN, ledState);

  // Initialize racer data arrays.
  ResetRaceVars();
  // Initialize racetime millisecond to default raceSetTime.
  raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
  // Set initial state to Menu and initial menu to MainMenu, turn initial entry flag on.
  // state = Menu;
  // entryFlag = true;
  ChangeStateTo(Menu);
  currentMenu = MainMenu;


  
  // Startup test song when using melody arrays.
  // melodyPlaying = true;
  // playingNotes = takeOnMeNotes;
  // playingLengths = takeOnMeLengths;
  // playingMelodySize = takeOnMeSize;
  // playingTempoBPM = takeOnMeTempo;

  // Startup song
  // startPlayRtttlPGM(buzzPin1, reveille);

  // Serial.println("setup enabled laneCount: ");
  // Serial.println(enabledLaneCount);
  
  // setup audio flags per default audio mode
  UpdateAudioBools();
  Beep();

// -------- BARGRAPH CODE ---------------
  // use actual address from documentation if not the same as '0x70'
  bar.begin(0x70);
// -------- END BARGRAPH CODE -----------

}
// ************* END SETUP *******************




// ***********************************************
// *************** MAIN LOOP *********************
void loop(){
  // Serial.println("MAIN LOOP START");
  // Serial.println(state);
  // This function is required to be called every loop to facilitate non-blocking audio.
  updatePlayRtttl();
  // ----- enable if using Note arrays ----------
  // if(melodyPlaying && musicAudioOn){
  //   if(millis() - lastNoteMillis >= noteDelay){
  //   // stop generation of square wave triggered by 'tone()'.
  //   // This is not required as all tones should stop at end of duration.
  //   // However, we do it just to make sure it 
  //     // noTone(buzzPin1);
  //     noteDelay = PlayNote(playingNotes, playingLengths, melodyIndex, playingTempoBPM);
  //   }
  // }
  // --------------------------------

  // The main loop is driven by switching between states and substates.
  switch (state) {
    // Serial.println("Entered Stat Switch");
    case Menu:{
      // In the 'Menu' state the program is focused on looking for keypad input
      // and using that keypad input to navigate the menu tree and adjust settings.
      // Serial.println("entering Menu STATE");
      char key = keypad.getKey();
      if (entryFlag) {
        // Clear the lap log interrupts on initial entry into menu state just to make sure,
        // but DON'T clear entry flag here, it is used at the menu level in the menu state.
        // This means these interrupts get disabled on every new menu, but keeps things simpler.
        EnablePinInterrupts(false);
        // if the last race was a Drag race then set data flag to 'no data'
        if (raceType == Drag) raceDataExists = false;
        // Turn on yellow startlights
        lc.clearDisplay(laneCount);
        for (byte i = 3; i < 6; i++){
          lc.setLed(laneCount, 0, i, true);
          lc.setLed(laneCount, 1, i, true);
        }
        setBargraph(LED_YELLOW);
      }
      // if (key || entryFlag) {
      if (key) {
        // Serial.print("key: ");
        // Serial.println(key);
        switch (key) {
          // distinguish back button sound from regular key press
          case '*':
            Boop();
            break;
          // stop any playing song if '#' is pressed while in menu state
          case '#':
            stopPlayRtttl();
            break;
          default:
            Beep();
            break;
        }
      };
      // All of the menus and sub-menus should been flattened to this single switch
      switch (currentMenu) {
        // The 'MainMenu' is the default and top level menu in the menu tree.
        // Within each menu case is another switch for each available key input.
        case MainMenu:{
          if (entryFlag) {
            UpdateLCDMenu(MainText);
            // Update Racer LED Displays
            UpdateAllNamesOnLEDs();
            entryFlag = false;
          }
          // Depending on seleted option, change menu state accordingly and set entry flag.
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
            UpdateLCDMenu(SettingsText);
            // Print the current audio modes active
            PrintAudioStatus();
            // Minute setting
            PrintNumbers(raceSetTime[1], 2, TIME_SETTING_POS, lcdDisp, true, 1);
            // Seconds setting
            PrintNumbers(raceSetTime[0], 2, TIME_SETTING_POS+3, lcdDisp, true, 1);
            // Lap count
            PrintNumbers(raceLaps, 3, 16, lcdDisp, true, 2);
            // Lane's Enabled
            PrintLaneSettings();
            // Update Racer LED Displays
            UpdateAllNamesOnLEDs();
            entryFlag = false;
          }
          switch (key) {
            // Audio Mode Select
            case 'A':{
              toggleAudioMode();
              PrintAudioStatus();
            }
            break;
            // TIME
            case 'B':{
              lcd.setCursor(13, 1);
              lcd.print("mm:ss");
              // Change minutes
              raceSetTime[1] = EditNumber(2, 60, 1, 13);
              // Change seconds
              raceSetTime[0] = EditNumber(2, 59, 1, 16);
              // update the race time in ms
              raceSetTimeMs = ClockToMillis(0, raceSetTime[1], raceSetTime[0]);
            }
            break;
            // LAP
            case 'C':{
              // change lap count
              raceLaps = EditNumber(3, 999, 2, 14);
            }
            break;
            // ENABLED LANES
            // If a number is pressed in menu state change enabled lane.
            case '0' ... '4':{
              ToggleLaneEnable(key - '0');
              // Update LCD with change
              PrintLaneSettings();
              // Update racer displays
              UpdateAllNamesOnLEDs();
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
            UpdateLCDMenu(SelectRacersText);
            // Write all current racer names to LCD
            for(byte i = 1; i <= laneCount; i++){
              // If the lane is disabled
              if(laneEnableStatus[i] == 0){
                // Print Skull icon
                lcd.setCursor(nameEndPos - 10, i - 1);
                lcd.write(3);
                // Then print disabled lane label (ie. '-Off-')
                PrintText(Racers[0], lcdDisp, nameEndPos, 9, false, i - 1);
                // and another skull
                lcd.setCursor(nameEndPos - 7 + strlen(Racers[laneRacer[0]]), i - 1);
                lcd.write(3);
              } else {
                PrintText(Racers[laneRacer[i]], lcdDisp, nameEndPos, 11, false, i - 1);
              }
            }
            entryFlag = false;
          }
          switch (key) {
            // Cycle racer name on selected row.
            case 'A' ... 'D':{
              byte laneNumber = key - 'A' + 1;
              // Make sure lane is enabled and exists before trying to set a racer name
              if(laneEnableStatus[laneNumber] && (laneNumber <= laneCount)){
                // Cycle to next racer name, if end of list, start back at 1 not 0.
                // The zero index is reserved for the disabled lane label.
                laneRacer[laneNumber] = IndexRacer(laneNumber);
                // Update LCD with new racer name
                PrintText(Racers[ laneRacer[laneNumber] ], lcdDisp, nameEndPos, 11, false, laneNumber-1);
                // Update Racer's LED
                PrintText(Racers[ laneRacer[laneNumber] ], displays(laneNumber), 7, 8);
                // Play racers victory song
                if (musicAudioOn && !SONGS_BY_PLACE) startPlayRtttlPGM(buzzPin1, victorySong[ laneRacer[laneNumber] ]);
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
          } // END of key switch
        } // END SelectRacers Menu Case
        break;

        case StartRaceMenu: {
          if (entryFlag) {
            // Draw non-editable text
            UpdateLCDMenu(StartRaceText);
            // Print set LAPS #
            PrintNumbers(raceLaps, 3, START_RACE_LAPS_ENDPOS_IDX, lcdDisp, true, 0);
            // Print set race time MINUTES
            PrintNumbers(raceSetTime[1], 2, START_RACE_TIME_ENDPOS_IDX-3, lcdDisp, true, 1);
            PrintText(":", lcdDisp, START_RACE_TIME_ENDPOS_IDX-2, 1, true, 1);
            // Print set race time SECONDS
            PrintNumbers(raceSetTime[0], 2, START_RACE_TIME_ENDPOS_IDX, lcdDisp, true, 1);
            // Print sey preStartCountDown
            PrintNumbers(preStartCountDown, 2, START_RACE_CNTDWN_ENDPOS_IDX, lcdDisp, true, 3);
            entryFlag = false;
          }
          switch (key) {
            // Laps or Timed Race Selected
            case 'A': case 'B': case 'C': {
              endLap = raceLaps;
              countingDown = false;
              if (key == 'A') raceType = Standard;
              else if (key == 'B') {raceType = Timed; countingDown = true;}
              else if (key == 'C') {raceType = Drag; endLap = 1;}
              ChangeStateTo(Staging);
            }
            break;
            case 'D': {
              // Change duration of pre-race countdown.
              preStartCountDown = EditNumber(2, 30, 3, 18);
            }
            break;
            case '*': {
              // Return to main menu
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
          curMillis = millis();
          if (entryFlag) {
            lastXMillis[0][0] = curMillis;
            // lcd.clear();
            if(raceDataExists){
              resultsMenuIdx = 0;
              lcd.print(COMPILING);
              CompileTopFastest();
              UpdateResultsMenu();
            } else {
              lcd.clear();
              lcd.print(NO_RACE_DATA);
            }
            entryFlag = false;
          }
          // This block tracks when to flip between,
          // title A (Racer # label) and B (racer name)
          // only execute if on an individual racer screen, not Top, or Final results.
          if ( (curMillis - lastXMillis[0][0] > RESULTS_RACER_BLINK)
                && (resultsMenuIdx != 0)
                && (resultsMenuIdx != laneCount+1) ) {
            titleA = !titleA;
            lastXMillis[0][0] = curMillis;
            UpdateResultsMenu(false);
            // Serial.println("lastMillis trigger: ");
          }
          switch (key) {
            // Scroll up or down the lap results list
            case 'A': case 'B':{
              // Only deal with data if it exists,
              // otherwise garbage will be displayed on screen.
              if(raceDataExists) {
                // Clear lines to remove extra ch from long names replace by short ones.
                PrintSpanOfChars(lcdDisp, 1);
                PrintSpanOfChars(lcdDisp, 2);
                PrintSpanOfChars(lcdDisp, 3);
                if (key == 'A' && resultsRowIdx > 0) resultsRowIdx--;
                // Subtract 2 because there are two rows printed after tracked index.
                if (key == 'B' && resultsRowIdx < DEFAULT_MAX_STORED_LAPS - 2) resultsRowIdx++;
                UpdateResultsMenu();
                // lastXMillis[0][0] = curMillis;
              }
            }
            break;
            // Cycle through results sub-menus.
            case 'C': case 'D':{
              // Only deal with data if it exists,
              // otherwise garbage will be displayed on screen
              if(raceDataExists) {
                // Index results menu to next racer.
                // The +2 is to account for top results and leader board pages.
                resultsMenuIdx = (resultsMenuIdx + 1) % (laneCount + 2);
                // Reset row index to 0 so new list starts at the top
                resultsRowIdx = 0;
                UpdateResultsMenu();
                // lastXMillis[0][0] = curMillis;
                // Serial.println("result idx");
                // Serial.println(resultsRowIdx);
                // Serial.println(key);
              }
            }
            break;
            case '*':{
              // Return to MainMenu
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
      }; // END of Menu switch
    } // END of Menu State
    break;

    // *********************************
    // *****  PRE-STAGE STATE  *********
    // The Staging state is held to allow racers for next heat
    // to put cars in the race ready, stagged positions.
    case Staging: {
      if (entryFlag) {
        // Serial.println(F("PreStg"));
        // Clear bargraph, and set LEDs for pre-stage pattern
        setBargraph(LED_OFF);
        // Clear MAX7219 start light
        lc.clearDisplay(laneCount);
        // Clear old screen text from LCD
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(F("---- Pre-Stage ----"));
        lcd.setCursor(7, 2);
        lcd.print(F("Get In"));
        lcd.setCursor(1, 3);
        lcd.print(F("Starting Positions"));
        lcd.setCursor(1, 0);
        lcd.print(F("*P|Exit    #S|Race"));
        switch (raceType) {
          case Drag: {
            // For drag racing, drop racer names and label the lanes
            PrintText("Lane   1", led1Disp, 7, 8);
            PrintText("Lane   2", led2Disp, 7, 8);
          }
          break;
          default:
          break;
        }
        setBargraph(LED_YELLOW, 12, 11);
        // Turn on MAX7219 pre-staging lights
        // 'laneCount' should be the device ID of the LED startlight, assume all lanes have a display.
        lc.setLed(laneCount, 0, 1, true);
        lc.setLed(laneCount, 1, 1, true);
        entryFlag = false;
      } // END of entryFlag

      // After entry, monitor for user input to decide next state
      char key = keypad.getKey();
      // If Start button or '#' key is pressed, initiate a race, and switch to PreStart state.
      if( buttonPressed(startButtonPin) || key == '#' ) {
        ChangeStateTo(PreStart);
        preStartTimerDrag = curMillis%5 + 2;
        newRace = true;
        // Serial.println("dgbt");
      } else
      // If Pause button, or '*' key is pressed, exit back to the Main Menu.
      if( buttonPressed(pauseStopPin)  || key == '*') {
        // clear start light displays
        lc.clearDisplay(laneCount);
        setBargraph(LED_OFF);
        // Move to Menu state
        ChangeStateTo(Menu);
        currentMenu = MainMenu;
        // Serial.println("psbt");
      }
    }
    break;

    // *********************************
    // *****  PRE-START state  *********
    // Pre-Start countdown period
    case PreStart: {
      curMillis = millis();
      if (entryFlag) {
        // Serial.println(F("Pst"));
        // Set live race time to preStartCountDown wich is in seconds, so convert to millis.
        currentTime[0] = (raceType == Drag ? preStartTimerDrag : preStartCountDown) * 1000;
        // Record current loop's ms timestamp to track display update, tick time.
        lastTickMillis = curMillis;
        lcd.clear();
        // If not restarting from a Pause, reset racer variables to initial values.
        if (newRace) ResetRaceVars();
        if (raceType == Drag){
          // Update the LCD to indicate race is now 'staged', aka in active pre-start state.
          lcd.setCursor(2,0);
          lcd.print(F("Cars Are Staged"));
          lcd.setCursor(7, 2);
          lcd.print(F("Ready!"));
          if(laneEnableStatus[1]) PrintText("Ready", led1Disp, 7, 8);
          if(laneEnableStatus[2]) PrintText("Ready", led2Disp, 7, 8);
          // set the countdown timer threshold to start updating pre-start LEDs
          nextStageCountdownTime = 1500;
          // set interval in ms that pre-start LEDs should update after threshold.
          preStartTick = 500;
          ledCountdownTemp = 1;
          // set initial staged phase pattern to LED bargraph
          // setBargraph(LED_OFF);
          setBargraph(LED_YELLOW, 14, 9);
          // MAX7219 startlight
          lc.setLed(laneCount, 0, 2, true);
          lc.setLed(laneCount, 1, 2, true);
          // set flag, triggering the race state to clear start lights after a delay.
          // clearStartLight = true;
          // enable triggers only on lanes 1 and 2
          // EnablePinInterrupts(true);
          if(laneEnableStatus[1]) pciSetup(lanes[1][0]);
          if(laneEnableStatus[2]) pciSetup(lanes[2][0]);
        } else {
        // Else, if a circuit race (standard or timed) then,
          // Draw pre-start circuit race text to LCD and LEDs
          PreStartDisplaysUpdate();
          // set the countdown timer threshold to start updating start light LEDs in final 3sec
          nextStageCountdownTime = 3000;
          // set interval in ms that pre-start LEDs should update after threshold.
          preStartTick = 1000;
          // set the start index for circuit races which will count index down by 1 for each interval
          ledCountdownTemp = 3;
          // set entire LED Bargraph to Red
          setBargraph(LED_RED);
          // MAX7219 startlight
          lc.setLed(laneCount, 0, 1, true);
          lc.setLed(laneCount, 1, 1, true);
          // Turn on interrupts for enabled lane pins
          EnablePinInterrupts(true);
        }
        // set flag to trigger turning off startlights after a delay.
        clearStartLight = true;
        entryFlag = false;
      } // END 'entryFlag' conditional

      // If the display update tick time has passed, update elapsed race time on LCD
      if (curMillis - lastTickMillis > displayTick){
        currentTime[0] = currentTime[0] - displayTick;
        // If not a drag race, update timer on LCD
        if (raceType != Drag) {
            PrintClock(currentTime[0], PRESTART_CLK_POS, 4, 2, lcdDisp, 2);
        } 
        lastTickMillis = curMillis;
      }

      // If pre-start countdown timer greater than 0, execute updates
      if (currentTime[0] > 0){
        // If remaining pre-start countdown drops below next update period, process new period
        if (currentTime[0] < nextStageCountdownTime) {
          nextStageCountdownTime -= preStartTick;
          if (raceType == Drag){
            // for the first cycle of the final countdown ticks, indicate final 3 ticks have begun
            if (ledCountdownTemp == 1){
              lcd.clear();
              lcd.setCursor(8, 2);
              lcd.print(F("Set!"));
              // clear the racer LED dipslays
              if(laneEnableStatus[1]) lc.clearDisplay(led1Disp-1);
              if(laneEnableStatus[2]) lc.clearDisplay(led2Disp-1);
              // clear, ready & set lights on MAX7219 start light tree.
              lc.clearDisplay(laneCount);
              // lc.setLed(laneCount, 1, 2+ledCountdownTemp, false);
            }
            // write start light final ticks to Racer LEDs
            if(laneEnableStatus[1]) PrintText("--", led1Disp, 1+3*(ledCountdownTemp-1), 2);
            if(laneEnableStatus[2]) PrintText("--", led2Disp, 1+3*(ledCountdownTemp-1), 2);
            // Add next start tick to Adafruit bar
            setBargraph(LED_YELLOW, (10 - ledCountdownTemp*3), (9 - ledCountdownTemp*3));
            setBargraph(LED_YELLOW, (14 + ledCountdownTemp*3), (13 + ledCountdownTemp*3));
            // update MAX7219 start light; light the next yellow LED
            // lc.setLed(laneCount, 0, 1+ledCountdownTemp, false);
            // lc.setLed(laneCount, 1, 1+ledCountdownTemp, false);
            lc.clearDisplay(laneCount);
            lc.setLed(laneCount, 0, 2+ledCountdownTemp, true);
            lc.setLed(laneCount, 1, 2+ledCountdownTemp, true);
            ledCountdownTemp++;
          } else { // if a circuit race (ie Standard or Timed)
            // Write current seconds digit to all active LEDs
            ledWriteDigits(ledCountdownTemp);
            // set next 3rd of LED bar yellow
            setBargraph(LED_YELLOW, ((ledCountdownTemp*8)-1), (ledCountdownTemp-1)*8);
            // update MAX7219 start light; light the next yellow LED
            lc.setLed(laneCount, 0, 3+(3-ledCountdownTemp), true);
            lc.setLed(laneCount, 1, 3+(3-ledCountdownTemp), true);
            ledCountdownTemp--;
          }
          Beep();
        }
      } else { // countdown is 0
        // The prestart state has finished
        if (raceType == Drag) {
          // for drag race, update LCD
          lcd.setCursor(0, 2);
          lcd.print(F("Go! Go! Go! Go! Go! "));
        }
        // reset ledCountdownTemp to default for next race
        ledCountdownTemp = 0;
        // Turn adafruit start light to green
        setBargraph(LED_GREEN);
        // Turn on MAX7219 start light tree - green LEDs
        lc.clearDisplay(laneCount);
        lc.setLed(laneCount, 0, 6, true);
        lc.setLed(laneCount, 1, 6, true);
        // Adjust race clock start time to ignore the paused period
        // currentTime[0] is the elapsed race time in ms at time of pause.
        
        // currentTime[0] = (raceType == Drag ? preStartTimerDrag : preStartCountDown) * 1000;
        startMillis[0] = startMillis[0] + (raceType == Drag ? preStartTimerDrag : preStartCountDown) * 1000;
        // Move on to live race, it's now legal for racers to cross start
        ChangeStateTo(Race);
      }
    } // END of PreStart state
    break;

    // *****************************************
    // **********  RACE state  *****************
    // The 'Race' state manages the active race.
    case Race:{
      curMillis = millis();
      // First cycle initialization of RACE and signalling of START.
      if (entryFlag) {
        // Serial.println(F("rce"));
        if(newRace){
          // make sure running race time is starting from 0
          currentTime[0] = 0;
          // record the ms clock time at race start
          startMillis[0] = curMillis;
          if (raceType == Drag) {
            // set lanes Active on race start and set current lap to 1
            for(byte i = 1; i <= laneCount; i++){
              if(laneEnableStatus[i] == StandBy){
                laneEnableStatus[i] = Active;
                // If using a start and finish line triggers, set lapCount to 0 (or 'false')
                // If using a finish line only, set lapCount to 1 (or 'true')
                lapCount[i] = SINGLE_DRAG_TRIGGER;
                flashStatus[i] = 1;
                startMillis[i] = curMillis;
              }
            }
          } else {
            // lcd.clear();
          }
          newRace = false;
        }
        // Cycle through possible lanes and write start notification to racer displays.
        for(byte i = 1; i <= laneCount; i++){
          if(laneEnableStatus[i] == StandBy){
            PrintText(Start, displays(i), 7, 8, true, 0, true);
          } else if(laneEnableStatus[i] == Off){
            PrintText(Racers[0], displays(i), 7, 8, true, 0, true);
          }
          // Set flash status to default, idle state.
          flashStatus[i] = 0;
          Bleep();
        }
        switch (raceType) {
          case Drag: {
            // lcd.setCursor(0, 1);
            // lcd.print(F("--Lane 1----Lane 2--"));
            // PrintSpanOfChars(lcdDisp, 1);
          }
          break;
          default: {
            // Write static text to main LCD for live race screen
            PrintLeaderBoard(false);
          }
          break;
        }
        // Reset display tick timestamp to current loop's timestamp.
        lastTickMillis = curMillis;
        entryFlag = false;
      } // END Race State entryFlag

      // ********* LIVE RACE **********
      // For Circuit Racing, after 2sec (ie 2000ms), turn start lights off.
      if(clearStartLight && raceType != Drag){
        if ((curMillis - startMillis[0]) >= START_LIGHT_OFF_DELAY) {
          // clear Adafruit Bargraph
          setBargraph(LED_OFF);
          clearStartLight = false;
          // clear MAX7219 start light tree
          lc.clearDisplay(laneCount);
        }
      }
      // Update current racetime.
      if (countingDown) {
        // When counting down we need to gaurd against negatives.
        currentTime[0] = raceSetTimeMs < (curMillis - startMillis[0]) ? 0 : raceSetTimeMs - (curMillis - startMillis[0]);
      } else {
        // Lap based race time counts up until final lap finished by 1st racer.
        currentTime[0] = curMillis - startMillis[0];
      }

      // For each possible lane check its status and process the data and displays accordingly.
      for(byte i = 1; i <= laneCount; i++){
        // Only bother to process lanes with 'Active' status.
        if(laneEnableStatus[i] == Active) {
          // Update elapsed, ms time for lane.
          // If it's a drag race, use the race time, timestamp, not the racer's lap start timestamp.
          currentTime[i] = curMillis - (raceType == Drag ? startMillis[0] : startMillis[i]);
          // Check the lane's flash status.
          switch(flashStatus[i]){
            // If Flash OFF - update running laptime to LED displays
            case 0:{
              if (curMillis - lastTickMillis >  displayTick){
                PrintNumbers(lapCount[i], 3, 2, displays(i));
                PrintClock(currentTime[i], 7, 5, 1, displays(i), 0);
              }
            }
            break;
            // If Flash START - a lap has just been completed, show its time on LED and process its data.
            case 1: {
              unsigned long lapTimeToLog;
              // Log the loop timestamp at start of this flash period.
              flashStartMillis[i] = curMillis;
              // Calculate the lap time of the just finished lap.
              // If it's a drag race use the race start time instead of lane's lap start trigger.

              switch (raceType) {
                case Drag: {
                  // lapTimeToLog = lastXMillis [i] [lapCount[i]-1] - startMillis[0];
                  lapTimeToLog = curMillis - startMillis[0];
                  flashStatus[i] = 0;
                }
                break;
                // Circuit Racing types, Standard & Timed
                default: {
                  lapTimeToLog = lastXMillis [i] [(lapCount[i]-1) % lapMillisQSize] - lastXMillis [i] [(lapCount[i]-2) % lapMillisQSize];
                  UpdateFastestLap(fastestTimes[i], fastestLaps[i], (lapCount[i] - 1), lapTimeToLog, laneRacer[i], DEFAULT_MAX_STORED_LAPS);
                  // Update the racer's LED with the completed lap # and laptime.
                  lc.clearDisplay( displays(i) - 1 );
                  // print the just completed lap # to left side of racer's  LED
                  PrintNumbers( (lapCount[i] - 1), 3, 2, displays(i));
                  // print the lap time of just completed lap to right side of racer's LED
                  PrintClock(lapTimeToLog, 7, 4, 3, displays(i));
                  UpdateLiveRaceLCD();
                  flashStatus[i] = 2;
                }
                break;
              }
              // update the total run time for racer
              racersTotalTime[i] = racersTotalTime[i] + lapTimeToLog;
              // Set the Results Menu data exist flag to true.
              raceDataExists = true;
            }
            break;
            // Flash HOLD - do nothing until flash period has elapsed.
            case 2:{
              // if flash time is up, set flag to zero ending display of last lap
              if (curMillis - flashStartMillis[i] > flashDisplayTime) flashStatus[i] = 0;
            }
            break;
            default:
            break;
          } // END flashStatus switch
        } else {
        // else if NOT an 'Active' lane
          currentTime[i] = 0;
        } // END of if then else, lane enabled, ie 'Active'.
      } // END for each lane loop
      
      // If a tick has passed then update displays.
      if (curMillis - lastTickMillis >  displayTick){
        // Update the main LCD
        if (raceType != Drag) PrintClock(currentTime[0], RACE_CLK_POS, 10, 1, lcdDisp, 0, true);
        lastTickMillis = curMillis;
      }

      // FINISHING CHECK - check for the race end conditions.
      switch (raceType) {
        case Standard: case Drag:{
          // ****** STANDARD FINSIH *******************
          // Check if any Active lanes have since finished.
          for(byte i = 1; i <= laneCount; i++){
            if(laneEnableStatus[i] == Active){
              // If a lanes's current lapCount is greater than the 'endLap' that means
              // they have finished the race. lapCount of an 'Active' lane, is +1 of finsished laps.
              // Because the final lap count can be updated at any time via interrupts,
              // we must also verify that the flash status is not still '1'.
              // If flash status = 1 then lap data has not been processed yet
              // so do not process racer as finished until that is done.
              if( (lapCount[i] > endLap) && (flashStatus[i] != 1)){
                // Change racer's status to finished
                laneEnableStatus[i] = Finished;
                // Turn off lap trigger interrupt of finished lane.
                clearPCI(lanes[i][0]);
                finishedCount++;
                // Update the racer's LED display with their finishing place.
                if (raceType == Drag) {
                  PrintClock(startMillis[i]-startMillis[0], 7, 5, 3, displays(i), 0);
                  // For a Drag Race, light up LED of 1st lane to finish
                  // if (finishedCount == 0) {
                  if (!winner) {
                    lc.clearDisplay(laneCount);
                    lc.setLed(laneCount, i-1, 0, true);
                    setBargraph(LED_OFF);
                    // index (i), will be 1 or 2, but the MAX7219 index is zero based.
                    // lane 1 should light LEDs 0-11; lane 2 should light LEDs 12-23.
                    setBargraph(LED_GREEN, i*12 - 1, (i-1)*12);
                    winner = true;
                  }
                } else { // if circuit race (ie Standard or Timed)
                  UpdateNameOnLED(i);
                  // Stop any currently playing song
                  stopPlayRtttl();
                  // Play finishing song of finishing racer
                  if (SONGS_BY_PLACE) {
                    if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[finishedCount]);
                  } else {
                    if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[laneRacer[i]]);
                  }
                }
                // // Stop any currently playing song
                // stopPlayRtttl();
                // // Play finishing song of finishing racer
                // if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[laneRacer[i]]);
                // Serial.print("Finished count: ");
                // Serial.println(finishedCount);
              } // END if Finish lapcount reached
            } // END if lane is active
          } // END for each lane
          // The race continues until all racers have finsihed.
          // If drag racing then only 2 racers should be used regardless of enabled laneCount
          if (finishedCount == (raceType == Drag ? 2 : enabledLaneCount)) {
            // If all racers have completed all laps, transition to Finished state.
            ChangeStateTo(Finish);
            // Serial.print("Finished then enabled count: ");
            // Serial.println(finishedCount);
            // Serial.println(enabledLaneCount);
          }
        } // END Standard Race Finishing case
        break;
        case Timed:{
          // ****** TIMED FINSIH *******************
          // if time <= 0 then race is over.
          if (currentTime[0] <= 0) {
            // Turn off all lap trigger interrupts.
            EnablePinInterrupts(false);
            for(byte i = 1; i <= laneCount; i++){
              if(laneEnableStatus[i] == Active){
                // Set lanes to finished state and write places to LEDs.
                laneEnableStatus[i] = Finished;
                UpdateNameOnLED(i);
              }
            }
            finishedCount = laneCount;
            // Stop any currently playing song
            stopPlayRtttl();
            // Play finishing song of 1st place racer.
            if (SONGS_BY_PLACE) {
              if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[1]);
            } else {
              if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[laneRacer[leaderBoard[0][1]]]);
            }
            // if (musicAudioOn) startPlayRtttlPGM(buzzPin1, victorySong[laneRacer[leaderBoard[0][1]]]);
            ChangeStateTo(Finish);
          }
        } // END Timed race case
        break;
        default:
        break;
      } // END RaceType Switch (Finishing Check)

      // Check analog pause button for press, debouncing is done in 'buttonPressed'
      if (buttonPressed(pauseStopPin)) ChangeStateTo(Paused);

    } // END of Race state case
    break;

    // *****************************************
    // **********  PAUSED state  *****************
    // The Paused state handles interaction to quite or restart race
    case Paused:{
      // Serial.println("Entered Paused state"); 
      if(entryFlag){
        // immediately turn off the interrupts on the lap sensing pins
        EnablePinInterrupts(false);
        // Put all 'Active' race lanes into 'StandBy'.
        for (byte i = 1; i <= laneCount; i++){
          if(laneEnableStatus[i] == Active) {
            laneEnableStatus[i] = StandBy;
            // lapCount[i] -= 1;
          }
          PrintText(TEXT_PAUSE, displays(i), 7, 5, true, 0, false);
        }
        // Turn entry flag off for next loop
        entryFlag = false;
      } // END entryFlag

      // check if pause button or '*' key has been pressed
      if (buttonPressed(pauseStopPin) || buttonPressed(startButtonPin)) {
        // ChangeStateTo(Race);
        ChangeStateTo((CTDWN_ON_RESTART ? PreStart : Race));
        unsigned long logMillis = millis();
        // reset the lap start timestamp, and ms elapsed lap time, of current lap for each racer
        for(byte i = 1; i <= laneCount; i++){
          startMillis[i] = logMillis;
          currentTime[i] = 0;
        }
        // Adjust race clock start time to ignore the paused period
        // currentTime[0] is the elapsed race time in ms at time of pause.
        startMillis[0] = startMillis[0] + (logMillis - startMillis[0] - currentTime[0]);
        // Re-enable lap triggers on active lanes.
        EnablePinInterrupts(true);
      } else {
        // Check if star is pressed on keypad, if so then end game and got to results.
        char key = keypad.getKey();
        if (key == '*') {
          UpdateAllNamesOnLEDs();
          ChangeStateTo(Finish);
          if (gameAudioOn) Boop();
        }
      }

    } // END of Paused state
    break;

    // Because a Fault is triggered by an interrupt, it can happen at any time.
    // Since all States share the same 'entryFlag', we need to use an
    // intermediate state, that is flag agnostic, to avoid flipping it at the wrong place.
    case PreFault:{
      ChangeStateTo(Fault);
    }
    break;
    // *******************************************
    // **********  FAULT State  *****************
    // This state is triggered if a racer crosses the start line while in the PreStart state.
    case Fault: {
      // Serial.println("fltReg");
      if (entryFlag){
        // Serial.println(F("Flt"));
        // Turn off interrupts for enabled lane pins
        EnablePinInterrupts(false);
        byte lnNum = 1;
        lcd.clear();
        lcd.print(F("Start Fault by:"));
        // Serial.println(lastTriggeredPins);

        while( (lastTriggeredPins > 0) && (lnNum <= laneCount) ){
          byte faultCount = 0;
          // If bit i is a 1, then process it as a trigger on that lane number
          if(lastTriggeredPins & lanes[lnNum][1]){
            faultCount++;
            if (raceType == Drag){
              PrintText("Lane ", lcdDisp, 3, 4, false, faultCount);
              PrintNumbers(lnNum, 1, 5, lcdDisp, false, 1);
            } else {
              PrintText(Racers[laneRacer[lnNum]], lcdDisp, 19, 20, false, faultCount);
            }
            // setLed(deviceID, digit index, segment, On?)
            lc.setLed(laneCount, lnNum-1, 7, true);
            setBargraph(LED_OFF);
            setBargraph(LED_RED, lnNum*12 - 1, (lnNum-1)*12);
          }
          // Move to next digit of faulting lanes
          lastTriggeredPins = lastTriggeredPins & ~lanes[lnNum][1];
          lnNum++;
          // lcd.setCursor(0, 3);
          // PrintText(Racers[laneRacer[lnNum]], lcdDisp, 19, 20, false, 3);
        }
        // ensure lastTriggeredPins is reset to 0, though it already should be.
        lastTriggeredPins = 0;
        entryFlag = false;
      } // END if(entryFlag)

      char key = keypad.getKey();
      if( buttonPressed(pauseStopPin) || buttonPressed(startButtonPin) || key == '*' || key == '#') {
        lcd.clear();
        // Clear bargraph and/or LED start tree
        lc.clearDisplay(laneCount);
        setBargraph(LED_OFF);
        ChangeStateTo(Staging);
      }

    } // END of Fault state
    break;

    // *****************************************
    // **********  FINISH state  ***************
    // The Finish state is entered at the conclusion of a race.
    // It provides a chance for the controller to execute end of race actions,
    // as well as a chance to receive user input.
    case Finish: {
      if (entryFlag) {
        // Serial.println(F("fnsh"));
        switch (raceType) {
          case Drag: {
            // Print finish text to LCD
            // lcd.clear();            
            lcd.setCursor(0, 1);
            lcd.print(F("--Lane 1----Lane 2--"));
            PrintSpanOfChars(lcdDisp, 2);
            // Set the fastest laps idx 0, to be lap #1
            fastestLaps[1][0] = 1;
            fastestLaps[2][0] = 1;
            // log drag finish time (lap 2 start - race start), as the top fastest lap for racer
            fastestTimes[1][0] = startMillis[1] - startMillis[0];
            fastestTimes[2][0] = startMillis[2] - startMillis[0];
            // correct total time to only include drag race time for each lane.
            racersTotalTime[1] = fastestTimes[1][0];
            racersTotalTime[2] = fastestTimes[2][0];
            PrintClock(fastestTimes[1][0], 7, 5, 3, lcdDisp, 2);
            PrintClock(fastestTimes[2][0], 17, 5, 3, lcdDisp, 2);

            // UpdateFastestLap(fastestTimes[i], fastestLaps[i], (lapCount[i] - 1), fastestTimes[1][0], laneRacer[i], DEFAULT_MAX_STORED_LAPS);
            if (fastestTimes[1][0] < fastestTimes[2][0]) {
              PrintText("Winner", lcdDisp, 7, 6, false, 3);
            } else if (fastestTimes[2][0] < fastestTimes[1][0]) {
              PrintText("Winner", lcdDisp, 17, 6, false, 3);
            }
            lcd.setCursor(1, 0);
            lcd.print(F("*P|Exit    #S|Race"));
          } // END Drag case
          break;
          default:
          break;
        } // END of raceType switch

        // Reset lane states to StandBay
        for (byte i = 1; i <= laneCount; i++){
          if(laneEnableStatus[i] > 0) laneEnableStatus[i] = StandBy;
        }
        entryFlag = false;

      } // END of Finish state entryFlag

      switch (raceType) {
        // If in drag race mode, then wait for user input to exit or return to staging.
        case Drag:{
          char key = keypad.getKey();
          if (buttonPressed(startButtonPin) || key == '#'){
            ChangeStateTo(Staging);
          } else if ( buttonPressed(pauseStopPin) || key == '*') {
            ChangeStateTo(Menu);
            currentMenu = MainMenu;
          }
        }
        break;
        // Timed and Standard races fall into the default
        // If it's a circuit race, then default to exiting to results menu on finish.
        default: {
          ChangeStateTo(Menu);
          currentMenu = ResultsMenu;
          resultsMenuIdx = 0;
          resultsRowIdx = 0;
        }
        break;
      }
    } // END of Finish State
    break;

    default:{
      // if the state becomes unknown then default back to 'Menu'
      // Serial.println("Entered default state");
      state = Menu;
    }
    break;
  } // END of States Switch

} // END of MAIN LOOP
// ********************************************************
// ********************************************************






// This function updates all the Racer names on the LEDS according to lane status.
void UpdateAllNamesOnLEDs(){
  for (byte i = 1; i <= laneCount; i++){
    UpdateNameOnLED(i);
  }
}

// Writes the place finish to racer's LED display if in finished state.
void UpdateNameOnLED(byte lane){
  // if the lane is 'Finished' then check place
  if(laneEnableStatus[lane] == Finished){
    // Find what place the racer is in
    for(byte i = 1; i <= laneCount; i++){
      // If the racer has finished then write their place then name.
      if(leaderBoard[i-1][1] == lane){
        // remember strings have an end string character so "DEF" is a 4 element char array.
        char finishPlace[] = "DEF";
        // Get place text prefix string
        strcpy_P(finishPlace, (char*)pgm_read_word(&(FinishPlaceText[i])));
        PrintText(finishPlace, displays(lane), 2, 3, false, 0, false);
        PrintText(Racers[laneRacer[lane]], displays(lane), 7, 4, false, 0, true);
        // Serial.print("Finished plce LED: ");
        // Serial.println(finishPlace);
      }
      // If the racer has not finished don't do anything.
    }
  } else
  // else if lane is in StandBy just print the name
  if(laneEnableStatus[lane] == StandBy){
    PrintText(Racers[laneRacer[lane]], displays(lane), 7, 8);
  } else
  // else if lane is Off, print label held in resreve idx, Racers[0].
  if(laneEnableStatus[lane] == Off){
    PrintText(Racers[laneRacer[0]], displays(lane), 7, 8);
  }
} // END UpdateNameOnLED()


// Integer power function as pow() is only good with floats
int ipow(int base, int exp){
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
int EditNumber(int digits, int maxValue, int line, int cursorPos){
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
      }
      break;
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

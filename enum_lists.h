
// using an enum to define reference id for attached diplays.
// Main LCD display should have index 0
// Each LED display should have a value equal to its lane #.
typedef enum: uint8_t {
  lcdDisp = 0,
  led1Disp,
  led2Disp,
  led3Disp,
  led4Disp
} displays;


// create enum to hold possible state values
typedef enum: uint8_t {
  Menu,
  Race,
  Paused,
  Fault,
  PreStart,
  Staging,
  Finish
} states;


// Lane states to indicate different status conditions a racer can be in.
// enums default to these values, but they are written in explicitly
// in order to indicate it's important to preserve those values.
typedef enum: uint8_t{
  Off = 0,      // lane is not being used
  Active = 1,   // lane is being used and live in a race
  StandBy = 2,  // lane is being used in a race, but currently not active
  Finished = 3  // lane has finished the race
} laneState ;


// Racetype options
typedef enum: uint8_t {
  Standard,  // First to finish the set number of laps
  Timed,     // Finish the most laps before time runs out
  Drag       // Drag racing mode first to 2nd lane trigger
} races;


// used to define desired clock time width when written to display;
typedef enum: uint8_t {
  S = 2,    // 00
  M = 4,    // 00:00
  H = 6,    // 00:00:00
} clockWidth;


// enum to name menu screens for easier reference
typedef enum: uint8_t {
  MainMenu,
    SettingsMenu,
    SelectRacersMenu,
    StartRaceMenu,
    ResultsMenu
} Menus;


// enum to use names with context instead of raw numbers when coding audio state
typedef enum: uint8_t {
  AllOn,
  GameOnly,
  Mute
} audioModes;


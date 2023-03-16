// LOCAL SETTING CUSTOMIZATIONS
// The 'localSettings.h' file provides a place for users to make local customizations that will not get overritten on subsequent software updates.

// To create a new 'localSettings.h' file copy and rename the 'example.localSettings.h' file included with the project.
// If using git, edit your local .gitignore to ignore your 'localSettings.h' file. If using the default ignore from the clone it may already be there.


// DEFINE DIRECTIVES
// Define's are a pre-compiler macro that finds every instance of a token name in the code and replaces it with the associated replacement string.

// Each '#define' directive is made of three parts seperated by a single space
// Part 1 - the '#define' declaration
// Part 2 - the TOKEN_NAME to be replaced, written in all caps (by convention).
// Part 3 - the replacement string expression.
// ONLY EDIT Part 3 with the newly desired value/term/expression

// To make customization of a setting, uncomment it and edit the replacement string.
// If the setting does not exist, but is in the defaultSettings file, copy the #define line only and paste it in the 'localSettings.h' file.

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// See the 'RaceTimerAndController.ino' for details on how these values are
// used in code, and for guidance on what values are valid before editing.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



// ******** SETTINGS ****************
// **********************************
// ucomment and edit to customize general settings


// // Number of physical lanes available for the system (max of 4 allowed)
// // Fewer lanes will support a higher number of DEFAULT_MAX_STORED_LAPS
// // Lane count also defines the number of LED racer displays wired
// // If using a MAX7219 based LED startlight it will be assume to be device LANE_COUNT +1 (aka at index = LANE_COUNT)
// #define LANE_COUNT 4

// // set the maximum number of fastest laps stored during a race
// // if this gets too large the system will run out of memory and crash
// recommended max is 20 if LANE_COUNT = 2, 15 if LANE_COUNT = 3, and 10 if LANE_COUNT = 4
// #define DEFAULT_MAX_STORED_LAPS 10

// // Drag Race lap triggers
// // If using only 1 set of finish line triggers for drag racing, set to 'true'
// // If using a start and finish line trigger setup for drag racing, set to 'false'
// #define SINGLE_DRAG_TRIGGER true

// // If true, this flag tells the controller to do a pre-start countdown when restarting from a pause.
// #define CTDWN_ON_RESTART true


// // set debounce time in ms
// #define DEBOUNCE 1000

// // set the default number of laps used for a standard race
// #define DEFAULT_LAPS 25

// // set the default min and sec used for a timed race on bootup
// #define DEFAULT_SET_MIN 0
// #define DEFAULT_SET_SEC 30

// // set default countdown time on boot up
// #define DEFAULT_COUNTDOWN 5

// // set the default number of ticks (ms) between display updates
// #define DEFAULT_REFRESH_TICKS 100

// Length of flash period in ms (time just completed lap is displayed to LED)
// #define DEFAULT_FLASH_PERIOD_LENGTH 1500

// // Length of time the start light stays lit after race start for circuit races.
// #define START_LIGHT_OFF_DELAY 2000


// // Lane/Racer's associated with which pin and interrupt byte mask pairs
// #define LANE1 {PIN_A0, 0b00000001}
// #define LANE2 {PIN_A1, 0b00000010}
// #define LANE3 {PIN_A2, 0b00000100}
// #define LANE4 {PIN_A3, 0b00001000}


// // INTERRUPT HARDWARE (ONLY CHANGE if using a non-Nano based Arduino)
// // For ATmega328 based Arduino use 'PCINT1_vect', case sensitive, no quotes
// // For ATmega2560 based Arduino use 'PCINT2_vect', case sensitive, no quotes
// #define PCINT_VECT PCINT2_vect

// // For ATmega328 based Arduino use 'PINC', case sensitive, no quotes
// // For ATmega2560 based Arduino use 'PINK', case sensitive, no quotes
// #define INTERRUPT_PORT PINK



// // Pin used for the pause button
// #define PAUSEPIN PIN_A6
// #define DRAGPIN PIN_A7

// // Pin used for the auido buzzer
// #define BUZZPIN 13

// // *** GAME SOUNDS ***
// // BEEP - basic game tone used for button press feedback and lap completion
// #define BEEP_FREQ 4000  // freq in Hz
// #define BEEP_DUR 200    // duration in ms

// // BOOP - secondary game tone used for menu back and start/re-start lap trigger
// #define BOOP_FREQ 1000  // freq in Hz
// #define BOOP_DUR 200    // duration in ms

// // BlEEP - longer tone used for race start and restart after pause.
// #define BLEEP_FREQ 2000  // freq in Hz
// #define BLEEP_DUR 600    // duration in ms

// // Audio Mode Default on bootup
// // use only 'AllOn', 'GameOnly', or 'Mute', case sensitive, no quotes
// #define DEFAULT_AUDIO_MODE AllOn



// // LCD Definition of size.
// #define LCD_COLS 20
// #define LCD_ROWS 4

// // Location, column index, that sets where to print race and prestart timer clock on the screen
// #define RACE_CLK_POS 8
// #define PRESTART_CLK_POS 11


// // 7-Segment LED bar pinout
// #define PIN_TO_LED_DIN 2
// #define PIN_TO_LED_CS 3
// #define PIN_TO_LED_CLK 4




// // Main Menu Screen option labels
// // max: 20ch
// #define A_SELECT_RACER "A| Select Racers"
// #define B_CHANGE_SETTINGS "B| Change Settings"
// #define C_START_RACE "C| Start a Race"
// #define D_SEE_RESULTS "D| See Results"



// // Select Racer's Menu Text
// // max: 8ch
// // The racer name will be printed to right of this label on same row starting with the 10th character position (column index 9)
// #define A_RACER1 "A|Lane1"
// #define B_RACER2 "B|Lane2"
// #define C_RACER3 "C|Lane3"
// #define D_RACER4 "D|Lane4"



// // Change Settings Menu Text
// // max: 11ch
// #define A_SETTING_AUDIO " A |Audio"

// // Set the end position of where the race time should be printed.
// // Should be chosen to align time digits with 'B_SETTING_TIME' label text.
// #define TIME_SETTING_POS 14
// #define B_SETTING_TIME " B |Time       :"

// #define C_SETTING_LAPS " C |Laps"

// #define D_SETTING_LANES "0-4|Lanes"



// // START A RACE MENU
// // max: 11ch
// // Set the end position of where the lap number should be printed.
// // Should be chosen to align number with A_START_RACE_STANDARD text.
// #define START_RACE_LAPS_ENDPOS_IDX 13
// #define A_START_RACE_STANDARD "A|First to     Laps"

// // Set the end position of where the race time is printed to the screen
// // Should be chosen to align time with label text.
// #define START_RACE_TIME_ENDPOS_IDX 16
// // Remember to place a ':' at START_RACE_TIME_ENDPOS_IDX - 2
// #define B_START_RACE_IMED "B|Most Laps in   :"

// #define START_RACE_3RD_ROW ""

// // Set the end position of the countdown number
// // Should be chosen to align countdown value with D_START_RACE_COUNTDOWN text.
// #define START_RACE_CNTDWN_ENDPOS_IDX 14
// #define D_START_RACE_COUNTDOWN "D|Countdown:    Sec"



// Text to display on main LCD during Pre-Start countdown
// #define TEXT_PRESTART "Race Begins in:"
// First character position of Pre-Start text string
// #define TEXT_PRESTART_STRPOS 3



// // Finishing Place Labels
// // MUST be 3 character string
// #define FINISH_DNF "DNF"
// #define FINISH_1ST "1st"
// #define FINISH_2ND "2nd"
// #define FINISH_3RD "3rd"
// #define FINISH_4TH "4th"



// // Results Menu Text
// // Text shown if no race data is present
// #define NO_RACE_DATA "-NO RACE DATA-"

// // text displayed while processing race data
// #define COMPILING "-Compiling-"

// // text displayed on top row of Top Results page
// #define RESULTS_TOP_LBL "C| TOP RESULTS"

// // text used to label 'Best' lap, lap# on the live and FINAL leader board
// #define RESULTS_TOP_TEXT_BEST "Best"
// // text used to label 'Best' lap, lap# on the live and FINAL leader board
// #define RESULTS_TOP_TEXT_LAP "Lap"


// // Blink rate between Racer#ID and Racer Name on individual results screen.
// #define RESULTS_RACER_BLINK 2000

// // text displayed on top row of individual racer results menu
// #define RESULTS_RACER_LBL "C| RACER "
// // position to print the racer # following/preceding RESULT_RACER_LBL text
// #define RESULTS_RACER_NUM_POS 10

// // text displayed on top row of the Finish results menu
// #define RESULTS_FINISH_LBL "C| FINISH"

// // Label displayed above race time total on individual racer's results screen
// // max 6 characters
// #define RESULTS_TOTAL_LBL "Total"
// // 2nd line of label above racer's total race time.
// // max 6 characters
// #define RESULTS_TOTAL_LBLA "Time"
// #define RESULTS_TOTAL_LBLB "Laps"

// // Start text used on LEDs
// // Should not exceed 5ch
// #define TEXT_START "Start"

// // 'Pause' text displayed on LEDs
// // Should not exceed 5ch
// #define TEXT_PAUSE "PAUSE"

// // text used for '-OFF-' state of audio and enabled lanes/racers
// // Code assumes this term is always 5ch long, use spaces to fill if necessary.
// #define TEXT_OFF "-Off-"




// // RACER_NAMES_LIST defins the list of names available to select from through UI.
// // If SONG_BY_PLACE is 'false', the number of terms should match the # of terms in 'RACER_SONGS_LIST"
// // !!!! The first term should always be 'TEXT_OFF', it is used to represent an inactive lane
// #define RACER_NAMES_LIST {TEXT_OFF, "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle 1", "Rat2020_longer", "The OG", "5318008"}

// // If SONG_BY_PLACE is 'false', songs are associated with racer name of matching array index
// // songs to choose from are defined in the RTTL_songs.h file
// // If SONG_BY_PLACE is 'false', the number of terms should match the # of terms in 'RACER_NAMES_LIST"
// // !!!! The first term is for the 'off' condition and should reference 'disabledTone'.
// #define RACER_SONGS_LIST {disabledTone, starWarsImperialMarch, takeOnMeMB, airWolfTheme, tmnt1, gameOfThrones, galaga, outrun, starWarsEnd, spyHunter}

// // Song Association
// // To associate a song with a racer name, set SONGS_BY_PLACE to 'false'.
// // Otherwise, to make songs played related to finishing place, set to 'true'.
// // If SONGS_BY_PLACE is 'true', 1st finisher will play RACERS_SONG_LIST[1], 2nd place finisher RACERS_SONG_LIST[2], etc.
// // RACER_NAMES_LIST & RACERS_SONG_LIST size do not need to match if SONGS_BY_PLACE is 'true'.
// #define SONGS_BY_PLACE false



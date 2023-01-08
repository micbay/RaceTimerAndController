// This file defines the default hardware and UI setup used by the master branch

// Many of the controller UI and default settings can be customized
// by the user by editing the 'localSettings.h' file.

// If a 'localSettings.h' file does not already exist, a new one can be
// created by copying the 'example.localSettings.h' file, and renaming it 'localSettings.h'.
// The 'localSettings.h' file should be included in the .gitignore list and
// will not be overwritten on subsequent controller code updates.
// Edits done direclty in the 'defaultSettings.h' will be overwritten on next pull from the master repo.

// ***** IF USING LOCAL SETTING CUSTOMIZATION *************
// *****************************************************
// Uncomment the following include after a localSettings.h file has been created.
// #include "localSettings.h"



// --------- DO NOT EDIT VALUES BELOW THIS LINE -----------
// --------------------------------------------------------
// Make customizations in the 'localSettings.h' file instead.


// *********************************************************
// *** default #defines in case not Set by localSettings ***
// *********************************************************

// set debounce time in ms
#if !defined ( DEBOUNCE )
  #define DEBOUNCE 1000
#endif

// set the default number of laps used for a standard race
#if !defined ( DEFAULT_LAPS )
  #define DEFAULT_LAPS 10
#endif
// set the default min and sec used for a timed race on bootup
#if !defined ( DEFAULT_SET_MIN )
  #define DEFAULT_SET_MIN 0
#endif
#if !defined ( DEFAULT_SET_SEC )
  #define DEFAULT_SET_SEC 30
#endif
// set the default value of the pre-start countdown
#if !defined ( DEFAULT_COUNTDOWN )
  #define DEFAULT_COUNTDOWN 5
#endif
// set the default number of ticks (ms) between display updates
#if !defined ( DEFAULT_REFRESH_TICKS )
  #define DEFAULT_REFRESH_TICKS 100
#endif
// set the maximum number of fastest laps stored during a race
// if this gets too large the system will run out of memory and crash
// recommended max is 10-15, a max of 20 has been shown to be problematic
#if !defined ( DEFAULT_MAX_STORED_LAPS )
  #define DEFAULT_MAX_STORED_LAPS 10
#endif

// Lane/Racer's associated with which pin and interrupt byte mask pairs
#if !defined ( LANE1 )
  #define LANE1 {PIN_A0, 0b00000001}
#endif
#if !defined ( LANE2 )
  #define LANE2 {PIN_A1, 0b00000010}
#endif
#if !defined ( LANE3 )
  #define LANE3 {PIN_A2, 0b00000100}
#endif
#if !defined ( LANE4 )
  #define LANE4 {PIN_A3, 0b00001000}
#endif

// Pin used for the pause button
#if !defined ( PAUSEPIN )
  #define PAUSEPIN PIN_A6
#endif
// Pin used for the auido buzzer
#if !defined ( BUZZPIN )
  #define BUZZPIN 13
#endif


// Main Menu Screen options labels
// max: 17ch
#if !defined( A_SELECT_RACER)
  #define A_SELECT_RACER "A| Select Racers"
#endif
#if !defined( B_CHANGE_SETTINGS)
  #define B_CHANGE_SETTINGS "B| Change Settings"
#endif
#if !defined( C_START_RACE)
  #define C_START_RACE "C| Start a Race"
#endif
#if !defined( D_SEE_RESULTS )
  #define D_SEE_RESULTS "D| See Results"
#endif


// Select Racer's Menu Text
// max: 8ch
// The racer name will be printed to right of this label on same row starting with the 10th character position (column index 9)
#if !defined( A_RACER1 )
  #define A_RACER1 "A|Racer1"
#endif
#if !defined( B_RACER2 )
  #define B_RACER2 "B|Racer2"
#endif
#if !defined( C_RACER3 )
  #define C_RACER3 "C|Racer3"
#endif
#if !defined( D_RACER4 )
  #define D_RACER4 "D|Racer4"
#endif


// Change Settings Menu Text
// max: 11ch
#if !defined( A_SETTING_AUDIO )
  #define A_SETTING_AUDIO " A |Audio"
#endif
// Set the end position of where the race time should be printed.
// Should be chosen to align time digits with 'B_SETTING_TIME' label text.
#if !defined( TIME_SETTING_POS)
  #define TIME_SETTING_POS 14
#endif
#if !defined( B_SETTING_TIME)
  #define B_SETTING_TIME " B |Time       :"
#endif
#if !defined( C_SETTING_LAPS )
  #define C_SETTING_LAPS " C |Laps"
#endif
#if !defined( D_SETTING_LANES )
  #define D_SETTING_LANES "0-4|Lanes"
#endif


// START A RACE MENU
// max: 11ch
// Set the end position of where the lap number should be printed.
// Should be chosen to align number with A_START_RACE_STANDARD text.
#if !defined( START_RACE_LAPS_ENDPOS_IDX )
  #define START_RACE_LAPS_ENDPOS_IDX 13
#endif
#if !defined( A_START_RACE_STANDARD)
  #define A_START_RACE_STANDARD "A|First to     Laps"
#endif
// Set the end position of where the race time is printed to the screen
// Should be chosen to align time with label text.
#if !defined( START_RACE_TIME_ENDPOS_IDX )
  #define START_RACE_TIME_ENDPOS_IDX 16
#endif
// Remember to place a ':' at START_RACE_TIME_ENDPOS_IDX - 2
#if !defined( B_START_RACE_IMED)
  #define B_START_RACE_IMED "B|Most Laps in   :"
#endif

#if !defined( START_RACE_3RD_ROW )
  #define START_RACE_3RD_ROW ""
#endif
// Set the end position of the countdown number
// Should be chosen to align countdown value with D_START_RACE_COUNTDOWN text.
#if !defined( START_RACE_CNTDWN_ENDPOS_IDX )
  #define START_RACE_CNTDWN_ENDPOS_IDX 14
#endif
#if !defined( D_START_RACE_COUNTDOWN )
  #define D_START_RACE_COUNTDOWN "D|Countdown:    Sec"
#endif


// Text to display on main LCD during Pre-Start countdown
#if !defined( TEXT_PRESTART )
  #define TEXT_PRESTART "Your Race Starts in:"
#endif
// First character position of Pre-Start text string
#if !defined( TEXT_PRESTART_STRPOS )
  #define TEXT_PRESTART_STRPOS 0
#endif


// Finishing Place Labels
#if !defined( FINISH_DNF)
  #define FINISH_DNF "DNF"
#endif
#if !defined( FINISH_1ST)
  #define FINISH_1ST "1st"
#endif
#if !defined( FINISH_2ND )
  #define FINISH_2ND "2nd"
#endif
#if !defined( FINISH_3RD )
  #define FINISH_3RD "3rd"
#endif
#if !defined( FINISH_4TH )
  #define FINISH_4TH "4th"
#endif


// Results Menu Text
#if !defined( NO_RACE_DATA )
  #define NO_RACE_DATA "-NO RACE DATA-"
#endif
// text displayed while processing race data
#if !defined( COMPILING )
  #define COMPILING "-Compiling-"
#endif
// text displayed on top row of Top Results page
#if !defined( RESULTS_TOP_LBL )
  #define RESULTS_TOP_LBL "C| TOP RESULTS"
#endif

// text used to label 'Best' lap, lap# on the live and FINAL leader board
#if !defined( RESULTS_TOP_TEXT_BEST )
  #define RESULTS_TOP_TEXT_BEST "Best"
#endif
// text used to label 'Best' lap, lap# on the live and FINAL leader board
#if !defined( RESULTS_TOP_TEXT_LAP )
  #define RESULTS_TOP_TEXT_LAP "Lap"
#endif

// Blink rate between Racer#ID and Racer Name on individual results screen.
#if !defined( RESULTS_RACER_BLINK )
  #define RESULTS_RACER_BLINK 2000
#endif

// text displayed on top row of individual racer results menu
#if !defined( RESULTS_RACER_LBL )
  #define RESULTS_RACER_LBL "C| RACER "
#endif
// position to print the racer # following/preceding RESULT_RACER_LBL text
#if !defined( RESULTS_RACER_NUM_POS )
  #define RESULTS_RACER_NUM_POS 10
#endif
// text displayed on top row of the Finish results menu
#if !defined( RESULTS_FINISH_LBL )
  #define RESULTS_FINISH_LBL "C| FINISH"
#endif
// Label displayed above racer's total race time on individual racer's results screen
// max 6 characters
#if !defined( RESULTS_TOTAL_LBL )
  #define RESULTS_TOTAL_LBL "Total"
#endif
// 2nd line of label above racer's total race time.
// max 6 characters
#if !defined( RESULTS_TOTAL_LBL2 )
  #define RESULTS_TOTAL_LBL2 "Time"
#endif


// Start text used on LEDs
// Should not exceed 5ch
#if !defined( TEXT_START )
  #define TEXT_START "Start"
#endif

// 'Pause' text displayed on LEDs
// Should not exceed 5ch
#if !defined( TEXT_PAUSE )
  #define TEXT_PAUSE "PAUSE"
#endif

// text used for '-OFF-' state of audio and enabled lanes/racers
// Code assumes this term is always 5ch long, use spaces to fill if necessary.
#if !defined( TEXT_OFF )
  #define TEXT_OFF "-Off-"
#endif


// !!! Make sure the number of terms in the racer name's & song's lists
// !!! matches the value of 'RACER_LIST_SIZE'
#if !defined ( RACER_LIST_SIZE )
  #define RACER_LIST_SIZE 10
#endif
// list of racer names available to select from through UI
// The number of terms should match the value of 'RACER_SIZE_LIST"
#if !defined ( RACER_NAMES_LIST )
  #define RACER_NAMES_LIST {TEXT_OFF, "Lucien", "Zoe", "Elise", "John", "Angie", "Uncle 1", "Rat2020_longer", "The OG", "5318008"}
#endif
// songs are associated with racer name of matching array index
// songs to choose from are defined in the RTTL_songs.h file
// The number of terms should match the value of 'RACER_SIZE_LIST"
#if !defined ( RACER_SONGS_LIST )
  #define RACER_SONGS_LIST {disabledTone, starWarsImperialMarch, takeOnMeMB, airWolfTheme, tmnt1, gameOfThrones, galaga, outrun, starWarsEnd, spyHunter}
#endif
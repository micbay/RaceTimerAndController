# Change Log
This document records the changes beteween updates of the 'RaceTimerAndController" code repository.

The versioning scheme will conform to the following format:

Ver. ##.##.## - Major.Minor.Patch
- **Major version #** - Increments on changes that are not backward compatible
- **Minor version #** - Increments on changes that add/remove functionlity, but can be used, without issue, in place of earlier versions of the same major release
- **Patch version #** - Increments when update is a bug fix, refactoring, or new data constant (like a song & custom ch arrays), as well as on updates to non functional aspects of the code repo, like documentation.


## Ver 2.0.0 - Added Drag Racing & Start Button, Pause Button Debounce Bug Fix
> This update is a significant update to the code. Though technically, this code is backward compatible with the same hardware setup as version, 1.x, the code base infrastructure has significant changes, and internal variable consistency is broken from earlier versions.
> 
> **Updates from Previous Commit**
>
> New Features/Enhancements
> - **Drag Racing Support** - This Race Controller now supports Drag Racing on lanes 1 & 2, with a new 'Drag Race' option, selectable from the 'Start a Race' menu. Drag racing can be setup with a single set of finish sensors, or with 2 sets of sensors, 1 at start, and 1 at finish. With only finish sensors, racers must make there own call on false starts. By default, the controller will expect a start and a finish sensor in each lane. Sensor configuration can be changed by editing the `SINGLE_DRAG_TRIGGER`flag in `localSettings.h`.
> NOTE: The number of sensors does not change the pin assignments. Both the start and finish sensors for a given lane should be wired as open switches on the same pin.
> - **Race Pre-Stage** - Previously, the race pre-start countdown started immediately after selecting a race type from the **Start a Race Menu**. Now, a 'Pre-Stage' has been added alerting racers to get to starting positions. When all racers are ready, the race can then be started by pressing the '#' key or the new **Start Button**.
>     - This extra phase facilitates rapid restarts of heats in drag racing.
>     - Also, coupled with adding a **Start Button**, the Pre-Stage allows for a more flexible system setup. Being able to start the race from a button allows the starter and controller's main keypad to be in different locations.
> - **Adafruit Bargraph** - The system now supports using an [Adafruit 24 Bi-Color LED Bargraph](https://www.adafruit.com/product/1721) as a start light tree and general race state indicator. The bargraph uses I2C and should be wired, sharing the same pins as the main LCD display.
> - **MAX7219 LED Start Light Tree** - The system now supports using a DIY, MAX7219 based, custom LED start light. The start light should be wired as the last device in the racer LED bar chain. The system will assume that the number of 7-seg race timers will equal the number of racers defiend by `LANE_COUNT`. As such it will assume the start light will be at device index `LANE_COUNT`, since device indexing starts with 0. See the project documentation for how to build the start light.
> - **Start Button** - Support for a 2nd analog buttton has been added. This button can used to trigger a race start, or restart, as well as clear start faults. The hardware wiring, and setup, mirrors the analog **Pause Button**. By default, the **Start Button** uses `PIN_A7`, but that can be changed using the `localSettings.h` file.
> - **Start Fault** - Previously with only circuit racing modes, the lap sensing pins were disabled until the race started. Now, lap triggers are enabled at the initiaton of the **Pre-Start** stage and crossing the start before the end of the  **Pre-Start** countdown will trigger a fault in all race modes. The offending racer/lane will be displayed on the main LCD and the associated lane's fault indicator LED, on start light tree, will light. Pressing the **Pause** or **Start** button, or the `#` or `*` key, will clear the fault and return to the `Staging` state.
> - **Song Association** - Users can now use the `SONGS_BY_PLACE` flag in `localSettings.h` to specify if the `RACERS_SONGS_LIST` should be associated with a place finish or with a particular racer. By default, songs are associated with racers, however, by setting `SONGS_BY_PLACE` to `true`, the song at index 1 will play when the 1st place finishes, the song at index 2 will play when the 2nd racer finishes, and so on.
> - **Restart Countdown** - Users can now use the `CTDWN_ON_RESTART` flag in `localSettings.h` to specify whether restarts after a pause, should start right away or if it should restart with a pre-start countdown.
>
> Bug Fixes
> - MAJOR BUG FIX - Pause button debouncing was not properly filtering the debounce time because the time was being tracked by an `int` variable instead of an `usinged long` which caused an error in the debounce calculation, effectively nullifying it.
>
> Notable Code Changes
> - All enums have been moved to a seperate file and imported into the main .ino. This forces the compiler to look at those first, making them available to be used in function definitions without throwing a compiler error.
> - New states have been created, `PreStart`, `Staging`, `Fault`, `PreFault`, and `Finished`, to better manage system status flow.
> - The need for `topFastestLaps[]` and `topFastestTimes[]` has been eliminated and their function rolled into the, previously unused, zero index of the main `fastestLaps[0][0]` and `fastestTimes[0][0]` arrays used by the racers.
> - Moved LCD and LED pin-outs and parameters to be `...Settings.h` macros
________________________

## Ver 1.1.0 - Implementation of Adjustable Lane Count
> This update completes the refactoring of code to make the maximum number of lanes (ie `laneCount`) a settable variable. This change will allow users who use only 2 lanes to increase the max laps recorded during a race without data corruption. Users should now set `LANE_COUNT` in `localSettings.h` to match the number of lanes that will be wired in the system. The new default `LANE_COUNT` is 2 lanes instead of 4 as this is the most common usage. Accordingly the new, out of box, `DEFAULT_MAX_STORED_LAPS` is now 20.
> 
> **Updates from Previous Commit**
>
> New Features/Enhancements
> - Made `laneCount` settable via macro called `LANE_COUNT` in `…Settings.h` files.
> - Updated `ToggleLaneEnabled()`, and select racer cycle code, to account for a changeable `laneCount`.
> - Updated `DEFAULT_MAX_STORED_LAPS` recommendations per lane count.
>
> Code Maintenance
> - `DEFAULT_LANES_ENABLED` setting is no longer used and has been commented out of `...Settings.h` files. All lanes as determined by `LANE_COUNT` are enabled on startup. They can still be subsequently toggled to be enabled/disabled through the menu settings.
> - `example.localSettings.h` settings have been synced with current `defaultSettings.h`.
> 
> README updates
> - Changed the ground loop reference link to wikipedia. ([old link](https://www.bapihvac.com/application_note/avoiding-ground-loops-application-note/))([new link](https://en.wikipedia.org/wiki/Ground_loop_(electricity)))
> - Added images and reference links for sub-miniature switches in Sensor Section.
> - In intro paragraph, added quick link to operation menus section at end. This link is to an internal paragraph header and may not work in all markdown readers.
> - Various typos and sentence structure clarification edits. 

________________________

## Ver 1.0.1 - Documenation Updates
> This update is primarily to bring certain parts of the readme file up to date with the latest settings and screenshot references.
> 
> **Updates from Previous Commit**
> 
> README updates
> - In section about adapting for ATmega2560, add reference to `...Settings.h` file setting token that is used to set that pin settings.
> - Add section explaining the audio modes
> - Add note about using `# key` to silence any playing songs (music audio)
> - Add clarification in custom character section that it is being used as an icon creation tool.
> - Update section about individual results screens to mention total laps, and show latest screenshots.
__________
## Ver 1.0.0 - Initial Release
> Though other versions of this repo have been available for some time, this is the first version from which future changes will be logged and commits tagged with an identifying version.
> 
> **Updates from Previous Commit**
>
> Bug Fixes
> - CRITICAL BUG FIX ([Issue #9](https://github.com/micbay/RaceTimerAndController/issues/9)) - Fixed string reference error in `UpdateNameOnLED()` function. The finishing place strings, like "1st", "2nd", etc., are actually 4 characters long, not 3, because every string has an invisible termination characters, however, the variable was set as as 3 char array. This was causing a system freeze for certain code compilitions, likely depending on how the physical memory was being allocated under the hood. Even non-direct code edits caused the problem to appear and dissapear. This may have been due to physical memory locations being moved for different code footprints. Sometimes the temp variable overwrite was near other crtical dynamic data, and in other code footprints it wasn't.
>   - The correction was to let the array variable size be implied by setting to default string "DEF".
> 
> New Features/Enhancements
> - Added total laps to individual racer results screens. Total time and total laps are now displayed intermittently with each other at a 'blink rate' defined by `RESULTS_RACER_BLINK` setting.
> - Game tone attributes for Beep, Boop, and Bleep, have been added to `...Settings.h` files.
>   - Allows for customization of frequncy and duration of game sounds.
> - Added default audio mode to `...Settings.h` files and updated audio mode handling functions to split mode iteration and flag updating to accomodate making this user settable.
> - Default lane Enabled Status is now added to `...Settings.h` files.
>   - Provides setting to establish which lanes are enabled on boot up.
> - Added interrupt port (ie: `PINC` for Nano) and trigger byte vector (ie: `PCINT1_vect` for Nano) to `...Settings.h'` files so users can effectively setup the system for ATmega328 (nano) or ATmega2560 just by adjusting `localSettings.h` file.
> - Stop Music feature added - while in menu mode, pressing the `# key`, at any time, will stop any actively playing music (song). Helpful when going through racer list, but don't want to listen to the whole song when finished with selection.
>
> Maintenance
> - Begin full release versioning and added a changelog file.
> - Minor updates to code comments for clarity and corrections.
> - Updates to the project README
>   - Fixed broken link referencing Arduino Nano.  ([old link](https://www.arduino.cc/en/pmwiki.php?n=Main/ArduinoBoardNano)) ([new link](https://docs.arduino.cc/hardware/nano))
>   - Replace reference to editing max recorded laps variable, with reference to editing setting.
>   - Updated code example about Beeps and Boops to sync with new settings tokens and ability to be turned on and off.
>   - Add details clarifying wiring of paper-clip example.
>   - Add links for libraries used to "Software Configuration" section.
>   - Various spelling and gramatical clarifications.
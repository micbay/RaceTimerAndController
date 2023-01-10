# Change Log
This document records the changes beteween updates of the 'RaceTimerAndController" code repository.

The versioning scheme will conform to the following format:

Ver. ##.##.## - Major.Minor.Patch
- **Major version #** - Increments on changes that are not backward compatible
- **Minor version #** - Increments on changes that add/remove functionlity, but can be used, without issue, in place of earlier versions of the same major release
- **Patch version #** - Increments when update is a bug fix, refactoring, or new data constant (like a song & custom ch arrays), as well as on updates to non functional aspects of the code repo, like documentation.

________________________

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
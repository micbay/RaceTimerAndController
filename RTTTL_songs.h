
const char disabledTone[] PROGMEM = "disabled:d=4,o=5,b=125:2c,4c,3c";

const char starWarsEnd[] PROGMEM = "StarWarsEnd:d=4,o=5,b=225:2c,1f,2g.,8g#,8a#,1g#,2c.,c,2f.,g,g#,c,8g#.,8c.,8c6,1a#.,2c,2f.,g,g#.,8f,c.6,8g#,1f6,2f,8g#.,8g.,8f,2c6,8c.6,8g#.,8f,2c,8c.,8c.,8c,2f,8f.,8f.,8f,2f";

const char starWars[] PROGMEM = "StarWars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";

const char starWarsIntro[] PROGMEM = "starwars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6,p,8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6";

const char starWarsImperialMarch[] PROGMEM = "starWarsImperialMarch:d=4,o=5,b=180:a.4,a.4,a.4,f4,16c.,a.4,f4,16c.,2a.4,e.,e.,e.,f,16c.,g#.4,f4,16c.,2a.4,a.,a4,16a.4,a.,g#,16g.,16f#.,16e.,8f.,8p.,8a#.4,d#.,d,16c#.,16c.,16b.4,8c.,8p.,8f.4,g#.4,f4,16a.4,c.,a4,16c.,2e.,a.,a4,16a.4,a.,g#,16g.,16f#.,16e.,8f.,8p.,8a#.4,d#.,d,16c#.,16c.,16b.4,8c.,8p.,8f.4,g#.4,f4,16c.,a.4,f4,16c.,2a.4";

const char spyHunter[] PROGMEM = "spyhunter:d=4,o=5,b=100:16f,32p,16f,32p,16g,32p,16f,32p,32g#.,16a,16f,32p,16a#,32p,16a,32p,16f,32p,16f,32p,16g,32p,16f,32p,32g#.,16a,16f,32p,16a#,32p,16a,32p";

const char tmnt1[] PROGMEM = "tmnt1:d=4,o=5,b=100:8e,8f#,8e,8f#,8e,16f#,8e.,8f#,8g,8a,8g,8a,8g,16a,8g.,8a,8c6,8d6,8c6,8d6,8c6,16d6,8c.6,8d6,16a,16a,16a,16a,8g,8a,8p,16a,16a,16a,16a";

const char gameOfThrones[] PROGMEM = "GOT:d=32,o=4,b=320:2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4e5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4e5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,1g.6,p,1c.6,p,4d#6,p,4f6,p,1g6,p,1c6,p,4d#6,p,4f6,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,1f.6,p,1a#.5,p,4d6,p,4d#6,p,1f6,p,1a#5,p,4d#6,p,4d6,p,2c5,p,4d#5";

// ,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,1g.6,p,1c.6,p,4d#6,p,4f6,p,1g6,p,1c6,p,4d#6,p,4f6,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,1f.6,p,1a#.5,p,4d6,p,4d#6,p,1f6,p,1a#5,p,4d#6,p,4d6,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,1g.7,p,1c.7,p,4d#7,p,4f7,p,1g7,p,1c7,p,4d#7,p,4f7,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,1f.7,p,1a#.6,p,1d7,p,1d#7,p,1d7,p,1a#6,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c7,p,4d#5,p,4f5,p,2g5,p,2c7,p,4d#5,p,4f5,p,2g5,p,2a#6,p,4d5,p,4d#5,p,2f5,p,2a#6,p,4d5,p,4d#5,p,2f5,p,2g#6,p,4c5,p,4d5,p,2d#5,p,2g#6,p,4c5,p,4d5,p,2d#5,p,2g6,p,4a#,p,4c5,p,2d5,p,2g6,p,4a#,p,4c5,p,2d5,p,2d#6,p,4f#,p,4g#,p,2a#,p,2d#6,p,4f#,p,4g#,p,2a#,p,2d#6,p,4f#,p,4f#,p,2d#6,p,1f6,2p,4g#,p,4g#,p,2f6,p,2c6,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c7,p,4d#5,p,4f5,p,2g5,p,2c7,p,4d#5,p,4f5,p,2g5,p,2a#6,p,4d5,p,4d#5,p,2f5,p,2a#6,p,4d5,p,4d#5,p,2f5,p,2g#6,p,4c5,p,4d5,p,2d#5,p,2g#6,p,4c5,p,4d5,p,2d#5,p,2g6,p,4a#,p,4c5,p,2d5,p,2g6,p,4a#,p,4c5,p,2d5,p,2d#6,p,4f#,p,4g#,p,2a#,p,2d#6,p,4f#,p,4g#,p,2a#,p,1d#6,p,2d#6,p,1d6,p,2d6,p,2c6,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#5,p,4f5,p,2g5,p,2c5,p,4d#7,p,4f7,p,2g7,p,2c7,p,4d#7,p,4f7,p,2g7,p,2c7,p,4d#7,p,4f7,p,2g7,p";

const char outrun[] PROGMEM = "outrun_magic:d=4,o=5,b=160:f6,d#6,8g#.6,f6,d#6,8c#.6,d#6,c6,2g#.,c#6,c6,8d#.6,c#6,c6,8f.,a#,16c.6,1a#,f6,d#6,8g#.6,f6,d#6,8c#.6,d#6,c6,2g#.,c#6,c6,8d#.6,c#6,c6,16f.,16g#.,c6,2a#.";

const char takeOnMe1[] PROGMEM = "takeOnMe1:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5";

const char takeOnMeMB[] PROGMEM = "takeOnMe1:d=8,o=5,b=160:f#,f#,d,b4,p,b4,p,e,p,e,p,e,g#,g#,a,b,a,a,a,e,p,d,p,f#,p,f#,p,f#,e,e,f#,e,f#,f#,d,b4,p,b4,p,e,p,e,p,e,g#,g#,a,b,a,a,a,e,p,d,p,f#,p,f#,p,f#,e,e,f#,e,f#,f#,d,b4,p,b4,p,e,p,e,p,e,g#,g#,a,b,a,a,a,e,p,d,p,f#,p,f#,p,f#";


const char airWolfTheme[] PROGMEM = "airWolfTheme:d=4,o=6,b=100:e5,16a5,16b5,16d,e,16g,16f_,16d,e,16g,16f_,16d,e,8d,16f_,b5,a5,8g5,16a5,8f_5,16d5,g5,16c,16d,16f,g,16c,16b,16f,g,16c,16b,16f,g,8f,16a,d,c,8b5,16d,8a5,16f5,g5,16c,16d,16f,g,16c,16b,16f";
// // TEST memory saver copy
// const char airWolfTheme[] PROGMEM = "airWolfTheme:d=4,o=6,b=100:e5,16a5,16b5";

const char galaga[] PROGMEM = "Galaga:d=4,o=5,b=125:8g4,32c,32p,8d,32f,32p,8e,32c,32p,8d,32a,32p,8g,32c,32p,8d,32f,32p,8e,32c,32p,8g,32b,32p,8c6,32a#,32p,8g#,32g,32p,8f,32d#,32p,8d,32a#4,32p,8a#,32c6,32p,8a#,32g,32p,16a,16f,16d,16g,16e,16d";
// // TEST memory saver copy
// const char galaga[] PROGMEM = "Galaga:d=4,o=5,b=125:8g4,32c,32p";

// const char nellyOne[] PROGMEM = "NumberOn:d=16,o=5,b=90:a,e,a,8e6,8c6,2a,a,e,a,8e6,8c6,2a,a,e,a,8e6,8c6,2a,a,e,8a,g,a,g,8c6,a,4a";

const char gnrSweetChild[] PROGMEM = "SweetChi:d=16,o=6,b=80:f5,f,c,a5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,g5,f,c,a#5,a#,c,a,c,g5,f,c,a#5,a#,c,a,c,a#5,f,c,a#5,a#,c,a,c,a#5,f,c,a#5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,g5,f,c,a#5,a#,c,a,c,g5,f,c,a#5,a#,c,a,c,a#5,f,c,a#5,a#,c,a,c,a#5,f,c,a#5,a#,c,a,c,f5,f,c,a5,a#,c,a,c,f5,f,c,a5,a#,c,a,c";

const char reveille[] PROGMEM = "Reveille:d=4,o=5,b=140:e6,8c6,g,8g,8c6,8g,8e6,8c6,8c6,8c6,e6,8c6,g,8g,8c6,8g,8e6,c.6,8g,8g,8g,8c6,8e6,8c6,8g,8g,8g,c6,8e6,8g,8g,8g,8c6,8e6,8c6,8g,8g,8g,c.6";
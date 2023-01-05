// These are custom character layouts that can be used,
// by an LCD driven with LiquidCrystal.h or hd44780.h library APIs.

// We can assign up to 8 characters, called by values (0-7).
// ex. lcd.createChar(0, Skull);  lcd.write(0);   Will print a skull.
byte Skull[] = {
 B00000,
 B01110,
 B10101,
 B11011,
 B01110,
 B01110,
 B00000,
 B00000
};

// byte Heart[] = {
//   B00000,
//   B01010,
//   B11111,
//   B11111,
//   B01110,
//   B00100,
//   B00000,
//   B00000
// };

// byte Alien[] = {
//   B11111,
//   B10101,
//   B11111,
//   B11111,
//   B01110,
//   B01010,
//   B11011,
//   B00000
// };

// byte Check[] = {
//   B00000,
//   B00001,
//   B00011,
//   B10110,
//   B11100,
//   B01000,
//   B00000,
//   B00000
// };

byte UpDownArrow[] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100
};

// byte UpArrow[] = {
//   B00100,
//   B01110,
//   B10101,
//   B00100,
//   B00100,
//   B00100,
//   B00100,
//   B00100
// };

// byte DownArrow[] = {
//   B00100,
//   B00100,
//   B00100,
//   B00100,
//   B00100,
//   B10101,
//   B01110,
//   B00100
// };

byte MusicNote[] = {
  B00111,
  B00110,
  B00100,
  B00111,
  B00110,
  B11100,
  B11100,
  B11100
};

byte GameSound[] = {
  B00001,
  B00101,
  B10101,
  B10101,
  B10101,
  B10101,
  B00101,
  B00001
};
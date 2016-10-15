#include "Arduino.h"
#include <LiquidCrystal.h>
#include "k197.h"

void setup_lcd(LiquidCrystal& lcd) {
  lcd.begin(16, 2);
  lcd.clear();
}

void display_measurement(LiquidCrystal& lcd, byte DMMReading[]) {
  String mode = get_mode(DMMReading, true);
  String reading = get_digits(DMMReading,true);
  String units = get_units(DMMReading, true);

  lcd.clear();
  
  lcd.setCursor(0,0); lcd.write(mode.c_str());
  lcd.setCursor(0,1); lcd.write(reading.c_str());lcd.write(" ");lcd.write(units.c_str());
  
}

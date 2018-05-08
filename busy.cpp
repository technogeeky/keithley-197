#include "Arduino.h"
#include <LiquidCrystal.h>
#include "busy.h"

unsigned long twiddle_delay = 33; //ms
unsigned long twiddle_delay_off = 66; //ms
  
char refresh_twiddle(LiquidCrystal& lcd, char twiddle, volatile unsigned long& twiddle_last, bool off) {
  char next;

  #define TIMER (millis() - twiddle_last)
  #define TIME_TO_TWIDDLE (TIMER >= twiddle_delay)
  #define TIME_TO_TWIDDLE_OFF (TIMER >= twiddle_delay_off)
  

//  Serial.print("in refresh_twiddle(): off = ");
//  Serial.print(off);
//  Serial.print("; timer = ");
//  Serial.print(TIMER);
//  Serial.println("");
  
  switch (off) {
    case true: if (!TIME_TO_TWIDDLE_OFF) return twiddle;
    case false: if (!TIME_TO_TWIDDLE) return twiddle;
  }

//  Serial.println("Got after twiddle");
  

  twiddle_last = millis();
    
  switch (twiddle) {
    case '.'   : next = '\xa1'; break;
    case '\xa1': next = '\xa5'; break;
    case '\xa5': next = 'o'; break;
    case 'o'   : next = 'O'; break;
    case 'O'   : next = '*'; break;
    case '*'   : next = '\xdb'; break;
    case '\xdb': next = '\xdf'; break;
    case '\xdf': next = '\xde'; break;
    case '\xde': next = '\xeb'; break;
    case '\xeb': next = '.'; break;

    default : next = '.'; break;
  }

  
  lcd.setCursor(15, 0);
  lcd.write(next);

  return next;
}

#include "Arduino.h"

String get_mode(byte DMMReading[], bool lcd) {

  String mode = "";
  
  #define BAT   (a & (1 << 6)) == (1 << 6)    // -- on BATtery, presumably.
  #define RCL   (a & (1 << 5)) == (1 << 5)    // -- ReCaLL (playback) mode.
  #define DB    (a & (1 << 3)) == (1 << 3)    // -- DB measure rel. to 1 dBmW @ 660 Ohms
  #define STO   (a & (1 << 2)) == (1 << 2)    // -- STOre (record) mode. See manual.
  #define REL   (a & (1 << 1)) == (1 << 1)    // -- RELative. Used to zero probes in 2-wire.
  #define AUTO  (a & (1 << 0)) == (1 << 0)    // -- AUTOranging. Volts and Ohms only.
  #define CAL   (c & (1 << 0)) == (1 << 0)    // -- CALibration process.
  #define RMT   0                             // -- REMOTE - under GPIB control.

  byte a = DMMReading[8];
  byte c = DMMReading[16];

  if (AUTO) mode += "AUTO ";
  else      mode += "     ";

  if (REL)  mode += "REL ";
  else      mode += "    ";

  if (DB) mode += "dB ";
  else    mode += "   ";

  if      (STO) mode += "STO";
  else if (RCL) mode += "RCL";
  else if (CAL) mode += "CAL";
  else          mode += "   ";
  
  return mode;
  
  
}
String get_units(byte DMMReading[], bool lcd) {

  /* Note:
   *    We must treat the unit types as special (Volts, Amps, Ohms), because
   *    they can be active in addition to the scaled unit type (mA, kO, uA, MO, mV).
   *    So we might expect the meter in milliVolts to read:
   *      (_mV && __V).
   * 
   *  Note:
   *    This also explains why the ordering in this function is important!
   */
  #define ___AC (a & (1 << 4)) == (1 << 4)  // AC mode (Volts, Amps, dB)
  #define _mA   (b & (1 << 5)) == (1 << 5)  // milliAmps
  #define _kO   (b & (1 << 4)) == (1 << 4)  // kiloOhms
  #define __V   (b & (1 << 3)) == (1 << 3)  // Volts
  #define _uA   (b & (1 << 2)) == (1 << 2)  // microAmps
  #define _MO   (b & (1 << 1)) == (1 << 1)  // MegaOhms
  #define _mV   (b & (1 << 0)) == (1 << 0)  // milliVolts
  
  #define __A   (c & (1 << 2)) == (1 << 2)  //  Amps
  #define __O   (c & (1 << 1)) == (1 << 1)  //  Ohms

  
  byte a = DMMReading[8];
  byte b = DMMReading[15];
  byte c = DMMReading[16];

  switch(lcd) {
    case false: 
      if (_kO) return "kΩ   ";
      if (_MO) return "MΩ   ";
      if (__O) return " Ω   ";
      if (_uA) { if (___AC) return "μA AC"; else return "μA   "; }
      if (_mA) { if (___AC) return "mA AC"; else return "mA   "; }
      if (__A) { if (___AC) return " A AC"; else return " A   "; }
      break;
    case true:
      if (_kO) return "k\xf4   ";
      if (_MO) return "M\xf4   ";
      if (__O) return " \xf4   ";
      if (_uA) { if (___AC) return "\xE4\x41 AC"; else return "\xE4\x41   "; }
      if (_mA) { if (___AC) return "mA AC"; else return "mA   "; }
      if (__A) { if (___AC) return " A AC"; else return " A   "; }
      break;
    }

  if (_mV) { if (___AC) return "mV AC"; else return "mV   ";}
  if (__V) { if (___AC) return " V AC"; else return " V   ";}
  
  else return "     ";
}


/*
 * This function decodes the SPI data sent from the K197 to the original
 *  controller. These were decoded by trial-and-error. Some codes remain
 *  unknown.
 *  
 * Note:
 *  The default is "_" to indicate something went wrong. This may need to be changed.
 *  
 * Note:
 *  In practice, the "_" is not because there was a character we didn't know how to decode,
 *  but that there is some kind of data timing / data corruption issue and we're reading
 *  the wrong bytes.
 */
String get_digits(byte DMMReading[], bool lcd) {

  #define DIGIT_HAS_DOT(x) ((x & 4) == 4)
  #define NEG (a & (1 << 7)) == (1 << 7)
  
  String reading = "";

  byte a = DMMReading[8];

  if (NEG)
    reading += "-";
  else
    reading += " ";
    
  for(int w = 9; w <= 14; w++) {
    
    if DIGIT_HAS_DOT(DMMReading[w])
      reading += ".";
      
    switch (DMMReading[w] & (~ 4)) {
      case 0xEB: reading += "0"; break;
      case 0xC0: reading += "1"; break;
      case 0x7A: reading += "2"; break;
      case 0xF8: reading += "3"; break;
      case 0xD1: reading += "4"; break;
      case 0xB9: reading += "5"; break;
      case 0xBB: reading += "6"; break;
      case 0xC8: reading += "7"; break;
      case 0xFB: reading += "8"; break;
      case 0xD9: reading += "9"; break;
      case 0x2B: reading += "C"; break;
      case 0xDB: reading += "A"; break;
      case 0x23: reading += "L"; break;
      case 0xB2: reading += "o"; break;
      case 0xA2: reading += "u"; break;
      case 0x33: reading += "t"; break;
      case 0xD3: reading += "H"; break;
      case 0x3B: reading += "E"; break;
      case 0x12: reading += "r"; break;
      case 0x92: reading += "n"; break;
      case 0x30: reading += "="; break;
      case 0x00: reading += " "; break;
      default: reading += "_"; break;
    }
  }
  return reading;
}



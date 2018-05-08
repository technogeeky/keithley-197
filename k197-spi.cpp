#include "Arduino.h"
#include "k197-spi.h"
#include "k197-state.h"


String get_mode(byte pkt[], bool lcd) {

  String mode = "";


  if (AUTO(pkt))      mode += "AUTO ";
  else                mode += "     ";
  
  if (REL(pkt))       mode += "REL ";
  else                mode += "    ";
  
  if (DB(pkt))        mode += "dB ";
  else                mode += "   ";
  
  if      (STO(pkt))  mode += "STO";
  else if (RCL(pkt))  mode += "RCL";
  else if (CAL(pkt))  mode += "CAL";
  else                mode += "   ";
  
  return mode;
  
  
}

byte merge_units(byte pkt[]) {
  byte merged = 0b00000000;

  byte units = pkt[UNITS_BYTE];
  byte units2 = pkt[UNIT2_BYTE];

  merged =  units | ((units2 & 0b00000110) << 5);

  
  return merged;
}
String get_units(byte reading[], bool lcd) {
  byte* pkt;
  

  pkt = reading;
  
  pkt[UNITS_BYTE] = merge_units(reading);
  
  /* Note:
   *    We must treat the unit types as special (Volts, Amps, Ohms), because
   *    they can be active in addition to the scaled unit type (mA, kO, uA, MO, mV).
   *    So we might expect the meter in milliVolts to read:
   *      (_mV && __V).
   * 
   *  Note:
   *    This also explains why the ordering in this function is important!
   */
  switch(lcd) {
    case false: 
      if (_kO(pkt)) return "kΩ   ";
      if (_MO(pkt)) return "MΩ   ";
      if (__O(pkt)) return " Ω   ";
      if (_uA(pkt)) { if (___AC(pkt)) return "μA AC"; else return "μA   "; }
      if (_mA(pkt)) { if (___AC(pkt)) return "mA AC"; else return "mA   "; }
      if (__A(pkt)) { if (___AC(pkt)) return " A AC"; else return " A   "; }
      break;
    case true:
      if (_kO(pkt)) return "k\xf4   ";
      if (_MO(pkt)) return "M\xf4   ";
      if (__O(pkt)) return " \xf4   ";
      if (_uA(pkt)) { if (___AC(pkt)) return "\xE4\x41 AC"; else return "\xE4\x41   "; }
      if (_mA(pkt)) { if (___AC(pkt)) return "mA AC"; else return "mA   "; }
      if (__A(pkt)) { if (___AC(pkt)) return " A AC"; else return " A   "; }
      break;
    }

  if (_mV(pkt)) { if (___AC(pkt)) return "mV AC"; else return "mV   ";}
  if (__V(pkt)) { if (___AC(pkt)) return " V AC"; else return " V   ";}
  
  else return "     ";
}


/*
 * This function decodes the SPI data sent from the K197 to the original
 *  controller. These were decoded by trial-and-error. These are thought to be *complete*.
 *
 *  
 * Note:
 *  The default is "_" to indicate something went wrong. This may need to be changed.
 *  
 * Note:
 *  In practice, the "_" is not because there was a character we didn't know how to decode,
 *  but that there is some kind of data timing / data corruption issue and we're reading
 *  the wrong bytes.
 */
String get_digits(byte pkt[], bool lcd) {

  String reading = "";
  bool found_dot = false;
  if (NEG(pkt))
    reading += "-";
  else
    reading += " ";
    
  for(int w = FIRST_DIGIT; w <= LAST_DIGIT; w++) {
    
    if DIGIT_HAS_DOT(pkt[w]) {
      reading += ".";
      found_dot = true;
    }
    switch (pkt[w] & (~ 4)) {
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
      case 0x32: reading += "c"; break;   // inferred
      case 0x00: reading += " "; break;
      default:   reading += "_"; break;
    }
  }

  if (!found_dot) reading = " " + reading;
  
  return reading;
}



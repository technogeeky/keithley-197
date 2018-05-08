#pragma once


#define MODES_BYTE  8
#define UNITS_BYTE  15
#define UNIT2_BYTE  16
#define PACKET      18    // our SPI packet is 18 bytes


#define FIRST_DIGIT 9
#define LAST_DIGIT 14

/* Keithley 197(a):
 *  
 *    The Keithley 197(a) multimeter sends status information on bytes 8, 15, and 16. This
 *      assumes that we are starting when SCK goes _SCK_ (low), which is usually 0x20.
 *      
 *    This information can be derived (in part?) from the schematic page of the LCD screen itself,
 *      under the portion CONTACT DESCRIPTION.
 *      
 *    The Keithley 175 may use different settings, so your mileage may vary.
 *    
 *    The MODES byte contains information about the overall operation of the meter. These
 *      bits can be set in multiple measuring modes (REL works in all modes, including dB).
 *      
 *      The following bits provide context to measurements:
 *      
 *      * STO - The measurement is stored in addition to being displayed. Depending on the n=
 *            setting, it may slow down measurement (to as slow as 1 per day). Not necessary with recording
 *            interface. Generates HI/LO from among the read values since STO started.
 *            
 *      * dB  - Returns a decibel value, referenced to 600 Ohm @ 1 mV = 1 dBm.
 *      
 *      The following bits imply a different mode of operation than the standard one, measurement:
 *      
 *      * RCL - There are up to 100 stored measurements, plus two extra (HI and LO), which can be played back
 *        as though they are live measurements.
 *        
 *      * CAL - When in the calibration mode, certain buttons are used to change the value displayed on the screen.
 *          These values are emitted *very* fast, much faster than normal measurements.
 *          FIXME: This currently does not work properly. Measurement on screen does not change. Must be in CAL store mode?
 *      
 *      * RMT - Indicates the device is in GPIB mode. FIXME: Currently untested.
 *      
 *      * AUTO - The device is in AUTORANGING mode. Implies that the device may change units at any time.
 *      
 *      
 *      Note also that multiple bits may be set as logic would demand, for instance:
 *      
 *        * If you are measuring a value in milliohms, then _mO() and __O() may be set.
 */


/*
 * Other information gleaned from reading the LCD control display, looking at logic analyser data:
 * 
 * In the COMMAND spots, a typical string (in fact, the only string one ever sees in normal operation) is:
 * 
 *  0x20  0x30  0x4A  0x14  0x00  0x18  0x11  0x20
 *  
 *  In command language, these may be:
 *  
 *  0x20  - CLEAR DATA MEMORY
 *  0x30  - UNSYNCHRONIZED TRANSFER (display data is re-written upon raising of C/S pin)
 *  0x4A  - MODE SET: (1/3rd bias mode, 3-time-sharing-drive, 2^9 scale ratio)
 *  0x14  - WITHOUT SEGMENT DECODER (data passed in directly to memory, without segment decoder)
 *  0x00  - CLEAR BLINKING DATA MEMORY
 *  0x18  - BLINKING OFF
 *  0x11  - DISPLAY ON  (after this upcoming data packet, display the data)
 *  0x20  - CLEAR DATA MEMORY (clear data memory again, for whatever reason)
 *  
 *  
 *  The startup sequence looks like:
 *  
 *  0x10  0x20  0x30  0x4A  0x14  0x00  0x18  0x11  
 *        0x20  0x30  0x4A  0x14  0x00  0x18  0x11  0x20  0x00  0xEB  0xEB  0xEB  0xEB  0xEB  0x00  0x00
 *  
 *  This is just a normal COMMAND sequence repeated twice (as above), but with the prefix:
 *  
 *  0x10 - DISPLAY OFF
 *  
 *  As well as the data packet required to display "000000", with all annunciators off.
 *  
 *  
 */


 
#define NEG(r)    (r[MODES_BYTE] & (1 << 7)) == (1 << 7)
#define BAT(r)    (r[MODES_BYTE] & (1 << 6)) == (1 << 6)    // -- on BATtery, presumably.
#define RCL(r)    (r[MODES_BYTE] & (1 << 5)) == (1 << 5)    // -- ReCaLL (playback) mode.
#define ___AC(r)  (r[MODES_BYTE] & (1 << 4)) == (1 << 4)    // -- AC mode (Volts, Amps, dB)
#define DB(r)     (r[MODES_BYTE] & (1 << 3)) == (1 << 3)    // -- DB measure rel. to 1 dBmW @ 660 Ohms
#define STO(r)    (r[MODES_BYTE] & (1 << 2)) == (1 << 2)    // -- STOre (record) mode. See manual.
#define REL(r)    (r[MODES_BYTE] & (1 << 1)) == (1 << 1)    // -- RELative. Used to zero probes in 2-wire.
#define AUTO(r)   (r[MODES_BYTE] & (1 << 0)) == (1 << 0)    // -- AUTOranging. Volts and Ohms only.

#define RMT(r)    0                                         // -- REMOTE - under GPIB control.



#define __A(r)   (r[UNITS_BYTE] & (1 << 7)) == (1 << 7)   //  -- Amps (NOTE: ONLY AFTER unit2_byte is masked and merged to here);
#define __O(r)   (r[UNITS_BYTE] & (1 << 6)) == (1 << 6)   //  -- Ohms (NOTE: ONLY AFTER unit2_byte is masked and merged to here);
#define __V(r)   (r[UNITS_BYTE] & (1 << 3)) == (1 << 3)   // Volts
#define _mA(r)   (__A(r)  && (  (r[UNITS_BYTE] & (1 << 5)) == (1 << 5)))  // milliAmps
#define _kO(r)   (__O(r)  && (  (r[UNITS_BYTE] & (1 << 4)) == (1 << 4)))  // kiloOhms
#define _uA(r)   (__A(r)  && (  (r[UNITS_BYTE] & (1 << 2)) == (1 << 2)))  // microAmps
#define _MO(r)   (__O(r)  && (  (r[UNITS_BYTE] & (1 << 1)) == (1 << 1)))  // MegaOhms
#define _mV(r)   (__V(r)  && (  (r[UNITS_BYTE] & (1 << 0)) == (1 << 0)))  // milliVolts


#define RMT(r)            (r[UNIT2_BYTE] & (1 << 5)) == (1 << 5)   //  REMote control using GPIB.
#define __A_old(r)        (r[UNIT2_BYTE] & (1 << 2)) == (1 << 2)   //  Amps
#define __O_old(r)_old    (r[UNIT2_BYTE] & (1 << 1)) == (1 << 1)   //  Ohms
#define CAL(r)            (r[UNIT2_BYTE] & (1 << 0)) == (1 << 0)   // -- CALibration process.

#define DIGIT_HAS_DOT(x) ((x & 4) == 4)



String get_mode(byte DMMReading[], bool lcd);
String get_units(byte DMMReading[], bool lcd);
String get_digits(byte DMMReading[], bool lcd);

byte merge_units(byte pkt[]);



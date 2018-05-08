#include <EnableInterrupt.h>
#include <LiquidCrystal.h>


#include "lcd.h"
#include "busy.h"
#include "k197-spi.h"
#include "printing.h"


/* LCD: Pin Definitions
 */
/*      name    pin   //  color   used?   req?  uniq?   notes */
#define RS_13   13    //          yes     yes   no      any ADC pin will work
#define RW_12   12    //          yes     no    no      any ADC pin will work
#define E__11   11    //          yes     yes   no      any ADC pin will work

/*      name    pin   //  color   used?   req?  uniq?   notes */
#define D4      28    //          yes     yes   no      any ADC pin will work
#define D5      26
#define D6      24
#define D7      22

/* LCD: Global Variables
 *  
 */
LiquidCrystal lcd (RS_13, RW_12, E__11, D4, D5, D6, D7);    // 8 data line mode

/* SPI: Global Variables (for debugging)
 *  
 *  
 * These are counters to examine how much time is spent
 *   in certain parts of the interrupt handler.
*/
volatile unsigned long innerloop = 0;
volatile unsigned long calls = 0;
volatile unsigned long outerloop = 0;
volatile unsigned long empty_reading = 0;


/* SPI: Global Variables (actual data)
 *  
 *  * DMMReading  -  is the 18 byte SPI packet (command AND data)
 *  * dT          -  is the amount of time the read took (in micros) (excluding trimming)
 *  * done        -  is used to flag that we have a full packet
 *  * onoff       -  will be used to guess when the multimeter has toggled on/off or frozen
*/
volatile byte DMMReading[18];
volatile bool done = false;
volatile unsigned long dT = 0;
volatile bool onoff = true;


/* SPI: Pin Definitions

    Note: the o_ is supposed to be a cute convention meaning "pin".
      so o_SCK is to be read "pin_SCK"
*/
/*      name    pin   //  color   used?   req?  uniq?   notes */
#define o_GND   GND   //  black   yes     yes   no      this isn't a real pin in the UI, but it must be connected
#define o_SS    53    //  red     yes     yes   no      (Mega2650) this is the only pin that can be SS or CS 
#define o_SCK   52    //  green   yes     yes   no      (Mega2650) this is the only pin that can be SPI clock
#define o_MOSI  51    //  blue    yes     yes   no      (Mega2650) this is the only pin that can listen from a master
#define o_MISO  50    //          no      no    no      (Mega2650) this is the pin you'd use to speak to slave as master
#define o_CD    49    //          no      no    no      one might be tempted to use command/data as a interrupt trigger, but we wouldn't get state info


/* SPI: THE INTERRUPT HANDLER FUNCTION
 *  
 *  
 *  This routine is the core of this program. The algorithim is simple. Since this function has been called
 *    SS has FALLEN.
 *    
 *    1. trimming   - first, dicard any 0x00 readings that may exist. (the Volts function often has extra 0x00s).
 *    2. read bytes - until we have collected (PACKET-1) words, continue reading bytes
 *    3. read byte  - read SPDR until SPSR and SPIF combined decide we've got a byte to read
 *    
 */
void handle_o_SS(void) {
  unsigned long start = micros();
  volatile bool trimming = true;
  volatile byte c;
  volatile int words = 0;

  calls++;

  while (trimming) {
    c = SPDR;
    empty_reading++;
    if (c == 0x00)
      trimming = true;
    else
      trimming = false;
  }
  
  while (done == false) {
    dT = micros() - start;
    while (!(SPSR & (1 << SPIF))) {
      c = SPDR;         // I don't understand yet if this read is necessary. It's certainly unused, but perhaps it's volatile.
    }

    outerloop++;
    DMMReading[words++] = SPDR;
    if (words == (PACKET - 1)) {
      done = true;
    }
  }
}


/* SPI: Pin Mode Configuration
 *  
 */
void Config_o_SSPI(void) {
  pinMode(o_SCK,  INPUT_PULLUP);    // this clock goes low to read
  pinMode(o_MOSI, INPUT);           // this is our data pin
  pinMode(o_MISO, INPUT_PULLUP);    // this is wrong (we don't use this pin), but if we set it to OUTPUT the old LCD will blank
  pinMode(o_SS,   OUTPUT);          // this is INPUT (slave mode) but it does not matter: the setting of INPUT is ignored by the micro
  pinMode(o_CD,   INPUT_PULLUP);    // this pin might help us determine if we are in data or not.
}


/* SPI: Register Configuration
 *  
 */
void Enable_o_SSPI() {

#define EMPTY B00000000

  SPCR = EMPTY  | (0 << SPIE);
  SPCR = SPCR   | (1 << SPE );
  SPCR = SPCR   | (0 << DORD);
  SPCR = SPCR   | (0 << MSTR);
  SPCR = SPCR   | (0 << CPOL);
  SPCR = SPCR   | (0 << CPHA);

  SPSR = SPSR   | (0 << SPI2X);
  SPCR = SPCR   | (0 << SPR1);
  SPCR = SPCR   | (0 << SPR0);

}



/* GENERAL PURPOSE MACROS
*/
#define NOOP __asm__ __volatile__ ("nop\n\t")


/* BUSY ICON: Global Variables
 *  
 * This is a feature I've called a "twiddle."
 * It's essentially a loading icon, and it's used here to convey how fast the mutlimeter is sampling. It's a homage to more
 *   modern Keithley multimeters which sample very fast (like the DMM7510 and similar). It's included here is to show a "slow" 
 *   thing when the multimeter is unpowered or locked up or not communicating, and a "fast" thing when it's displaying samples.
 * 
 *   twiddle_last   - the millis() stamp from the last time we changed the character
 *   twiddle        - the current twiddle character
*/
volatile unsigned long twiddle_last = 0;
volatile char twiddle;





/* SERIAL CONSOLE: DEBUG
 *  
 *  This function prints a very long line of information to show the packets recieved.
 */
void print_debug(byte DMMReading[]) {

  String reading = get_digits(DMMReading, false);
  String units = get_units(DMMReading, false);
  String mode = get_mode(DMMReading, false);


  Serial.print(mode + reading + " " + units + " ");

  for (int w = 0; w < (PACKET - 1) ; w++) {
    sprintf("%02u: ", w);
    if (w != MODES_BYTE && w != UNITS_BYTE && w != UNIT2_BYTE)
      sprintf("%02X ", DMMReading[w]);  // print our status bits in padded binary
    else {
      print_binary(DMMReading[w], 8);   // print everything else in normal format
      Serial.print(" ");
    }
  }

  Serial.print("SPCR: ");
  print_binary(SPCR, 8);
  Serial.print(" SPSR: ");
  print_binary(SPSR, 8);
  Serial.print(" ");

  sprintf("CEIO: [%1lu %4lu %5lu %2lu] ", calls, empty_reading, innerloop, outerloop);
  sprintf("%4lu μs", dT);

  Serial.println();

}



/* ARDUINO: main setup()
 *  
 */
void setup() {

  /* start serial */
  Serial.begin(115200);
  Serial.println("Starting...");

  /* start SPI */
  Config_o_SSPI();
  Enable_o_SSPI();

  /* listen for SPI interrupts */
  enableInterrupt(o_SS, handle_o_SS, FALLING);
  
  /* start LCD */
  setup_lcd(lcd);
  
  /* start busy timer */
  twiddle_last = millis();
}

/* ARDUINO: main loop()
 *  
 */
void loop() {

  twiddle = refresh_twiddle(lcd, twiddle, twiddle_last, onoff);

  /* we may have just turned on so we have a spurious SPI command to discard */
  if (done == true && onoff == true) {

    NOOP;                       // discard the first measurement
    volatile byte c = SPDR;     // also discard the first byte, which is an extraneous 0x20.
    delay(1);
    onoff = false;

  /* we have a reading */
  } else if (done == true && onoff == false) {
    print_debug(DMMReading);
    display_measurement(lcd, DMMReading);
    calls         = 0;
    empty_reading = 0;
    innerloop     = 0;
    outerloop     = 0;
    done          = false;

  } /* do something here to detect a timeout */
}

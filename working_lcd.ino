#include <EnableInterrupt.h>
#include <LiquidCrystal.h>
#include <SPI.h>

#include "Plotter.h"
#include <stdarg.h>

#include "lcd.h"
#include "busy.h"
#include "k197.h"
#include "plotter.h"
#include "printing.h"


/* LCD-related pin assignments
 *  
 */
#define RS_13 13
#define RW_12 12
#define E__11 11

#define D0    21
#define D1    20
#define D2    19
#define D3    18
#define D4    17
#define D5    16
#define D6    15
#define D7    14

/* SPI bus-related pin assignments
 *  
 */
#define o_SCK   52    // _SCK_ actually
#define o_MISO  50    // unused here
#define o_MOSI  51    // our data pin
#define o_SS    53    // _SS_ or _CS_: slave select
#define o_CD    46    // CMD / _DATA_

#define PACKET  18    // our SPI packet is 18 bytes


/*
 * Note: This is a NO-OP command, used to do nothing on purpose.
 */
#define NOOP __asm__ __volatile__ ("nop\n\t")


LiquidCrystal lcd (RS_13, RW_12, E__11, D0, D1, D2, D3, D4, D5, D6, D7);

Plotter p;

/*
 * The measurables taken during SPI bus reading.
 * 
 *  * DMMReading is the 18 byte SPI command.
 *  * dT is the amount of time the read took (in micros)
 */
volatile byte DMMReading[18];
volatile bool done = false;
volatile unsigned long dT = 0;

/*
 * These are counters to examine how much time is spent
 *  in certain parts of the SS pin interrupt handler.
 *  These are essentially a debugging feature.
 */
volatile unsigned long innerloop = 0;
volatile unsigned long calls = 0;
volatile unsigned long outerloop = 0;
volatile unsigned long empty_reading = 0;

// for tracking when the multimeter is turned off (not yet supported)
volatile bool off = true;


/*
 * The timestamp of the last busy icon setting,
 *  and the last setting itself.
 */
volatile unsigned long twiddle_last = 0;
volatile char twiddle;

/*
 * Plotting variables
 */
double x = 0;
double x_0 = 0;
double y = 0;
double x2 = 0;
double mv[5];


void setup() {

  
  Serial.begin(115200);

  Config_o_SSPI();
  Enable_o_SSPI();

  setup_lcd(lcd);

  enableInterrupt(o_SS, handle_o_SS, FALLING);

  twiddle_last = millis();
  
  setup_plotter(p,x_0,x,y,x2,mv);


}


void handle_o_SS(void) {
  unsigned long start = micros();
  volatile bool trimming = true;
  volatile byte c;
  volatile int words = 0;
  
  
  calls++;
  
  // This loop burns off 0x00 bytes before the data packet, which only appear to happen during Volts mode
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
      innerloop++;
      c = SPDR;
    }
    
    outerloop++;
    DMMReading[words++] = SPDR;
    if (words == (PACKET - 1)) {
      done = true;
    }
  }
  
}


void Config_o_SSPI(void) {
  pinMode(o_SCK,  INPUT_PULLUP);  // this clock goes low to read
  pinMode(o_MOSI, INPUT);         // this is our data pin
  pinMode(o_MISO, INPUT_PULLUP);  // this is wrong (we don't use this pin), but if we set it to OUTPUT the old LCD will blank
  pinMode(o_SS,  OUTPUT);         // this is INPUT (slave mode) but it does not matter: the setting of INPUT is ignored by the micro
  pinMode(o_CD, INPUT_PULLUP);    // this pin might help us determine if we are in data or not.
}


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


void print_debug(byte DMMReading[]) {

  String reading = get_digits(DMMReading,false);
  String units = get_units(DMMReading,false);
  String mode = get_mode(DMMReading,false);

  
  Serial.print(mode + reading + " " + units + " ");

  for (int w = 0; w < (PACKET - 1) ; w++) {
    sprintf("%02u: ",w);
    if (w != 8 && w != 15 && w != 16)
      sprintf("%02X ", DMMReading[w]);  // print our status bits in padded binary
     else {
      print_binary(DMMReading[w],8);    // print everything else in normal format
      Serial.print(" ");
     }
  }

  Serial.print("SPCR: ");
  print_binary(SPCR,8);
  Serial.print(" SPSR: ");
  print_binary(SPSR,8);
  Serial.print(" ");

  sprintf("CEIO: [%1lu %3lu %4lu %2lu] ", calls, empty_reading, innerloop, outerloop);
  sprintf("%4lu μs", dT);
  
  Serial.println();


}



void loop() {
  
  if (done == true && off == true) {
    
    NOOP;                       // discard the first measurement
    volatile byte c = SPDR;     // also discard the first byte, which is an extraneous 0x20.
    delay(1);
    off = false;
    
  } else if (done == true /* && off = false */) {
  
    //print_debug(DMMReading);
    
    display_measurement(lcd, DMMReading);
    
    loop_plotter(p, DMMReading, x_0, x, y, x2,mv);
    
    calls         = 0;
    empty_reading = 0;
    innerloop     = 0;
    outerloop     = 0;
    done          = false;
    
  }

  twiddle = refresh_twiddle(lcd, twiddle, twiddle_last, off);
}









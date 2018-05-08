#include "Arduino.h"


void print_binary(int v, int num_places)
{
    int mask=0, n;

    for (n=1; n<=num_places; n++) {
        mask = (mask << 1) | 0x0001;
    }
    v = v & mask;  // truncate v to specified number of places

    while(num_places) {
        if (v & (0x0001 << num_places-1))
             Serial.print("1");
        else
             Serial.print("0");
        --num_places;
    }
}


void sprintf(const __FlashStringHelper *fmt, ... ){
  char buf[128]; // resulting string limited to 128 chars
  va_list args;
  va_start (args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);
  Serial.print(buf);
}


void sprintf(char *fmt, ... ){
        char buf[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(buf, 128, fmt, args);
        va_end (args);
        Serial.print(buf);
}




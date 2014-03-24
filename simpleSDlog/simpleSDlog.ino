#include <Wire.h>

// for the SD card
// include the SD library:
#include <SD.h>
#include <SPI.h>
#include <avr/pgmspace.h>

// for the RTC clock
#include <stdlib.h>
#include "RTClib.h"

// set up variables using the SD utility library functions:
//Sd2Card card;
//SdVolume volume;
//SdFile root;

// Debug info
#ifdef DEBUG_FLAG
#undef DEBUG_FLAG
#endif

//#undef DEBUG_FLAG
#define DEBUG_FLAG 1

#ifdef DEBUG_FLAG
#define DBPRINT(x) Serial.println(x)
#else
#define DBPRINT ;
#endif

// #define constants
#define BUFFER_SIZE 64

uint8_t analogData[BUFFER_SIZE];

int buffer_count = 0;
RTC_DS1307 RTC;
unsigned long timer;
File logfile;
char filename[] = "DATA00.BIN";

void setup()
{
    Serial.begin(115200);
    Wire.begin();
    
    // Real time clock init
    RTC_DS1307 RTC;
    RTC.begin();
    if (! RTC.isrunning()) {
        while(1);
    } else {
        DBPRINT("RTC done");
    }

    // SD card init
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    //Serial.print(F("Initializing SD card..."));
    pinMode(10, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(10)) {
        DBPRINT("Card failed");
        // don't do anything more:
        while(1);
    } else {
        DBPRINT("SD card done");
    }

    // init buffer count
    for (buffer_count = 0; buffer_count<BUFFER_SIZE; buffer_count++) {
        analogData[buffer_count] = 0;
    }

    buffer_count = 0;

    for (uint8_t i = 0; i < 100; i++) {
        filename[4] = i/10 + '0';
        filename[5] = i%10 + '0';
        if (! SD.exists(filename)) {
            // only open a new file if it doesn't exist
            logfile = SD.open(filename, FILE_WRITE);
            logfile.close();
            break; // leave the loop!
        }
    }
    if (! logfile) {
        DBPRINT("couldnt create file");
    }

    DBPRINT("Init done");
}

//int cnt = 0;
void loop(void) {
    int tempRead;

    tempRead = analogRead(0);
    //Serial.println(tempRead, HEX);
    analogData[buffer_count] = tempRead;
    buffer_count = buffer_count + 1;
    analogData[buffer_count] = tempRead >> 8;
    buffer_count = buffer_count + 1;

    // TIME TEST
    if (buffer_count == BUFFER_SIZE) {
        DateTime now = RTC.now();

        logfile = SD.open(filename, FILE_WRITE);
        logfile.write(analogData,BUFFER_SIZE);
        DBPRINT(logfile.size());

        logfile.close();
                
        //timer = millis();
        //Serial.println(timer);
        buffer_count = 0;
        //logfile.println(now.unixtime());

        //cnt = cnt + 1;
        //if (cnt == 10) while(1);
    }
}



#include "LowPower.h"
#include <Wire.h>

// for the SD card
// include the SD library:
#include <SD.h>
#include <SPI.h>
#include <avr/pgmspace.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 10;
File logfile;
char filename[] = "LOGGER00.CSV";

// for the RTC clock
#include<stdlib.h>
#include "RTClib.h"

RTC_DS1307 RTC;

void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Wire.begin();
  
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  // SD card init
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  // create a new file
  /*
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
    // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE);
      break; // leave the loop!
    }
  }
  if (!logfile) {
    Serial.println("couldnt create file");
  }
  logfile.close();
  Serial.print("Logging to: ");
  Serial.println(filename);
  delay(1000);
  */
}

void loop(void) {
  //uint32_t timeStamp;
  
  // Sleep for 8 s with ADC module and BOD module off
  //LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                //SPI_OFF, USART0_OFF, TWI_OFF);
  // Do something here
  // Example: read sensor, log data, transmit data
  //getTime(&timeStamp);
  //Serial.println(timeStamp);
  //write2SD(timeStamp);
  
  //need this delay to allow enough time to print before goto sleep
  //delay(1);
}

void getTime(uint32_t* timeStamp) {    
  // Get the time from RTC chip
  DateTime now = RTC.now();
  
  *timeStamp = now.unixtime();
}

void write2SD(uint32_t timeStamp) {
  // Start logging data
  logfile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (logfile) {
    //Serial.println(timeStamp);
    logfile.println(timeStamp);
    //Serial.println("written");
  } else { // if the file isn't open, pop up an error:
    Serial.println("error opening data.txt");
  }
  logfile.close();
  delay(15);
}

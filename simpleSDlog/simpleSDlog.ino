#include <Wire.h>

// include timer library
#include "Timer.h"
Timer t;

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

#undef DEBUG_FLAG
//#define DEBUG_FLAG 1

#ifdef DEBUG_FLAG
#define DBPRINT(x) Serial.println(F(x))
#else
#define DBPRINT ;
#endif

RTC_DS1307 RTC;
unsigned long timer;
File logfile;
char filename[] = "DATA00.DAT";

#define TIME_STAMP 0x01
#define ACC_TYPE 0x02
#define PULSE_TYPE 0x03
#define RECORD_ARRAY_SIZE 10

typedef struct {
    uint32_t    timeStamp;
    uint8_t     dataType;
    float       sensorXaxis;
    float       sensorYaxis;
    float       sensorZaxis;
} record_t;

static record_t recordArray[RECORD_ARRAY_SIZE];
int recordCount;

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>
Adafruit_LSM303_Accel accel = Adafruit_LSM303_Accel(54321);

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    // Real time clock init
    RTC.begin();
    if (! RTC.isrunning()) {
        DBPRINT("RTC error");
        while(1);
    } else {
        DBPRINT("RTC done");
    }

    if(!accel.begin())
    {
        DBPRINT("Acc failed");
        /* There was a problem detecting the ADXL345 ... check your connections */
        //Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
        while(1);
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

    for (uint8_t i = 0; i < 100; i++) {
        filename[4] = i/10 + '0';
        filename[5] = i%10 + '0';
        if (! SD.exists(filename)) {
            // only open a new file if it doesn't exist
            logfile = SD.open(filename, FILE_WRITE);
            //logfile.close();
            break; // leave the loop!
        }
    }

    //logfile = SD.open("HELLO.TES", FILE_WRITE);

    if (! logfile) {
        DBPRINT("couldnt create file");
    }

    logfile.close();
    
    DBPRINT("Init done");

    attachInterrupt(1, recordPulse, FALLING);//set interrupt 0,digital port 2
    
    t.every(1000, programRecord);
    t.every(250, recordAcc);
}

void programRecord()
{
    uint32_t local_timeStamp;
    local_timeStamp = RTC.now().unixtime();
    delay(10);

    // make a string for assembling the data to log:
    logfile = SD.open(filename, FILE_WRITE);

    if (! logfile) {
        DBPRINT("couldnt open file");
    }

    logfile.print(TIME_STAMP);
    logfile.print(",");
    logfile.println(local_timeStamp);
    
    for (int i=0; i<recordCount; i++){

        logfile.print(recordArray[i].dataType);
        
        if (recordArray[i].dataType == ACC_TYPE){
            DBPRINT("pgm acc");
            logfile.print(",");
            logfile.print(recordArray[i].sensorXaxis);
            logfile.print(",");
            logfile.print(recordArray[i].sensorYaxis);
            logfile.print(",");
            logfile.println(recordArray[i].sensorYaxis);
        } else if (recordArray[i].dataType == PULSE_TYPE) {
            DBPRINT("pgm pulse");
            logfile.print("\n");
        } else {
            DBPRINT("Error pgm!");
        }
        
    }

    logfile.close();
    recordCount = 0;

    delay(50);
}

void recordPulse() 
{
    if (recordCount<RECORD_ARRAY_SIZE) {
        DBPRINT("read pulse");
        recordArray[recordCount].timeStamp = 0;
        recordArray[recordCount].dataType = PULSE_TYPE;
        recordCount = recordCount + 1;
    } else {
        DBPRINT("record array overflow");
    }
}

void recordAcc()
{    
    sensors_event_t eventAcc; 
    accel.getEvent(&eventAcc);

    if (recordCount<RECORD_ARRAY_SIZE) {
        DBPRINT("read time");
        DBPRINT(recordCount);
        recordArray[recordCount].timeStamp = 0;
        recordArray[recordCount].dataType = ACC_TYPE;
        recordArray[recordCount].sensorXaxis = eventAcc.acceleration.x;
        recordArray[recordCount].sensorYaxis = eventAcc.acceleration.y;
        recordArray[recordCount].sensorZaxis = eventAcc.acceleration.z;
        recordCount = recordCount + 1;
    } else {
        DBPRINT("record array overflow");
    }
}

//int cnt = 0;
void loop(void) {
    t.update();
    // TIME TEST
}


// === General debug setup ===
#include "debug_setup.h"

// === BLE part include ===
#include "BLEpart.h"

//=== Start of code ======
#include <Wire.h>

// include timer library
#include "Timer.h"
Timer t;

// for the SD card include the SD library:
#include <SD.h>
#include <SPI.h>
#include <avr/pgmspace.h>

// for the RTC clock
#include <stdlib.h>
#include "RTClib.h"

RTC_DS1307 RTC;
File logfile;
char filename[] = "D00.D";

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

void programRecord()
{
    delay(10);

    // make a string for assembling the data to log:
    logfile = SD.open(filename, FILE_WRITE);

    if (! logfile) {
        DBPRINT("couldnt open file");
    }

    logfile.print(TIME_STAMP);
    logfile.print(",");
    logfile.println(RTC.now().unixtime());

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
        DBPRINT("read acc");
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

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    // Real time clock init
    RTC.begin();
    if (! RTC.isrunning()) {
        DBPRINT("RTC err");
        //while(1);
    } else {
        DBPRINT("RTC done");
    }

    if(!accel.begin())
    {
        /* There was a problem detecting the ADXL345 ... check your connections */
        DBPRINT("Acc fail");
        while(1);
    }

    // SD card init
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    //Serial.print(F("Initializing SD card..."));
    pinMode(10, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(10)) {
        DBPRINT("SD fail");
        // don't do anything more:
        while(1);
    } else {
        DBPRINT("SD done");
    }

    for (uint8_t i = 0; i < 100; i++) {
        filename[1] = i/10 + '0';
        filename[2] = i%10 + '0';
        if (! SD.exists(filename)) {
            // only open a new file if it doesn't exist
            logfile = SD.open(filename, FILE_WRITE);
            //logfile.close();
            break; // leave the loop!
        }
    }

    //logfile = SD.open("HELLO.TES", FILE_WRITE);

    if (! logfile) {
        DBPRINT("cant new file");
    }

    logfile.close();
    
    DBPRINT("Init done");

    attachInterrupt(1, recordPulse, FALLING);//set interrupt 0,digital port 3
    
    t.every(1000, programRecord);
    t.every(250, recordAcc);

#ifdef BLE_CODE
    /** Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001 */ 
    if (services_pipe_type_mapping != NULL ) {
        aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
    } else {
        aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
    }
  
    aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
    aci_state.aci_setup_info.setup_msgs         = setup_msgs;
    aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;
    
    //We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
    //and initialize the data structures required to setup the nRF8001
    lib_aci_init(&aci_state);
#endif
}

void loop(void) {
    t.update();
    // TIME TEST

#ifdef BLE_CODE
    aci_loop(&isReady);
    DBPRINT(isReady);

    binaryFloat tempReading1;

    if (isReady == 1) {
        tempReading1.floatingPoint = 4.9 * analogRead(0);
        uart_buffer_len = 5;
        uart_buffer[0] = 'm';
        uart_buffer[1] = tempReading1.binary[0];
        uart_buffer[2] = tempReading1.binary[1];
        uart_buffer[3] = tempReading1.binary[2];
        uart_buffer[4] = tempReading1.binary[3];
        uart_tx();
    }
#endif

}


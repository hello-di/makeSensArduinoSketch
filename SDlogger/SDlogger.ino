#include <Wire.h>

// for the SD card
// include the SD library:
#include <SD.h>
#include <SPI.h>
#include <avr/pgmspace.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>

// set up variables using the SD utility library functions:
//Sd2Card card;
//SdVolume volume;
//SdFile root;

// for the RTC clock
#include <stdlib.h>
#include "RTClib.h"

void setup()
{
    RTC_DS1307 RTC;
    /* Assign a unique ID to this sensor at the same time */
    Adafruit_LSM303_Accel accel = Adafruit_LSM303_Accel(54321);

    /* Assign a unique ID to this sensor at the same time */
    //Adafruit_LSM303_Mag mag = Adafruit_LSM303_Mag(12345);

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    Wire.begin();
    
    RTC.begin();
    if (! RTC.isrunning()) {
        //Serial.println(F("RTC is NOT running!"));
        // following line sets the RTC to the date & time this sketch was compiled
        // uncomment it & upload to set the time, date and start run the RTC!
        //RTC.adjust(DateTime(__DATE__, __TIME__));
        while(1);
    }
  
    // SD card init
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    //Serial.print(F("Initializing SD card..."));
    pinMode(10, OUTPUT);
    
    // see if the card is present and can be initialized:
    if (!SD.begin(10)) {
        //Serial.println(F("Card failed, or not present"));
        // don't do anything more:
        while(1);
        return;
    }
    //Serial.println(F("card initialized."));

    //Setup to read the sensors
    //Serial.print(F("Sensor Test..."));
    
    /* Initialise the sensor */
    if(!accel.begin())
    {
        /* There was a problem detecting the ADXL345 ... check your connections */
        //Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
        while(1);
    }
    
    /* Initialise the sensor */
    //if(!mag.begin())
    //{
    //    /* There was a problem detecting the LSM303 ... check your connections */
    //    Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
    //    while(1);
    //}
  
    Serial.println(F("done"));
  
    //displayMagSensorDetails();
    //displaySensorDetails();
}

int i=0;

void loop(void) {
    uint32_t timeStamp[10];
    float accSensorData[10][3];
    //float magSensorData[10][3];

    //getMagData(magSensorData[i]);
    getAccData(accSensorData[i]);
    getTime(&timeStamp[i]);
    //Serial.print("Mag data: ");
    //Serial.print(magSensorData[i][0]);
    //Serial.print(",");
    //Serial.print(magSensorData[i][1]);
    //Serial.print(",");
    //Serial.println(magSensorData[i][2]);
    //Serial.print("Acc data: ");
    //Serial.print(accSensorData[i][0]);
    //Serial.print(",");
    //Serial.print(accSensorData[i][1]);
    //Serial.print(",");
    //Serial.println(accSensorData[i][2]);
    //Serial.print("timeStamp: ");
    //Serial.println(timeStamp[i]);

    // Sleep for 8 s with ADC module and BOD module off
    //LowPower.idle(SLEEP_120MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  //SPI_OFF, USART0_OFF, TWI_OFF);
    // Do something here
    // Example: read sensor, log data, transmit data
    if (i==9) {
        //Serial.println("Saving data to Sd");
        write2SD(timeStamp, accSensorData);
        //write2SD(timeStamp, accSensorData, magSensorData);
        //write2SD(accSensorData, magSensorData);
        i=0;
    }

    i++;
}

void getTime(uint32_t* timeStamp) {    
    RTC_DS1307 RTC;
    // Get the time from RTC chip
    DateTime now = RTC.now();
  
    *timeStamp = now.unixtime();
}

void getAccData(float* accData) {
    /* Assign a unique ID to this sensor at the same time */
    Adafruit_LSM303_Accel accel = Adafruit_LSM303_Accel(54321);
    /* Get a new sensor event */ 
    sensors_event_t eventAcc; 
    accel.getEvent(&eventAcc);

    accData[0] = eventAcc.acceleration.x;
    accData[1] = eventAcc.acceleration.y;
    accData[2] = eventAcc.acceleration.z;

    delay(100);
}


//void getMagData(float* magData) {
//    /* Assign a unique ID to this sensor at the same time */
//    Adafruit_LSM303_Mag mag = Adafruit_LSM303_Mag(12345);
//    
//    /* Get a new sensor event */ 
//    sensors_event_t eventAcc; 
//
//    mag.getEvent(&eventAcc);
//
//    magData[0] = eventAcc.magnetic.x;
//    magData[1] = eventAcc.magnetic.y;
//    magData[2] = eventAcc.magnetic.z;
//
//    delay(100);
//}

void write2SD(uint32_t *timeStampArray, float accArray[][3]) {
//void write2SD(uint32_t *timeStampArray, float accArray[][3], float magArray[][3]) {
//void write2SD(float accArray[][3], float magArray[][3]) {
    int j;
    // Start logging data
    File logfile = SD.open("data6.txt", FILE_WRITE);
    // if the file is available, write to it:
    if (logfile) {
        //Serial.println(timeStamp);
        for (j=0;j<=9;j++){
            logfile.print(timeStampArray[j]);
            logfile.print(",");
            logfile.print(accArray[j][0]);
            logfile.print(",");
            logfile.print(accArray[j][1]);
            logfile.print(",");
            logfile.println(accArray[j][2]);
            //logfile.print(",");
            //logfile.print(magArray[j][0]);
            //logfile.print(",");
            //logfile.print(magArray[j][1]);
            //logfile.print(",");
            //logfile.println(magArray[j][2]);

            //Serial.print(timeStampArray[j]);
            //Serial.print(accArray[j][0]);
            //Serial.print(accArray[j][1]);
            //Serial.print(accArray[j][2]);
            //Serial.print(magArray[j][0]);
            //Serial.print(magArray[j][1]);
            //Serial.println(magArray[j][2]);
        }
    } else { // if the file isn't open, pop up an error:
      Serial.println(F("error opening data.txt"));
    }

    logfile.close();
    delay(5);
}

//void displaySensorDetails(void)
//{
//    sensor_t sensor;
//    accel.getSensor(&sensor);
//    Serial.println("------------------------------------");
//    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
//    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
//    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
//    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
//    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
//    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
//    Serial.println("------------------------------------");
//    Serial.println("");
//    delay(100);
//}
//
//
//void displayMagSensorDetails(void)
//{
//    sensor_t sensor;
//    mag.getSensor(&sensor);
//    Serial.println("------------------------------------");
//    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
//    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
//    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
//    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
//    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
//    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
//    Serial.println("------------------------------------");
//    Serial.println("");
//    delay(100);
//}
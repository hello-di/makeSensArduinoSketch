#ifndef _DATABASECLASS_H_
#define _DATABASECLASS_H_

// SD card lib
#include "debug_setup.h"
#include <SD.h>
#include <SPI.h>
#include <stdlib.h>

// Accelerometer lib
#include "AccelerometerClass.h"

#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>

#define MAX_STEPS_PER_WRITE 40

#define TIMESTAMP_TYPECODE 0x01
#define ACCEL_TYPECODE 0x02
#define PULSE_TYPECODE 0x03
#define TEMPERATURE_TYPECODE 0x04

class DatabaseClass
{
public:
    File _logfile;
    char _filename[5];

private:
    Adafruit_LSM303_Accel _accel;
    float _accelRead[MAX_STEPS_PER_WRITE][3];

    uint8_t _stepCounter;
    uint32_t _fakeRTC;

public:
    DatabaseClass();
    ~DatabaseClass();
    bool newCollection();
    bool stepForward(uint8_t stepLength);

private:
    bool _write2disk();

};

#endif

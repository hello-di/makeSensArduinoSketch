#include "DatabaseClass.h"

DatabaseClass::DatabaseClass(){
    _filename[0] = 'D';
    _filename[1] = '0';
    _filename[2] = '0';
    _filename[3] = '.';
    _filename[4] = 'D';

    _stepCounter = 0;
    _fakeRTC = 0;

    _accel = Adafruit_LSM303_Accel(54321);
}

DatabaseClass::~DatabaseClass(){
	;
}

bool DatabaseClass::newCollection(){
	    // SD card init
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(10, OUTPUT);

    // see if the card is present and can be initialized:
    if (!SD.begin(6)) {
        DBPRINT("SD fail");
        // don't do anything more:
        while(1);
    } else {
        DBPRINT("SD done");
    }

    for (uint8_t i = 0; i < 100; i++) {
        _filename[1] = i/10 + '0';
        _filename[2] = i%10 + '0';
        if (! SD.exists(_filename)) {
            // only open a new file if it doesn't exist
            _logfile = SD.open(_filename, FILE_WRITE);
            //_logfile.close();
            break; // leave the loop!
        }
    }

    if (! _logfile) {
        DBPRINT("cant new file");
    }

    _logfile.close();

    /* Initialise the sensor */
    if(!_accel.begin())
    {
      	/* There was a problem detecting the ADXL345 ... check your connections */
      	DBPRINTLN("no LSM303!");
    }

	return true;
}

bool DatabaseClass::stepForward(uint8_t stepLength) {
	int timeKeeper1, timeKeeper2;
	timeKeeper1 = millis();

	sensors_event_t eventAcc; 
    _accel.getEvent(&eventAcc);
	_accelRead[_stepCounter][0] = eventAcc.acceleration.x;
	_accelRead[_stepCounter][1] = eventAcc.acceleration.y;
	_accelRead[_stepCounter][2] = eventAcc.acceleration.z;
	DBPRINTLN(eventAcc.acceleration.x);

	_stepCounter = _stepCounter + 1;
	DBPRINT(_stepCounter);
    
    if (_stepCounter >= MAX_STEPS_PER_WRITE) {
    	DBPRINT("WRITE");
    	_write2disk();
    	_stepCounter = 0;
    }

    timeKeeper2 = millis();

    if ((timeKeeper2 - timeKeeper1) < stepLength) {
    	DBPRINT("sleep");
    	RFduino_ULPDelay(MILLISECONDS(stepLength - (timeKeeper2-timeKeeper1)));
    }

	return true;
}

bool DatabaseClass::_write2disk() {
	_logfile = SD.open(_filename, FILE_WRITE);

	if (! _logfile) {
	    DBPRINT("couldnt open file");
	}

	_logfile.print(TIMESTAMP_TYPECODE);
	_logfile.print(",");
	_logfile.println(_fakeRTC++);

	DBPRINT("pgm temperature");
	_logfile.print(TEMPERATURE_TYPECODE);
	_logfile.print(",");
	_logfile.println(RFduino_temperature(CELSIUS));

	for (int i=0; i<MAX_STEPS_PER_WRITE; i++){

	    _logfile.print(ACCEL_TYPECODE);
	   
	    DBPRINT("pgm acc");
	    _logfile.print(",");
	    _logfile.print(_accelRead[i][0]);
	    _logfile.print(",");
	    _logfile.print(_accelRead[i][1]);
	    _logfile.print(",");
	    _logfile.println(_accelRead[i][2]);
	    
	}

	_logfile.close();

	return true;
}
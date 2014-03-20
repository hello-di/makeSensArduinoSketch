/**************************************************************************/
/*! 
    Adafruit invests time and resources providing this open source code, 
    please support Adafruit and open-source hardware by purchasing 
    products from Adafruit!
*/
/**************************************************************************/
#include <SPI.h>
#include <avr/pgmspace.h>
#include <ble_system.h>
#include <lib_aci.h>
#include <aci_setup.h>

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/


#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

// aci_struct that will contain 
// total initial credits
// current credit
// current state of the aci (setup/standby/active/sleep)
// open remote pipe pending
// close remote pipe pending
// Current pipe available bitmap
// Current pipe closed bitmap
// Current connection interval, slave latency and link supervision timeout
// Current State of the the GATT client (Service Discovery)
// Status of the bond (R) Peer address
static struct aci_state_t aci_state;

/*
Temporary buffers for sending ACI commands
*/
static hal_aci_evt_t  aci_data;
//static hal_aci_data_t aci_cmd;

/*
Timing change state variable
*/
static bool timing_change_done = false;

/*
Used to test the UART TX characteristic notification
*/
static uint8_t uart_buffer[20];
static uint8_t uart_buffer_len = 0;

typedef union {
  float floatingPoint;
  byte binary[4];
} binaryFloat;

/*
Initialize the radio_ack. This is the ack received for every transmitted packet.
*/
//static bool radio_ack_pending = false;

/*
Description:

In this template we are using the BTLE as a UART and can send and receive packets.
The maximum size of a packet is 20 bytes.
When a command it received a response(s) are transmitted back.
Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
The pipe for the UART_TX becomes available once the peer opens it.
See section 20.4.1 -> Opening a Transmit pipe 
In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.

The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
*/

void uart_tx()
{
  lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, uart_buffer, uart_buffer_len);
  aci_state.data_credit_available--;
}

void aci_loop(int datax, int *flag)
{
  
  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;
    
    aci_evt = &aci_data.evt;
    Serial.println("====");
    Serial.println(aci_evt->evt_opcode, HEX);
    switch(aci_evt->evt_opcode)
    {
        /**
        As soon as you reset the nRF8001 you will get an ACI Device Started Event
        */
        case ACI_EVT_DEVICE_STARTED:
        {          
          aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
          switch(aci_evt->params.device_started.device_mode)
          {
            case ACI_DEVICE_SETUP:
            /**
            When the device is in the setup mode
            */
            Serial.println(F("Evt Device Started: Setup"));
            if (ACI_STATUS_TRANSACTION_COMPLETE != do_aci_setup(&aci_state))
            {
              Serial.println(F("Error in ACI Setup"));
            }
            break;
            
            case ACI_DEVICE_STANDBY:
              Serial.println(F("Evt Device Started: Standby"));
              //Looking for an iPhone by sending radio advertisements
              //When an iPhone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
              lib_aci_connect(180/* in seconds */, 0x0050 /* advertising interval 50ms*/);
              Serial.println(F("Advertising started"));
              break;
          }
        }
        break; //ACI Device Started Event
        
      case ACI_EVT_CMD_RSP:
        //If an ACI command response event comes with an error -> stop
        if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
        {
          //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
          //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
          //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
          Serial.print(F("ACI Command "));
          Serial.println(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
          Serial.println(F("Evt Cmd respone: Error. Arduino is in an while(1); loop"));
          while (1);
        }
        if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode)
        {
          //Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
          lib_aci_set_local_data(&aci_state, PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET, 
            (uint8_t *)&(aci_evt->params.cmd_rsp.params.get_device_version), sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
        }        
        break;
        
      case ACI_EVT_CONNECTED:
        Serial.println(F("Evt Connected"));
        aci_state.data_credit_available = aci_state.data_credit_total;
        
        /*
        Get the device version of the nRF8001 and store it in the Hardware Revision String
        */
        lib_aci_device_version();
        break;
        
      case ACI_EVT_PIPE_STATUS:
        Serial.println(F("Evt Pipe Status"));
        if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) && (false == timing_change_done))
        {
          lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP. 
                                            // Used to increase or decrease bandwidth
          timing_change_done = true;
        }
        if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX)) {
          Serial.println("GGid ti ti");
          *flag = 1;
        }
        
        break;
        
      case ACI_EVT_TIMING:
        Serial.println(F("Evt link connection interval changed"));
        break;
        
      case ACI_EVT_DISCONNECTED:
        Serial.println(F("Evt Disconnected/Advertising timed out"));
        lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
        Serial.println(F("Advertising started"));
        *flag = 0;       
        break;
        
      case ACI_EVT_DATA_RECEIVED:
        Serial.print(F("UART RX: 0x"));
        Serial.print(aci_evt->params.data_received.rx_data.pipe_number, HEX);
        {
          Serial.print(F(" Data(Hex) : "));
          for(int i=0; i<aci_evt->len - 2; i++)
          {
            Serial.print(aci_evt->params.data_received.rx_data.aci_data[i], HEX);
            uart_buffer[i] = aci_evt->params.data_received.rx_data.aci_data[i];
            Serial.print(F(" "));
          }
          uart_buffer_len = aci_evt->len - 2;
        }
        Serial.println(F(""));
        //if (uart_buffer[0] == 'a' && uart_buffer[1] == 'b'){
        Serial.print(F("I got the request"));
        //if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX))
        //{
        //  uart_buffer_len = 10;
        //  uart_buffer[0] = datax;
        //  uart_buffer[1] = datax;
        //  uart_buffer[2] = datax;
        //  uart_buffer[3] = datax;
        //  uart_buffer[4] = datax; 
        //  uart_tx();
        //}
          
        break;
   
      case ACI_EVT_DATA_CREDIT:
        aci_state.data_credit_available = aci_state.data_credit_available + aci_evt->params.data_credit.credit;
        break;
      
      case ACI_EVT_PIPE_ERROR:
        //See the appendix in the nRF8001 Product Specication for details on the error codes
        Serial.print(F("ACI Evt Pipe Error: Pipe #:"));
        Serial.print(aci_evt->params.pipe_error.pipe_number, DEC);
        Serial.print(F("  Pipe Error Code: 0x"));
        Serial.println(aci_evt->params.pipe_error.error_code, HEX);
                
        //Increment the credit available as the data packet was not sent
        aci_state.data_credit_available++;
        break;
           
    }
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }
}

#define __DEBUG__

#include<stdlib.h>
#include <Wire.h>
#include "RTClib.h"
//RTC_DS1307 RTC;

// include the SD library:
// #include <SD.h>
// include necessary file for the sensor operations
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303.h>

// set up variables using the SD utility library functions:
// Sd2Card card;
// SdVolume volume;
// SdFile root;

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel accel = Adafruit_LSM303_Accel(54321);
/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Mag mag = Adafruit_LSM303_Mag(12345);

// const int chipSelect = 10;

void setup(void) {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Wire.begin();
  TWBR = 12;
  /*
  RTC.begin();
 
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    // uncomment it & upload to set the time, date and start run the RTC!
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  */
  //Setup to read the sensors
  Serial.print("Sensor Test...");
  
  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  /* Initialise the sensor */
  if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while(1);
  }
  
  Serial.println("done");
  
  
  /*
  //Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  */
  
  Serial.println(F("BLE setup"));
  
  /**
  Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
  */   
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  //We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
  //and initialize the data structures required to setup the nRF8001
  lib_aci_init(&aci_state);
  
}

int isReady;

void loop() {
  float accSensorData[3];
  float magSensorData[3];
  uint32_t timeStamp;
  binaryFloat tempReading1, tempReading2,tempReading3;
  
  // get_serial_command();
  //getTime(&timeStamp);
  //Serial.print("\ndata: :");
  //Serial.print(accSensorData[0]);
  
  //write2SD(accSensorData, magSensorData, timeStamp);
  
    
  getMagData(magSensorData);
  /*
  Serial.println(magSensorData[0]);
  Serial.println(magSensorData[1]);
  Serial.println(magSensorData[2]);
  */
  aci_loop(64, &isReady);
  delay(100);
  if (isReady == 1) {
    
    // send out mag data
    uart_buffer_len = 13;
    tempReading1.floatingPoint = magSensorData[0];
    tempReading2.floatingPoint = magSensorData[1];
    tempReading3.floatingPoint = magSensorData[2];
    
    Serial.println(tempReading1.floatingPoint);
    Serial.println(tempReading2.floatingPoint);
    Serial.println(tempReading3.floatingPoint);
    
    uart_buffer[0] = 'm';
    
    uart_buffer[1] = tempReading1.binary[0];
    uart_buffer[2] = tempReading1.binary[1];
    uart_buffer[3] = tempReading1.binary[2];
    uart_buffer[4] = tempReading1.binary[3];
    
    uart_buffer[5] = tempReading2.binary[0];
    uart_buffer[6] = tempReading2.binary[1];
    uart_buffer[7] = tempReading2.binary[2];
    uart_buffer[8] = tempReading2.binary[3];
    
    uart_buffer[9] = tempReading3.binary[0];
    uart_buffer[10] = tempReading3.binary[1];
    uart_buffer[11] = tempReading3.binary[2];
    uart_buffer[12] = tempReading3.binary[3];
    
    Serial.print(magSensorData[0]);
    Serial.print(magSensorData[1]);
    Serial.println(magSensorData[2]);
    uart_tx();
    // send out mag data
  }
  
  getAccData(accSensorData);
  aci_loop(64, &isReady);
  delay(100);
  
  Serial.println(4.9 * analogRead(0));

  if (isReady == 1) {
    
    // send out acc data
    uart_buffer_len = 13;
    //tempReading1.floatingPoint = accSensorData[0];
    tempReading1.floatingPoint = 4.9 * analogRead(0);
    tempReading2.floatingPoint = accSensorData[1];
    tempReading3.floatingPoint = accSensorData[2];
    
    uart_buffer[0] = 'a';
    
    uart_buffer[1] = tempReading1.binary[0];
    uart_buffer[2] = tempReading1.binary[1];
    uart_buffer[3] = tempReading1.binary[2];
    uart_buffer[4] = tempReading1.binary[3];
    
    uart_buffer[5] = tempReading2.binary[0];
    uart_buffer[6] = tempReading2.binary[1];
    uart_buffer[7] = tempReading2.binary[2];
    uart_buffer[8] = tempReading2.binary[3];
    
    uart_buffer[9] = tempReading3.binary[0];
    uart_buffer[10] = tempReading3.binary[1];
    uart_buffer[11] = tempReading3.binary[2];
    uart_buffer[12] = tempReading3.binary[3];
    
    Serial.print(accSensorData[0]);
    Serial.print(accSensorData[1]);
    Serial.println(accSensorData[2]);
    uart_tx();
    
    // send out mag data
  }
  if (isReady == 0) {
    delay(1000);
  }
}

/*
void get_serial_command(){
  if (Serial.available())
  {
    char ch = Serial.read();
    if (ch == 'x')
    {
      //Serial.println("Send out data");
      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File dataFile = SD.open("data.txt");

      // if the file is available, write to it:
      if (dataFile) {
        while (dataFile.available()) {
          Serial.write(dataFile.read());
        }
        dataFile.close();
        
        //Serial.write("Reset data.txt...");
        if (!SD.remove("data.txt")) 
          Serial.println("Error resetting file");
        //else 
          //Serial.write("Done\n");
        
      } else { // if the file isn't open, pop up an error:
        Serial.println("error opening data.txt");
      }
    }
  }
}
*/


void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(100);
}


void displayMagSensorDetails(void)
{
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(100);
}

void getAccData(float* accData) {
  /* Get a new sensor event */ 
  sensors_event_t eventAcc; 
  accel.getEvent(&eventAcc);
  // make a string for assembling the data to log:
  String dataString = "";
  char buffer[9];

  accData[0] = eventAcc.acceleration.x;
  accData[1] = eventAcc.acceleration.y;
  accData[2] = eventAcc.acceleration.z;

  delay(50);
}

void getMagData(float* magData) {
  /* Get a new sensor event */ 
  sensors_event_t eventAcc; 
  // make a string for assembling the data to log:
  String dataString = "";
  char buffer[9];

  mag.getEvent(&eventAcc);

  magData[0] = eventAcc.magnetic.x;
  magData[1] = eventAcc.magnetic.y;
  magData[2] = eventAcc.magnetic.z;

  delay(50);
}

/*
void getTime(uint32_t* timeStamp) {    
  String dataString = "";
  // Get the time from RTC chip
  DateTime now = RTC.now();
  
  *timeStamp = now.unixtime(); 
  
  delay(100);
}
*/

/*
void write2SD(float* accData, float* magData, uint32_t timeStamp) {
  int i;
  // Start logging data

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("data.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    // print to the serial port too:
    for (i=0;i<3;i++){
      //Serial.print(accData[i]);
      //Serial.print(',');
      dataFile.print(accData[i]);
      dataFile.print(',');
    }
    for (i=0;i<3;i++){
      //Serial.print(magData[i]);
      //Serial.print(',');
      dataFile.print(magData[i]);
      dataFile.print(',');
    }
    //Serial.println(timeStamp);
    dataFile.println(timeStamp);

    dataFile.close();
    
  } else { // if the file isn't open, pop up an error:
    Serial.println("error opening data.txt");
  } 
  
  delay(100);
}
*/

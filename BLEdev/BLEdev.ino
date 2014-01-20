// === Header file declaration part ===
#include <SPI.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include <ble_system.h>
#include <lib_aci.h>
#include <aci_setup.h>
#include <services.h>

// === #define part ===
#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

// === Global variable definition part ===
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

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

// Logic functions part
void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.print("Device powered on\n");
  
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
    
    Serial.print("BLE init done.\n");
}

/* Temporary buffers for sending ACI commands */
static hal_aci_evt_t  aci_data;

/* Timing change state variable */
static bool timing_change_done = false;

/* Used to test the UART TX characteristic notification */
static uint8_t uart_buffer[20];
static uint8_t uart_buffer_len = 0;

void aci_loop()
{
  
  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;
    
    aci_evt = &aci_data.evt;
    switch(aci_evt->evt_opcode)
    {
        /* As soon as you reset the nRF8001 you will get an ACI Device Started Event */
        case ACI_EVT_DEVICE_STARTED:
        {          
            aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
            switch(aci_evt->params.device_started.device_mode)
            {
                /** When the device is in the setup mode */
                case ACI_DEVICE_SETUP:
                    Serial.println(F("Evt Device Started: Setup"));
                    if (ACI_STATUS_TRANSACTION_COMPLETE != do_aci_setup(&aci_state))
                        Serial.println(F("Error in ACI Setup"));
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
                Serial.println(F("Evt Cmd respone: Error."));
                //while (1);
            }
            if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode)
            {
                // Debug print
                Serial.println("Debug: printting configuration id, aci version, setup format, id, and status from the cmd rsp opcode:");
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_version.configuration_id);
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_version.aci_version);
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_version.setup_format);
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_version.setup_id);
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_version.setup_status);

                //Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
                lib_aci_set_local_data(&aci_state, PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET, 
                (uint8_t *)&(aci_evt->params.cmd_rsp.params.get_device_version), sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
            }
            if (ACI_CMD_GET_DEVICE_ADDRESS == aci_evt->params.cmd_rsp.cmd_opcode)
            {
                // Debug print
                Serial.println("Debug: printting device address, and address type:");
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_address.bd_addr_own[0]);
                Serial.println(aci_evt->params.cmd_rsp.params.get_device_address.bd_addr_type);
            }        
        break;
        
        case ACI_EVT_CONNECTED:
            Serial.println(F("Evt Connected"));
            aci_state.data_credit_available = aci_state.data_credit_total;
            // Get the device version of the nRF8001 and store it in the Hardware Revision String
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
        }
        
        break;
        
      case ACI_EVT_TIMING:
        Serial.println(F("Evt link connection interval changed"));
        break;
        
      case ACI_EVT_DISCONNECTED:
        Serial.println(F("Evt Disconnected/Advertising timed out"));
        lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
        Serial.println(F("Advertising started"));
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

#define SYS_STATUS_INIT 0x01
#define SYS_STATUS_SENSING 0x01
#define SYS_STATUS_CONNECTING_BLE 0x02
#define SYS_STATUS_TX 0x03
#define SYS_STATUS_RX 0x04

int system_status = SYS_STATUS_INIT;
int i=0;
void loop() {
    delay(200);

    if (i == 10) lib_aci_sleep();
    if (i == 25) lib_aci_wakeup();
    if (i == 30) lib_aci_device_version();
    if (i == 40) lib_aci_get_address();
    i++;
    aci_loop();

}

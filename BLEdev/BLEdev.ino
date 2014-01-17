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

void loop() {
}

#ifndef H_ACI_SETUP
#define H_ACI_SETUP

#define ACI_CMD_Q_SIZE 16

/**
Do the ACI setup by sending the setup messages generated by the nRFgo studio to the nRF8001.
After all the setup messages are sent. The nRF8001 will send a Device Started Event (Mode = STANDBY)
*/
aci_status_code_t do_aci_setup(aci_state_t *aci_stat);

#endif

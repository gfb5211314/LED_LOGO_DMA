#ifndef   __ESP8266_H__
#define   __ESP8266_H__

#include "stm32f1xx_hal.h"



extern uint8_t atk_8266_send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime);

extern uint8_t* esp8266_Capture_data(uint8_t *rec_data,uint8_t *head_buf);


#endif
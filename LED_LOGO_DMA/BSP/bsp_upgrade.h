#ifndef __BSP_UPGRADE_H__
#define __BSP_UPGRADE_H__


#include "stm32f1xx_hal.h"


typedef struct{
	uint8_t  version[2];  //version
	uint8_t  upgrade_state; //upgrade_state
	uint16_t  block;       //upgrade_block 
}upgrade_type;

/**************************************************
                  分区划分
model :  stm32f103c8t6       flash :   64K

bootloader :   5K          0x8000000-0x8001400     0-5K

APP1       :   25K         0x8001800-0x8008800    6k-34k

APP2       :   25K         0x8008c00-0X800FC00    35k-63K

***************************************************/

/***************************************************************
                  version_flash
***************************************************************/
 #define   version_flash_addr        0x800c800             //50K为首地址   version
 #define   upgrade_state_addr      0x800c804             //50K            flag

/**************************************************************
                 code_flash
**************************************************************/
 #define   code_start_addr           0x8001800            //6K       










#endif
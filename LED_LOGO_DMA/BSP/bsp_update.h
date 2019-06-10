#ifndef __BSP_UPDATE_H__
#define __BSP_UPDATE_H__

#include "stm32f1xx_hal.h"





typedef struct
{
    uint8_t defat;    //默认模式，运行系统ws2812数据包
    uint8_t set_shade;    //设置模式 1. 灯灭  2.发送数据  3.设置完成
    uint8_t oneline_shade; //在线调试每段渐变效果
    uint8_t oneline_all; //在线调试229个灯效果
	  uint8_t pattern_flay;
} system_mode_type;



extern void  Usart_Logo_data(uint8_t * p_buf,uint8_t * tep_buf,uint16_t r_buf_lenght);


extern void flash_data_to_color_data();


#endif


/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2019 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

/* USER CODE BEGIN Includes */     
#include "bsp_commun.h"
#include "bsp_ws2812.h"
#include "stm32f1xx_hal.h"
#include "math.h"
#include "bsp_dma.h"
#include "string.h"
#include <stdlib.h>
#include "crc.h"
#include "bsp_usart.h"
#include "cmsis_os.h"
#include  "ws2812_app.h"
#include "bsp_update.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/

/* USER CODE BEGIN Variables */
/***********全部亮一种颜色渐变*******************/
#define  one_jianbian_open     0
/*******************************/
/*********全部亮多端颜色渐变****************/
#define  more_jianbian_open     0
/*******************************/
/*******全部亮循环多种颜色变化*******************/
#define   circulation_more_color_open     0
/*******************************/
/*******单中颜色全亮*******************/
#define  one_color_open     0
/*******************************/
/*******多种颜色逐渐亮*******************/
#define  more_color_light_open     0
/*******************************/
/*******多种颜色逐渐亮*******************/
#define  more_color_light1_open     0
/*******************************/
/*******每一条灯逐渐移位*******************/
#define  each_light_open     0
/*******************************/
/*******每一个灯依次显示每一种颜色, 灯效一边走，另一边返回*******************/
#define  each_light_order_open     0
/*******************************/
/*******每一个灯依次显示每一种颜色,等效是2边走中间，中间走2边*******************/
#define  each_light_order_two_open     0
/*******************************/
/*******随机亮*******************/
#define  random_light_open     0
/*******************************/
#define DMA_WS2812_circulationopen 0
/*********************************/
#define DMA_WS2812_circulation_moreopen 0
/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/

/* USER CODE BEGIN FunctionPrototypes */
uint8_t  ws2812_write_flag;  //数据区是否被写入
uint8_t  select_area;  //选择片区的数据  1为就片区   2.为最新片区
extern USART_RECEIVETYPE  UsartType3;
extern system_mode_type system_mode;
/* USER CODE END FunctionPrototypes */

/* Hook prototypes */

/* USER CODE BEGIN Application */
    /* APP_TASK function */
void APP_TASK(void const * argument)
{

  /* USER CODE BEGIN 5 */
   DMA_USART_IDLE_INIT();
		#if  esp32_wifi_on
	 esp32_wifi_init(1,50);
	esp_check_state(50);
   	#endif
//	 for(uint16_t i=0;i<229;i++)
//	{
//		printf("00 00 ff");
//		printf(" ");
//	}
//nrf24l01_init(115200,2000,"AT+RXA=0xb0,0x43,0x10,0x10,0x01","AT+TXA=0xb0,0x43,0x10,0x10,0x01","AT+FREQ=2.440",50);
  /* Infinite loop */
  for(;;)
  {  
	 
Usart_Logo_data_(UsartType3.tem_RX_pData,UsartType3.RX_pData,UsartType3.RX_Size);
		
		osDelay(1);//只能间隔>50ms才能发送
  }
  /* USER CODE END 5 */ 
}
//SemaphoreHandle_t xSemaphore;
 
uint8_t  a_1[1][3]={241,216,128};
uint8_t  a_2[1][3]={249,5,86};
uint8_t  a_3[1][3]={29,49,142};
uint8_t  a_4[1][3]={109,249,253};	
uint8_t  a_5[1][3]={249,5,86};
uint8_t  a_6[1][3]={104,160,247};	
uint8_t  a_7[1][3]={241,216,128};
uint8_t  a_8[1][3]={52,204,204};	
/* WS128_TASK function */
void WS128_TASK(void const * argument)
{
  /* USER CODE BEGIN WS128_TASK */
	
			reset_led_light(); 
//	 xSemaphore = xSemaphoreCreateBinary();
  /* Infinite loop */
  for(;;)
  {

#if one_jianbian_open
  DMA_WS2812_Ramp(LED_MAX, 255, 1);
  DMA_WS2812_Ramp(LED_MAX, 255, 2);
  DMA_WS2812_Ramp(LED_MAX, 255, 3);
#endif
#if more_jianbian_open
 DMA_WS2812_Rampping(LED_MAX, 255, 2);
#endif
#if circulation_more_color_open
 DMA_WS2812_Rampping_1(LED_MAX, 255, 1);
#endif

#if one_color_open
      DMA_WS2812_one_light_run(2, 2);
#endif
          
#if more_color_light_open
      DMA_WS2812_Run(LED_MAX );
#endif

#if more_color_light1_open
     DMA_WS2812_Running(LED_MAX);
#endif
#if each_light_open 
    DMA_WS2812_Running_more(LED_MAX, 720);
#endif
#if each_light_order_open 
    arrange_display(LED_MAX);
#endif

#if each_light_order_two_open 
  	arrange_display_two(LED_MAX);
#endif
#if random_light_open 
  	ws2812_rand_light(LED_MAX);
#endif
#if  DMA_WS2812_circulationopen
    DMA_WS2812_circulation(229);
#endif
#if DMA_WS2812_circulation_moreopen
 DMA_WS2812_circulation_more(229,0);
 
  DMA_WS2812_circulation_more(LED_MAX,1);
	
	DMA_WS2812_circulation_more(LED_MAX,2);
	
#endif
//DMA_WS2812_shade_logo_rgb(LED_MAX,a_1,a_2);
//  osDelay(2000);
//DMA_WS2812_shade_logo_rgb(LED_MAX,a_3,a_4);
//  osDelay(2000);
//DMA_WS2812_shade_logo_rgb(LED_MAX,a_5,a_6);
//  osDelay(2000);
//DMA_WS2812_shade_logo_rgb(LED_MAX,a_7,a_8);
//  osDelay(2000);
//   if((ws2812_write_flag==1)||(select_area==1))
//	 {
//		 //从旧片区运行
//		 ws2812_rand_light(LED_MAX);
//   
//	 }else if((ws2812_write_flag==2)&&(select_area==2)) //数据被写入
//	 {
//		 //从新片区运行
//		 DMA_WS2812_shade_light(LED_MAX);
//		 
//	 }
//	   DMA_WS2812_shade_light(LED_MAX);
        if(system_mode.pattern_flay==1)
				{
	DMA_WS2812_Ramp(LED_MAX, 255, 1);
  DMA_WS2812_Ramp(LED_MAX, 255, 2);
  DMA_WS2812_Ramp(LED_MAX, 255, 3);
	DMA_WS2812_Rampping(LED_MAX, 255, 2);				   
	DMA_WS2812_Rampping_1(LED_MAX, 255, 1);
//	DMA_WS2812_one_light_run(2, 2);	
  DMA_WS2812_Run(LED_MAX );	
	DMA_WS2812_Running(LED_MAX);
  DMA_WS2812_Running_more(LED_MAX, 720);		
  arrange_display(LED_MAX);	
 	arrange_display_two(LED_MAX);
	ws2812_rand_light(LED_MAX);
  DMA_WS2812_circulation(LED_MAX);
	DMA_WS2812_circulation_more(LED_MAX,0);
  DMA_WS2812_circulation_more(LED_MAX,1);
	DMA_WS2812_circulation_more(LED_MAX,2);				
				}     
			
	 osDelay(1);//只能间隔>50ms才能发送
	 
  }
  /* USER CODE END WS128_TASK */
}

/* FFT_TASK function */ 
void FFT_TASK(void const * argument)
{
	
  /* USER CODE BEGIN FFT_TASK */

//   fft_adc_tim_start();
  /* Infinite loop */
  for(;;)
  {
//		  DMA_WS2812_shade_light(LED_MAX);
//	send_fft_data();
		 if(system_mode.pattern_flay==2)
				{
					
					  
					if(system_mode.set_shade==1)
					{
						DMA_WS2812_shade_light(LED_MAX);
					}
					
				}
			  else if(system_mode.pattern_flay==3)
				{
//					
					   DMA_WS2812_shade_light(LED_MAX);
					
				}
				else if(system_mode.pattern_flay==4)
				{
					
//					   reset_led_light(); 
					   DMA_WS2812_shade_light(LED_MAX);
					
				}
    osDelay(1);
  }
  /* USER CODE END FFT_TASK */
} 
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

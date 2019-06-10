#ifndef __WS2812_APP_H__
#define __WS2812_APP_H__
#include "stm32f1xx_hal.h"
#include   "bsp_ws2812.h"

#define LED_MAX                229   //项目最大灯数
extern void ws2812_rand_light(volatile uint16_t  amount);

/**
 * @brief  Each color to run
 * @param  uint16_t amount :  maximum LED number of ws28128
 * @param  uint16_t color_type_n :  Color number
 * @retval None
 */
/***********每个色循环往下跑**********未完成****/
extern void  arrange_display_one_run(volatile uint16_t  amount, uint16_t color_type_n);

/**
 * @brief  Each color to run
 * @param  uint16_t amount :  maximum LED number of ws28128
 * @param  uint16_t color_type_n :  Color number
 * @retval None
 */
/***********每个色转一遍**************/
extern void  arrange_display_two_run(volatile uint16_t  amount);

/***多种颜色一起渐变***************/
extern void DMA_WS2812_Rampping(volatile uint16_t  amount, uint8_t pwm, colors_kind color);

/***********色表**************/
extern void  arrange_display(volatile uint16_t  amount);

/***********色表2合一**************/
extern void  arrange_display_two(volatile uint16_t  amount);

/*********单中颜色渐变***************/
extern void DMA_WS2812_Rampping_1(volatile uint16_t  amount, uint8_t pwm, colors_kind color);


extern void DMA_WS2812_Running(volatile uint16_t  amount);

extern void DMA_WS2812_Run(volatile uint16_t  amount);

extern void DMA_WS2812_Ramp(volatile uint16_t  amount, uint8_t pwm, colors_kind color);

/***********多条灯跑起来****************/
extern void DMA_WS2812_Running_more(volatile uint16_t  amount, volatile uint16_t run_number);

extern void DMA_WS2812_shade_logo(uint8_t group_num,uint8_t (*start_buf)[3],uint8_t (*terminus_buf)[3]);
extern void DMA_WS2812_shade_light(uint16_t amount);
extern void DMA_WS2812_data_refresh(uint8_t (*sta)[3],uint8_t *pbuf,uint16_t lendata);
extern void DMA_WS2812_shade_logo_one(uint8_t group_num,uint8_t (*def_buf)[6]);
extern void DMA_WS2812_shade_60_run(uint16_t amount,uint8_t *def_buf);

extern void DMA_WS2812_data_refresh(uint8_t (*sta)[3],uint8_t *pbuf,uint16_t lendata);

/*****************************************************************************

******************************************************************************/

extern void DMA_WS2812_data_flash(uint8_t (*sta)[6],uint8_t *pbuf,uint16_t lendata);
extern void DMA_WS2812_data_flash_1(uint8_t *pbuf,uint8_t (*sta)[6],uint16_t lendata);

extern void  DMA_WS2812_shade_logo_all(uint8_t (*def_buf)[6]);
#endif


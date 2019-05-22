#include "bsp_update.h"
#include "stm_flash.h"
#include "crc.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "bsp_ws2812.h"
#include "string.h"
#include <stdlib.h>
#include "ws2812_app.h"
#include "bsp_usart.h"
/**************************************************
                  分区划分
model :  stm32f103c8t6       flash :   64K

bootloader :   5K          0x8000000-0x8001400     0-5K

APP1       :   25K         0x8001800-0x8008800    6k-34k

APP2       :   25K         0x8008c00-0X800FC00    35k-63K

***************************************************/

/***************************************************
                  参数地址分配  (0X800FC00-0x8010000 64K)
uint8_t APP1.jump_flag                          0X800FC00             2
uint8_t APP1.upgrade_flag                       0X800FC06
uint8_t APP1.upgrade_version                    0X800FC10             2
uint8_t APP1.Total_blocks                       0X800FC20             2
uint8_t APP1.local_version                      0X800FC30             2
uint16_t APP1.NOW_blocks                        0X800FC40
uint16_t APP1.flash_blocks                      0X800FC60

*****************************************************/
#define             App2_Start_Addr            0x8008c00

/***************************************************************************
                             Function
***************************************************************************/
#define             set_mode_command_function         0x00   //设置mode命令   SET command
#define             set_command_function              0x01       //设置命令   SET command
#define             send_data_command_function        0x02       //发送数据命令
#define             set_dir_command_function          0x03       //设置方向命令
#define             set_finsih_command_function       0x04       //设置完成命令
#define             set_data_now_command_function     0x05       //在线发送数据
#define             set_dir_now_command_function      0x06       //在线设置方向数据
#define             set_data_all_now_command_function  0x07       //在线设置方向数据
/***************************************************************************
                           Lighting effects
***************************************************************************/

#define           effects_one      0x01                      //逐渐亮起来
#define           effects_two      0x02                      //60度角平移
#define           effects_three    0x03                      //多种颜色渐变
#define           effects_four     0x04                      //全亮变色
#define           effects_five     0x05                      //随机亮
#define           effects_six      0x06                      //
#define           effects_seven    0x07
#define           effects_eight    0x08
#define           effects_nine     0x09
#define           effects_ten      0x10

/****************************************************************************
                            decare variables
*****************************************************************************/
extern UART_HandleTypeDef huart3;
extern osThreadId WS12TASKHandle;
extern uint8_t   ws28128_color_buf[1000][3];
extern USART_RECEIVETYPE  UsartType3;
typedef struct
{
    uint8_t  index;
} Ws2818_Led_Type;
Ws2818_Led_Type   Ws2818_Led;
system_mode_type system_mode;

/**************************************************************************
                    协议切换
**************************************************************************/
#define  esp32_wifi_on         0
#define  e61_433_on         1
/*******************************************************
      第一步   编写接收数据函数处理函数
********************************************************/


void  Usart_Logo_data_(uint8_t * p_buf,uint8_t * tep_buf,uint16_t r_buf_lenght)
{

    uint16_t  crc_data;
    uint8_t  sta_logo_data[1][3];
    uint8_t  ter_logo_data[1][3];

    if(UsartType3.RX_flag==1)
    {
#if  esp32_wifi_on

        r_buf_lenght = atoi((const char *)tep_buf + 9);
        printf("r_buf_lenght=%d\r\n",r_buf_lenght);
        for(uint8_t i=0; i<r_buf_lenght; i++)
        {
            p_buf[i]=tep_buf[i+12];

        }
#endif
#if  e61_433_on


//			printf("r_buf_lenght=%d\r\n",r_buf_lenght);
        for(uint16_t i=0; i<r_buf_lenght; i++)
        {
            p_buf[i]=tep_buf[i];
//			 	printf("%x",p_buf[i]);
        }
#endif




        if((p_buf[0]==0xff)&&(p_buf[1]==0xfe)&&(p_buf[r_buf_lenght-3]==0xff)&&(p_buf[r_buf_lenght-4]==0xfe)&&(p_buf[2]==0x01))
        {

            crc_data=Calc_CRC16(p_buf,r_buf_lenght-2);
            printf("creamCRC1:%04x\r\n",crc_data);
            printf("creamCRC2:%04x\r\n",p_buf[r_buf_lenght-1]+(p_buf[r_buf_lenght-2]*256));
            /**************************CRC校验**************************************/
            if(crc_data==(p_buf[r_buf_lenght-1]+(p_buf[r_buf_lenght-2]*256)))
            {
//			                for(uint8_t i=0;i<r_buf_lenght;i++)
//			{
//				printf("i=%d=%d\r\n",i,p_buf[i]);
//			}
                switch(p_buf[3]) //功能码
                {


                case set_mode_command_function :
                /********模式一*********/
                    if(p_buf[9]==0x01)
                    {
										
									 vTaskResume(WS12TASKHandle);	//恢复任务1
                      system_mode.pattern_flay=1;   
                    			reset_led_light();									
                       printf("moshi1");
                    }
										/********模式二*************/
                    else if(p_buf[9]==0x02)
                    {
											 
											 vTaskSuspend(WS12TASKHandle);//挂起任务
                      system_mode.pattern_flay=2;    
											printf("moshi2");
											 reset_led_light(); 
                    }
										/**********模式三***********/
                    else if(p_buf[9]==0x03)
                    {
											 
											 vTaskSuspend(WS12TASKHandle);//挂起任务
											system_mode.pattern_flay=3;
											printf("moshi3");
											 reset_led_light(); 
                    }
										/*********模式四**************/
                    else if (p_buf[9]==0x04)
                    {
											 
											 vTaskSuspend(WS12TASKHandle);//挂起任务
                       system_mode.pattern_flay=4;
											printf("moshi4");
											 reset_led_light(); 
                    }
                    break;
                /**设置模块***/
                case set_command_function :
                  if(system_mode.pattern_flay==2)
									{
                    /*第一步：停掉所有灯*/
						 HAL_UART_Transmit_DMA(&huart3, p_buf,r_buf_lenght);
system_mode.set_shade=0;
                   
                    DMA_WS2812_Mie(LED_MAX);
									}
////                memset(ws28128_color_buf, 0, sizeof(ws28128_color_buf));
//						   Ws2818_Led.index=0;//Clear led munber 0
                    /*第二步：发送ACK*/
//                    HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                    break;
                /**数据处理模块**/
                case send_data_command_function :
               if(system_mode.pattern_flay==2)
									{
										system_mode.set_shade=0;
                    sta_logo_data[0][0]=p_buf[9];
                    sta_logo_data[0][1]=p_buf[10];
                    sta_logo_data[0][2]=p_buf[11];
                    ter_logo_data[0][0]=p_buf[12];
                    ter_logo_data[0][1]=p_buf[13];
                    ter_logo_data[0][2]=p_buf[14];
                    DMA_WS2812_shade_logo(p_buf[7],sta_logo_data,ter_logo_data);
                    HAL_UART_Transmit_DMA(&huart3, p_buf,r_buf_lenght);
									}
                    break;
                /**设置灯运动方向**/
                case set_dir_command_function :
										 if(system_mode.pattern_flay==2)
									{
                    HAL_UART_Transmit_DMA(&huart3, p_buf,18);
									}
                    break;
                /**设置完成****/
                case set_finsih_command_function :
                     if(system_mode.pattern_flay==2)
									{
										system_mode.set_shade=1;
                    HAL_UART_Transmit_DMA(&huart3, p_buf,r_buf_lenght);
                  
									}
                    break;
							  case set_data_now_command_function :
										    if(system_mode.pattern_flay==3)
									{
										  
										sta_logo_data[0][0]=p_buf[9];
                    sta_logo_data[0][1]=p_buf[10];
                    sta_logo_data[0][2]=p_buf[11];
                    ter_logo_data[0][0]=p_buf[12];
                    ter_logo_data[0][1]=p_buf[13];
                    ter_logo_data[0][2]=p_buf[14];
                    DMA_WS2812_shade_logo(p_buf[7],sta_logo_data,ter_logo_data);
                    HAL_UART_Transmit_DMA(&huart3, p_buf,r_buf_lenght);
									}
									  break;
                case set_data_all_now_command_function :
									    if(system_mode.pattern_flay==4)
									{
										 
                    DMA_WS2812_data_refresh(ws28128_color_buf,p_buf+9,r_buf_lenght-13);
									}
                    break;
                }
            }
        }
        memset(UsartType3.RX_pData, 0, sizeof(UsartType3.RX_pData));
        UsartType3.RX_flag=0;
        r_buf_lenght=0;
    }
}
/************************************************************
                 验证数据是否写入SUCCEED
************************************************************/
Write_State  write_flash(uint16_t now_flash_blocks,uint8_t * p_buf,uint16_t r_buf_lenght)
{

    uint8_t  t_p_buf[200];
    STMFLASH_Write (0x8008c00+128*now_flash_blocks, (uint16_t *)p_buf,r_buf_lenght); //版本升级总块数

    STMFLASH_Read  (0x8008c00+128*now_flash_blocks, (uint16_t *)t_p_buf,r_buf_lenght); //版本升级总块数
    if(strcmp((const char *) p_buf,(const char *)t_p_buf)==0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
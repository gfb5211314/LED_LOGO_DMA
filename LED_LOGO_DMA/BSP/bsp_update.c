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
#include "esp8266.h"
#include "bsp_upgrade.h"
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
//#define             App2_Start_Addr            0x8008c00
/*************************************************************
                    ws28128 参数地址分配
0x800f000-0x800f400	 60K-61k     轨迹1        渐变102    5个渐变颜色 102*5=510 <512个字节  只能以半字写进去，所以就剩下最大只能写512
0x800f400-0x800f800  61K-62k     轨迹2        渐变102    5个渐变颜色    102*5=510 <512个字节
0x800f800-0x800fc00  62k-63k     轨迹3        渐变102    5个渐变颜色    102*5=510 <512个字节
0x800ec00-0x800f000  59K-60K     轨迹4          
0x800e800-0x800ec00  58K-59K     轨迹5
0x800e400       mode1   1  2  3 4      57K   0x12  512
*******************************************************************/
/********************************
              轨迹1
0x800f000  track_1_color_1     204
0x800f0cc  track_1_color_2     408
0x800f198  track_1_color_3     612
0x800f264  track_1_color_4     816
0x800f330  track_1_color_5     1020
0x800f3fe  write_flash_1        1

*********************************/
/*******************************
              轨迹2
		//61K-62k

0x800f400  track_2_color_1     204
0x800f4cc  track_2_color_2     408
0x800f598  track_2_color_3     612
0x800f664  track_2_color_4     816
0x800f730  track_2_color_5     1020
0x800f7fe  write_flash_2        1
*********************************/
/*******************************
              轨迹3
		//62K-63k

0x800f800  track_3_color_1     204
0x800f8cc  track_3_color_2     408
0x800f998  track_3_color_3     612
0x800fa64  track_3_color_4     816
0x800fb30  track_3_color_5     1020
0x800fbfe  write_flash_3        1
*********************************/
/*******************************
              轨迹4
		//59K-60K 0x800ec00-0x800f000

0x800ec00  track_4_color_1     204
0x800eccc  track_4_color_2     408
0x800ed98  track_4_color_3     612
0x800ee64  track_4_color_4     816
0x800ef30  track_4_color_5     1020
0x800effe  write_flash_4        1
*********************************/
/********************************
              轨迹5
		//58K-59k  0x800e800-0x800ec00

0x800e800  track_5_color_1     102
0x800e8cc  track_5_color_2     102
0x800e998  track_5_color_3     102
0x800ea64  track_5_color_4     102
0x800eb30  track_5_color_5     102
0x800ebfe  write_flash_5        1
*********************************/
#define mode_addr   0x800e400
#define track_1_color_1_addr   0x800f000
#define track_1_color_2_addr   0x800f0cc
#define track_1_color_3_addr   0x800f198
#define track_1_color_4_addr   0x800f264
#define track_1_color_5_addr   0x800f330
#define track_1_color_all_addr 0x800f3fe       

#define track_2_color_1_addr   0x800f400
#define track_2_color_2_addr   0x800f4cc
#define track_2_color_3_addr   0x800f598
#define track_2_color_4_addr   0x800f664
#define track_2_color_5_addr   0x800f730
#define track_2_color_all_addr 0x800f7fe 

#define track_3_color_1_addr   0x800f800
#define track_3_color_2_addr   0x800f8cc
#define track_3_color_3_addr   0x800f998
#define track_3_color_4_addr   0x800fa64
#define track_3_color_5_addr   0x800fb30
#define track_3_color_all_addr 0x800fbfe 

#define track_4_color_1_addr   0x800ec00  
#define track_4_color_2_addr   0x800ed98 
#define track_4_color_3_addr   0x800ee64 
#define track_4_color_4_addr   0x800ee64 
#define track_4_color_5_addr   0x800ef30 
#define track_4_color_all_addr 0x800effe 

#define track_5_color_1_addr   0x800e800 
#define track_5_color_2_addr   0x800e8cc 
#define track_5_color_3_addr   0x800e998 
#define track_5_color_4_addr   0x800ea64 
#define track_5_color_5_addr   0x800eb30 
#define track_5_color_all_addr 0x800ebfe

/*****************************************************/
uint8_t  track_1_color_1[17][6];
uint8_t  track_1_color_2[17][6];
uint8_t  track_1_color_3[17][6];
uint8_t  track_1_color_4[17][6];
uint8_t  track_1_color_5[17][6];

uint8_t  track_2_color_1[17][6];
uint8_t  track_2_color_2[17][6];
uint8_t  track_2_color_3[17][6];
uint8_t  track_2_color_4[17][6];
uint8_t  track_2_color_5[17][6];

uint8_t  track_3_color_1[17][6];
uint8_t  track_3_color_2[17][6];
uint8_t  track_3_color_3[17][6];
uint8_t  track_3_color_4[17][6];
uint8_t  track_3_color_5[17][6];

uint8_t  track_4_color_1[17][6];
uint8_t  track_4_color_2[17][6];
uint8_t  track_4_color_3[17][6];
uint8_t  track_4_color_4[17][6];
uint8_t  track_4_color_5[17][6];

uint8_t  track_5_color_1[17][6];
uint8_t  track_5_color_2[17][6];
uint8_t  track_5_color_3[17][6];
uint8_t  track_5_color_4[17][6];
uint8_t  track_5_color_5[17][6];
uint8_t  write_flash_1;
uint8_t  write_flash_2;
uint8_t  write_flash_3;
uint8_t  write_flash_4;
uint8_t  write_flash_5;
uint8_t  temp_color_data[102];
/***************************************************************************
                             Function
***************************************************************************/
#define             set_mode_command_function         0x00       //设置mode命令   SET mode command
#define             set_command_function              0x01       //设置命令   SET command
#define             send_data_command_function        0x02       //发送数据命令
#define             set_dir_command_function          0x03       //设置方向命令
#define             set_finsih_command_function       0x04       //设置完成命令
#define             set_data_now_command_function     0x05       //在线发送数据
#define             set_dir_now_command_function      0x06       //在线设置方向数据
#define             set_data_all_now_command_function 0x07       //在线设置方向数据
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
void read_mode_start()
{
STMFLASH_Read(mode_addr, (uint16_t *)&system_mode.pattern_flay,1); //开机模式
}

/**************************************************************************
                    协议切换
**************************************************************************/
#define  esp32_wifi_on         0
#define  e61_433_on            0
#define  esp8266_wifi_on       1
/*******************************************************
      第一步   编写接收数据函数处理函数
********************************************************/


void  Usart_Logo_data(uint8_t * p_buf,uint8_t * tep_buf,uint16_t r_buf_lenght)
{

    uint16_t  crc_data;
    uint8_t  sta_logo_data[1][3];
    uint8_t  ter_logo_data[1][3];
   uint8_t *  p1;
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

      
			printf("r_buf_lenght=%d\r\n",r_buf_lenght);
        for(uint16_t i=0; i<r_buf_lenght; i++)
        {
            p_buf[i]=tep_buf[i];
			 	printf("%c",p_buf[i]);
        }
#endif
#if  esp8266_wifi_on
    //the fast plan 

  
	p_buf=(uint8_t*)esp8266_Capture_data(tep_buf,(uint8_t *)":");
	 
			  r_buf_lenght=strlen((char *)p_buf);
				printf("r_buf_lenght=%d\r\n",	r_buf_lenght);
             printf("%s",	p_buf);
    //the  two plan
    /* r_buf_lenght = atoi((const char *)tep_buf + 9);
        printf("r_buf_lenght=%d\r\n",r_buf_lenght);
        for(uint8_t i=0; i<r_buf_lenght; i++)
        {
            p_buf[i]=tep_buf[i+12];

        }*/		

#endif
    /**********upgrade code***************/
   if((p_buf[0]==0x7b)&&(p_buf[1]==0xb7)&&(p_buf[r_buf_lenght-3]==0x7b)&&(p_buf[r_buf_lenght-4]==0xb7)&&(p_buf[2]==0x01))
			 {
	                   crc_data=Calc_CRC16(p_buf,r_buf_lenght-2);
            printf("upgradeCRC1:%04x\r\n",crc_data);
            printf("upgradeCRC2:%04x\r\n",p_buf[r_buf_lenght-1]+(p_buf[r_buf_lenght-2]*256));
				  if(crc_data==(p_buf[r_buf_lenght-1]+(p_buf[r_buf_lenght-2]*256)))
            {
                switch(p_buf[3]) //功能码
                {   
									case 0x01 : 
										   
									    	HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                             
									break;
									
								}
							}
			 }
			 /***********设置esp8266************/
       else if((p_buf[0]=='A')&&(p_buf[1]=='T'))
			 {
				    p_buf[r_buf_lenght]=0x0d;
				    p_buf[r_buf_lenght+1]=0x0a;
              if(atk_8266_send_cmd(p_buf,(uint8_t *)"OK",100)!=0)
							{
								printf("设置成功\r\n");
								
							}
							else
							{
								
								printf("设置失败\r\n");
								
							}
				}
///      ESP8266_unpack_string(UsartType3.RX_pData,UsartType3.RX_Size);
/************************设置COLOR************************/
        else if((p_buf[0]==0xff)&&(p_buf[1]==0xfe)&&(p_buf[r_buf_lenght-3]==0xff)&&(p_buf[r_buf_lenght-4]==0xfe)&&(p_buf[2]==0x01))
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
                 
											HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        vTaskResume(WS12TASKHandle);	//恢复任务1
                        system_mode.pattern_flay=1;
                        if(write_read_flash_jiaoyan(mode_addr,&system_mode.pattern_flay,1))
                        {
                            printf("write mode1 sucess");
                        }
                        else
                        {
                            printf("write mode1 fail");
                        }
                        reset_led_light();
//                       printf("moshi1");
                    }
                    /********模式二*************/
                    else if(p_buf[9]==0x02)
                    {
											   system_mode.set_shade=0;
                       HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        vTaskSuspend(WS12TASKHandle);//挂起任务
                        system_mode.pattern_flay=2;
                        if(write_read_flash_jiaoyan(mode_addr,&system_mode.pattern_flay,1))
                        {
                            printf("write mode2 sucess");
                        }
                        else
                        {
                            printf("write mode2 fail");
                        }
                        reset_led_light();
                    }
                    /**********模式三***********/
                    else if(p_buf[9]==0x03)
                    {
                      HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        vTaskSuspend(WS12TASKHandle);//挂起任务
                        system_mode.pattern_flay=3;
                        if(write_read_flash_jiaoyan(mode_addr,&system_mode.pattern_flay,1))
                        {
                            printf("write mode3 sucess");
                        }
                        else
                        {
                            printf("write mode3 fail");
                        }
                        reset_led_light();
                    }
                    /*********模式四**************/
                    else if (p_buf[9]==0x04)
                    {
                 HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        vTaskSuspend(WS12TASKHandle);//挂起任务
                        system_mode.pattern_flay=4;
                        if(write_read_flash_jiaoyan(mode_addr,&system_mode.pattern_flay,1))
                        {
                            printf("write mode4 sucess");
                        }
                        else
                        {
                            printf("write mode4 fail");
                        }
                      //  reset_led_light();
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
                    break;
                /**数据处理模块**/
                case send_data_command_function :
                    if(system_mode.pattern_flay==2)
                    {

                        HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        //clear track1 buf data
                       /* memset(track_1_color_1, 0, sizeof(track_1_color_1));
                        memset(track_1_color_2, 0, sizeof(track_1_color_2));
                        memset(track_1_color_3, 0, sizeof(track_1_color_3));
                        memset(track_1_color_4, 0, sizeof(track_1_color_4));
                        memset(track_1_color_5, 0, sizeof(track_1_color_5));
                        //clear track2 buf data
                        memset(track_2_color_1, 0, sizeof(track_2_color_1));
                        memset(track_2_color_2, 0, sizeof(track_2_color_2));
                        memset(track_2_color_3, 0, sizeof(track_2_color_3));
                        memset(track_2_color_4, 0, sizeof(track_2_color_4));
                        memset(track_2_color_5, 0, sizeof(track_2_color_5));
                        //clear track3 buf data
                        memset(track_3_color_1, 0, sizeof(track_3_color_1));
                        memset(track_3_color_2, 0, sizeof(track_3_color_2));
                        memset(track_3_color_3, 0, sizeof(track_3_color_3));
                        memset(track_3_color_4, 0, sizeof(track_3_color_4));
                        memset(track_3_color_5, 0, sizeof(track_3_color_5));
                        //clear track4 buf data
                        memset(track_4_color_1, 0, sizeof(track_4_color_1));
                        memset(track_4_color_2, 0, sizeof(track_4_color_2));
                        memset(track_4_color_3, 0, sizeof(track_4_color_3));
                        memset(track_4_color_4, 0, sizeof(track_4_color_4));
                        memset(track_4_color_5, 0, sizeof(track_4_color_5));
                        //clear track5 buf data
                        memset(track_5_color_1, 0, sizeof(track_5_color_1));
                        memset(track_5_color_2, 0, sizeof(track_5_color_2));
                        memset(track_5_color_3, 0, sizeof(track_5_color_3));
                        memset(track_5_color_4, 0, sizeof(track_5_color_4));
                        memset(track_5_color_5, 0, sizeof(track_5_color_5));*/
                        p_buf[7]=p_buf[7]-1;
                        /******轨迹1**************/
                        if(p_buf[4]==effects_one)
                        {

                            /**********1区是否被写入**************/
                            if(write_flash_1==1)
                            {
                                track_1_color_2[p_buf[7]][0]=p_buf[9];
                                track_1_color_2[p_buf[7]][1]=p_buf[10];
                                track_1_color_2[p_buf[7]][2]=p_buf[11];
                                track_1_color_2[p_buf[7]][3]=p_buf[12];
                                track_1_color_2[p_buf[7]][4]=p_buf[13];
                                track_1_color_2[p_buf[7]][5]=p_buf[14];
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_1==3)
                            {
                                track_1_color_3[p_buf[7]][0]=p_buf[9];
                                track_1_color_3[p_buf[7]][1]=p_buf[10];
                                track_1_color_3[p_buf[7]][2]=p_buf[11];
                                track_1_color_3[p_buf[7]][3]=p_buf[12];
                                track_1_color_3[p_buf[7]][4]=p_buf[13];
                                track_1_color_3[p_buf[7]][5]=p_buf[14];
                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_1==7)
                            {
                                track_1_color_4[p_buf[7]][0]=p_buf[9];
                                track_1_color_4[p_buf[7]][1]=p_buf[10];
                                track_1_color_4[p_buf[7]][2]=p_buf[11];
                                track_1_color_4[p_buf[7]][3]=p_buf[12];
                                track_1_color_4[p_buf[7]][4]=p_buf[13];
                                track_1_color_4[p_buf[7]][5]=p_buf[14];
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_1==15)
                            {
                                track_1_color_5[p_buf[7]][0]=p_buf[9];
                                track_1_color_5[p_buf[7]][1]=p_buf[10];
                                track_1_color_5[p_buf[7]][2]=p_buf[11];
                                track_1_color_5[p_buf[7]][3]=p_buf[12];
                                track_1_color_5[p_buf[7]][4]=p_buf[13];
                                track_1_color_5[p_buf[7]][5]=p_buf[14];
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_1==31)
                            {
															
                                track_1_color_1[p_buf[7]][0]=p_buf[9];
                                track_1_color_1[p_buf[7]][1]=p_buf[10];
                                track_1_color_1[p_buf[7]][2]=p_buf[11];
                                track_1_color_1[p_buf[7]][3]=p_buf[12];
                                track_1_color_1[p_buf[7]][4]=p_buf[13];
                                track_1_color_1[p_buf[7]][5]=p_buf[14];
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                track_1_color_1[p_buf[7]][0]=p_buf[9];
                                track_1_color_1[p_buf[7]][1]=p_buf[10];
                                track_1_color_1[p_buf[7]][2]=p_buf[11];
                                track_1_color_1[p_buf[7]][3]=p_buf[12];
                                track_1_color_1[p_buf[7]][4]=p_buf[13];
                                track_1_color_1[p_buf[7]][5]=p_buf[14];
                            }
                        }
                        /******轨迹2**************/
                        else if(p_buf[4]==effects_two)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_2==1)
                            {
                                track_2_color_2[p_buf[7]][0]=p_buf[9];
                                track_2_color_2[p_buf[7]][1]=p_buf[10];
                                track_2_color_2[p_buf[7]][2]=p_buf[11];
                                track_2_color_2[p_buf[7]][3]=p_buf[12];
                                track_2_color_2[p_buf[7]][4]=p_buf[13];
                                track_2_color_2[p_buf[7]][5]=p_buf[14];
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_2==3)
                            {
                                track_2_color_3[p_buf[7]][0]=p_buf[9];
                                track_2_color_3[p_buf[7]][1]=p_buf[10];
                                track_2_color_3[p_buf[7]][2]=p_buf[11];
                                track_2_color_3[p_buf[7]][3]=p_buf[12];
                                track_2_color_3[p_buf[7]][4]=p_buf[13];
                                track_2_color_3[p_buf[7]][5]=p_buf[14];
                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_2==7)
                            {
                                track_2_color_4[p_buf[7]][0]=p_buf[9];
                                track_2_color_4[p_buf[7]][1]=p_buf[10];
                                track_2_color_4[p_buf[7]][2]=p_buf[11];
                                track_2_color_4[p_buf[7]][3]=p_buf[12];
                                track_2_color_4[p_buf[7]][4]=p_buf[13];
                                track_2_color_4[p_buf[7]][5]=p_buf[14];
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_2==15)
                            {
                                track_2_color_5[p_buf[7]][0]=p_buf[9];
                                track_2_color_5[p_buf[7]][1]=p_buf[10];
                                track_2_color_5[p_buf[7]][2]=p_buf[11];
                                track_2_color_5[p_buf[7]][3]=p_buf[12];
                                track_2_color_5[p_buf[7]][4]=p_buf[13];
                                track_2_color_5[p_buf[7]][5]=p_buf[14];
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_2==31)
                            {
                                track_2_color_1[p_buf[7]][0]=p_buf[9];
                                track_2_color_1[p_buf[7]][1]=p_buf[10];
                                track_2_color_1[p_buf[7]][2]=p_buf[11];
                                track_2_color_1[p_buf[7]][3]=p_buf[12];
                                track_2_color_1[p_buf[7]][4]=p_buf[13];
                                track_2_color_1[p_buf[7]][5]=p_buf[14];
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                track_2_color_1[p_buf[7]][0]=p_buf[9];
                                track_2_color_1[p_buf[7]][1]=p_buf[10];
                                track_2_color_1[p_buf[7]][2]=p_buf[11];
                                track_2_color_1[p_buf[7]][3]=p_buf[12];
                                track_2_color_1[p_buf[7]][4]=p_buf[13];
                                track_2_color_1[p_buf[7]][5]=p_buf[14];
                            }


                        }
                        /******轨迹3**************/
                        else if(p_buf[4]==effects_three)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_3==1)
                            {
                                track_3_color_2[p_buf[7]][0]=p_buf[9];
                                track_3_color_2[p_buf[7]][1]=p_buf[10];
                                track_3_color_2[p_buf[7]][2]=p_buf[11];
                                track_3_color_2[p_buf[7]][3]=p_buf[12];
                                track_3_color_2[p_buf[7]][4]=p_buf[13];
                                track_3_color_2[p_buf[7]][5]=p_buf[14];
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_3==3)
                            {
                                track_3_color_3[p_buf[7]][0]=p_buf[9];
                                track_3_color_3[p_buf[7]][1]=p_buf[10];
                                track_3_color_3[p_buf[7]][2]=p_buf[11];
                                track_3_color_3[p_buf[7]][3]=p_buf[12];
                                track_3_color_3[p_buf[7]][4]=p_buf[13];
                                track_3_color_3[p_buf[7]][5]=p_buf[14];
                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_3==7)
                            {
                                track_3_color_4[p_buf[7]][0]=p_buf[9];
                                track_3_color_4[p_buf[7]][1]=p_buf[10];
                                track_3_color_4[p_buf[7]][2]=p_buf[11];
                                track_3_color_4[p_buf[7]][3]=p_buf[12];
                                track_3_color_4[p_buf[7]][4]=p_buf[13];
                                track_3_color_4[p_buf[7]][5]=p_buf[14];
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_3==15)
                            {
                                track_3_color_5[p_buf[7]][0]=p_buf[9];
                                track_3_color_5[p_buf[7]][1]=p_buf[10];
                                track_3_color_5[p_buf[7]][2]=p_buf[11];
                                track_3_color_5[p_buf[7]][3]=p_buf[12];
                                track_3_color_5[p_buf[7]][4]=p_buf[13];
                                track_3_color_5[p_buf[7]][5]=p_buf[14];
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_3==31)
                            {
                                track_3_color_1[p_buf[7]][0]=p_buf[9];
                                track_3_color_1[p_buf[7]][1]=p_buf[10];
                                track_3_color_1[p_buf[7]][2]=p_buf[11];
                                track_3_color_1[p_buf[7]][3]=p_buf[12];
                                track_3_color_1[p_buf[7]][4]=p_buf[13];
                                track_3_color_1[p_buf[7]][5]=p_buf[14];
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                track_3_color_1[p_buf[7]][0]=p_buf[9];
                                track_3_color_1[p_buf[7]][1]=p_buf[10];
                                track_3_color_1[p_buf[7]][2]=p_buf[11];
                                track_3_color_1[p_buf[7]][3]=p_buf[12];
                                track_3_color_1[p_buf[7]][4]=p_buf[13];
                                track_3_color_1[p_buf[7]][5]=p_buf[14];
                            }

                        }
                        /******轨迹4**************/
                        else if(p_buf[4]==effects_four)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_4==1)
                            {
                                track_4_color_2[p_buf[7]][0]=p_buf[9];
                                track_4_color_2[p_buf[7]][1]=p_buf[10];
                                track_4_color_2[p_buf[7]][2]=p_buf[11];
                                track_4_color_2[p_buf[7]][3]=p_buf[12];
                                track_4_color_2[p_buf[7]][4]=p_buf[13];
                                track_4_color_2[p_buf[7]][5]=p_buf[14];
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_4==3)
                            {
                                track_4_color_3[p_buf[7]][0]=p_buf[9];
                                track_4_color_3[p_buf[7]][1]=p_buf[10];
                                track_4_color_3[p_buf[7]][2]=p_buf[11];
                                track_4_color_3[p_buf[7]][3]=p_buf[12];
                                track_4_color_3[p_buf[7]][4]=p_buf[13];
                                track_4_color_3[p_buf[7]][5]=p_buf[14];
                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_4==7)
                            {
                                track_4_color_4[p_buf[7]][0]=p_buf[9];
                                track_4_color_4[p_buf[7]][1]=p_buf[10];
                                track_4_color_4[p_buf[7]][2]=p_buf[11];
                                track_4_color_4[p_buf[7]][3]=p_buf[12];
                                track_4_color_4[p_buf[7]][4]=p_buf[13];
                                track_4_color_4[p_buf[7]][5]=p_buf[14];
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_4==15)
                            {
                                track_4_color_5[p_buf[7]][0]=p_buf[9];
                                track_4_color_5[p_buf[7]][1]=p_buf[10];
                                track_4_color_5[p_buf[7]][2]=p_buf[11];
                                track_4_color_5[p_buf[7]][3]=p_buf[12];
                                track_4_color_5[p_buf[7]][4]=p_buf[13];
                                track_4_color_5[p_buf[7]][5]=p_buf[14];
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_4==31)
                            {
                                track_4_color_1[p_buf[7]][0]=p_buf[9];
                                track_4_color_1[p_buf[7]][1]=p_buf[10];
                                track_4_color_1[p_buf[7]][2]=p_buf[11];
                                track_4_color_1[p_buf[7]][3]=p_buf[12];
                                track_4_color_1[p_buf[7]][4]=p_buf[13];
                                track_4_color_1[p_buf[7]][5]=p_buf[14];
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                track_4_color_1[p_buf[7]][0]=p_buf[9];
                                track_4_color_1[p_buf[7]][1]=p_buf[10];
                                track_4_color_1[p_buf[7]][2]=p_buf[11];
                                track_4_color_1[p_buf[7]][3]=p_buf[12];
                                track_4_color_1[p_buf[7]][4]=p_buf[13];
                                track_4_color_1[p_buf[7]][5]=p_buf[14];
                            }
                        }
                        /******轨迹5**************/
                        else if(p_buf[4]==effects_five)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_5==1)
                            {
                                track_5_color_2[p_buf[7]][0]=p_buf[9];
                                track_5_color_2[p_buf[7]][1]=p_buf[10];
                                track_5_color_2[p_buf[7]][2]=p_buf[11];
                                track_5_color_2[p_buf[7]][3]=p_buf[12];
                                track_5_color_2[p_buf[7]][4]=p_buf[13];
                                track_5_color_2[p_buf[7]][5]=p_buf[14];
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_5==3)
                            {
                                track_5_color_3[p_buf[7]][0]=p_buf[9];
                                track_5_color_3[p_buf[7]][1]=p_buf[10];
                                track_5_color_3[p_buf[7]][2]=p_buf[11];
                                track_5_color_3[p_buf[7]][3]=p_buf[12];
                                track_5_color_3[p_buf[7]][4]=p_buf[13];
                                track_5_color_3[p_buf[7]][5]=p_buf[14];
                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_5==7)
                            {
                                track_5_color_4[p_buf[7]][0]=p_buf[9];
                                track_5_color_4[p_buf[7]][1]=p_buf[10];
                                track_5_color_4[p_buf[7]][2]=p_buf[11];
                                track_5_color_4[p_buf[7]][3]=p_buf[12];
                                track_5_color_4[p_buf[7]][4]=p_buf[13];
                                track_5_color_4[p_buf[7]][5]=p_buf[14];
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_5==15)
                            {
                                track_5_color_5[p_buf[7]][0]=p_buf[9];
                                track_5_color_5[p_buf[7]][1]=p_buf[10];
                                track_5_color_5[p_buf[7]][2]=p_buf[11];
                                track_5_color_5[p_buf[7]][3]=p_buf[12];
                                track_5_color_5[p_buf[7]][4]=p_buf[13];
                                track_5_color_5[p_buf[7]][5]=p_buf[14];
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_5==31)
                            {
                                track_5_color_1[p_buf[7]][0]=p_buf[9];
                                track_5_color_1[p_buf[7]][1]=p_buf[10];
                                track_5_color_1[p_buf[7]][2]=p_buf[11];
                                track_5_color_1[p_buf[7]][3]=p_buf[12];
                                track_5_color_1[p_buf[7]][4]=p_buf[13];
                                track_5_color_1[p_buf[7]][5]=p_buf[14];
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                track_5_color_1[p_buf[7]][0]=p_buf[9];
                                track_5_color_1[p_buf[7]][1]=p_buf[10];
                                track_5_color_1[p_buf[7]][2]=p_buf[11];
                                track_5_color_1[p_buf[7]][3]=p_buf[12];
                                track_5_color_1[p_buf[7]][4]=p_buf[13];
                                track_5_color_1[p_buf[7]][5]=p_buf[14];
                            }
                        }
                        system_mode.set_shade=0;
                        sta_logo_data[0][0]=p_buf[9];
                        sta_logo_data[0][1]=p_buf[10];
                        sta_logo_data[0][2]=p_buf[11];
                        ter_logo_data[0][0]=p_buf[12];
                        ter_logo_data[0][1]=p_buf[13];
                        ter_logo_data[0][2]=p_buf[14];
                        DMA_WS2812_shade_logo(p_buf[7]+1,sta_logo_data,ter_logo_data);

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
											HAL_UART_Transmit(&huart3,p_buf,r_buf_lenght,0xffffffff);
                        /******轨迹1**************/
                        if(p_buf[4]==effects_one)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_1==1)
                            {
                                printf("one area already write,writing to two area\r\n");

                                DMA_WS2812_data_flash(track_1_color_2,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_2_addr,temp_color_data,51)==1)
                                {
                                    printf("two write data sucess\r\n");
                                    write_flash_1=3;
																	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==3)
																		{
																			 printf("two write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("two write flag fail\r\n");
                                    }
																}
                                else {
                                    printf("two write data fail\r\n ");
                                }
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_1==3)
                            {
                              
                                 printf("two area already write,writing to three area\r\n");
                                DMA_WS2812_data_flash(track_1_color_3,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_3_addr,temp_color_data,51)==1)
                                {
                                     printf("three write data sucess\r\n");
                                    write_flash_1=7;
                                  	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==7)
																		{
																			 printf("three write flag suceess\r\n");
																		}
																			else
																		{
																			
																			 printf("three write flag fail\r\n");
                                     }
																		
                                }
                                else {
                                       printf("three write data fail \r\n");
                                }

                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_1==7)
                            {
                                 printf("three area already write,writing to four area\r\n");

                                DMA_WS2812_data_flash(track_1_color_4,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_4_addr,temp_color_data,51)==1)
                                {
                                   printf("four write data sucess\r\n");
                                    write_flash_1=15;
                                  	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==15)
																		{
																			 printf("four write flag suceess\r\n");
																		}
																				else
																		{
																			
																			 printf("four write flag fail\r\n");
                                     }
                                }
                                else {
                                    printf("four write data fail\r\n ");
                                }
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_1==15)
                            {
                                 printf("four area already write,writing to five area\r\n");

                                DMA_WS2812_data_flash(track_1_color_5,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_5_addr,temp_color_data,51)==1)
                                {
                                      printf("five write data sucess\r\n");
                                    write_flash_1=31;
                                   	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==31)
																		{
																			 printf("five write flag suceess\r\n");
																		}
																					else
																		{
																			
																			 printf("five write flag fail\r\n");
                                     }
                                }
                                else {
                                     printf("five write data fail\r\n ");
                                }
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_1==31)
                            {
                               printf("five area already write,writing to one area\r\n");

                                DMA_WS2812_data_flash(track_1_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_1_addr,temp_color_data,51)==1)
                                {
                                     printf("one write data sucess\r\n");
                                    write_flash_1=1;
                                  	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																					else
																		{
																			
																			 printf("one write flag fail\r\n");
                                     }
                                }
                                else {
                                     printf("one write data fail\r\n ");
                                }
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                DMA_WS2812_data_flash(track_1_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_1_color_1_addr,temp_color_data,51)==1)
                                {
                                    printf("one write data sucess\r\n");
                                    write_flash_1=1;
                                   	STMFLASH_Write (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); 

                                   STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1);
                                    if(write_flash_1==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																					else
																		{
																			
																			 printf("one write flag fail\r\n");
                                     }
																		
                                }
                                else {
                                    printf("one write data fail\r\n ");
                                }
                          }
                        }
                        /******轨迹2**************/
                        if(p_buf[4]==effects_two)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_2==1)
                            {
                                printf("1区已经写入,写第二区");

                                DMA_WS2812_data_flash(track_2_color_2,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_2_addr,temp_color_data,51)==1)
                                {
                                     printf("two write data sucess\r\n");
                                    write_flash_2=3;
                                   	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                    STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==3)
																		{
																			 printf("two write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("two write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("two write data fail\r\n");
                                }
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_2==3)
                            {
                                printf("2区已经写入,写入第三区");

                                DMA_WS2812_data_flash(track_2_color_3,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_3_addr,temp_color_data,51)==1)
                                {
                                    printf("three write data suceess\r\n");
                                    write_flash_2=7;
                                    	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                   STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==7)
																		{
																			 printf("three write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("three write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("three write data fail\r\n");
                                }

                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_2==7)
                            {
                                printf("3区已经写入,写入第四区");

                                DMA_WS2812_data_flash(track_2_color_4,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_4_addr,temp_color_data,51)==1)
                                {
                                     printf("four write data suceess\r\n");
                                    write_flash_2=15;
                                    	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                      STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==15)
																		{
																			 printf("four write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("four write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("four write data fail\r\n");
                                }
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_2==15)
                            {
                                printf("4区已经写入,写入第5区");

                                DMA_WS2812_data_flash(track_2_color_5,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_5_addr,temp_color_data,51)==1)
                                {
                                    printf("five write data suceess\r\n");
                                    write_flash_2=31;
                                    	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                   STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==31)
																		{
																			 printf("five write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("five write flag fail\r\n");
                                    }
                                }
                                else {
                                   printf("five write data fail\r\n");;
                                }
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_2==31)
                            {
                                printf("5区已经写入,写入第1区");

                                DMA_WS2812_data_flash(track_2_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_1_addr,temp_color_data,51)==1)
                                {
                                   printf("one write data suceess\r\n");
                                    write_flash_2=1;
                                    	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                   STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("one write data fail\r\n");
                                }
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                DMA_WS2812_data_flash(track_2_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_1_addr,temp_color_data,51)==1)
                                {
                                   	 printf("one write data suceess\r\n");
                                    write_flash_2=1;
                                    	STMFLASH_Write (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); 

                                     STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1);
                                    if(write_flash_2==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("one write data fail\r\n");
                                }
                            }
                        }
                        /******轨迹3**************/
                        if(p_buf[4]==effects_three)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_3==1)
                            {
                                printf("1区已经写入,写第二区");

                                DMA_WS2812_data_flash(track_3_color_2,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_3_color_2_addr,temp_color_data,51)==1)
                                {
                                     printf("two write data suceess\r\n");
                                    write_flash_3=3;
                                    	STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==3)
																		{
																			 printf("two write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("two write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("two write data fail\r\n");
                                }
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_3==3)
                            {
                                printf("2区已经写入,写入第三区");

                                DMA_WS2812_data_flash(track_3_color_3,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_3_color_3_addr,temp_color_data,51)==1)
                                {
                                    printf("three write data suceess\r\n");
                                    write_flash_3=7;
                                   	STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==7)
																		{
																			 printf("three write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("three write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("three write data suceess\r\n");
                                }

                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_3==7)
                            {
                                printf("3区已经写入,写入第四区");

                                DMA_WS2812_data_flash(track_3_color_4,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_3_color_4_addr,temp_color_data,51)==1)
                                {
                                    printf("four write data suceess\r\n");
                                    write_flash_3=15;
                                   	STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==15)
																		{
																			 printf("four write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("four write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("four write data fail\r\n");
                                }
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_3==15)
                            {
                                printf("4区已经写入,写入第5区");

                                DMA_WS2812_data_flash(track_3_color_5,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_3_color_5_addr,temp_color_data,51)==1)
                                {
                                     printf("five write data suceess\r\n");
                                    write_flash_3=31;
                                   STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==31)
																		{
																			 printf("five write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("five write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("five write data fail\r\n");
                                }
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_3==31)
                            {
                                printf("5区已经写入,写入第1区");

                                DMA_WS2812_data_flash(track_3_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_3_color_1_addr,temp_color_data,51)==1)
                                {
                                        printf("one write data sucess\r\n");
                                    write_flash_3=1;
                                   	STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                  printf("one write data fail\r\n");
                                }
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                DMA_WS2812_data_flash(track_2_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_2_color_1_addr,temp_color_data,51)==1)
                                {
                                      printf("one write data sucess\r\n");
                                    write_flash_3=1;
                                   	STMFLASH_Write (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); 

                                   STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1);
                                    if(write_flash_3==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                        printf("one write data fail\r\n");
                                }
                            }
                        }
												/******轨迹4**************/
                        if(p_buf[4]==effects_four)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_4==1)
                            {
                                printf("1区已经写入,写第二区");

                                DMA_WS2812_data_flash(track_4_color_2,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_2_addr,temp_color_data,51)==1)
                                {
                                    printf("two write data sucess");
                                    write_flash_4=3;
                                    	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                   STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==3)
																		{
																			 printf("two write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("two write flag fail\r\n");
                                    }
                                }
                                else {
                                   printf("two write data fail");
                                }
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_4==3)
                            {
                                printf("2区已经写入,写入第三区");

                                DMA_WS2812_data_flash(track_4_color_3,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_3_addr,temp_color_data,51)==1)
                                {
                                    printf("three write data sucess");
                                    write_flash_4=7;
                                   	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                   STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==7)
																		{
																			 printf("three write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("three write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("three write data fail");
                                }

                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_4==7)
                            {
                                printf("3区已经写入,写入第四区");

                                DMA_WS2812_data_flash(track_4_color_4,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_4_addr,temp_color_data,51)==1)
                                {
                                    printf("four write data sucess\r\n");
                                    write_flash_4=15;
                                    	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                      STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==15)
																		{
																			 printf("four write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("four write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("four write data fail\r\n");
                                }
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_4==15)
                            {
                                printf("4区已经写入,写入第5区");

                                DMA_WS2812_data_flash(track_4_color_5,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_5_addr,temp_color_data,51)==1)
                                {
                                    printf("five write data sucess");
                                    write_flash_4=31;
                                   	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                   STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==31)
																		{
																			 printf("five write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("five write flag fail\r\n");
                                    }
                                }
                                else {
                                     printf("five write data fail\r\n");
                                }
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_4==31)
                            {
                                printf("5区已经写入,写入第1区");

                                DMA_WS2812_data_flash(track_4_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_1_addr,temp_color_data,51)==1)
                                {
                                    printf("one write data sucess");
                                    write_flash_4=1;
                                   	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                   STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                     printf("one write data fail");
                                }
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                DMA_WS2812_data_flash(track_4_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_4_color_1_addr,temp_color_data,51)==1)
                                {
                                    printf("one write data sucess");
                                    write_flash_4=1;
                                   	STMFLASH_Write (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); 

                                   STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1);
                                    if(write_flash_4==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("one write data fail");
                                }
                            }
                        }
										    /******轨迹5**************/
                        if(p_buf[4]==effects_five)
                        {
                            /**********1区是否被写入**************/
                            if(write_flash_5==1)
                            {
                                printf("1区已经写入,写第二区");

                                DMA_WS2812_data_flash(track_5_color_2,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_2_addr,temp_color_data,51)==1)
                                {
                                    printf("two write data sucess");
                                    write_flash_5=3;
                                  	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==3)
																		{
																			 printf("two write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("two write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("two write data fail");
                                }
                            }
                            /**********2区是否被写入**************/
                            else if(write_flash_5==3)
                            {
                                printf("2区已经写入,写入第三区");

                                DMA_WS2812_data_flash(track_5_color_3,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_3_addr,temp_color_data,51)==1)
                                {
                                    printf("three write data sucess");
                                    write_flash_5=7;
                                     	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==7)
																		{
																			 printf("three write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("three write flag fail\r\n");
                                    }
                                }
                                else {
                                       printf("three write data fail");
                                }

                            }
                            /**********3区是否被写入**************/
                            else if(write_flash_5==7)
                            {
                                printf("3区已经写入,写入第四区");

                                DMA_WS2812_data_flash(track_5_color_4,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_4_addr,temp_color_data,51)==1)
                                {
                                    printf("four write data sucess");
                                    write_flash_5=15;
                                   	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==15)
																		{
																			 printf("four write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("four write flag fail\r\n");
                                    }
                                }
                                else {
                                     printf("four write data fail");
                                }
                            }
                            /**********4区是否被写入**************/
                            else if(write_flash_5==15)
                            {
                                printf("4区已经写入,写入第5区");

                                DMA_WS2812_data_flash(track_5_color_5,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_5_addr,temp_color_data,51)==1)
                                {
                                    printf("five write data sucess");
                                    write_flash_5=31;
                                   	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==31)
																		{
																			 printf("five write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("five write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("five write data fail");
                                }
                            }
                            /**********5区是否被写入**************/
                            else if(write_flash_5==31)
                            {
                                printf("5区已经写入,写入第1区");

                                DMA_WS2812_data_flash(track_5_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_1_addr,temp_color_data,51)==1)
                                {
                                    printf("one write data suceess");
                                    write_flash_5=1;
                                   	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                    printf("one write data fail");
                                }
                            }
                            /*****12345都没有写  初始化写1区*************/
                            else {
                                DMA_WS2812_data_flash(track_5_color_1,temp_color_data,102);
//														 STMFLASH_Write (0x8008c00, (uint16_t *)track_1_color_2,102);

//                             STMFLASH_Read  (0x8008c00, (uint16_t *)track_1_color_2,102);

                                if(write_read_flash_jiaoyan(track_5_color_1_addr,temp_color_data,51)==1)
                                {
                                    printf("one write data sucess");
                                    write_flash_5=1;
                                    	STMFLASH_Write (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); 

                                   STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1);
                                    if(write_flash_5==1)
																		{
																			 printf("one write flag suceess\r\n");
																		}
																		else
																		{
																			
																			 printf("one write flag fail\r\n");
                                    }
                                }
                                else {
                                   printf("one write data fail");
                                }
                            }
                        }
                       system_mode.set_shade=1;
             

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
				memset(UsartType3.tem_RX_pData, 0, sizeof(UsartType3.tem_RX_pData));
			
        UsartType3.RX_flag=0;
				UsartType3.RX_Size=0;
				UsartType3.tem_RX_Size=0;
				r_buf_lenght=0;
				

    }
}
/*from flash read data to color buf*/
void flash_data_to_color_data()
{
    // from track1 flash read data to color buf
    STMFLASH_Read  (track_1_color_1_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_1_color_1,102);
    	        
    STMFLASH_Read  (track_1_color_2_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_1_color_2,102);
//		   DMA_WS2812_shade_logo_all(track_1_color_2);
//	for(uint8_t i=0;i<17;i++)
//	{   
//		for(uint8_t j=0;j<6;j++)
//		{
//	      printf("ws28128_color_buf[%d]=[%02x] ",i,ws28128_color_buf[i][j]);
//		}
//		printf("\r\n");
//	}
    STMFLASH_Read  (track_1_color_3_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_1_color_3,102);
//   DMA_WS2812_shade_logo_all(track_1_color_3);

    STMFLASH_Read  (track_1_color_4_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_1_color_4,102);
//	   DMA_WS2812_shade_logo_all(track_1_color_4);

    STMFLASH_Read  (track_1_color_5_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_1_color_5,102);
//	   DMA_WS2812_shade_logo_all(track_1_color_5);;
  DMA_WS2812_shade_logo_all(track_1_color_1);
						  DMA_WS2812_shade_logo_all(track_1_color_2);
						  DMA_WS2812_shade_logo_all(track_1_color_3);
						  DMA_WS2812_shade_logo_all(track_1_color_4);
						  DMA_WS2812_shade_logo_all(track_1_color_5);
    STMFLASH_Read  (track_1_color_all_addr, (uint16_t *)&write_flash_1,1); //
//		
	  if((write_flash_1==1)|(write_flash_1==3)|(write_flash_1==7)|(write_flash_1==15)|(write_flash_1==31))
		{
			system_mode.set_shade=1;
			
		}
		else
		{
			system_mode.set_shade=0;
		}
    STMFLASH_Read  (track_2_color_1_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_2_color_1,102);
		
    STMFLASH_Read  (track_2_color_2_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_2_color_2,102);
		
    STMFLASH_Read  (track_2_color_3_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_2_color_3,102);
		
    STMFLASH_Read  (track_2_color_4_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_2_color_4,102);
		
    STMFLASH_Read  (track_2_color_5_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_2_color_5,102);
		
    STMFLASH_Read  (track_2_color_all_addr, (uint16_t *)&write_flash_2,1); //

//		    // from track3 flash read data to color buf
    STMFLASH_Read  (track_3_color_1_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_3_color_1,102);

    STMFLASH_Read  (track_3_color_2_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_3_color_2,102);

    STMFLASH_Read  (track_3_color_3_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_3_color_3,102);

    STMFLASH_Read  (track_3_color_4_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_3_color_4,102);

    STMFLASH_Read  (track_3_color_5_addr, (uint16_t *)temp_color_data,102); //
    DMA_WS2812_data_flash_1(temp_color_data,track_3_color_5,102);

    STMFLASH_Read  (track_3_color_all_addr, (uint16_t *)&write_flash_3,1); //
		  // from track4 flash read data to color buf
    STMFLASH_Read  (track_4_color_1_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_4_color_1,102);

    STMFLASH_Read  (track_4_color_2_addr, (uint16_t *)temp_color_data,102); //
    DMA_WS2812_data_flash_1(temp_color_data,track_4_color_2,102);

    STMFLASH_Read  (track_4_color_3_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_4_color_3,102);

    STMFLASH_Read  (track_4_color_4_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_4_color_4,102);

    STMFLASH_Read  (track_4_color_5_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_4_color_5,102);

    STMFLASH_Read  (track_4_color_all_addr, (uint16_t *)&write_flash_4,1); //
		  // from track4 flash read data to color buf
    STMFLASH_Read  (track_5_color_1_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_5_color_1,102);

    STMFLASH_Read  (track_5_color_2_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_5_color_2,102);

    STMFLASH_Read  (track_5_color_3_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_5_color_3,102);

    STMFLASH_Read  (track_5_color_4_addr, (uint16_t *)temp_color_data,102); //
    DMA_WS2812_data_flash_1(temp_color_data,track_5_color_4,102);

    STMFLASH_Read  (track_5_color_5_addr, (uint16_t *)temp_color_data,51); //
    DMA_WS2812_data_flash_1(temp_color_data,track_5_color_5,102);

    STMFLASH_Read  (track_5_color_all_addr, (uint16_t *)&write_flash_5,1); //

}
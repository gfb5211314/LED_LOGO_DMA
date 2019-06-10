#include  "esp8266.h"
#include   "crc.h"
#include "stm_flash.h"
#include "string.h"
#include "bsp_usart.h"
#include "stdlib.h"

extern DMA_HandleTypeDef hdma_usart3_tx;
typedef struct {
	uint8_t  name[20];
  uint8_t  password[20]; 
}wifi_type;
wifi_type   wifi8266;
extern UART_HandleTypeDef huart3;
extern USART_RECEIVETYPE  UsartType3;
//uint8_t AT_CWSAP[]="AT+CWSAP=\"ATK_ESP8266\",\"12345678\",6,4\r\n";
uint8_t AT_CWSAP[50];
#define  wifi8266_name_addr      0x800e410                      //57K片区         
#define  wifi8266_password_addr  0x800e440                      //57K片区
/*********************************************
             需求分析
						 1.用户手机连接WIFI，设置WIFI控制板WiFi名字和密码
						 2.拨码开关。选择模式
						 3.升级固件
**********************************************/	

//void  ESP8266_unpack(uint8_t *buf,uint8_t data_length)
//{
//	
//	    uint16_t   crc_data;
//	    uint16_t   data_len;
//	   /*********set wifi name and password**********/
//    if((buf[0]==0xaa)&&(buf[1]==0xbb)&&(buf[2]==0x01)&&(buf[data_length-3]==0xaa)&&(buf[data_length-4]==0xbb))
//		{              
//              crc_data=Calc_CRC16(buf,data_length-2);
//            printf("creamCRC1:%04x\r\n",crc_data);
//            printf("creamCRC2:%04x\r\n",buf[data_length-1]+(buf[data_length-2]*256));
//			   /**********CRCjaonian********************/
//       if(crc_data==(buf[data_length-1]+(buf[data_length-2]*256)))
//            {
//							   switch(buf[3]) //功能码
//                {
//                /*****set wifi name ***********/
//                case  0x01 :   
//                       data_len=buf[4]+buf[5];       //data length
//								   for(uint8_t i=0;i<data_len;i++)
//								{
//									  wifi8266.password[i]=buf[6+i];
//								}
//								//get char length
//								//strlen((char *)wifi8266.password);
//								STMFLASH_Write (wifi8266_name_addr,(uint16_t * )wifi8266.password,data_len/2);
//								  break;
//								/******set wifi password*******/
//                case  0x02 :   
//                       data_len=buf[4]+buf[5];       //data length             								
//         
//								break;
//								}
//						}

//		}			
//	
//	
//	
//}
uint8_t* atk_8266_check_cmd(uint8_t *str)
{
	
	char *strx=0;
	if(UsartType3.RX_flag)		//接收到一次数据了
	{ 
		UsartType3.RX_pData[UsartType3.RX_Size]=0;//添加结束符
		strx=strstr((const char*)UsartType3.RX_pData,(const char*)str);
	} 
	return (uint8_t*)strx;
}
//esp8266 Capture data
uint8_t* esp8266_Capture_data(uint8_t *rec_data,uint8_t *head_buf)
{   
	   	char *strx=0;
     strx=strstr((const char*)rec_data,(const char*)head_buf);
		return (uint8_t*)(strx+1);

}
uint8_t atk_8266_send_cmd(uint8_t *cmd,uint8_t *ack,uint16_t waittime)
{
	uint8_t res=0; 
 HAL_UART_Transmit_DMA(&huart3, (uint8_t*)cmd,strlen((char *)cmd));
     while(__HAL_DMA_GET_COUNTER(&hdma_usart3_tx));
	if(ack&&waittime)		//需要等待应答
	{
		while(--waittime)	//等待倒计时
		{
			HAL_Delay(10);
			if(UsartType3.RX_flag)//接收到期待的应答结果
			{
				if(atk_8266_check_cmd(ack))
				{
					printf("ack:%s\r\n",(uint8_t*)ack);
					break;//得到有效数据 
				}
					UsartType3.RX_Size=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 

/*****目前不用*******************/
uint8_t esp8266_set_WIFI_command(const uint8_t *pstart, uint16_t wait_time)
{

    HAL_UART_Transmit_DMA(&huart3, (uint8_t*)pstart,strlen((char *)pstart));
        while(__HAL_DMA_GET_COUNTER(&hdma_usart3_tx))
    HAL_Delay(wait_time);

    if(strstr((const char*)UsartType3.RX_pData, "OK") != NULL)
    {
        UsartType3.RX_flag = 0;
        memset(UsartType3.RX_pData, 0, sizeof(UsartType3.RX_pData));
        return 1;

    }
    else
    {
        UsartType3.RX_flag = 0;
        memset(UsartType3.RX_pData, 0, sizeof(UsartType3.RX_pData));
        return 0;
    }

}
/*****************目前不用********************/
//void  ESP8266_unpack_string(uint8_t *buf,uint8_t data_length)
//{ 
////	 0   1  2  3  4  5  6  7  8  9 10 11 12 13
////	 0D 0A 2B 49 50 44 2C 30 2C 33 3A 31 32 33
//	   uint16_t   data_len;
//	 if((buf[0]==0x0d)&&(buf[1]==0x0a)&&(buf[2]==0x2B)&&(buf[3]==0x49)&&(buf[4]==0x50)&&(buf[5]==0x44))
//		{
//			
//			  data_len=atoi((const char *)&buf[9]);
//	     for(uint8_t i=0;i<data_len;i++)
//			{
//		   AT_CWSAP[i]=buf[12+i];
//			
//					
//			}
//			  AT_CWSAP[data_len]=0x0d;
//			   AT_CWSAP[data_len+1]=0x0a;
////	printf("len=%d",data_len);
//			for(uint16_t i=0; i<strlen((char *)AT_CWSAP); i++)
//          {
//           
//			 	      printf("%c",AT_CWSAP[i]);
//          }
//				if((AT_CWSAP[0]='A')&&(AT_CWSAP[1]='T')&&(AT_CWSAP[2]='+')&&(AT_CWSAP[3]='C')&&(AT_CWSAP[4]='W')&&(AT_CWSAP[5]='S')&&(AT_CWSAP[6]='A')&&(AT_CWSAP[7]='P'))
//				{

//					  if(esp8266_set_WIFI_command(AT_CWSAP, 50))
//						{
//								printf("set sucess\r\n");
//							
//						}
//						else
//						{
//							printf("set fail\r\n");
//						}
////					
//			  }
//	
//			
//		}
//	  else
//		{
//			printf("格式不对\r\n");
//		}

//}
#include "bsp_upgrade.h"
#include  "stm_flash.h"
#include "string.h"
#include <stdlib.h>
extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart3;
/**********************************************
            upgrade.h   file
            upgrade mcu code
 ********************************************/
upgrade_type  logo_upgrade;
//write data to flash
uint8_t upgrate_flash_write(uint32_t start_addr,uint8_t *pbuf,uint16_t data_len,uint16_t flash_bolck)
{
	    uint8_t  t_p_buf[512];
	    STMFLASH_Write (start_addr+(flash_bolck-1)*data_len,(uint16_t *)pbuf,data_len/2);
	    STMFLASH_Read  (start_addr+(flash_bolck-1)*data_len,(uint16_t *)t_p_buf,data_len/2); 
	   if(strcmp((const char *) pbuf,(const char *)t_p_buf)==0)
      {
        return 1;
     }
     else
     {

        return 0;
     }
	
}
void SoftReset(void)
{
__set_FAULTMASK(1);      // 关闭所有中断
NVIC_SystemReset();//   复位
}
void ack_data(uint8_t* tx_buf)
{
	       
     HAL_UART_Transmit_DMA(&huart3, (uint8_t*)tx_buf,strlen((char *)tx_buf));
          while(__HAL_DMA_GET_COUNTER(&hdma_usart3_tx));
}
/**
  * @brief  upgrade app function
  * @param  start_addr:  wrtie flash start addr                      
  * @param  pbuf:    upgrade data 
  * @param  tepbuf:     pack data
  * @param  data_len:  write data length
  * @param  flash_bolck:  write  block
  * @param  all_flash_bolck:  upgrade_
  * @retval HAL_StatusTypeDef HAL Status
  */
void upgrade_app_function(uint32_t start_addr,uint8_t *pbuf,uint8_t *tepbuf,uint16_t data_len,uint16_t flash_bolck,uint16_t all_flash_bolck,void (*callfuct)())
{     
	  if(flash_bolck<(all_flash_bolck+1))
		{
			   
	    if(upgrate_flash_write(start_addr,pbuf,data_len,flash_bolck))
			{
				 				(*callfuct)(tepbuf);
				printf("write %d block suceess\r\n",flash_bolck);
			 	 	
			}
			else
			{
				printf("write %d block fail\r\n",flash_bolck);
				
			}
			//upgrade  complete
			if(flash_bolck==all_flash_bolck)
			{
				    logo_upgrade.upgrade_state=1;
				  STMFLASH_Write (upgrade_state_addr,(uint16_t*)&logo_upgrade.upgrade_state,1); 
				 //reset to bootloader
			      SoftReset();
			}
		}
}
	
	

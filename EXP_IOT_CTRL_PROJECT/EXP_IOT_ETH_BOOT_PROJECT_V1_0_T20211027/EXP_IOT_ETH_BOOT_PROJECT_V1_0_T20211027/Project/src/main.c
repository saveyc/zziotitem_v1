/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32_eth.h"
#include "netconf.h"
#include "main.h"
#include "tftpserver.h"
#include "memp.h"
#include "udpclient.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SYSTEMTICK_PERIOD_MS  1

u8  mac_addr[6] = {0};

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
uint32_t timingdelay;
u16 sec_reg = 250;//1000;
u16 sec_flag = 0;
u8  Local_Station;//本机站号
__IO u16 app_flag = 0;

typedef  void (*pFunction)(void);
pFunction Jump_To_Application;
uint32_t JumpAddress;
extern uint8_t Programm_Completed_Success;
/* Private function prototypes -----------------------------------------------*/
void TFTP_IAP(void);

u16 readFlash_uint(uint32_t address)
{
	u8 *source;
	u32 i;
	u16 out_point;

	source=(u8*)(address);
	out_point = 0;
	for(i = 0;i < 2;i++)
	{
		out_point = out_point << 8;
		out_point |= *(source+(1-i));	
	}
	return(out_point);
}


void read_mac_addrs(void)
{
    u32 Mac_Code;
    u32 CpuID[3];
    
    //获取CPU唯一ID
    CpuID[0] = *(u32 *)(0x1FFFF7E8);
    CpuID[1] = *(u32 *)(0x1FFFF7EC);
    CpuID[2] = *(u32 *)(0x1FFFF7F0);
    
    Mac_Code = (CpuID[0] >> 1) + (CpuID[1] >> 2) + (CpuID[2] >> 3);
    
    mac_addr[0] = (u8) ((Mac_Code >> 24) & 0xFF);
    mac_addr[1] = (u8) ((Mac_Code >> 16) & 0xFF);
    mac_addr[2] = (u8) ((Mac_Code >> 8) & 0xFF);
    mac_addr[3] = (u8) ( Mac_Code & 0xFF);
    mac_addr[4] = 0x00;
    mac_addr[5] = 0x02;
}
void scan_local_station(void)
{
    u8 tmp,i;
    
    tmp = 0;
    for(i = 0; i < 20; i++)
    {
        if( DIP1_STATE )
        {
            tmp |= 0x01;
        }
        if( DIP2_STATE )
        {
            tmp |= 0x02;
        }
        if( DIP3_STATE )
        {
            tmp |= 0x04;
        }
        if( DIP4_STATE )
        {
            tmp |= 0x08;
        }
        if( DIP5_STATE )
        {
            tmp |= 0x10;
        }
        if( DIP6_STATE )
        {
            tmp |= 0x20;
        }
        if( DIP7_STATE )
        {
            tmp |= 0x40;
        }
        if( DIP8_STATE )
        {
            tmp |= 0x80;
        }
        if( Local_Station != tmp )
        {
            Local_Station = tmp;
            i = 0;
        }
    }

    Local_Station = (~Local_Station) & 0x7F;
}
//1s
void sec_process(void)
{
    if( sec_flag == 1 )
    {
        sec_flag = 0;
        if(LED_STATE)
	{
	    LED_ON;
	}
	else
	{
	    LED_OFF;
	}
    }
}
void Jump_To_User_App(void)
{
    /* Test if user code is programmed starting from address "ApplicationAddress" */
    if (((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
    {
        /* Jump to user application */
        JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);
        Jump_To_Application = (pFunction) JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t *) ApplicationAddress);
        Jump_To_Application();
    }
}
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
    System_Setup();
    read_mac_addrs();
    scan_local_station();
    
    app_flag = *((u16*)UserAppEnFlagAddress);
    
    if( app_flag != 0xAA)
    {
        if(Local_Station == 1 || Local_Station == 0)
            TFTP_IAP();
        else
            can_bus_reply_process();
    }
    else
    {
        Jump_To_User_App();
    }
}

void TFTP_IAP(void)
{
    while(EthInitStatus == 0 )
    {
        Ethernet_Configuration();
    }
    InitSendMsgQueue();
    /* Initialize the LwIP stack */
    LwIP_Init();
    /* Initialize the tftpd*/
    tftpd_init();
    /* Infinite loop */
    while (1)
    {
        /* Periodic tasks */
        LwIP_Periodic_Handle(LocalTime);
        if(Programm_Completed_Success == 1)
        {
            Programm_Completed_Success = 0;
            
            if(sDLoad_Para_Data.mode == DLOAD_MODE_LOCAL)//烧写主机
            {
                g_download_status = DLOAD_INIT;
                FLASH_Unlock();
                FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
               if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE)
               {
                 FLASH_ProgramHalfWord(UserAppEnFlagAddress,0xAA);
               }
                FLASH_Lock();
                Delay(2000);  //2s
                NVIC_SystemReset();
            }
            else if(sDLoad_Para_Data.mode == DLOAD_MODE_TRANS)
            {
                g_download_status = DLOAD_TRAN_INIT;
                cur_tran_station = sDLoad_Para_Data.st_addr; //起始站点
                if(file_tot_bytes % CAN_SEND_DATA_PK_SIZE) //计算包总数
                    pk_tot_num = (file_tot_bytes / CAN_SEND_DATA_PK_SIZE)+1;
                else
                    pk_tot_num = (file_tot_bytes / CAN_SEND_DATA_PK_SIZE);
                pk_cur_num = 1; //当前的包序号
                trans_flag = 0;
            }
        }
        udp_client_process();
        send_message_to_sever();
        host_transmit_process();
        can_send_frame_process();
    }
}
/**
  * @brief  Inserts a delay time.
  * @param  nCount: number of 10ms periods to wait for.
  * @retval None
  */
void Delay(uint32_t nCount)
{
    /* Capture the current local time */
    timingdelay = LocalTime + nCount;

    /* wait until the desired delay finish */
    while(timingdelay > LocalTime)
    {
    }
}

/**
  * @brief  Updates the system local time
  * @param  None
  * @retval None
  */
void Time_Update(void)
{
    LocalTime += SYSTEMTICK_PERIOD_MS;

    if( sec_reg != 0 )
    {
        sec_reg--;
        if( sec_reg == 0 )
        {
            sec_reg = 250;//1000;
            sec_flag = 1;
        }
    }
    sec_process();
    can_recv_timeout();
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
}
#endif


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

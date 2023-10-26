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
#include "udpclient.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SYSTEMTICK_PERIOD_MS  1

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
uint32_t timingdelay;
u16 sec_reg = 1000;
u16 sec_flag = 0;

u16 GetHz_inteval_cnt = 317;
u16 GetHz_inteval_flag = 0;
extern u16 Cur_ActualHz;

u8  heart_dely = 0;
u8  local_station;//本机站号
u8  isHost;//主站或从站
u16 pid_inteval_reg = 200;//PID调节间隔时间
u16 pid_inteval_flag = 0;

u16 upload_time_delay = 100;

u8  mac_addr[6] = {0};

u16 Forbid_ReadHz_cnt = 0;
u16 ReadHz_flag = 0;

/* Private function prototypes -----------------------------------------------*/
void System_Periodic_Handle(void);

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
    u8 tmp, i;
    u8 dip_value;

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
        if( dip_value != tmp )
        {
            dip_value = tmp;
            i = 0;
        }
    }
    local_station = (~dip_value) & 0xFF;
    isHost = (local_station == 1) ? 1 : 0;
}
//1s
void sec_process(void)
{
    if( sec_flag == 1 )
    {
        sec_flag = 0;

        if( heart_dely != 0 )
        {
            heart_dely--;
        }
        if( LED_STATE )
        {
            LED_ON;
        }
        else
        {
            LED_OFF;
        }
        if(isHost)//主机
        {
            module_status_buffer[1].flag = 1;
            module_status_buffer[1].status = g_read_start_status | (bModeInfo.button_state<<1) | (bHSpeedInfo.button_state<<2)
                                                                 | (bPauseInfo.button_state<<3) | (B_PHOTO_1_IN_STATE<<4);
            module_status_buffer[1].alarm = g_alarm_type;
            module_status_buffer[1].cur_speed = (u16)(rt_speed*100);
            module_status_buffer[1].inverter_hz = (u16)Cur_ActualHz;//fwHz;////
            
            AddSendMsgToQueue(SEND_MSG_MODULE_STATUS_TYPE);
            can_upload_slave_can_err_process();
        }
        else//非主机
        {
            can_bus_send_module_status();
        }
    }

    if(pid_inteval_flag == 1)
    {
        pid_inteval_flag = 0;
        speed_ctrl_process();
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
    InitSendMsgQueue();
    InitCanSendQueue();
    scan_local_station();
    read_mac_addrs();
    read_user_paras();
    Pre_Read_UpdateFlag();
//                    runctrl.start_cnt = DEFAULT_TIME;
//                    startflash.flash_reg = FLASH_TIME;
    /* Infinite loop */
    while (1)
    {
        if(isHost)//主机
        {
            if(EthInitStatus == 0 )
            {
                Ethernet_Configuration();
                if( EthInitStatus != 0 )
                {
                    LwIP_Init();
                }
            }
            else
            {
                LwIP_Periodic_Handle(LocalTime);
                udp_client_process();
                send_message_to_server();
            }
        }
        sec_process();
        can_send_frame_process();
        GetOutHz();
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


void Pre_Read_UpdateFlag(void)
{
     if(readFlash_uint(UserAppEnFlagAddress) != 0x00AA)
     { 
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
        if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE) 
        {
            FLASH_ProgramHalfWord(UserAppEnFlagAddress, 0x00AA);
        }
         FLASH_Lock();
     }
}

void GetOutHz(void)
{
     if( (GetHz_inteval_flag == 1)&&(ReadHz_flag == 1) )
     {
         GetHz_inteval_flag = 0;
         ReadHz_flag = 0;
         Modbus_RTU_Read_Reg_Cmd(OUT_HZ_REG_ADDR,1);
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
            sec_reg = 1000;
            sec_flag = 1;
        }
    }
    if(pid_inteval_reg != 0)
    {
        pid_inteval_reg--;
        if( pid_inteval_reg == 0 )
        {
            pid_inteval_reg = 200;
            pid_inteval_flag = 1;
        }
    }
    
    
     if(GetHz_inteval_cnt != 0)
     {
        GetHz_inteval_cnt--;
        if( GetHz_inteval_cnt == 0 )
        {
            GetHz_inteval_cnt = 317;
            GetHz_inteval_flag = 1;
        }
     }

     if(Forbid_ReadHz_cnt != 0)
    {
        Forbid_ReadHz_cnt--;
        if( Forbid_ReadHz_cnt == 0 )
        {        
            ReadHz_flag = 1;
        }
    }
//    if(upload_time_delay != 0)
//    {
//        upload_time_delay--;
//        if(upload_time_delay == 0)
//        {
//            if(isHost)
//            {
//                AddSendMsgToQueue(SEND_MSG_MODULE_STATUS_TYPE);
//            }
//            upload_time_delay = 100;
//        }
//    }
    uart_recv_timeout();
    calculate_rt_speed();
    InputScanProc();
    Logic_Ctrl_Process();
    Motor_DelayAction();
    Modbus_RTU_Comm_Process();
//    logit_start_flash();
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

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
u16 half_sec_reg = 500;
u16 half_sec_flag = 0;
u8  heart_dely = 0;
u8  local_station;//����վ��
u8  isHost;//��վ���վ
u16 upload_time_delay = 10;

u8  mac_addr[6] = { 0 };
u16 upload_runcnt = 0;
u8  msone_flag = 0;
 
u16 modulestate_readcnt = 0; 
u16 modulestate_index = 1;

u16 upload_55ms = 0;
u16 upload_600ms = 0;

u16 upload_600ms_3 = 0;
u16 upload_600ms_2 = 0;
u16 upload_600ms_1 = 0;

void main_upload_run_state(void);

/* Private function prototypes -----------------------------------------------*/
void System_Periodic_Handle(void);
void main_msone_process(void);

void read_mac_addrs(void)
{
    u32 Mac_Code;
    u32 CpuID[3];
    
    //��ȡCPUΨһID
    CpuID[0] = *(u32 *)(0x1FFFF7E8);
    CpuID[1] = *(u32 *)(0x1FFFF7EC);
    CpuID[2] = *(u32 *)(0x1FFFF7F0);
    
    Mac_Code = (CpuID[0] >> 1) + (CpuID[1] >> 2) + (CpuID[2] >> 3);
    
    mac_addr[0] = 0x00;
    mac_addr[1] = 0x02;
    mac_addr[2] = (u8) ((Mac_Code >> 24) & 0xFF);
    mac_addr[3] = (u8) ((Mac_Code >> 16) & 0xFF);
    mac_addr[4] = (u8) ((Mac_Code >> 8) & 0xFF);
    mac_addr[5] = (u8) ( Mac_Code & 0xFF);
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
  u8 buff[10] = {0};
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
        Block_Check_Ctrl_Handle();
    }
    if(half_sec_flag == 1)
    {
        half_sec_flag = 0;
        
        if(isHost)//����
        {
            MODULE_STATUS_T module_status_t;
            
            module_status_t.station_no = 1;
            module_status_t.belt_number = user_paras_local.Belt_Number;
            memcpy((u8*)module_status_t.inverter_status,(u8*)inverter_status_buffer,sizeof(INVERTER_STATUS_T)*user_paras_local.Belt_Number);
            AddModuleStatusData2Queue(module_status_t);
        }
//        can_bus_send_module_status();
            if(isHost){
                if(modulestate_readcnt > 30){
                      modulestate_readcnt = 0;
                      modulestate_index++;
                      can_bus_send_msg(buff,0,CAN_FUNC_ID_READ_MODULE_STATUS,modulestate_index);
                      if(modulestate_index > 30){
                      modulestate_index = 1;
                      }
                }
            }
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
    InitUartSendQueue();
    InitModuleStatusQueue();
    scan_local_station();
    read_mac_addrs();
    read_user_paras();
    logic_uarttmp_init();

    /* Infinite loop */
    while (1)
    {
        if(isHost)//����
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
        Modbus_RTU_Comm_Process();
        can_send_frame_process();
        Reset_Start_Inverter_Handle();
        logic_upstream_io_allow_output();
        main_upload_run_state();
        main_msone_process();
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
    upload_runcnt++;
    freq_check_cnt++;
    msone_flag = 1;

    if( sec_reg != 0 )
    {
        sec_reg--;
        if( sec_reg == 0 )
        {
            sec_reg = 1000;
            sec_flag = 1;
        }
    }
    if(half_sec_reg != 0)
    {
        half_sec_reg--;
        if(half_sec_reg == 0)
        {
            half_sec_reg = 500;
            half_sec_flag = 1;
        }
    }
    if(reset_start_time_cnt != 0)
    {
        reset_start_time_cnt--;
        if(reset_start_time_cnt == 0)
        {
            reset_start_flag = 1;
        }
    }
    if(upload_time_delay != 0)
    {
        upload_time_delay--;
        if(upload_time_delay == 0)
        {
            if((isHost == 1) && (IsModuleStatusQueueFree() == 0))
            {
                AddSendMsgToQueue(SEND_MSG_MODULE_STATUS_TYPE);
            }
            upload_time_delay = 10;
        }
    }
    uart_recv_timeout();
    InputScanProc();
}

void main_upload_run_state(void)
{
u16 i = 0;
u8  link_down_status = 1;
COMM_NODE_T  comm_node_new;

if (user_paras_local.Belt_Number == 0) return;



if (upload_runcnt >= 800) {
    upload_runcnt = 0;
    if (g_speed_gear_status != 0) {
        for (i = 0; i < user_paras_local.Belt_Number; i++) {
            if ((user_paras_local.belt_para[i].Func_Select_Switch >> 4) & 0x1)
            {
                if ((user_paras_local.Down_Stream_No == 0) && (bStreamInfo.input_info.input_state == 0))//û����������վ��
                {
                    link_down_status = 0;
                }
                else if ((user_paras_local.Down_Stream_No == 0) && (bStreamInfo.input_info.input_state == 1)) {
                    link_down_status = 1;
                }
                else
                {
                    link_down_status = g_link_down_stream_status;
                }
                break;
            }
        }
        if (user_paras_local.Down_Stream_No != 0)//��������վ��
        {
            //            link_down_status = 1;
            link_down_status = g_link_down_stream_status;
        }

        if ((user_paras_local.belt_para[user_paras_local.Belt_Number - 1].Func_Select_Switch >> 1) & 0x1)//�������ܿ���
        {
            if ((((inverter_status_buffer[user_paras_local.Belt_Number - 1].fault_code >> 4) & 0x1) == 0)
                && (get_inverter_fault_status(inverter_status_buffer[user_paras_local.Belt_Number - 1]) != 1))//����Ƥ��ֹͣ״̬��û�б���
            {
                if (link_down_status == 1) {                    //����
                    comm_node_new.rw_flag = 1;
                    comm_node_new.inverter_no = user_paras_local.Belt_Number;
                    comm_node_new.speed_gear = g_speed_gear_status;
                    comm_node_new.comm_interval = user_paras_local.belt_para[user_paras_local.Belt_Number - 1].Start_Delay_Time;
                    comm_node_new.comm_retry = 3;
                    AddUartSendData2Queue(comm_node_new);
                }
            }
        }
    }
    if (g_speed_gear_status != 0) { //�м��Ƥ��
        if (user_paras_local.Belt_Number > 1) {
            for (i = user_paras_local.Belt_Number - 1; i > 0; i--) {
                if (((inverter_status_buffer[i].fault_code >> 4) & 0x1) == 1) {
                    if ((((inverter_status_buffer[i - 1].fault_code >> 4) & 0x1) == 0)
                        && (get_inverter_fault_status(inverter_status_buffer[i - 1]) != 1)) {
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = i;
                        comm_node_new.speed_gear = g_speed_gear_status;
                        comm_node_new.comm_interval = user_paras_local.belt_para[i - 1].Start_Delay_Time;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                    }

                }
            }
        }
    }
}


}

void main_msone_process(void)
{
    COMM_NODE_T  comm_node_new;
   
    if (msone_flag == 1)
    {
        msone_flag = 0;
        upload_55ms++;

        modulestate_readcnt++;

        //�����ѯһ��
        if (freq_check_cnt > 300) {
            freq_check_cnt = 0;

            polling_num++;
            if (polling_num > user_paras_local.Belt_Number)
            {
                polling_num = 1;
            }
            comm_node_new.rw_flag = 0;
            comm_node_new.inverter_no = polling_num;
            comm_node_new.comm_interval = 10;
            comm_node_new.comm_retry = 0;
            AddUartSendData2Queue(comm_node_new);
        }

        logic_cycle_decrease();

        if (upload_55ms >= 500) {
            upload_55ms = 0;
            if ((((inverter_status_buffer[4].fault_code >> 4) & 0x1) == 0) && (((inverter_status_buffer[3].fault_code >> 4) & 0x1) == 1)) {
                comm_node_new.rw_flag = 1;
                comm_node_new.inverter_no = 4;
                comm_node_new.speed_gear = 0;
                comm_node_new.comm_interval = 10;
                comm_node_new.comm_retry = 3;
                AddUartSendData2Queue(comm_node_new);
            }
        }

        if (upload_600ms != 0) {
            upload_600ms--;
        }
        if (upload_600ms_3 != 0) {
            upload_600ms_3--;
        }
        if (upload_600ms_2 != 0) {
            upload_600ms_2--;
        }
        if (upload_600ms_1 != 0) {
            upload_600ms_1--;
        }
    }
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

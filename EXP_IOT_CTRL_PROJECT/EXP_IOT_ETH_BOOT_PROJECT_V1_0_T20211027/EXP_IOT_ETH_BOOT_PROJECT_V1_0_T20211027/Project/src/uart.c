#include "stm32f10x.h"
#include "stm32f107.h"
#include <stdio.h>
#include "main.h"
//RS485_3
u8  uart4_send_buff[UART4_BUFF_SIZE];
u8  uart4_recv_buff[UART4_BUFF_SIZE];
u16 uart4_send_count;
u16 uart4_recv_count;
u16 uart4_send_retry = 0;
u16 uart4_recv_timeout = 0;
u8  uart4_commu_state = 0;
u8  cur_tran_station;
u16 pk_tot_num;
u16 pk_cur_num;
u16 pk_len;
u16 slave_reply_cmd = UART_CMD_NULL;
u16 slave_recv_pk_num = 0;

void uart_recv_timeout(void)
{
    if( uart4_recv_timeout != 0 && uart4_commu_state == RECV_DATA)
    {
        uart4_recv_timeout--;
        if( uart4_recv_timeout == 0 )
        {
            if(uart4_send_retry >=3)
            {
                g_download_status = DLOAD_INIT;
                uart4_send_retry = 0;
            }
            else
            {
                uart4_send_retry++;
            }
            uart4_commu_state = SEND_DATA;
        }
    }
}
u8 sum_check(u8* pdata,u16 len)
{
    u16 i;
    u8  sum = 0;
    
    for(i=0 ; i<len ; i++)
    {
        sum += pdata[i];
    }
    return sum;
}
void uart4_send(void)
{
    UART4_TX_485;

    USART_DMACmd(UART4, USART_DMAReq_Tx, DISABLE);
    DMA_Cmd(DMA2_Channel5, DISABLE);
    DMA_SetCurrDataCounter(DMA2_Channel5, uart4_send_count);
    USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);
    DMA_Cmd(DMA2_Channel5, ENABLE);
}
void uart_send_boot_cmd()
{
    uart4_send_buff[0] = 0xAA;
    uart4_send_buff[1] = 0xAA;
    uart4_send_buff[2] = cur_tran_station;
    uart4_send_buff[3] = UART_CMD_ENTER_BOOT;
    uart4_send_buff[4] = sum_check(uart4_send_buff,4);
    uart4_send_count = 5;
    uart4_send();
    uart4_recv_timeout = 1000;
    uart4_send_retry = 0;
    uart4_commu_state = RECV_DATA;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send boot command to:%d\n",cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}
void uart_send_erase_flash_cmd()
{
    uart4_send_buff[0] = 0xAA;
    uart4_send_buff[1] = 0xAA;
    uart4_send_buff[2] = cur_tran_station;
    uart4_send_buff[3] = UART_CMD_ERASE_FLASH;
    uart4_send_buff[4] = sum_check(uart4_send_buff,4);
    uart4_send_count = 5;
    uart4_send();
    uart4_recv_timeout = 5000;
    uart4_send_retry = 0;
    uart4_commu_state = RECV_DATA;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send erase flash command to:%d\n",cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}
void uart_send_one_pack_data()
{
    u8* pDataSrc = (u8*)AppTempAddress;
    u16 send_pack_len;
    u16 send_len;
    
    if(pk_cur_num < pk_tot_num)
    {
        send_pack_len = UART_SEND_DATA_PK_SIZE;
    }
    else
    {
        send_pack_len = file_tot_bytes - (pk_tot_num-1)*UART_SEND_DATA_PK_SIZE;
    }
    pDataSrc += (pk_cur_num-1)*UART_SEND_DATA_PK_SIZE;
    send_len = send_pack_len + 9;
    
    uart4_send_buff[0] = 0xAA;
    uart4_send_buff[1] = 0xAA;
    uart4_send_buff[2] = cur_tran_station;
    uart4_send_buff[3] = UART_CMD_TRAN_DATA;
    uart4_send_buff[4] = pk_tot_num;
    uart4_send_buff[5] = pk_cur_num;
    uart4_send_buff[6] = send_pack_len&0xFF;
    uart4_send_buff[7] = (send_pack_len>>8)&0xFF;
    memcpy(uart4_send_buff+8,pDataSrc,send_pack_len);
    uart4_send_buff[send_len-1] = sum_check(uart4_send_buff,send_len-1);
    uart4_send_count = send_len;
    uart4_send();
    uart4_recv_timeout = 2000;
    uart4_send_retry = 0;
    uart4_commu_state = RECV_DATA;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send pack %d of %d to:%d\n",pk_cur_num,pk_tot_num,cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}
void uart_send_tran_data_cmd()
{
    if(pk_cur_num > pk_tot_num)//完成烧写一个站
    {
        cur_tran_station++;
        pk_cur_num = 1;
        g_download_status = DLOAD_TRAN_INIT;
        return;
    }
    uart_send_one_pack_data();
}
u8 uart_recv_frame_check(void)
{
    if(local_station > 1)//从机
    {
        if(uart4_recv_buff[0]!=0xAA || uart4_recv_buff[1]!=0xAA)
            return 0;
        if(uart4_recv_buff[2]!=local_station)
            return 0;
    }
    else if(local_station == 1)//主机
    {
        if(uart4_recv_buff[0]!=0x55 || uart4_recv_buff[1]!=0x55)
            return 0;
        if(uart4_recv_buff[2]!=cur_tran_station)
            return 0;
    }
    else
    {
        return 0;
    }
    if(uart4_recv_buff[uart4_recv_count-1]!=sum_check(uart4_recv_buff,uart4_recv_count-1))
        return 0;
    return 1;
}
void uart_recv_process(void)
{
    u8 recv_cmd;
    
    if(uart_recv_frame_check() !=1)
    {
        return;
    }
    recv_cmd = uart4_recv_buff[3];
    if(local_station == 1)//主机
    {
        if(recv_cmd == UART_CMD_ENTER_BOOT && g_download_status == DLOAD_TRAN_INIT)
        {
            print_msg_len = sprintf((char*)print_msg_buff,"Reply boot command from:%d\n",cur_tran_station);
            udp_send_message(print_msg_buff,print_msg_len);
            
            g_download_status = DLOAD_TRAN_ERASE;
        }
        else if(recv_cmd == UART_CMD_ERASE_FLASH && g_download_status == DLOAD_TRAN_ERASE)
        {
            print_msg_len = sprintf((char*)print_msg_buff,"Reply erase flash command from:%d\n",cur_tran_station);
            udp_send_message(print_msg_buff,print_msg_len);
            
            g_download_status = DLOAD_TRAN_PROG;
        }
        else if(recv_cmd == UART_CMD_TRAN_DATA && g_download_status == DLOAD_TRAN_PROG)
        {
            print_msg_len = sprintf((char*)print_msg_buff,"Reply pack %d of %d from:%d\n",pk_cur_num,pk_tot_num,cur_tran_station);
            udp_send_message(print_msg_buff,print_msg_len);
            
            pk_cur_num++;
        }
    }
    else//从机
    {
        if(recv_cmd == UART_CMD_ENTER_BOOT)
        {
            g_download_status = DLOAD_TRAN_INIT;
            slave_reply_cmd = UART_CMD_ENTER_BOOT;
            slave_recv_pk_num = 0;
        }
        else if(recv_cmd == UART_CMD_ERASE_FLASH)
        {
            g_download_status = DLOAD_TRAN_ERASE;
            slave_reply_cmd = UART_CMD_ERASE_FLASH;
            slave_recv_pk_num = 0;
        }
        else if(recv_cmd == UART_CMD_TRAN_DATA && (g_download_status == DLOAD_TRAN_ERASE||g_download_status == DLOAD_TRAN_PROG))
        {
            pk_tot_num = uart4_recv_buff[4];
            pk_cur_num = uart4_recv_buff[5];
            pk_len     = uart4_recv_buff[6]|(uart4_recv_buff[7]<<8);
            if(pk_len > UART_SEND_DATA_PK_SIZE)
                return;
            if(pk_cur_num > pk_tot_num)
                return;
            if((slave_recv_pk_num+1) != pk_cur_num)
            {
                if(slave_recv_pk_num == pk_cur_num)
                {
                    g_download_status = DLOAD_TRAN_PROG;
                    slave_reply_cmd = UART_CMD_TRAN_DATA;
                }
            }
            else
            {
                g_download_status = DLOAD_TRAN_PROG;
                slave_reply_cmd = UART_CMD_TRAN_DATA;
            }
        }
        else
        {
            g_download_status = DLOAD_INIT;
            slave_reply_cmd = UART_CMD_NULL;
        }
    }
    uart4_commu_state = SEND_DATA;
}
//主机转发数据
void uart_transmit_process(void)
{
    if(g_download_status == DLOAD_INIT || g_download_status == DLOAD_PARA)
    {
        return;
    }
    if(file_tot_bytes == 0)
    {
        g_download_status = DLOAD_INIT;
        return;
    }
    if(cur_tran_station >= (sDLoad_Para_Data.st_addr + sDLoad_Para_Data.addr_num))
    {
        g_download_status = DLOAD_INIT;
        return;
    }
    if(g_download_status == DLOAD_TRAN_INIT && uart4_commu_state == SEND_DATA)
    {
        uart_send_boot_cmd();
    }
    if(g_download_status == DLOAD_TRAN_ERASE && uart4_commu_state == SEND_DATA)
    {
        uart_send_erase_flash_cmd();
    }
    if(g_download_status == DLOAD_TRAN_PROG && uart4_commu_state == SEND_DATA)
    {
        uart_send_tran_data_cmd();
    }
}
void uart_reply_boot_cmd()
{
    uart4_send_buff[0] = 0x55;
    uart4_send_buff[1] = 0x55;
    uart4_send_buff[2] = local_station;
    uart4_send_buff[3] = UART_CMD_ENTER_BOOT;
    uart4_send_buff[4] = sum_check(uart4_send_buff,4);
    uart4_send_count = 5;
    uart4_send();
    uart4_commu_state = RECV_DATA;
}
void uart_reply_erase_flash_cmd()
{
    uart4_send_buff[0] = 0x55;
    uart4_send_buff[1] = 0x55;
    uart4_send_buff[2] = local_station;
    uart4_send_buff[3] = UART_CMD_ERASE_FLASH;
    uart4_send_buff[4] = sum_check(uart4_send_buff,4);
    uart4_send_count = 5;
    uart4_send();
    uart4_commu_state = RECV_DATA;
}
void uart_reply_tran_data_cmd()
{
    uart4_send_buff[0] = 0x55;
    uart4_send_buff[1] = 0x55;
    uart4_send_buff[2] = local_station;
    uart4_send_buff[3] = UART_CMD_TRAN_DATA;
    uart4_send_buff[4] = pk_cur_num;
    uart4_send_buff[5] = sum_check(uart4_send_buff,5);
    uart4_send_count = 6;
    uart4_send();
    uart4_commu_state = RECV_DATA;
}
void erase_user_app_flash()
{
    FLASH_Status f_status = FLASH_COMPLETE;
    u32 pageNum;
    u32 eraseCnt;
    
    pageNum = FLASH_PagesMask(0x18000);//擦除96K
    FLASH_Unlock();
    f_status = FLASH_ErasePage(UserAppEnFlagAddress);
    for (eraseCnt = 0; (eraseCnt < pageNum) && (f_status == FLASH_COMPLETE); eraseCnt++)
    {
        f_status = FLASH_ErasePage(ApplicationAddress + (PAGE_SIZE * eraseCnt));
    }
    FLASH_Lock();
}
void program_user_app_flash()
{
    u32* source;
    u32  flashDst;
    u16  n;
    
    if(slave_recv_pk_num == pk_cur_num)
        return;
    else
        slave_recv_pk_num = pk_cur_num;
    
    source = (u32*)(uart4_recv_buff+8);
    flashDst = ApplicationAddress + (pk_cur_num-1)*UART_SEND_DATA_PK_SIZE;
    
    for (n = 0; n < pk_len; n += 4)
    {
        FLASH_Unlock();
        FLASH_ProgramWord(flashDst, *source);
        FLASH_Lock();
        if(*(u32 *)flashDst != *source)
        {
            g_download_status = DLOAD_INIT;
            return;
        }
        flashDst += 4;
        source += 1;
    }
    if(pk_cur_num == pk_tot_num)
    {
        FLASH_Unlock();
        FLASH_ProgramHalfWord(UserAppEnFlagAddress,0xAA);
        FLASH_Lock();
        g_download_status = DLOAD_INIT;
    }
}
//从机接收应答
void uart_reply_process(void)
{
    while(1)
    {
        if(slave_reply_cmd == UART_CMD_ENTER_BOOT && uart4_commu_state == SEND_DATA)
        {
            uart_reply_boot_cmd();
            slave_reply_cmd = UART_CMD_NULL;
        }
        if(slave_reply_cmd == UART_CMD_ERASE_FLASH && uart4_commu_state == SEND_DATA)
        {
            erase_user_app_flash();
            uart_reply_erase_flash_cmd();
            slave_reply_cmd = UART_CMD_NULL;
        }
        if(slave_reply_cmd == UART_CMD_TRAN_DATA && uart4_commu_state == SEND_DATA)
        {
            program_user_app_flash();
            uart_reply_tran_data_cmd();
            slave_reply_cmd = UART_CMD_NULL;
        }
    }
}
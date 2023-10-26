#include "main.h"
#include <stdio.h>

sCanFrameExt canTxMsg;

u8  cur_tran_station;
u16 pk_tot_num;
u16 pk_cur_num;
u16 pk_len;
u8  trans_flag = 0;
u16 trans_timeout = 0;

u16 can_resend_delay = 0;
u8  can_resend_cnt = 0;
u8  can_send_flag = 0;
u8  can_send_frame_start = 0;

u8  can_send_buff[300];
u16 can_send_tot = 0;
u16 can_send_len = 0;

u8  can_recv_buff[CAN_RX_BUFF_SIZE];
u16 can_recv_len = 0;
u8  g_SegPolo = CAN_SEG_POLO_NONE;
u8  g_SegNum = 0;
u16 g_SegBytes = 0;

u16 slave_reply_cmd = DLOAD_CMD_NULL;
u16 slave_recv_pk_num = 0;

u8 SlaveApp_UpdateFlag = 0;


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
void can_recv_timeout()
{
    if((can_resend_delay != 0) && (trans_flag == 1))
    {
        can_resend_delay--;
        if(can_resend_delay == 0)
        {
            if(can_resend_cnt >=3)
            {
                g_download_status = DLOAD_INIT;
                can_resend_cnt = 0;
            }
            else
            {
                can_resend_cnt++;
            }
            trans_flag = 0;
        }
    }
}
void can_bus_send_one_frame(sCanFrameExt sTxMsg)
{
    CanTxMsg TxMessage;
    u16 retry = 0;
    
    TxMessage.ExtId = (sTxMsg.extId.src_id)|((sTxMsg.extId.func_id&0xF)<<8)|((sTxMsg.extId.seg_num&0xFF)<<12)
                     |((sTxMsg.extId.seg_polo&0x3)<<20)|((sTxMsg.extId.dst_id&0x7F)<<22);
    TxMessage.IDE = CAN_ID_EXT;
    TxMessage.RTR = CAN_RTR_DATA;
    TxMessage.DLC = sTxMsg.data_len;
    memcpy(TxMessage.Data, sTxMsg.data, TxMessage.DLC);
    
    while(CAN_Transmit(CAN1,&TxMessage)==CAN_NO_MB)
    {
        retry++;
        if(retry == 10000)
        {
            break;
        }
    }
}
//CAN总线分包传输
void can_send_frame_process()
{
    if(can_send_flag)
    {
        canTxMsg.extId.func_id  = CAN_FUNC_ID_BOOT_MODE;
        canTxMsg.extId.src_id   = 1;
        canTxMsg.extId.dst_id   = cur_tran_station;
        
        if(can_send_tot <= CAN_PACK_DATA_LEN)//不需分段传输
        {
            canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
            canTxMsg.extId.seg_num  = 0;
            canTxMsg.data_len = can_send_tot;
            memcpy(canTxMsg.data, can_send_buff, canTxMsg.data_len);
            can_bus_send_one_frame(canTxMsg);
            
            can_resend_delay = trans_timeout;
            can_send_flag = 0;
        }
        else//需要分段传输
        {
            if(can_send_frame_start)
            {
                canTxMsg.extId.seg_polo = CAN_SEG_POLO_FIRST;
                canTxMsg.extId.seg_num  = 0;
                canTxMsg.data_len = CAN_PACK_DATA_LEN;
                memcpy(canTxMsg.data, can_send_buff, canTxMsg.data_len);
                can_bus_send_one_frame(canTxMsg);
                
                can_send_len = CAN_PACK_DATA_LEN;
                can_send_frame_start = 0;
            }
            else
            {
                if(can_send_len + CAN_PACK_DATA_LEN < can_send_tot)
                {
                    canTxMsg.extId.seg_polo = CAN_SEG_POLO_MIDDLE;
                    canTxMsg.extId.seg_num ++;
                    canTxMsg.data_len = CAN_PACK_DATA_LEN;
                    memcpy(canTxMsg.data, can_send_buff+can_send_len, canTxMsg.data_len);
                    can_bus_send_one_frame(canTxMsg);
                    
                    can_send_len += CAN_PACK_DATA_LEN;
                }
                else
                {
                    canTxMsg.extId.seg_polo = CAN_SEG_POLO_FINAL;
                    canTxMsg.extId.seg_num ++;
                    canTxMsg.data_len = can_send_tot-can_send_len;
                    memcpy(canTxMsg.data, can_send_buff+can_send_len, canTxMsg.data_len);
                    can_bus_send_one_frame(canTxMsg);
                    
                    can_resend_delay = trans_timeout;
                    can_send_flag = 0;
                }
            }
        }
    }
}


void master_can_bus_frame_receive(CanRxMsg rxMsg)
{
    u8 recv_cmd = rxMsg.Data[3];
    
    if(recv_cmd == DLOAD_CMD_ENTER_BOOT && g_download_status == DLOAD_TRAN_INIT)
    {
        print_msg_len = sprintf((char*)print_msg_buff,"Reply boot command from:%d\n",cur_tran_station);
        udp_send_message(print_msg_buff,print_msg_len);
        
        g_download_status = DLOAD_TRAN_ERASE;
    }
    else if(recv_cmd == DLOAD_CMD_ERASE_FLASH && g_download_status == DLOAD_TRAN_ERASE)
    {
        print_msg_len = sprintf((char*)print_msg_buff,"Reply erase flash command from:%d\n",cur_tran_station);
        udp_send_message(print_msg_buff,print_msg_len);
        
        g_download_status = DLOAD_TRAN_PROG;
    }
    else if(recv_cmd == DLOAD_CMD_TRAN_DATA && g_download_status == DLOAD_TRAN_PROG)
    {
        print_msg_len = sprintf((char*)print_msg_buff,"Reply pack %d of %d from:%d\n",pk_cur_num,pk_tot_num,cur_tran_station);
        udp_send_message(print_msg_buff,print_msg_len);
        
        pk_cur_num++;
    }
    
    trans_flag = 0;
    can_resend_delay = 0;
    can_resend_cnt = 0;
}

void host_send_boot_cmd()
{
    can_send_buff[0] = 0xAA;
    can_send_buff[1] = 0xAA;
    can_send_buff[2] = cur_tran_station;
    can_send_buff[3] = DLOAD_CMD_ENTER_BOOT;
    can_send_buff[4] = sum_check(can_send_buff,4);
    can_send_tot = 5;
    
    can_send_flag = 1;
    trans_timeout = 1000;
    trans_flag = 1;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send boot command to:%d\n",cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}

void host_send_erase_flash_cmd()
{
    can_send_buff[0] = 0xAA;
    can_send_buff[1] = 0xAA;
    can_send_buff[2] = cur_tran_station;
    can_send_buff[3] = DLOAD_CMD_ERASE_FLASH;
    can_send_buff[4] = sum_check(can_send_buff,4);
    can_send_tot = 5;
    
    can_send_flag = 1;
    trans_timeout = 5000;
    trans_flag = 1;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send erase flash command to:%d\n",cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}

void host_send_tran_data_cmd()
{
    u8* pDataSrc = (u8*)AppTempAddress;
    u16 send_pack_len;
    u16 send_len;
    
    if(pk_cur_num > pk_tot_num)//完成烧写一个站
    {
        cur_tran_station++;
        pk_cur_num = 1;
        g_download_status = DLOAD_TRAN_INIT;
        return;
    }
    if(pk_cur_num < pk_tot_num)
    {
        send_pack_len = CAN_SEND_DATA_PK_SIZE;
    }
    else
    {
        send_pack_len = file_tot_bytes - (pk_tot_num-1)*CAN_SEND_DATA_PK_SIZE;
    }
    pDataSrc += (pk_cur_num-1)*CAN_SEND_DATA_PK_SIZE;
    send_len = send_pack_len + 9;
    
    can_send_buff[0] = 0xAA;
    can_send_buff[1] = 0xAA;
    can_send_buff[2] = cur_tran_station;
    can_send_buff[3] = DLOAD_CMD_TRAN_DATA;
    can_send_buff[4] = pk_tot_num;
    can_send_buff[5] = pk_cur_num;
    can_send_buff[6] = send_pack_len&0xFF;
    can_send_buff[7] = (send_pack_len>>8)&0xFF;
    memcpy(can_send_buff+8,pDataSrc,send_pack_len);
    can_send_buff[send_len-1] = sum_check(can_send_buff,send_len-1);
    can_send_tot = send_len;
    
    can_send_frame_start = 1;
    can_send_flag = 1;
    trans_timeout = 2000;
    trans_flag = 1;
    
    print_msg_len = sprintf((char*)print_msg_buff,"Send pack %d of %d to:%d\n",pk_cur_num,pk_tot_num,cur_tran_station);
    udp_send_message(print_msg_buff,print_msg_len);
}

//主机转发数据
void host_transmit_process(void)
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
    if(cur_tran_station >= (sDLoad_Para_Data.st_addr + sDLoad_Para_Data.addr_num))//转发完所有的站点
    {
        g_download_status = DLOAD_INIT;
        if(readFlash_uint(ApplicationAddress)!=0xFFFF)
        {
          FLASH_Unlock();
          FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
          if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE)
         {
           FLASH_ProgramHalfWord(UserAppEnFlagAddress,0x00AA);
         }
          FLASH_Lock();
          Delay(2000);  //2s
          NVIC_SystemReset();
        }
        return;
    }
    if(g_download_status == DLOAD_TRAN_INIT && trans_flag == 0)
    {
        host_send_boot_cmd();
    }
    if(g_download_status == DLOAD_TRAN_ERASE && trans_flag == 0)
    {
        host_send_erase_flash_cmd();
    }
    if(g_download_status == DLOAD_TRAN_PROG && trans_flag == 0)
    {
        host_send_tran_data_cmd();
    }
}

///////   以下处理从机接收   ///////
u8 slave_recv_frame_check(void)
{
    if(can_recv_len < 5)
        return 0;
    if(can_recv_buff[0]!=0xAA || can_recv_buff[1]!=0xAA)
        return 0;
    if(can_recv_buff[2]!=Local_Station)
        return 0;
    if(can_recv_buff[can_recv_len-1]!=sum_check(can_recv_buff,can_recv_len-1))
        return 0;
    return 1;
}

void slave_recv_process(void)
{
    u8 recv_cmd;
    
    if(slave_recv_frame_check() !=1)
    {
        return;
    }
    recv_cmd = can_recv_buff[3];
    
    if(recv_cmd == DLOAD_CMD_ENTER_BOOT)
    {
        slave_reply_cmd = DLOAD_CMD_ENTER_BOOT;
        slave_recv_pk_num = 0;
    }
    else if(recv_cmd == DLOAD_CMD_ERASE_FLASH)
    {
        slave_reply_cmd = DLOAD_CMD_ERASE_FLASH;
        slave_recv_pk_num = 0;
    }
    else if(recv_cmd == DLOAD_CMD_TRAN_DATA)
    {
        pk_tot_num = can_recv_buff[4];
        pk_cur_num = can_recv_buff[5];
        pk_len     = can_recv_buff[6]|(can_recv_buff[7]<<8);
        if(pk_len > CAN_RX_DATA_PK_SIZE)
            return;
        if(pk_cur_num > pk_tot_num)
            return;
        if((slave_recv_pk_num+1) != pk_cur_num)
        {
            if(slave_recv_pk_num == pk_cur_num)
            {
                slave_reply_cmd = DLOAD_CMD_TRAN_DATA;
            }
        }
        else
        {
            slave_reply_cmd = DLOAD_CMD_TRAN_DATA;
        }
    }
    else
    {
        slave_reply_cmd = DLOAD_CMD_NULL;
    }
}

void slave_can_bus_frame_receive(CanRxMsg rxMsg)
{
    sCanFrameExtID extID;
    
    extID.func_id  = (rxMsg.ExtId>>8)&0xF;
    extID.dst_id   = (rxMsg.ExtId>>22)&0x7F;
    extID.seg_polo = (rxMsg.ExtId>>20)&0x3;
    extID.seg_num  = (rxMsg.ExtId>>12)&0xFF;
    
    if(extID.dst_id != Local_Station) return;
    if(extID.func_id != CAN_FUNC_ID_BOOT_MODE) return;
    
    if(extID.seg_polo == CAN_SEG_POLO_NONE)
    {
        memcpy(can_recv_buff,rxMsg.Data,rxMsg.DLC);
        can_recv_len = rxMsg.DLC;
        slave_recv_process();
    }
    else if(extID.seg_polo == CAN_SEG_POLO_FIRST)
    {
        memcpy(can_recv_buff,rxMsg.Data,rxMsg.DLC);
        
        g_SegPolo  = CAN_SEG_POLO_FIRST;
        g_SegNum   = extID.seg_num;
        g_SegBytes = rxMsg.DLC;
    }
    else if(extID.seg_polo == CAN_SEG_POLO_MIDDLE)
    {
        if(   (g_SegPolo == CAN_SEG_POLO_FIRST) 
           && (extID.seg_num == (g_SegNum+1)) 
           && ((g_SegBytes+rxMsg.DLC) <= CAN_RX_BUFF_SIZE) )
        {
            memcpy(can_recv_buff+g_SegBytes,rxMsg.Data,rxMsg.DLC);
            
            g_SegNum ++;
            g_SegBytes += rxMsg.DLC;
        }
    }
    else if(extID.seg_polo == CAN_SEG_POLO_FINAL)
    {
        if(   (g_SegPolo == CAN_SEG_POLO_FIRST) 
           && (extID.seg_num == (g_SegNum+1)) 
           && ((g_SegBytes+rxMsg.DLC) <= CAN_RX_BUFF_SIZE) )
        {
            memcpy(can_recv_buff+g_SegBytes,rxMsg.Data,rxMsg.DLC);
            can_recv_len = g_SegBytes + rxMsg.DLC;
            slave_recv_process();
            
            g_SegPolo = CAN_SEG_POLO_NONE;
            g_SegNum = 0;
            g_SegBytes = 0;
        }
    }
}
void slave_reply_boot_cmd()
{
    sCanFrameExt canTxMsg;
    
    canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
    canTxMsg.extId.seg_num  = 0;
    canTxMsg.extId.func_id  = CAN_FUNC_ID_BOOT_MODE;
    canTxMsg.extId.src_id   = Local_Station;
    canTxMsg.extId.dst_id   = 1;
    canTxMsg.data_len = 5;
    canTxMsg.data[0] = 0x55;
    canTxMsg.data[1] = 0x55;
    canTxMsg.data[2] = Local_Station;
    canTxMsg.data[3] = DLOAD_CMD_ENTER_BOOT;
    canTxMsg.data[4] = sum_check(canTxMsg.data,4);
    
    can_bus_send_one_frame(canTxMsg);
}
void slave_reply_erase_flash_cmd()
{
    sCanFrameExt canTxMsg;
    
    canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
    canTxMsg.extId.seg_num  = 0;
    canTxMsg.extId.func_id  = CAN_FUNC_ID_BOOT_MODE;
    canTxMsg.extId.src_id   = Local_Station;
    canTxMsg.extId.dst_id   = 1;
    canTxMsg.data_len = 5;
    canTxMsg.data[0] = 0x55;
    canTxMsg.data[1] = 0x55;
    canTxMsg.data[2] = Local_Station;
    canTxMsg.data[3] = DLOAD_CMD_ERASE_FLASH;
    canTxMsg.data[4] = sum_check(canTxMsg.data,4);
    
    can_bus_send_one_frame(canTxMsg);
}
void slave_reply_tran_data_cmd()
{
    sCanFrameExt canTxMsg;
    
    canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
    canTxMsg.extId.seg_num  = 0;
    canTxMsg.extId.func_id  = CAN_FUNC_ID_BOOT_MODE;
    canTxMsg.extId.src_id   = Local_Station;
    canTxMsg.extId.dst_id   = 1;
    canTxMsg.data_len = 6;
    canTxMsg.data[0] = 0x55;
    canTxMsg.data[1] = 0x55;
    canTxMsg.data[2] = Local_Station;
    canTxMsg.data[3] = DLOAD_CMD_TRAN_DATA;
    canTxMsg.data[4] = pk_cur_num;
    canTxMsg.data[5] = sum_check(canTxMsg.data,5);
    
    can_bus_send_one_frame(canTxMsg);
}
void erase_user_app_flash()
{
    FLASH_Status f_status = FLASH_COMPLETE;
    u32 pageNum;
    u32 eraseCnt;
    
    //pageNum = FLASH_PagesMask(0x18000);//擦除96K
    pageNum = FLASH_PagesMask(0x17800);//擦除94K
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
    
    source = (u32*)(can_recv_buff+8);
    flashDst = ApplicationAddress + (pk_cur_num-1)*CAN_RX_DATA_PK_SIZE;
    
    for (n = 0; n < pk_len; n += 4)
    {
        FLASH_Unlock();
        FLASH_ProgramWord(flashDst, *source);
        FLASH_Lock();
        if(*(u32 *)flashDst != *source)
        {
            return;
        }
        flashDst += 4;
        source += 1;
    }
    if(pk_cur_num == pk_tot_num)
    {
         FLASH_Unlock();
         FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
        if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE)
        {
          FLASH_ProgramHalfWord(UserAppEnFlagAddress,0x00AA);
        }
         FLASH_Lock();
         SlaveApp_UpdateFlag = 1;
    }
}
void can_bus_reply_process(void)
{
  while(1)
  {
        if(slave_reply_cmd == DLOAD_CMD_ENTER_BOOT)
        {
            slave_reply_boot_cmd();
            slave_reply_cmd = DLOAD_CMD_NULL;
        }
        else if(slave_reply_cmd == DLOAD_CMD_ERASE_FLASH)
        {
            erase_user_app_flash();
            slave_reply_erase_flash_cmd();
            slave_reply_cmd = DLOAD_CMD_NULL;
        }
        else if(slave_reply_cmd == DLOAD_CMD_TRAN_DATA)
        {
            program_user_app_flash();
            slave_reply_tran_data_cmd();
            slave_reply_cmd = DLOAD_CMD_NULL;
            if(SlaveApp_UpdateFlag == 1)
          {
               SlaveApp_UpdateFlag = 0;
                Delay(2000);  //2s
                NVIC_SystemReset();
          }
        }
  }
}
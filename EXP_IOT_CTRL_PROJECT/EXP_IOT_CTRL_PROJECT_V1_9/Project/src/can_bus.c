#include "main.h"

#define canSendQueueSize  50
sCanFrameExt canSendQueueBuff[canSendQueueSize];
sCAN_SEND_QUEUE canSendQueue;

u8  can_send_buff[CAN_TX_BUFF_SIZE];
u16 can_send_len = 0;
u8  can_recv_buff[CAN_RX_BUFF_SIZE];
u16 can_recv_len = 0;
u8  g_SegPolo = CAN_SEG_POLO_NONE;
u8  g_SegNum = 0;
u16 g_SegBytes = 0;

u16 slave_cnt = 0;

u16 pre_slave_count[50];
u16 cur_slave_count[50];
u16 slave_err_cnt[50];
u16 upload_can_err_buff[300];

extern u16 Cur_ActualHz;


void InitCanSendQueue(void)
{
    sCAN_SEND_QUEUE *q;
    
    q = &canSendQueue;
    q->maxSize = canSendQueueSize;
    q->queue = canSendQueueBuff;
    q->front = q->rear = 0;
}

void AddCanSendData2Queue(sCanFrameExt x)
{
    sCAN_SEND_QUEUE *q = &canSendQueue;
    //队列满
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // 求出队尾的下一个位置
    q->queue[q->rear] = x; // 把x的值赋给新的队尾
}

u8 can_bus_send_one_frame(sCanFrameExt sTxMsg)
{
    CanTxMsg TxMessage;
    
    TxMessage.ExtId = (sTxMsg.extId.src_id)|((sTxMsg.extId.func_id&0xF)<<8)|((sTxMsg.extId.seg_num&0xFF)<<12)|((sTxMsg.extId.seg_polo&0x3)<<20) | ((sTxMsg.extId.seg_polo & 0x7F) << 22);
    TxMessage.IDE = CAN_ID_EXT;
    TxMessage.RTR = CAN_RTR_DATA;
    TxMessage.DLC = sTxMsg.data_len;
    memcpy(TxMessage.Data, sTxMsg.data, TxMessage.DLC);
    
    return CAN_Transmit(CAN1,&TxMessage);
}

void para_data_recv_process(u8* pbuf,u16 recv_len)
{
    u16 data_len;
    u8  cmd;
    
    data_len = pbuf[0]|(pbuf[1]>>8);
    cmd = pbuf[2];
    
    if(data_len != recv_len)
        return;
    
    switch(cmd)
    {
    case 0x1://读参数
        //从机把本机参数通过CAN发送给主机
        can_bus_reply_read_user_paras(&user_paras_local);
        break;
    case 0x91://读参数回复
        //主机收到从机参数后通过网络转发给上位机
        user_paras_slave = *((USER_PARAS_T *)(pbuf+3));
        AddSendMsgToQueue(REPLY_RECV_MSG_READ_PARA_CMD_TYPE);
        break;
    case 0x2://写参数
        //从机收到参数后保存到flash并返回烧写完成命令给主机
        write_user_paras((u16*)(pbuf+9));
        read_user_paras();
        can_bus_reply_write_user_paras(&user_paras_local);
        break;
    case 0x92://写参数回复
        //主机收到从机写参数完成命令通过网络转发给上位机
        user_paras_slave = *((USER_PARAS_T *)(pbuf+3));
        AddSendMsgToQueue(REPLY_RECV_MSG_WRITE_PARA_CMD_TYPE);
        break;
    default:
        break;
    }
}

void can_bus_frame_receive(CanRxMsg rxMsg)
{
    sCanFrameExtID extID;
    u8 buff[2];
    
    extID.seg_polo = (rxMsg.ExtId>>20)&0x3;
    extID.seg_num  = (rxMsg.ExtId>>12)&0xFF;
    extID.func_id  = (rxMsg.ExtId>>8)&0xF;
    extID.src_id = rxMsg.ExtId & 0xFF;
    extID.dst_id  = (rxMsg.ExtId>>22)&0x7F;
    
    if(extID.func_id == CAN_FUNC_ID_BOOT_MODE && extID.dst_id == local_station)
    {
       // BKP_WriteBackupRegister(BKP_DR8, 0x55);
       // NVIC_SystemReset();
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
        if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE)
        {
            FLASH_ProgramHalfWord(UserAppEnFlagAddress,0x0000);
        }
        FLASH_Lock();
        NVIC_SystemReset();
    }
    if(extID.func_id == CAN_FUNC_ID_PARA_DATA && (extID.src_id == local_station || isHost == 1))
    {
        if(extID.seg_polo == CAN_SEG_POLO_NONE)
        {
            memcpy(can_recv_buff,rxMsg.Data,rxMsg.DLC);
            can_recv_len = rxMsg.DLC;
            para_data_recv_process(can_recv_buff,can_recv_len);
        }
        else if(extID.seg_polo == CAN_SEG_POLO_FIRST)
        {
            memcpy(can_recv_buff,rxMsg.Data,rxMsg.DLC);
            
            g_SegPolo = CAN_SEG_POLO_FIRST;
            g_SegNum = extID.seg_num;
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
                para_data_recv_process(can_recv_buff,can_recv_len);
                
                g_SegPolo = CAN_SEG_POLO_NONE;
                g_SegNum = 0;
                g_SegBytes = 0;
            }
        }
    }
    else if(extID.func_id == CAN_FUNC_ID_START_CMD)
    {
        if(g_remote_start_status != rxMsg.Data[0])
        {
            if(rxMsg.Data[0] == 1)//启动
            {
                if (runctrl.start_cnt != 0 || start_delay_time_cnt !=0) {
                    runctrl.stop_cnt = 0;
                    stop_delay_time_cnt = 0;
                }
                else {
                    runctrl.start_cnt = DEFAULT_TIME;
                    startflash.flash_reg = FLASH_TIME;
                    runctrl.stop_cnt = 0;
                    stop_delay_time_cnt = 0;
                }
            }
            else//停止
            {
                if (runctrl.stop_cnt != 0 || stop_delay_time_cnt !=0) {
                    runctrl.start_cnt = 0;
                    start_delay_time_cnt = 0;
                }
                else {
                    runctrl.stop_cnt = 10;
                    runctrl.start_cnt = 0;
                    startflash.flash_reg = FLASH_TIME;
                    start_delay_time_cnt = 0;
                }
            }

        }

        g_remote_speed_status = rxMsg.Data[1];
    }
    else if(extID.func_id == CAN_FUNC_ID_MODULE_STATUS)
    {
        if(extID.src_id == user_paras_local.Up_Stream_No)
        {
            g_link_up_stream_status = rxMsg.Data[0]&0x1;
        }
        else if(extID.src_id == user_paras_local.Down_Stream_No)
        {
            g_link_down_stream_status = rxMsg.Data[0]&0x1;
        }
        if(isHost)//主站
        {
            module_status_buffer[extID.src_id].flag = 1;
            module_status_buffer[extID.src_id].status = rxMsg.Data[0];
            module_status_buffer[extID.src_id].alarm = rxMsg.Data[1];
            module_status_buffer[extID.src_id].cur_speed = rxMsg.Data[2]|(rxMsg.Data[3]<<8);
            module_status_buffer[extID.src_id].inverter_hz = rxMsg.Data[4]|(rxMsg.Data[5]<<8);
            cur_slave_count[extID.src_id] = rxMsg.Data[6] | (rxMsg.Data[7] << 8);

            if (cur_slave_count[extID.src_id] == pre_slave_count[extID.src_id]+1) {
                pre_slave_count[extID.src_id] = cur_slave_count[extID.src_id];
            }
            else {
                slave_err_cnt[extID.src_id]++;
                pre_slave_count[extID.src_id] = cur_slave_count[extID.src_id];
            }
            if(bModeInfo.button_state == 1){
              
              if ((module_status_buffer[extID.src_id].status & 0x1) != runstate.runstatus) {
                  buff[0] = runstate.runstatus;
                  buff[1] = runstate.runspeed;
                  can_bus_send_start_cmd(&buff[0]);
              }
            }


        }
    }
}
void can_bus_send_msg(u8* pbuf, u16 send_tot_len, u8 func_id, u8 src_id)
{
    sCanFrameExt canTxMsg;
    u16 can_send_len;
    
    canTxMsg.extId.func_id  = func_id;
    canTxMsg.extId.src_id   = src_id;
    if(send_tot_len <= CAN_PACK_DATA_LEN)//不需分段传输
    {
        canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
        canTxMsg.extId.seg_num  = 0;
        canTxMsg.data_len = send_tot_len;
        memcpy(canTxMsg.data, pbuf, canTxMsg.data_len);
        AddCanSendData2Queue(canTxMsg);
    }
    else//需要分段传输
    {
        canTxMsg.extId.seg_polo = CAN_SEG_POLO_FIRST;
        canTxMsg.extId.seg_num  = 0;
        canTxMsg.data_len = CAN_PACK_DATA_LEN;
        memcpy(canTxMsg.data, pbuf, canTxMsg.data_len);
        AddCanSendData2Queue(canTxMsg);
        can_send_len = CAN_PACK_DATA_LEN;
        while(1)
        {
            if(can_send_len + CAN_PACK_DATA_LEN < send_tot_len)
            {
                canTxMsg.extId.seg_polo = CAN_SEG_POLO_MIDDLE;
                canTxMsg.extId.seg_num ++;
                canTxMsg.data_len = CAN_PACK_DATA_LEN;
                memcpy(canTxMsg.data, pbuf+can_send_len, canTxMsg.data_len);
                AddCanSendData2Queue(canTxMsg);
                can_send_len += CAN_PACK_DATA_LEN;
            }
            else
            {
                canTxMsg.extId.seg_polo = CAN_SEG_POLO_FINAL;
                canTxMsg.extId.seg_num ++;
                canTxMsg.data_len = send_tot_len-can_send_len;
                memcpy(canTxMsg.data, pbuf+can_send_len, canTxMsg.data_len);
                AddCanSendData2Queue(canTxMsg);
                break;
            }
        }
    }
}
void can_bus_send_read_user_paras(u8 station_no)
{
    can_send_len = 3;
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x01;//读参数
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,station_no);
}
void can_bus_reply_read_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x91;//读参数回复
    memcpy(can_send_buff+3,(u8*)user_para,sizeof(USER_PARAS_T));
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,user_para->Station_No);
}
void can_bus_send_write_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x02;//写参数命令
    memcpy(can_send_buff+3,(u8*)user_para,sizeof(USER_PARAS_T));
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,user_para->Station_No);
}
void can_bus_reply_write_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x92;//写参数回复
    memcpy(can_send_buff+3,(u8*)user_para,sizeof(USER_PARAS_T));
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,user_para->Station_No);
}
void can_bus_send_start_cmd(u8* pbuf)
{
    can_send_len = 2;
    can_send_buff[0] = pbuf[0];
    can_send_buff[1] = pbuf[1];
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_START_CMD,local_station);
}
void can_bus_send_module_status()
{
    u8  status;
    u16 cur_speed;
    u16 hz;
    
    status = g_read_start_status | (bModeInfo.button_state<<1) | (bHSpeedInfo.button_state<<2)
                                 | (bPauseInfo.button_state<<3) | (B_PHOTO_1_IN_STATE<<4);
    cur_speed = (u16)(rt_speed*100);
    hz = (u16)Cur_ActualHz;//fwHz;//
    
    slave_cnt++;
    can_send_len = 8;
    can_send_buff[0] = status;
    can_send_buff[1] = g_alarm_type;
    can_send_buff[2] = cur_speed&0xFF;
    can_send_buff[3] = (cur_speed>>8)&0xFF;
    can_send_buff[4] = hz&0xFF;
    can_send_buff[5] = (hz>>8)&0xFF;
    can_send_buff[6] = slave_cnt & 0xFF;
    can_send_buff[7] = (slave_cnt >> 8) & 0xFF;
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_MODULE_STATUS,local_station);
}
void can_send_frame_process()
{
    sCAN_SEND_QUEUE *q = &canSendQueue;
    sCanFrameExt* canTxMsg = NULL;
    u16 front;
    u8  tx_mailbox;
    
    //队列空
    if(q->front == q->rear)
    {
        return;
    }
    front = q->front;
    front = (front + 1) % q->maxSize;
    canTxMsg = (sCanFrameExt*)(&(q->queue[front]));
    tx_mailbox = can_bus_send_one_frame(*canTxMsg);
    if(tx_mailbox != CAN_NO_MB)
    {
        q->front = front;//出列
    }
}

void can_upload_slave_can_err_process(void)
{
    u16 i = 0;
    u16 num = 0;
    for (i = 0; i < 40; i++) {
        if (slave_err_cnt[i] != 0) {
            upload_can_err_buff[4 * num  + 2] = i & 0xFF;
            upload_can_err_buff[4 * num + 1 + 2] = (i>>8) & 0xFF;
            
            upload_can_err_buff[4 * num+2  + 2] = (slave_err_cnt[i]) & 0xFF;
            upload_can_err_buff[4 * num + 3 + 2] = (slave_err_cnt[i]>>8) & 0xFF;
            num++;
        }
    }
    upload_can_err_buff[0] = num & 0xFF;
    upload_can_err_buff[1] = (num >> 8) & 0xFF;
    AddSendMsgToQueue(SEND_MSG_CAN_ERR_TYPE);
}
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
    
    TxMessage.ExtId = (sTxMsg.extId.src_id)|((sTxMsg.extId.func_id&0xF)<<8)|((sTxMsg.extId.seg_num&0xFF)<<12)|((sTxMsg.extId.seg_polo&0x3)<<20);
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
void module_status_recv_process(u8* pbuf,u16 recv_len,u8 src_id)
{
    MODULE_STATUS_T module_status_t;
    u16 data_len;
    u8  belt_num;
    
    data_len = pbuf[0]|(pbuf[1]>>8);
    belt_num = pbuf[2];
    
    if(data_len != recv_len || belt_num == 0)
        return;
    module_status_t.station_no = src_id;
    module_status_t.belt_number = belt_num;
    memcpy((u8*)module_status_t.inverter_status,(u8*)(pbuf+3),sizeof(INVERTER_STATUS_T)*belt_num);
    
    if(isHost)//主站接收从机状态信息
    {
        AddModuleStatusData2Queue(module_status_t);
    }
    if(src_id == user_paras_local.Up_Stream_No)
    {
        g_link_up_stream_status = (module_status_t.inverter_status[belt_num-1].fault_code>>4)&0x1;
    }
    else if(src_id == user_paras_local.Down_Stream_No)
    {
        if(user_paras_local.Belt_Number != 0)
        {
            if(g_link_down_stream_status != ((module_status_t.inverter_status[0].fault_code>>4)&0x1))//下游运行状态改变
            {
                if(g_link_down_stream_status == 1)
                {
                    Linkage_Stream_Ctrl_Handle(user_paras_local.Belt_Number-1,0);
                }
                else
                {
                    Linkage_Stream_Ctrl_Handle(user_paras_local.Belt_Number-1,1);
                }
            }
        }
        g_link_down_stream_status = (module_status_t.inverter_status[0].fault_code>>4)&0x1;
    }
}

void can_bus_frame_receive(CanRxMsg rxMsg)
{
    sCanFrameExtID extID;
    u8 recv_finish_flag = 0;
    
    extID.seg_polo = (rxMsg.ExtId>>20)&0x3;
    extID.seg_num  = (rxMsg.ExtId>>12)&0xFF;
    extID.func_id  = (rxMsg.ExtId>>8)&0xF;
    extID.src_id = rxMsg.ExtId & 0xFF;
    extID.dst_id  = (rxMsg.ExtId>>22)&0x7F;
    
    if(extID.func_id == CAN_FUNC_ID_BOOT_MODE && extID.dst_id == local_station)
    {
        //BKP_WriteBackupRegister(BKP_DR8, 0x55);
        //NVIC_SystemReset();
        FLASH_Unlock();
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);	
        if(FLASH_ErasePage(UserAppEnFlagAddress) == FLASH_COMPLETE)
        {
            FLASH_ProgramHalfWord(UserAppEnFlagAddress,0x0000);
        }
        FLASH_Lock();
        NVIC_SystemReset();
    }
    //接收数据
    if(extID.seg_polo == CAN_SEG_POLO_NONE)
    {
        memcpy(can_recv_buff,rxMsg.Data,rxMsg.DLC);
        can_recv_len = rxMsg.DLC;
        recv_finish_flag = 1;
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
            recv_finish_flag = 1;
            
            g_SegPolo = CAN_SEG_POLO_NONE;
            g_SegNum = 0;
            g_SegBytes = 0;
        }
    }
    if(recv_finish_flag != 1) return;
    
    if((extID.func_id == CAN_FUNC_ID_PARA_DATA) && (extID.src_id == local_station || isHost == 1))
    {
        para_data_recv_process(can_recv_buff,can_recv_len);
    }
    else if(extID.func_id == CAN_FUNC_ID_START_CMD)
    {
        Speed_Ctrl_Process(can_recv_buff[0]);
    }
    else if(extID.func_id == CAN_FUNC_ID_MODULE_STATUS)
    {
        module_status_recv_process(can_recv_buff,can_recv_len,extID.src_id);
    }
    else if(extID.func_id == CAN_FUNC_ID_RESET_CMD)
    {
        Reset_Ctrl_Handle();
    }
    else if(extID.func_id == CAN_FUNC_ID_FUNC_SELECT_CMD)
    {
        g_block_disable_flag = can_recv_buff[0];
    }
    else if((extID.func_id == CAN_FUNC_ID_READ_MODULE_STATUS) && ((extID.src_id == local_station) && (isHost != 1))){
        can_bus_send_module_status();
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
void can_bus_send_start_cmd(u8 cmd)
{
    can_send_len = 1;
    can_send_buff[0] = cmd;
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_START_CMD,local_station);
}
void can_bus_send_reset_cmd(void)
{
    can_send_len = 0;
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_RESET_CMD,local_station);
}
void can_bus_send_func_select_cmd(u8* pbuf)
{
    can_send_len = 1;
    can_send_buff[0] = pbuf[0];
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_FUNC_SELECT_CMD,local_station);
}
void can_bus_send_module_status()
{
    if(  user_paras_local.Belt_Number == 0 
      || user_paras_local.Belt_Number > 10 )
    {
        return;
    }
    
    can_send_len = 3 + sizeof(INVERTER_STATUS_T)*user_paras_local.Belt_Number;
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = user_paras_local.Belt_Number;
    memcpy(can_send_buff+3,(u8*)inverter_status_buffer,sizeof(INVERTER_STATUS_T)*user_paras_local.Belt_Number);
    
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
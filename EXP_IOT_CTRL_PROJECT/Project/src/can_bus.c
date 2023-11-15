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
    //������
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // �����β����һ��λ��
    q->queue[q->rear] = x; // ��x��ֵ�����µĶ�β
}

u8 can_bus_send_one_frame(sCanFrameExt sTxMsg)
{
    CanTxMsg TxMessage;
    
    TxMessage.ExtId = (sTxMsg.extId.src_id)|((sTxMsg.extId.func_id&0xF)<<8)|((sTxMsg.extId.seg_num&0xFF)<<12)|((sTxMsg.extId.seg_polo&0x3)<<20) | ((sTxMsg.extId.dst_id & 0x7F) << 22);
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
    case 0x1://������
        //�ӻ��ѱ�������ͨ��CAN���͸�����
        can_bus_reply_read_user_paras(&user_paras_local);
        break;
    case 0x91://�������ظ�
        //�����յ��ӻ�������ͨ������ת������λ��
        user_paras_slave = *((USER_PARAS_T *)(pbuf+3));
        AddSendMsgToQueue(REPLY_RECV_MSG_READ_PARA_CMD_TYPE);
        break;
    case 0x2://д����
        //�ӻ��յ������󱣴浽flash��������д������������
        write_user_paras((u16*)(pbuf+9));
        read_user_paras();
        can_bus_reply_write_user_paras(&user_paras_local);
        break;
    case 0x92://д�����ظ�
        //�����յ��ӻ�д�����������ͨ������ת������λ��
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
    
    if ((data_len != recv_len) || (belt_num == 0)) {
        return;
    }

    module_status_t.station_no = src_id;
    module_status_t.belt_number = belt_num;
    memcpy((u8*)module_status_t.inverter_status,(u8*)(pbuf+3),sizeof(INVERTER_STATUS_T)*belt_num);
    
    if((isHost == 1) && (module_status_t.station_no != 1))//��վ���մӻ�״̬��Ϣ
    {
        AddModuleStatusData2Queue(module_status_t);
    }
    if(src_id == user_paras_local.Up_Stream_No)
    {
        //������ͣ״̬
        g_link_up_stream_status = (module_status_t.inverter_status[belt_num-1].fault_code>>4)&0x1;
    }
    else if(src_id == user_paras_local.Down_Stream_No)
    {
        if(user_paras_local.Belt_Number != 0)
        {
            if(g_link_down_stream_status != ((module_status_t.inverter_status[0].fault_code>>4)&0x1))//��������״̬�ı�
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
        //��������״̬
        g_link_down_stream_status = (module_status_t.inverter_status[0].fault_code>>4)&0x1;
        // ���ι���д�����
        g_link_down_phototrig_status |= (module_status_t.inverter_status[0].input_status >> 7) & 0x1;
    }
}

void can_bus_frame_receive(CanRxMsg rxMsg)
{
    sCanFrameExtID extID;
    u8 recv_finish_flag = 0;
    COMM_NODE_T  comm_node_new;
    
    extID.seg_polo = (rxMsg.ExtId>>20)&0x3;
    extID.seg_num  = (rxMsg.ExtId>>12)&0xFF;
    extID.func_id  = (rxMsg.ExtId>>8)&0xF;
    extID.src_id = rxMsg.ExtId & 0xFF;
    extID.dst_id  = (rxMsg.ExtId>>22)&0x7F;
    
    if((extID.func_id == CAN_FUNC_ID_BOOT_MODE) && (extID.dst_id == local_station))
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
    

    if (extID.func_id == CAN_FUNC_ID_START_CMD)
    {
        if (rxMsg.Data[0] != 0) {
            Reset_Ctrl_Handle();                         //�й����ȸ�λ���� ���ܸ�λ��ͣ
            reset_start_time_cnt = 0;
        }
        Speed_Ctrl_Process(rxMsg.Data[0]);
    }
    else if (extID.func_id == CAN_FUNC_ID_RESET_CMD)
    {
        Reset_Ctrl_Handle();                         //�й����ȸ�λ����
        reset_start_time_cnt = 500;
        g_emergency_stop = 0;                        //��λ��ͣ״̬
    }
    else if (extID.func_id == CAN_FUNC_ID_FUNC_SELECT_CMD)
    {
        g_block_disable_flag = rxMsg.Data[0];
    }
    else if ((extID.func_id == CAN_FUNC_ID_READ_MODULE_STATUS) && ((extID.src_id == local_station) && (isHost != 1))) {
        can_bus_send_module_status();
    }
    else if (extID.func_id == CAN_FUNC_ID_EMERGENCY_STOP_STATUS) {
        g_emergency_stop = 1;
        Speed_Ctrl_Process(EMERGENCY_STOP);
    }
    else if ((extID.func_id == CAN_FUNC_ID_UPSTREAM_STOP_CMD) && (extID.src_id == user_paras_local.Down_Stream_No)) {    
        //���η���ͬ��ֹͣ�ź�  ���ΰ�ֹͣʱ��ֹͣ
        if (user_paras_local.Belt_Number == 0) {
            return;
        }
        comm_node_new.rw_flag = 1;
        comm_node_new.inverter_no = user_paras_local.Belt_Number;
        comm_node_new.speed_gear = 0;
        comm_node_new.comm_interval = user_paras_local.belt_para[user_paras_local.Belt_Number - 1].Stop_Delay_Time;
        comm_node_new.comm_retry = 3;
        AddUartSendData2Queue(comm_node_new);
    }

    //���ն������
    if (extID.seg_polo == CAN_SEG_POLO_NONE)
    {
        memcpy(can_recv_buff, rxMsg.Data, rxMsg.DLC);
        can_recv_len = rxMsg.DLC;
        recv_finish_flag = 1;
    }
    else if (extID.seg_polo == CAN_SEG_POLO_FIRST)
    {
        memcpy(can_recv_buff, rxMsg.Data, rxMsg.DLC);

        g_SegPolo = CAN_SEG_POLO_FIRST;
        g_SegNum = extID.seg_num;
        g_SegBytes = rxMsg.DLC;
    }
    else if (extID.seg_polo == CAN_SEG_POLO_MIDDLE)
    {
        if ((g_SegPolo == CAN_SEG_POLO_FIRST)
            && (extID.seg_num == (g_SegNum + 1))
            && ((g_SegBytes + rxMsg.DLC) <= CAN_RX_BUFF_SIZE))
        {
            memcpy(can_recv_buff + g_SegBytes, rxMsg.Data, rxMsg.DLC);

            g_SegNum++;
            g_SegBytes += rxMsg.DLC;
        }
    }
    else if (extID.seg_polo == CAN_SEG_POLO_FINAL)
    {
        if ((g_SegPolo == CAN_SEG_POLO_FIRST)
            && (extID.seg_num == (g_SegNum + 1))
            && ((g_SegBytes + rxMsg.DLC) <= CAN_RX_BUFF_SIZE))
        {
            memcpy(can_recv_buff + g_SegBytes, rxMsg.Data, rxMsg.DLC);
            can_recv_len = g_SegBytes + rxMsg.DLC;
            recv_finish_flag = 1;

            g_SegPolo = CAN_SEG_POLO_NONE;
            g_SegNum = 0;
            g_SegBytes = 0;
        }
    }

    if(recv_finish_flag != 1) return;
    
    if((extID.func_id == CAN_FUNC_ID_PARA_DATA) && ((extID.src_id == local_station) || (isHost == 1))){
        para_data_recv_process(can_recv_buff,can_recv_len);
    }
    else if(extID.func_id == CAN_FUNC_ID_MODULE_STATUS){
        module_status_recv_process(can_recv_buff,can_recv_len,extID.src_id);
    }  
}

void can_bus_send_msg(u8* pbuf, u16 send_tot_len, u8 func_id, u8 src_id)
{
    sCanFrameExt canTxMsg;
    u16 can_send_len;
    
    canTxMsg.extId.func_id  = func_id;
    canTxMsg.extId.src_id   = src_id;
    if(send_tot_len <= CAN_PACK_DATA_LEN)//����ֶδ���
    {
        canTxMsg.extId.seg_polo = CAN_SEG_POLO_NONE;
        canTxMsg.extId.seg_num  = 0;
        canTxMsg.data_len = send_tot_len;
        memcpy(canTxMsg.data, pbuf, canTxMsg.data_len);
        AddCanSendData2Queue(canTxMsg);
    }
    else//��Ҫ�ֶδ���
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
    can_send_buff[2] = 0x01;//������
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,station_no);
}
void can_bus_reply_read_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x91;//�������ظ�
    memcpy(can_send_buff+3,(u8*)user_para,sizeof(USER_PARAS_T));
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,user_para->Station_No);
}
void can_bus_send_write_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x02;//д��������
    memcpy(can_send_buff+3,(u8*)user_para,sizeof(USER_PARAS_T));
    
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_PARA_DATA,user_para->Station_No);
}
void can_bus_reply_write_user_paras(USER_PARAS_T* user_para)
{
    can_send_len = 3 + sizeof(USER_PARAS_T);
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = 0x92;//д�����ظ�
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
//
void can_bus_send_module_status()
{
    if(  (user_paras_local.Belt_Number == 0) 
      || (user_paras_local.Belt_Number > 10) )
    {
        return;
    }
    
    can_send_len = 3 + sizeof(INVERTER_STATUS_T)*user_paras_local.Belt_Number;
    can_send_buff[0] = can_send_len&0xFF;
    can_send_buff[1] = (can_send_len>>8)&0xFF;
    can_send_buff[2] = user_paras_local.Belt_Number;
    memcpy(can_send_buff+3,(u8*)inverter_status_buffer,sizeof(INVERTER_STATUS_T)*user_paras_local.Belt_Number);
    //��� ���봥��״̬ ���ڶ°�����
    inverter_status_buffer[0].input_status &= ~(0x1 << 7);
    can_bus_send_msg(can_send_buff,can_send_len,CAN_FUNC_ID_MODULE_STATUS,local_station);
}

// ���ͼ�ͣ�ź�
void can_bus_send_func_emergency_cmd(void)
{
    u8 buf[10] = { 0 };
    u16 sendlen = 0;

    sendlen = 0;


    can_bus_send_msg(buf, sendlen, CAN_FUNC_ID_EMERGENCY_STOP_STATUS, local_station);
}

//��������ֹͣ�ź�
void can_bus_send_func_upStreamStop_cmd(void)
{
    u8 buf[10] = { 0 };
    u16 sendlen = 0;

    sendlen = 0;


    can_bus_send_msg(buf, sendlen, CAN_FUNC_ID_UPSTREAM_STOP_CMD, local_station);
}

void can_send_frame_process()
{
    sCAN_SEND_QUEUE *q = &canSendQueue;
    sCanFrameExt* canTxMsg = NULL;
    u16 front;
    u8  tx_mailbox;
    
    //���п�
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
        q->front = front;//����
    }
}
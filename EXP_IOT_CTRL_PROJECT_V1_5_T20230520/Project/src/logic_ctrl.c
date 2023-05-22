#include "main.h"

USER_PARAS_T  user_paras_local;
USER_PARAS_T  user_paras_slave;
USER_PARAS_T  user_paras_temp;

sButton_Info  bPhoto1Info;
sButton_Info  bPhoto2Info;
sButton_Info  bPhoto3Info;
sButton_Info  bPhoto4Info;
sButton_Info  bPhoto5Info;
sButton_Info  bResetInfo;
sButton_Info  bStartInfo;
sButton_Info  bStopInfo;
sButton_Info  bStreamInfo;
sButton_Info  bEmergencyInfo;


INVERTER_STATUS_T  inverter_status_buffer[10];//����״̬

#define moduleStatusQueueSize  20
MODULE_STATUS_T  moduleStatusBuff[moduleStatusQueueSize];
MODULE_STATUS_QUEUE  moduleStatusQueue;

#define uartSendQueueSize  30
COMM_NODE_T uartSendQueueBuff[uartSendQueueSize];
COMM_SEND_QUEUE uartSendQueue;
COMM_NODE_T comm_node;
u8  comm_busy_flag;
u8  polling_num;//��ѯվ��(��1��ʼ)

u8  g_remote_start_flag;   //Զ����ͣ״̬�ı���(0:�ޱ仯 1:ֹͣ������ 2:������ֹͣ)
u8  g_remote_start_status; //Զ����ͣ״̬(0:ֹͣ״̬ 1:����״̬)
u8  g_remote_speed_status; //Զ�̸ߵ���״̬(0:���� 1:����)
u8  g_link_up_stream_status; //���������ź�
u8  g_link_down_stream_status; //���������ź�
u8  g_set_start_status; //���õ����ͣ״̬(0:ֹͣ 1:����)
u8  g_set_speed_status; //���õ���ߵ���״̬(0:���� 1:����)
u8  g_read_start_status; //��ǰ�ĵ����ͣ״̬(0:ֹͣ 1:����)
u8  g_alarm_type; //�������(bit0:�������,bit1:�°�����,bit2:485ͨѶ����)
u8  g_speed_gear_status; //��ǰ���ٶȵ�λ(0:ֹͣ 1~5:�嵵�ٶ�)
u8  g_block_disable_flag; //�°���⹦�ܽ�ֹ���

//u16 start_delay_time_cnt;
//u16 stop_delay_time_cnt;
u32 block_check_time_cnt[10];
u16 reset_start_time_cnt;
u8  reset_start_flag;

void InitUartSendQueue(void)
{
    COMM_SEND_QUEUE *q;
    
    q = &uartSendQueue;
    q->maxSize = uartSendQueueSize;
    q->queue = uartSendQueueBuff;
    q->front = q->rear = 0;
}
void AddUartSendData2Queue(COMM_NODE_T x)
{
    COMM_SEND_QUEUE *q = &uartSendQueue;
    //������
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // �����β����һ��λ��
    q->queue[q->rear] = x; // ��x��ֵ�����µĶ�β
}
COMM_NODE_T* GetUartSendDataFromQueue(void)
{
    COMM_SEND_QUEUE *q = &uartSendQueue;
    //���п�
    if(q->front == q->rear)
    {
        return NULL;
    }
    q->front = (q->front + 1) % q->maxSize; // ʹ����ָ��ָ����һ��λ��
    return (COMM_NODE_T*)(&(q->queue[q->front])); // ���ض���Ԫ��
}
u8 IsUartSendQueueFree(void)
{
    COMM_SEND_QUEUE *q = &uartSendQueue;
    //���п�
    if(q->front == q->rear)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



void InitModuleStatusQueue(void)
{
    MODULE_STATUS_QUEUE *q;
    
    q = &moduleStatusQueue;
    q->maxSize = moduleStatusQueueSize;
    q->queue = moduleStatusBuff;
    q->front = q->rear = 0;
}
void AddModuleStatusData2Queue(MODULE_STATUS_T x)
{
    MODULE_STATUS_QUEUE *q = &moduleStatusQueue;
    //������
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // �����β����һ��λ��
    q->queue[q->rear] = x; // ��x��ֵ�����µĶ�β
}
MODULE_STATUS_T* GetModuleStatusDataFromQueue(void)
{
    MODULE_STATUS_QUEUE *q = &moduleStatusQueue;
    //���п�
    if(q->front == q->rear)
    {
        return NULL;
    }
    q->front = (q->front + 1) % q->maxSize; // ʹ����ָ��ָ����һ��λ��
    return (MODULE_STATUS_T*)(&(q->queue[q->front])); // ���ض���Ԫ��
}
u8 IsModuleStatusQueueFree(void)
{
    MODULE_STATUS_QUEUE *q = &moduleStatusQueue;
    //���п�
    if(q->front == q->rear)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* scan every 1ms */
void InputScanProc()
{
    u8 l_bit_reset_in;
    u8 l_bit_photo_1_in;
    u8 l_bit_photo_2_in;
    u8 l_bit_photo_3_in;
    u8 l_bit_photo_4_in;
    u8 l_bit_photo_5_in;
    u8 l_bit_start_in;
    u8 l_bit_stop_in;
    u8 l_bit_stream_in;
    u8 l_bit_emergency_in;
    
    l_bit_reset_in = B_RESET_IN_STATE;
    l_bit_photo_1_in = B_PHOTO_1_IN_STATE;
    l_bit_photo_2_in = B_PHOTO_2_IN_STATE;
    l_bit_photo_3_in = B_PHOTO_3_IN_STATE;
    l_bit_photo_4_in = B_PHOTO_4_IN_STATE;
    l_bit_photo_5_in = B_PHOTO_5_IN_STATE;
    l_bit_start_in = B_START_IN_STATE;
    l_bit_stop_in = B_STOP_IN_STATE;
    l_bit_stream_in = B_STREAM_IN_STATE;
    l_bit_emergency_in = B_EMERGENCY_IN_STATE;


    
    //����λ�����ź�
    if(  bResetInfo.input_info.input_state != l_bit_reset_in
      && bResetInfo.input_info.input_confirm_times == 0)
    {
        bResetInfo.input_info.input_middle_state = l_bit_reset_in;
    }
    if(  bResetInfo.input_info.input_middle_state == l_bit_reset_in
      && bResetInfo.input_info.input_middle_state != bResetInfo.input_info.input_state)
    {
        bResetInfo.input_info.input_confirm_times++;
        if(bResetInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
        {
            bResetInfo.input_info.input_state = bResetInfo.input_info.input_middle_state;
            bResetInfo.input_info.input_confirm_times = 0;
            Reset_Ctrl_Handle();
        }
    }
    else
    {
        bResetInfo.input_info.input_middle_state = bResetInfo.input_info.input_state;
        bResetInfo.input_info.input_confirm_times = 0;
    }
    //������1�����ź�
    bPhoto1Info.trig_cnt++;
    if(  bPhoto1Info.input_info.input_state != l_bit_photo_1_in
      && bPhoto1Info.input_info.input_confirm_times == 0)
    {
        bPhoto1Info.input_info.input_middle_state = l_bit_photo_1_in;
    }
    if(  bPhoto1Info.input_info.input_middle_state == l_bit_photo_1_in
      && bPhoto1Info.input_info.input_middle_state != bPhoto1Info.input_info.input_state)
    {
        bPhoto1Info.input_info.input_confirm_times++;
        if(bPhoto1Info.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bPhoto1Info.input_info.input_state = bPhoto1Info.input_info.input_middle_state;
            bPhoto1Info.input_info.input_confirm_times = 0;
            Linkage_Stop_Photo_Ctrl_Handle(0);
        }
    }
    else
    {
        bPhoto1Info.input_info.input_middle_state = bPhoto1Info.input_info.input_state;
        bPhoto1Info.input_info.input_confirm_times = 0;
    }
    if (bPhoto1Info.trig_cnt > 300) {
        bPhoto1Info.trig_cnt = 0;
        if (bPhoto1Info.input_info.input_state == 1) {
            Linkage_Stop_Photo_Ctrl_Handle(0);
        }
    }
    //������2�����ź�
    bPhoto2Info.trig_cnt++;
    if(  bPhoto2Info.input_info.input_state != l_bit_photo_2_in
      && bPhoto2Info.input_info.input_confirm_times == 0)
    {
        bPhoto2Info.input_info.input_middle_state = l_bit_photo_2_in;
    }
    if(  bPhoto2Info.input_info.input_middle_state == l_bit_photo_2_in
      && bPhoto2Info.input_info.input_middle_state != bPhoto2Info.input_info.input_state)
    {
        bPhoto2Info.input_info.input_confirm_times++;
        if(bPhoto2Info.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bPhoto2Info.input_info.input_state = bPhoto2Info.input_info.input_middle_state;
            bPhoto2Info.input_info.input_confirm_times = 0;
            Linkage_Stop_Photo_Ctrl_Handle(1);
        }
    }
    else
    {
        bPhoto2Info.input_info.input_middle_state = bPhoto2Info.input_info.input_state;
        bPhoto2Info.input_info.input_confirm_times = 0;
    }
    if (bPhoto2Info.trig_cnt > 300) {
        bPhoto2Info.trig_cnt = 0;
        if (bPhoto2Info.input_info.input_state == 1) {
            Linkage_Stop_Photo_Ctrl_Handle(1);
        }
    }

    //������3�����ź�
    bPhoto3Info.trig_cnt++;
    if(  bPhoto3Info.input_info.input_state != l_bit_photo_3_in
      && bPhoto3Info.input_info.input_confirm_times == 0)
    {
        bPhoto3Info.input_info.input_middle_state = l_bit_photo_3_in;
    }
    if(  bPhoto3Info.input_info.input_middle_state == l_bit_photo_3_in
      && bPhoto3Info.input_info.input_middle_state != bPhoto3Info.input_info.input_state)
    {
        bPhoto3Info.input_info.input_confirm_times++;
        if(bPhoto3Info.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bPhoto3Info.input_info.input_state = bPhoto3Info.input_info.input_middle_state;
            bPhoto3Info.input_info.input_confirm_times = 0;
            Linkage_Stop_Photo_Ctrl_Handle(2);
        }
    }
    else
    {
        bPhoto3Info.input_info.input_middle_state = bPhoto3Info.input_info.input_state;
        bPhoto3Info.input_info.input_confirm_times = 0;
    }
    if (bPhoto3Info.trig_cnt > 300) {
        bPhoto3Info.trig_cnt = 0;
        if (bPhoto3Info.input_info.input_state == 1) {
            Linkage_Stop_Photo_Ctrl_Handle(2);
        }
    }
    //������4�����ź�
    bPhoto4Info.trig_cnt++;
    if(  bPhoto4Info.input_info.input_state != l_bit_photo_4_in
      && bPhoto4Info.input_info.input_confirm_times == 0)
    {
        bPhoto4Info.input_info.input_middle_state = l_bit_photo_4_in;
    }
    if(  bPhoto4Info.input_info.input_middle_state == l_bit_photo_4_in
      && bPhoto4Info.input_info.input_middle_state != bPhoto4Info.input_info.input_state)
    {
        bPhoto4Info.input_info.input_confirm_times++;
        if(bPhoto4Info.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bPhoto4Info.input_info.input_state = bPhoto4Info.input_info.input_middle_state;
            bPhoto4Info.input_info.input_confirm_times = 0;
            Linkage_Stop_Photo_Ctrl_Handle(3);
        }
    }
    else
    {
        bPhoto4Info.input_info.input_middle_state = bPhoto4Info.input_info.input_state;
        bPhoto4Info.input_info.input_confirm_times = 0;
    }

    if (bPhoto4Info.trig_cnt > 300) {
        bPhoto4Info.trig_cnt = 0;
        if (bPhoto4Info.input_info.input_state == 1) {
            Linkage_Stop_Photo_Ctrl_Handle(3);
        }
    }
    //������5�����ź�
    bPhoto5Info.trig_cnt++;
    if(  bPhoto5Info.input_info.input_state != l_bit_photo_5_in
      && bPhoto5Info.input_info.input_confirm_times == 0)
    {
        bPhoto5Info.input_info.input_middle_state = l_bit_photo_5_in;
    }
    if(  bPhoto5Info.input_info.input_middle_state == l_bit_photo_5_in
      && bPhoto5Info.input_info.input_middle_state != bPhoto5Info.input_info.input_state)
    {
        bPhoto5Info.input_info.input_confirm_times++;
        if(bPhoto5Info.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bPhoto5Info.input_info.input_state = bPhoto5Info.input_info.input_middle_state;
            bPhoto5Info.input_info.input_confirm_times = 0;
            Linkage_Stop_Photo_Ctrl_Handle(4);
        }
    }
    else
    {
        bPhoto5Info.input_info.input_middle_state = bPhoto5Info.input_info.input_state;
        bPhoto5Info.input_info.input_confirm_times = 0;
    }
    if (bPhoto5Info.trig_cnt > 300) {
        bPhoto5Info.trig_cnt = 0;
        if (bPhoto5Info.input_info.input_state == 1) {
            Linkage_Stop_Photo_Ctrl_Handle(4);
        }
    }

    //�������������ź�
    if(  bStartInfo.input_info.input_state != l_bit_start_in
      && bStartInfo.input_info.input_confirm_times == 0)
    {
        bStartInfo.input_info.input_middle_state = l_bit_start_in;
    }
    if(  bStartInfo.input_info.input_middle_state == l_bit_start_in
      && bStartInfo.input_info.input_middle_state != bStartInfo.input_info.input_state)
    {
        bStartInfo.input_info.input_confirm_times++;
        if(bStartInfo.input_info.input_confirm_times > 100)//����ʱ��100ms
        {
            bStartInfo.input_info.input_state = bStartInfo.input_info.input_middle_state;
            bStartInfo.input_info.input_confirm_times = 0;
            if(bStartInfo.input_info.input_state == 1)
            {
                if(isHost)//����
                {
                    Speed_Ctrl_Process(2);
                    can_bus_send_start_cmd(2);
                }
            }
        }
    }
    else
    {
        bStartInfo.input_info.input_middle_state = bStartInfo.input_info.input_state;
        bStartInfo.input_info.input_confirm_times = 0;
    }
    //����ֹͣ�����ź�
    if(  bStopInfo.input_info.input_state != l_bit_stop_in
      && bStopInfo.input_info.input_confirm_times == 0)
    {
        bStopInfo.input_info.input_middle_state = l_bit_stop_in;
    }
    if(  bStopInfo.input_info.input_middle_state == l_bit_stop_in
      && bStopInfo.input_info.input_middle_state != bStopInfo.input_info.input_state)
    {
        bStopInfo.input_info.input_confirm_times++;
        if(bStopInfo.input_info.input_confirm_times > 100)//����ʱ��100ms
        {
            bStopInfo.input_info.input_state = bStopInfo.input_info.input_middle_state;
            bStopInfo.input_info.input_confirm_times = 0;
            if(bStopInfo.input_info.input_state == 1)
            {
                if(isHost)//����
                {
                    Speed_Ctrl_Process(0);
                    can_bus_send_start_cmd(0);
                }
            }
        }
    }
    else
    {
        bStopInfo.input_info.input_middle_state = bStopInfo.input_info.input_state;
        bStopInfo.input_info.input_confirm_times = 0;
    }

    //��������׼���ź�
    bStreamInfo.trig_cnt++;
    if (bStreamInfo.input_info.input_state != l_bit_stream_in
        && bStreamInfo.input_info.input_confirm_times == 0)
    {
        bStreamInfo.input_info.input_middle_state = l_bit_stream_in;
    }
    if (bStreamInfo.input_info.input_middle_state == l_bit_stream_in
        && bStreamInfo.input_info.input_middle_state != bStreamInfo.input_info.input_state)
    {
        bStreamInfo.input_info.input_confirm_times++;
        if ((bStreamInfo.input_info.input_middle_state == 1) && (bStreamInfo.input_info.input_confirm_times > 20))//����ʱ��100ms
        {
            bStreamInfo.input_info.input_state = bStreamInfo.input_info.input_middle_state;
            bStreamInfo.input_info.input_confirm_times = 0;
            if (bStreamInfo.input_info.input_state == 1) {
                Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 1);
            }
        }

        if ((bStreamInfo.input_info.input_middle_state == 0) && (bStreamInfo.input_info.input_confirm_times > 200))//����ʱ��100ms
        {
            bStreamInfo.input_info.input_state = bStreamInfo.input_info.input_middle_state;
            bStreamInfo.input_info.input_confirm_times = 0;
            //if (bStreamInfo.input_info.input_state == 1) {
            //Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 1);
        }



        if (bStreamInfo.input_info.input_state == 0) {
//            Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 0);
        }



    }
    else
    {
        bStreamInfo.input_info.input_middle_state = bStreamInfo.input_info.input_state;
        bStreamInfo.input_info.input_confirm_times = 0;
    }

    if (bStreamInfo.trig_cnt > 2000) {
        bStreamInfo.trig_cnt = 0;
        if (bStreamInfo.input_info.input_state == 1) {
            Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 1);
        }
        //if (bStreamInfo.input_info.input_state == 0) {
        //    Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 0);
        //}
    }

    //����ͣ�����ź�
    bEmergencyInfo.trig_cnt++;
    if (bEmergencyInfo.input_info.input_state != l_bit_emergency_in
        && bEmergencyInfo.input_info.input_confirm_times == 0)
    {
        bEmergencyInfo.input_info.input_middle_state = l_bit_emergency_in;
    }
    if (bEmergencyInfo.input_info.input_middle_state == l_bit_emergency_in
        && bEmergencyInfo.input_info.input_middle_state != bEmergencyInfo.input_info.input_state)
    {
        bEmergencyInfo.input_info.input_confirm_times++;
        if (bEmergencyInfo.input_info.input_confirm_times > 50)//����ʱ��50ms
        {
            bEmergencyInfo.input_info.input_state = bEmergencyInfo.input_info.input_middle_state;
            bEmergencyInfo.input_info.input_confirm_times = 0;
            Speed_Ctrl_Process(EMERGENCY_STOP);
        }
    }
    else
    {
        bEmergencyInfo.input_info.input_middle_state = bEmergencyInfo.input_info.input_state;
        bEmergencyInfo.input_info.input_confirm_times = 0;
    }
    if (bEmergencyInfo.trig_cnt > 2000) {
        bEmergencyInfo.trig_cnt = 0;
        if (bEmergencyInfo.input_info.input_state == 1) {
            Speed_Ctrl_Process(EMERGENCY_STOP);
        }
    }
}

//����,ֹͣ,����ʱ�ı�����Ƥ�����ٶ�
void Speed_Ctrl_Process(u8 speed_gear)
{
    COMM_NODE_T  comm_node_new;
    u8 i;
    
//    if(speed_gear > 6) return;

    if (speed_gear == EMERGENCY_STOP) {
        InitUartSendQueue();
        for (i = 0; i < user_paras_local.Belt_Number; i++)
        {
            comm_node_new.rw_flag = 1;
            comm_node_new.inverter_no = i + 1;
            comm_node_new.speed_gear = 0;
            comm_node_new.comm_interval = 20;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
        g_speed_gear_status = 0;
        B_RUN_STA_OUT_(Bit_RESET);
        return;
    }
    
    if(g_speed_gear_status == 0 && speed_gear > 0)//����
    {
        for(i=0; i<user_paras_local.Belt_Number; i++)
        {
            comm_node_new.rw_flag = 1;
            comm_node_new.inverter_no = user_paras_local.Belt_Number-i;
            comm_node_new.speed_gear = speed_gear;
            comm_node_new.comm_interval = user_paras_local.belt_para[user_paras_local.Belt_Number-i-1].Start_Delay_Time;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
        g_speed_gear_status = speed_gear;
        B_RUN_STA_OUT_(Bit_SET);
    }
    else if( speed_gear == 0)//ֹͣ
    {
        InitUartSendQueue();
        for(i=0; i<user_paras_local.Belt_Number; i++)
        {
            comm_node_new.rw_flag = 1;
            comm_node_new.inverter_no = i+1;
            comm_node_new.speed_gear = speed_gear;
            comm_node_new.comm_interval = user_paras_local.belt_para[i].Stop_Delay_Time;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
        g_speed_gear_status = speed_gear;
        B_RUN_STA_OUT_(Bit_RESET);
    }
    else if(g_speed_gear_status != 0 && speed_gear != 0)//����
    {
        for(i=0; i<user_paras_local.Belt_Number; i++)
        {
            comm_node_new.rw_flag = 1;
            comm_node_new.inverter_no = i+1;
            comm_node_new.speed_gear = speed_gear;
            comm_node_new.comm_interval = 20;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
        g_speed_gear_status = speed_gear;
        B_RUN_STA_OUT_(Bit_SET);
    }
}
u8 get_photo_input_status(u8 belt_index)
{
    if(belt_index == 0)
    {
        return B_PHOTO_1_IN_STATE;
    }
    else if(belt_index == 1)
    {
        return B_PHOTO_2_IN_STATE;
    }
    else if(belt_index == 2)
    {
        return B_PHOTO_3_IN_STATE;
    }
    else if(belt_index == 3)
    {
        return B_PHOTO_4_IN_STATE;
    }
    else if(belt_index == 4)
    {
        return B_PHOTO_5_IN_STATE;
    }
    else
    {
        return 0;
    }
}
u8 get_inverter_fault_status(INVERTER_STATUS_T inverter_status)
{
    if(inverter_status.fault_code & 0x1)//485ͨѶ����
        return 1;
    if((inverter_status.fault_code>>1) & 0x1)//�°�����
        return 1;
    if(((inverter_status.fault_code>>8)&0xFF) != 0)//��Ƶ��������
        return 1;
    return 0;;
}
//����ֹͣ_��紥������
void Linkage_Stop_Photo_Ctrl_Handle(u8 photo_index)
{
    COMM_NODE_T  comm_node_new;
    u8  link_down_status = 1;
    u16 i = 0;
    
    if(user_paras_local.Belt_Number == 0) return;

    //for (i = 0; i < user_paras_local.Belt_Number; i++) {
    //    if ((user_paras_local.belt_para[i].Func_Select_Switch >> 4) && (bStreamInfo.input_info.input_state == 1))
    //    {
    //        return;
    //    }
    //}
    
    if(((user_paras_local.belt_para[photo_index].Func_Select_Switch>>3)&0x1) == 0)//δ���û��Ź���
    {
        return;
    }
    
    if(photo_index+1 == user_paras_local.Belt_Number)
    {
        for (i = 0; i < user_paras_local.Belt_Number; i++) {
            if ((user_paras_local.belt_para[i].Func_Select_Switch >> 4) & 0x1)
            {
                if ((user_paras_local.Down_Stream_No == 0)  && (bStreamInfo.input_info.input_state == 0))//û����������վ��
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
        if(user_paras_local.Down_Stream_No != 0)//����������վ��
        {
//            link_down_status = 1;
            link_down_status = g_link_down_stream_status;
        }
        //else
        //{
        //    link_down_status = g_link_down_stream_status;
        //}
    }
    else
    {
        link_down_status = (inverter_status_buffer[photo_index+1].fault_code>>4)&0x1;
    }
    if((user_paras_local.belt_para[photo_index].Func_Select_Switch>>1)&0x1)//�������ܿ���
    {
        if((inverter_status_buffer[photo_index].fault_code>>4)&0x1)//����Ƥ������״̬
        {
            if(link_down_status == 0)//���β��������
            {
                //ֹͣ
                comm_node_new.rw_flag = 1;
                comm_node_new.inverter_no = photo_index+1;
                comm_node_new.speed_gear = 0;
                comm_node_new.comm_interval = 10;
                comm_node_new.comm_retry = 3;
                AddUartSendData2Queue(comm_node_new);
            }
        }
    }
}

void Linkage_stream_extra_signal(u8 inverter_index, u8 start_flag)
{
    COMM_NODE_T  comm_node_new;
    u16 i = 0;

    for (i = 0; i < user_paras_local.Belt_Number; i++) {
        if ((user_paras_local.belt_para[i].Func_Select_Switch >> 4) & 0x1)
        {
            if ((user_paras_local.belt_para[inverter_index].Func_Select_Switch >> 1) & 0x1)//�������ܿ���
            {
                if (start_flag == 1)//��������
                {
                    if ((((inverter_status_buffer[inverter_index].fault_code >> 4) & 0x1) == 0)
                        && (get_inverter_fault_status(inverter_status_buffer[inverter_index]) != 1))//����Ƥ��ֹͣ״̬��û�б���
                    {
                        //����
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = inverter_index + 1;
                        comm_node_new.speed_gear = g_speed_gear_status;
                        comm_node_new.comm_interval = user_paras_local.belt_para[inverter_index].Start_Delay_Time;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                    }
                }
                else//����ֹͣ
                {
                    if ((inverter_status_buffer[inverter_index].fault_code >> 4) & 0x1)//����Ƥ������״̬
                    {
                        //ֹͣ
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = inverter_index + 1;
                        comm_node_new.speed_gear = 0;
                        comm_node_new.comm_interval = 10;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                    }
                }
            }
            return;
        }
    }

}

//������ͣ_�����δ�������
void Linkage_Stream_Ctrl_Handle(u8 inverter_index,u8 start_flag)
{
    COMM_NODE_T  comm_node_new;
    
    if((user_paras_local.belt_para[inverter_index].Func_Select_Switch>>1)&0x1)//�������ܿ���
    {
        if(start_flag == 1)//��������
        {
            if(  (((inverter_status_buffer[inverter_index].fault_code>>4)&0x1) == 0)
              && (get_inverter_fault_status(inverter_status_buffer[inverter_index]) != 1) )//����Ƥ��ֹͣ״̬��û�б���
            {
                //����
                comm_node_new.rw_flag = 1;
                comm_node_new.inverter_no = inverter_index+1;
                comm_node_new.speed_gear = g_speed_gear_status;
                comm_node_new.comm_interval = user_paras_local.belt_para[inverter_index].Start_Delay_Time;
                comm_node_new.comm_retry = 3;
                AddUartSendData2Queue(comm_node_new);
            }
        }
        else//����ֹͣ
        {
            if(((user_paras_local.belt_para[inverter_index].Func_Select_Switch>>3)&0x1) == 1)//���û��Ź��ܾ��ɹ�紥��ֹͣ
                return;
            if((inverter_status_buffer[inverter_index].fault_code>>4)&0x1)//����Ƥ������״̬
            {
                //ֹͣ
                comm_node_new.rw_flag = 1;
                comm_node_new.inverter_no = inverter_index+1;
                comm_node_new.speed_gear = 0;
                comm_node_new.comm_interval = 10;
                comm_node_new.comm_retry = 3;
                AddUartSendData2Queue(comm_node_new);
            }
        }
    }
}
//�°����
void Block_Check_Ctrl_Handle(void)
{
    COMM_NODE_T  comm_node_new;
    u8  i;
    
    if(g_block_disable_flag == 1) return;
    
    for(i=0; i<user_paras_local.Belt_Number; i++)
    {
        if((user_paras_local.belt_para[i].Func_Select_Switch>>2)&0x1)//�°���⿪��
        {
            if((inverter_status_buffer[i].fault_code>>4)&0x1)//����Ƥ������״̬
            {
                if(get_photo_input_status(i) == 1)//��类�ڵ�
                {
                    block_check_time_cnt[i]++;
                    if(block_check_time_cnt[i] >= user_paras_local.belt_para[i].Block_Check_Time)
                    {
                        block_check_time_cnt[i] = 0;
                        //ֹͣ
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = i+1;
                        comm_node_new.speed_gear = 0;
                        comm_node_new.comm_interval = 10;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                        L_ALARM_OUT_(Bit_SET);//����ָʾ�����
                        inverter_status_buffer[i].fault_code |= (0x1<<1);//�°�����
                    }
                }
                else
                {
                    block_check_time_cnt[i] = 0;
                }
            }
        }
    }
}
//�°���λ,��Ƶ�����ϸ�λ
void Reset_Ctrl_Handle(void)
{
    COMM_NODE_T  comm_node_new;
    u8  i;
    
    for(i=0; i<user_paras_local.Belt_Number; i++)
    {
        if((inverter_status_buffer[i].fault_code>>1)&0x1)//�°���λ
        {
            inverter_status_buffer[i].fault_code &= ~(0x1<<1);
        }
        if(((inverter_status_buffer[i].fault_code>>8)&0xFF) != 0)//��Ƶ����λ
        {
            comm_node_new.rw_flag = 2;
            comm_node_new.inverter_no = i+1;
            comm_node_new.speed_gear = 0;
            comm_node_new.comm_interval = 20;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
    }
    L_ALARM_OUT_(Bit_RESET);//����ָʾ��Ϩ��
    reset_start_time_cnt = 500;
}
//��λ��������Ƶ��
void Reset_Start_Inverter_Handle(void)
{
    COMM_NODE_T  comm_node_new;
    u8 i;
    
    if(user_paras_local.Belt_Number == 0) return;
    if(g_speed_gear_status == 0) return;
    if(reset_start_flag != 1) return;
    
    reset_start_flag = 0;
    
    if (bStreamInfo.input_info.input_state == 0) {
          for(i=user_paras_local.Belt_Number; i>0; i--)
    {
        if(  (((inverter_status_buffer[i-1].fault_code>>4)&0x1) == 0))
         // && (get_inverter_fault_status(inverter_status_buffer[i-1]) != 1) )//����Ƥ��ֹͣ״̬��û�б���
        {
            comm_node_new.rw_flag = 1;
            comm_node_new.inverter_no = i;
            comm_node_new.speed_gear = g_speed_gear_status;
            comm_node_new.comm_interval = user_paras_local.belt_para[i-1].Start_Delay_Time;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
    }
   }
    
}

void read_user_paras(void)
{
    u16 i;
    u16 data;
    
    for(i=0; i<USER_PARA_DATA_LEN; i++)
    {
        data = *((u16*)(UserParaStartAddress+2*i));
        *((u16*)(&user_paras_local)+3+i) = data;
    }
    user_paras_local.Station_No = local_station;
    user_paras_local.Version_No_L = 0x0004;
    user_paras_local.Version_No_H = 0x1000;
    //������Ч�Խ���
    if(user_paras_local.Up_Stream_No > 255)
    {
        user_paras_local.Up_Stream_No = 0;
    }
    if(user_paras_local.Down_Stream_No > 255)
    {
        user_paras_local.Down_Stream_No = 0;
    }
    if(user_paras_local.Belt_Number > 10)
    {
        user_paras_local.Belt_Number = 0;
    }

    for (i = 0; i < user_paras_local.Belt_Number; i++) {
        stopspeed_default[i] = user_paras_local.belt_para[i].Gear_2_Speed_Freq;
    }
}
void write_user_paras(u16* para)
{
    u8 i;
    
    FLASH_Unlock();
    FLASH_ErasePage(UserParaStartAddress);
    
    for(i=0; i<USER_PARA_DATA_LEN; i++)
    {
        FLASH_ProgramHalfWord(UserParaStartAddress+2*i,para[i]);
    }
    
    FLASH_Lock();
}

void logic_upstream_io_allow_output(void)
{
    if (((inverter_status_buffer[0].fault_code >> 4) & 0x1) == 1) {
        B_STREAM_OUT_(Bit_SET);
    }
    else {
        B_STREAM_OUT_(Bit_RESET);
    }
}
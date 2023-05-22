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


INVERTER_STATUS_T  inverter_status_buffer[10];//本机状态

#define moduleStatusQueueSize  20
MODULE_STATUS_T  moduleStatusBuff[moduleStatusQueueSize];
MODULE_STATUS_QUEUE  moduleStatusQueue;

#define uartSendQueueSize  30
COMM_NODE_T uartSendQueueBuff[uartSendQueueSize];
COMM_SEND_QUEUE uartSendQueue;
COMM_NODE_T comm_node;
u8  comm_busy_flag;
u8  polling_num;//轮询站号(从1开始)

u8  g_remote_start_flag;   //远程起停状态改变标记(0:无变化 1:停止变启动 2:启动变停止)
u8  g_remote_start_status; //远程起停状态(0:停止状态 1:运行状态)
u8  g_remote_speed_status; //远程高低速状态(0:低速 1:高速)
u8  g_link_up_stream_status; //联动上游信号
u8  g_link_down_stream_status; //联动下游信号
u8  g_set_start_status; //设置电机起停状态(0:停止 1:运行)
u8  g_set_speed_status; //设置电机高低速状态(0:低速 1:高速)
u8  g_read_start_status; //当前的电机起停状态(0:停止 1:运行)
u8  g_alarm_type; //故障类别(bit0:电机故障,bit1:堵包故障,bit2:485通讯故障)
u8  g_speed_gear_status; //当前的速度档位(0:停止 1~5:五档速度)
u8  g_block_disable_flag; //堵包检测功能禁止标记

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
    //队列满
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // 求出队尾的下一个位置
    q->queue[q->rear] = x; // 把x的值赋给新的队尾
}
COMM_NODE_T* GetUartSendDataFromQueue(void)
{
    COMM_SEND_QUEUE *q = &uartSendQueue;
    //队列空
    if(q->front == q->rear)
    {
        return NULL;
    }
    q->front = (q->front + 1) % q->maxSize; // 使队首指针指向下一个位置
    return (COMM_NODE_T*)(&(q->queue[q->front])); // 返回队首元素
}
u8 IsUartSendQueueFree(void)
{
    COMM_SEND_QUEUE *q = &uartSendQueue;
    //队列空
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
    //队列满
    if((q->rear + 1) % q->maxSize == q->front)
    {
        return;
    }
    q->rear = (q->rear + 1) % q->maxSize; // 求出队尾的下一个位置
    q->queue[q->rear] = x; // 把x的值赋给新的队尾
}
MODULE_STATUS_T* GetModuleStatusDataFromQueue(void)
{
    MODULE_STATUS_QUEUE *q = &moduleStatusQueue;
    //队列空
    if(q->front == q->rear)
    {
        return NULL;
    }
    q->front = (q->front + 1) % q->maxSize; // 使队首指针指向下一个位置
    return (MODULE_STATUS_T*)(&(q->queue[q->front])); // 返回队首元素
}
u8 IsModuleStatusQueueFree(void)
{
    MODULE_STATUS_QUEUE *q = &moduleStatusQueue;
    //队列空
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


    
    //处理复位输入信号
    if(  bResetInfo.input_info.input_state != l_bit_reset_in
      && bResetInfo.input_info.input_confirm_times == 0)
    {
        bResetInfo.input_info.input_middle_state = l_bit_reset_in;
    }
    if(  bResetInfo.input_info.input_middle_state == l_bit_reset_in
      && bResetInfo.input_info.input_middle_state != bResetInfo.input_info.input_state)
    {
        bResetInfo.input_info.input_confirm_times++;
        if(bResetInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
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
    //处理光电1输入信号
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
        if(bPhoto1Info.input_info.input_confirm_times > 50)//消抖时间50ms
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
    //处理光电2输入信号
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
        if(bPhoto2Info.input_info.input_confirm_times > 50)//消抖时间50ms
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

    //处理光电3输入信号
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
        if(bPhoto3Info.input_info.input_confirm_times > 50)//消抖时间50ms
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
    //处理光电4输入信号
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
        if(bPhoto4Info.input_info.input_confirm_times > 50)//消抖时间50ms
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
    //处理光电5输入信号
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
        if(bPhoto5Info.input_info.input_confirm_times > 50)//消抖时间50ms
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

    //处理启动输入信号
    if(  bStartInfo.input_info.input_state != l_bit_start_in
      && bStartInfo.input_info.input_confirm_times == 0)
    {
        bStartInfo.input_info.input_middle_state = l_bit_start_in;
    }
    if(  bStartInfo.input_info.input_middle_state == l_bit_start_in
      && bStartInfo.input_info.input_middle_state != bStartInfo.input_info.input_state)
    {
        bStartInfo.input_info.input_confirm_times++;
        if(bStartInfo.input_info.input_confirm_times > 100)//消抖时间100ms
        {
            bStartInfo.input_info.input_state = bStartInfo.input_info.input_middle_state;
            bStartInfo.input_info.input_confirm_times = 0;
            if(bStartInfo.input_info.input_state == 1)
            {
                if(isHost)//主机
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
    //处理停止输入信号
    if(  bStopInfo.input_info.input_state != l_bit_stop_in
      && bStopInfo.input_info.input_confirm_times == 0)
    {
        bStopInfo.input_info.input_middle_state = l_bit_stop_in;
    }
    if(  bStopInfo.input_info.input_middle_state == l_bit_stop_in
      && bStopInfo.input_info.input_middle_state != bStopInfo.input_info.input_state)
    {
        bStopInfo.input_info.input_confirm_times++;
        if(bStopInfo.input_info.input_confirm_times > 100)//消抖时间100ms
        {
            bStopInfo.input_info.input_state = bStopInfo.input_info.input_middle_state;
            bStopInfo.input_info.input_confirm_times = 0;
            if(bStopInfo.input_info.input_state == 1)
            {
                if(isHost)//主机
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

    //处理下游准入信号
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
        if ((bStreamInfo.input_info.input_middle_state == 1) && (bStreamInfo.input_info.input_confirm_times > 20))//消抖时间100ms
        {
            bStreamInfo.input_info.input_state = bStreamInfo.input_info.input_middle_state;
            bStreamInfo.input_info.input_confirm_times = 0;
            if (bStreamInfo.input_info.input_state == 1) {
                Linkage_stream_extra_signal(user_paras_local.Belt_Number - 1, 1);
            }
        }

        if ((bStreamInfo.input_info.input_middle_state == 0) && (bStreamInfo.input_info.input_confirm_times > 200))//消抖时间100ms
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

    //处理急停输入信号
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
        if (bEmergencyInfo.input_info.input_confirm_times > 50)//消抖时间50ms
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

//启动,停止,变速时改变所有皮带的速度
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
    
    if(g_speed_gear_status == 0 && speed_gear > 0)//启动
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
    else if( speed_gear == 0)//停止
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
    else if(g_speed_gear_status != 0 && speed_gear != 0)//变速
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
    if(inverter_status.fault_code & 0x1)//485通讯故障
        return 1;
    if((inverter_status.fault_code>>1) & 0x1)//堵包故障
        return 1;
    if(((inverter_status.fault_code>>8)&0xFF) != 0)//变频器故障码
        return 1;
    return 0;;
}
//联动停止_光电触发控制
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
    
    if(((user_paras_local.belt_para[photo_index].Func_Select_Switch>>3)&0x1) == 0)//未启用积放功能
    {
        return;
    }
    
    if(photo_index+1 == user_paras_local.Belt_Number)
    {
        for (i = 0; i < user_paras_local.Belt_Number; i++) {
            if ((user_paras_local.belt_para[i].Func_Select_Switch >> 4) & 0x1)
            {
                if ((user_paras_local.Down_Stream_No == 0)  && (bStreamInfo.input_info.input_state == 0))//没有配置下游站号
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
        if(user_paras_local.Down_Stream_No != 0)//配置了下游站号
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
    if((user_paras_local.belt_para[photo_index].Func_Select_Switch>>1)&0x1)//联动功能开启
    {
        if((inverter_status_buffer[photo_index].fault_code>>4)&0x1)//本地皮带运行状态
        {
            if(link_down_status == 0)//下游不允许进入
            {
                //停止
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
            if ((user_paras_local.belt_para[inverter_index].Func_Select_Switch >> 1) & 0x1)//联动功能开启
            {
                if (start_flag == 1)//联动启动
                {
                    if ((((inverter_status_buffer[inverter_index].fault_code >> 4) & 0x1) == 0)
                        && (get_inverter_fault_status(inverter_status_buffer[inverter_index]) != 1))//本地皮带停止状态且没有报警
                    {
                        //启动
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = inverter_index + 1;
                        comm_node_new.speed_gear = g_speed_gear_status;
                        comm_node_new.comm_interval = user_paras_local.belt_para[inverter_index].Start_Delay_Time;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                    }
                }
                else//联动停止
                {
                    if ((inverter_status_buffer[inverter_index].fault_code >> 4) & 0x1)//本地皮带运行状态
                    {
                        //停止
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

//联动起停_上下游触发控制
void Linkage_Stream_Ctrl_Handle(u8 inverter_index,u8 start_flag)
{
    COMM_NODE_T  comm_node_new;
    
    if((user_paras_local.belt_para[inverter_index].Func_Select_Switch>>1)&0x1)//联动功能开启
    {
        if(start_flag == 1)//联动启动
        {
            if(  (((inverter_status_buffer[inverter_index].fault_code>>4)&0x1) == 0)
              && (get_inverter_fault_status(inverter_status_buffer[inverter_index]) != 1) )//本地皮带停止状态且没有报警
            {
                //启动
                comm_node_new.rw_flag = 1;
                comm_node_new.inverter_no = inverter_index+1;
                comm_node_new.speed_gear = g_speed_gear_status;
                comm_node_new.comm_interval = user_paras_local.belt_para[inverter_index].Start_Delay_Time;
                comm_node_new.comm_retry = 3;
                AddUartSendData2Queue(comm_node_new);
            }
        }
        else//联动停止
        {
            if(((user_paras_local.belt_para[inverter_index].Func_Select_Switch>>3)&0x1) == 1)//启用积放功能就由光电触发停止
                return;
            if((inverter_status_buffer[inverter_index].fault_code>>4)&0x1)//本地皮带运行状态
            {
                //停止
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
//堵包检测
void Block_Check_Ctrl_Handle(void)
{
    COMM_NODE_T  comm_node_new;
    u8  i;
    
    if(g_block_disable_flag == 1) return;
    
    for(i=0; i<user_paras_local.Belt_Number; i++)
    {
        if((user_paras_local.belt_para[i].Func_Select_Switch>>2)&0x1)//堵包检测开启
        {
            if((inverter_status_buffer[i].fault_code>>4)&0x1)//本地皮带运行状态
            {
                if(get_photo_input_status(i) == 1)//光电被遮挡
                {
                    block_check_time_cnt[i]++;
                    if(block_check_time_cnt[i] >= user_paras_local.belt_para[i].Block_Check_Time)
                    {
                        block_check_time_cnt[i] = 0;
                        //停止
                        comm_node_new.rw_flag = 1;
                        comm_node_new.inverter_no = i+1;
                        comm_node_new.speed_gear = 0;
                        comm_node_new.comm_interval = 10;
                        comm_node_new.comm_retry = 3;
                        AddUartSendData2Queue(comm_node_new);
                        L_ALARM_OUT_(Bit_SET);//报警指示灯输出
                        inverter_status_buffer[i].fault_code |= (0x1<<1);//堵包故障
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
//堵包复位,变频器故障复位
void Reset_Ctrl_Handle(void)
{
    COMM_NODE_T  comm_node_new;
    u8  i;
    
    for(i=0; i<user_paras_local.Belt_Number; i++)
    {
        if((inverter_status_buffer[i].fault_code>>1)&0x1)//堵包复位
        {
            inverter_status_buffer[i].fault_code &= ~(0x1<<1);
        }
        if(((inverter_status_buffer[i].fault_code>>8)&0xFF) != 0)//变频器复位
        {
            comm_node_new.rw_flag = 2;
            comm_node_new.inverter_no = i+1;
            comm_node_new.speed_gear = 0;
            comm_node_new.comm_interval = 20;
            comm_node_new.comm_retry = 3;
            AddUartSendData2Queue(comm_node_new);
        }
    }
    L_ALARM_OUT_(Bit_RESET);//报警指示灯熄灭
    reset_start_time_cnt = 500;
}
//复位后启动变频器
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
         // && (get_inverter_fault_status(inverter_status_buffer[i-1]) != 1) )//本地皮带停止状态且没有报警
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
    //参数有效性交验
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
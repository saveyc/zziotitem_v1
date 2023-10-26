#include "main.h"

USER_PARAS_T  user_paras_local;
USER_PARAS_T  user_paras_slave;
USER_PARAS_T  user_paras_temp;

sButton_Info  bModeInfo;
sButton_Info  bStartInfo;
sButton_Info  bHSpeedInfo;
sButton_Info  bLSpeedInfo;
sButton_Info  bPauseInfo;
sButton_Info  bremoteInfo;

MODULE_STATUS_T  module_status_buffer[256];

u8  g_remote_start_flag;   //远程起停状态改变标记(0:无变化 1:停止变启动 2:启动变停止)
u8  g_remote_start_status; //远程起停状态(0:停止状态 1:运行状态)
u8  g_remote_speed_status; //远程高低速状态(0:低速 1:高速)
u8  g_link_up_stream_status; //联动上游信号
u8  g_link_down_stream_status; //联动下游信号
u8  g_set_start_status; //设置电机起停状态(0:停止 1:运行)
u8  g_set_speed_status; //设置电机高低速状态(0:低速 1:高速)
u8  g_read_start_status; //当前的电机起停状态(0:停止 1:运行)
u8  g_alarm_type; //故障类别(bit0:电机故障,bit1:堵包故障,bit2:485通讯故障)

u16 start_delay_time_cnt;
u16 stop_delay_time_cnt;
u16 block_check_time_cnt;
u16 NotiLED_time_cnt;

srun_ctrl runctrl;
srun_state runstate;
sstart_flash startflash;

/* scan every 1ms */
void InputScanProc()
{
    u8 l_bit_mode_in;
    u8 l_bit_start_in;
    u8 l_bit_h_speed_in;
    u8 l_bit_l_speed_in;
    u8 l_bit_pause_in;
    u8 l_bit_remote_start_in;
    
    l_bit_mode_in    = B_MODE_IN_STATE;
    l_bit_start_in   = B_START_IN_STATE;
    l_bit_h_speed_in = B_H_SPEED_IN_STATE;
    l_bit_l_speed_in = B_L_SPEED_IN_STATE;
    l_bit_pause_in   = B_PAUSE_IN_STATE;
    l_bit_remote_start_in = B_REMOTE_START_IN;

    
    //处理本地/远程模式切换输入信号
    if(  bModeInfo.input_info.input_state != l_bit_mode_in
      && bModeInfo.input_info.input_confirm_times == 0)
    {
        bModeInfo.input_info.input_middle_state = l_bit_mode_in;
    }
    if(  bModeInfo.input_info.input_middle_state == l_bit_mode_in
      && bModeInfo.input_info.input_middle_state != bModeInfo.input_info.input_state)
    {
        bModeInfo.input_info.input_confirm_times++;
        if(bModeInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bModeInfo.input_info.input_state = bModeInfo.input_info.input_middle_state;
            bModeInfo.input_info.input_confirm_times = 0;
            if(bModeInfo.input_info.input_state == 1)
            {
                bModeInfo.button_state = 1;
            }
            else
            {
                bModeInfo.button_state = 0;
            }
        }
    }
    else
    {
        bModeInfo.input_info.input_middle_state = bModeInfo.input_info.input_state;
        bModeInfo.input_info.input_confirm_times = 0;
    }
    //处理启动按钮输入信号
    if(  bStartInfo.input_info.input_state != l_bit_start_in
      && bStartInfo.input_info.input_confirm_times == 0)
    {
        bStartInfo.input_info.input_middle_state = l_bit_start_in;
    }
    if(  bStartInfo.input_info.input_middle_state == l_bit_start_in
      && bStartInfo.input_info.input_middle_state != bStartInfo.input_info.input_state)
    {
        bStartInfo.input_info.input_confirm_times++;
        if(bStartInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bStartInfo.input_info.input_state = bStartInfo.input_info.input_middle_state;
            bStartInfo.input_info.input_confirm_times = 0;
            if(bStartInfo.input_info.input_state == 1)
            {
                bStartInfo.button_state = 1;
            }
            else
            {
                bStartInfo.button_state = 0;
            }
        }
    }
    else
    {
        bStartInfo.input_info.input_middle_state = bStartInfo.input_info.input_state;
        bStartInfo.input_info.input_confirm_times = 0;
    }
    //处理高速旋钮输入信号
    if(  bHSpeedInfo.input_info.input_state != l_bit_h_speed_in
      && bHSpeedInfo.input_info.input_confirm_times == 0)
    {
        bHSpeedInfo.input_info.input_middle_state = l_bit_h_speed_in;
    }
    if(  bHSpeedInfo.input_info.input_middle_state == l_bit_h_speed_in
      && bHSpeedInfo.input_info.input_middle_state != bHSpeedInfo.input_info.input_state)
    {
        bHSpeedInfo.input_info.input_confirm_times++;
        if(bHSpeedInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bHSpeedInfo.input_info.input_state = bHSpeedInfo.input_info.input_middle_state;
            bHSpeedInfo.input_info.input_confirm_times = 0;
            if(bHSpeedInfo.input_info.input_state == 1)
            {
                bHSpeedInfo.button_state = 1;
            }
            else
            {
                bHSpeedInfo.button_state = 0;
            }
        }
    }
    else
    {
        bHSpeedInfo.input_info.input_middle_state = bHSpeedInfo.input_info.input_state;
        bHSpeedInfo.input_info.input_confirm_times = 0;
    }
    //处理低速旋钮输入信号
    if(  bLSpeedInfo.input_info.input_state != l_bit_l_speed_in
      && bLSpeedInfo.input_info.input_confirm_times == 0)
    {
        bLSpeedInfo.input_info.input_middle_state = l_bit_l_speed_in;
    }
    if(  bLSpeedInfo.input_info.input_middle_state == l_bit_l_speed_in
      && bLSpeedInfo.input_info.input_middle_state != bLSpeedInfo.input_info.input_state)
    {
        bLSpeedInfo.input_info.input_confirm_times++;
        if(bLSpeedInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bLSpeedInfo.input_info.input_state = bLSpeedInfo.input_info.input_middle_state;
            bLSpeedInfo.input_info.input_confirm_times = 0;
            if(bLSpeedInfo.input_info.input_state == 1)
            {
                bLSpeedInfo.button_state = 1;
            }
            else
            {
                bLSpeedInfo.button_state = 0;
            }
        }
    }
    else
    {
        bLSpeedInfo.input_info.input_middle_state = bLSpeedInfo.input_info.input_state;
        bLSpeedInfo.input_info.input_confirm_times = 0;
    }
    //处理人工暂停输入信号
    if(  bPauseInfo.input_info.input_state != l_bit_pause_in
      && bPauseInfo.input_info.input_confirm_times == 0)
    {
        bPauseInfo.input_info.input_middle_state = l_bit_pause_in;
    }
    if(  bPauseInfo.input_info.input_middle_state == l_bit_pause_in
      && bPauseInfo.input_info.input_middle_state != bPauseInfo.input_info.input_state)
    {
        bPauseInfo.input_info.input_confirm_times++;
        if(bPauseInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bPauseInfo.input_info.input_state = bPauseInfo.input_info.input_middle_state;
            bPauseInfo.input_info.input_confirm_times = 0;
            if(bPauseInfo.input_info.input_state == 1)
            {
                if(bModeInfo.button_state == 1)//远程模式有效
                {
                    bPauseInfo.button_state = 1;
                }
            }
            else
            {
                bPauseInfo.button_state = 0;
            }
        }
    }
    else
    {
        bPauseInfo.input_info.input_middle_state = bPauseInfo.input_info.input_state;
        bPauseInfo.input_info.input_confirm_times = 0;
    }

    //处理远程启停输入信号
    if (bremoteInfo.input_info.input_state != l_bit_remote_start_in
        && bremoteInfo.input_info.input_confirm_times == 0)
    {
        bremoteInfo.input_info.input_middle_state = l_bit_remote_start_in;
    }
    if (bremoteInfo.input_info.input_middle_state == l_bit_remote_start_in
        && bremoteInfo.input_info.input_middle_state != bremoteInfo.input_info.input_state)
    {
        bremoteInfo.input_info.input_confirm_times++;
        if (bremoteInfo.input_info.input_confirm_times > 50)//按钮消抖时间50ms
        {
            bremoteInfo.input_info.input_state = bremoteInfo.input_info.input_middle_state;
            bremoteInfo.input_info.input_confirm_times = 0;
            if (bremoteInfo.input_info.input_state == 1)
            {
                    bremoteInfo.button_state = 1;
                    bremoteInfo.input_info.input_trig_mode = INPUT_TRIG_UP;
            }
            else
            {
                bremoteInfo.input_info.input_trig_mode = INPUT_TRIG_DOWN;
                bremoteInfo.button_state = 0;
            }
        }
    }
    else
    {
        bremoteInfo.input_info.input_middle_state = bremoteInfo.input_info.input_state;
        bremoteInfo.input_info.input_confirm_times = 0;
    }

    //下游联动信号
    if((user_paras_local.Func_Select_Switch>>3)&0x1)//端子输入
    {
        g_link_down_stream_status = B_STREAM_IN_STATE;
        if (user_paras_local.Down_Stream_No == 0)//没有配置下游站号
        {
            g_link_down_stream_status = 1;
        }
    }
    else//CAN输入
    {
        if(user_paras_local.Down_Stream_No == 0)//没有配置下游站号
        {
            g_link_down_stream_status = 1;
        }
    }
}
//处理起停控制逻辑
void Logic_Ctrl_Process(void)
{
    if(bModeInfo.button_state == 0)//本地模式
    {
        g_set_start_status = bStartInfo.button_state;
        g_set_speed_status = (bLSpeedInfo.button_state == 1)?0:2;
        if(g_alarm_type)//有报警
        {
            g_set_start_status = 0;
        }
    }
    else//远程模式
    {
        g_set_speed_status = g_remote_speed_status;

        if (bremoteInfo.input_info.input_trig_mode == INPUT_TRIG_UP)
        {
            bremoteInfo.input_info.input_trig_mode = INPUT_TRIG_NULL;
            g_remote_speed_status = 2;
            runctrl.start_cnt = DEFAULT_TIME;
            startflash.flash_reg = FLASH_TIME;
            runctrl.stop_cnt = 0;
            stop_delay_time_cnt = 0;
            runstate.runstatus = 1;
            runstate.runspeed = 2;
        }
        else if(bremoteInfo.input_info.input_trig_mode == INPUT_TRIG_DOWN)
        {
            bremoteInfo.input_info.input_trig_mode = INPUT_TRIG_NULL;
            runctrl.stop_cnt = 10;
            runctrl.start_cnt = 0;
            startflash.flash_reg = FLASH_TIME;
            g_remote_speed_status = 2;
            start_delay_time_cnt = 0;
            runstate.runstatus = 0;
            runstate.runspeed = 2;
        }
        
        

        g_set_speed_status = g_remote_speed_status;

        if(bStartInfo.button_state)
        {
            g_set_start_status = 1;
        }
        else
        {
            g_set_start_status = 0;
        }
    }
   
    if(bPauseInfo.button_state == 1)//人工暂停
    {
        g_set_start_status = 0;
    }
    if(g_set_start_status)
    {
        B_STREAM_OUT_(Bit_SET);
        L_START_OUT_(Bit_SET);
    }
    else
    {
        B_STREAM_OUT_(Bit_RESET);
        L_START_OUT_(Bit_RESET);
    }
    if(g_read_start_status != g_set_start_status)//起停状态改变
    {
        g_read_start_status = g_set_start_status;
        can_bus_send_module_status();
    }
    //联动控制
    if(  ((user_paras_local.Func_Select_Switch>>1)&0x1 == 1)
      && (g_remote_start_status == 1) )//联动功能开启并且处于远程启动状态
    {
         if(g_read_start_status == 1)//本地皮带运行状态
         {
             if(g_link_down_stream_status == 0 && B_PHOTO_1_IN_STATE == 1)//下游不允许进入且光电被触发
             {
                 B_START_OUT_(Bit_RESET);//停止
             }
         }
         else//本地皮带停止状态
         {
             if((g_link_down_stream_status == 1) && (g_alarm_type == 0))//下游允许进入且没有报警
             {
                 B_START_OUT_(Bit_SET);//启动
             }
         }
    }
    //堵包检测
    if(((user_paras_local.Func_Select_Switch>>2)&0x1 == 1)&&(g_read_start_status == 1))//堵包检测开启,运行状态
    {
        if(B_PHOTO_1_IN_STATE == 1)//光电被遮挡
        {
            block_check_time_cnt++;
            if(block_check_time_cnt >= user_paras_local.Block_Check_Time)
            {
                B_START_OUT_(Bit_RESET);//停止
                L_ALARM_OUT_(Bit_SET);//堵包报警指示灯输出
                g_alarm_type |= (0x1<<1);//堵包故障
//                L_NOTI_START_OUT_(Bit_SET);//故障报警提示
            }
            
        }
        else
        {
            block_check_time_cnt = 0;
           // L_ALARM_OUT_(Bit_RESET);//堵包报警指示灯输出
           // g_alarm_type &= ~(0x1<<1);//堵包故障

        }
    }
    //堵包复位
    if((g_alarm_type & 0x2) != 0 && B_RESET_IN_STATE == 1 && B_PHOTO_1_IN_STATE == 0)
    {
        g_alarm_type &= ~(0x1<<1);//解除堵包故障
        L_ALARM_OUT_(Bit_RESET);//堵包报警指示灯输出
    }


    //电机故障检测
    if(B_ALARM_IN_STATE == 1)
    {
        B_START_OUT_(Bit_RESET);//停止
        g_alarm_type |= 0x1;//电机故障
//        L_NOTI_START_OUT_(Bit_SET);//故障报警提示
        L_ALARM_OUT_(Bit_SET);//堵包报警指示灯输出
    }
    //电机故障复位
    if((g_alarm_type & 0x1) != 0 && B_ALARM_IN_STATE == 0)
    {
        g_alarm_type &= ~(0x1);//解除电机故障
//        L_NOTI_START_OUT_(Bit_RESET);//解除故障报警提示
        L_ALARM_OUT_(Bit_RESET);//堵包报警指示灯输出
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
    user_paras_local.Version_No_L = 0x0821;
    user_paras_local.Version_No_H = 0x2021;
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

void Motor_DelayAction(void)
{

    logit_start_flash();

    if (runctrl.start_cnt != 0) {
        runctrl.start_cnt--;
        if (runctrl.start_cnt == 0) {
            g_remote_start_flag = 1;
            start_delay_time_cnt = user_paras_local.Start_Delay_Time;
            L_ALARM_OUT_(Bit_RESET);
        }
    }

    if (runctrl.stop_cnt != 0) {
        runctrl.stop_cnt--;
        if (runctrl.stop_cnt == 0) {
            g_remote_start_flag = 2;
            stop_delay_time_cnt = user_paras_local.Stop_Delay_Time;
            L_ALARM_OUT_(Bit_RESET);
        }
    }

    if(g_remote_start_flag == 1)//延时启动逻辑 
    {
        if((g_link_down_stream_status == 1)&&(g_alarm_type == 0)) //下游允许进入且没有报警
        {
            if (start_delay_time_cnt > 0) {
                start_delay_time_cnt--;
            }
            if (start_delay_time_cnt == 0) {
                g_remote_start_status = 1;
                g_remote_start_flag = 0;
                B_START_OUT_(Bit_SET);
            }
        }
    }
    else if(g_remote_start_flag == 2)//延时停止逻辑
    {
        if (stop_delay_time_cnt > 0) {
            stop_delay_time_cnt--;
        }

        if (stop_delay_time_cnt == 0) {
            g_remote_start_status = 0;
            g_remote_start_flag = 0;
            B_START_OUT_(Bit_RESET);
        }
    }
}

void logit_start_flash(void)
{
    if (runctrl.start_cnt == 0) return;

    if (startflash.flash_reg != 0) {
        startflash.flash_reg--;
    }
    if (startflash.flash_reg == 0) {
        startflash.flash_reg = FLASH_TIME;
        if (startflash.flash_flag == 0) {
            startflash.flash_flag = 1;
            L_ALARM_OUT_(Bit_SET);
        }
        else {
            startflash.flash_flag = 0;
             L_ALARM_OUT_(Bit_RESET);
        }
    }
}
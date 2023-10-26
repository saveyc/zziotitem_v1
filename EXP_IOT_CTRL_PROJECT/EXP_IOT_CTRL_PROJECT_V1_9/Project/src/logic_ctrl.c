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

u8  g_remote_start_flag;   //Զ����ͣ״̬�ı���(0:�ޱ仯 1:ֹͣ������ 2:������ֹͣ)
u8  g_remote_start_status; //Զ����ͣ״̬(0:ֹͣ״̬ 1:����״̬)
u8  g_remote_speed_status; //Զ�̸ߵ���״̬(0:���� 1:����)
u8  g_link_up_stream_status; //���������ź�
u8  g_link_down_stream_status; //���������ź�
u8  g_set_start_status; //���õ����ͣ״̬(0:ֹͣ 1:����)
u8  g_set_speed_status; //���õ���ߵ���״̬(0:���� 1:����)
u8  g_read_start_status; //��ǰ�ĵ����ͣ״̬(0:ֹͣ 1:����)
u8  g_alarm_type; //�������(bit0:�������,bit1:�°�����,bit2:485ͨѶ����)

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

    
    //������/Զ��ģʽ�л������ź�
    if(  bModeInfo.input_info.input_state != l_bit_mode_in
      && bModeInfo.input_info.input_confirm_times == 0)
    {
        bModeInfo.input_info.input_middle_state = l_bit_mode_in;
    }
    if(  bModeInfo.input_info.input_middle_state == l_bit_mode_in
      && bModeInfo.input_info.input_middle_state != bModeInfo.input_info.input_state)
    {
        bModeInfo.input_info.input_confirm_times++;
        if(bModeInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
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
    //����������ť�����ź�
    if(  bStartInfo.input_info.input_state != l_bit_start_in
      && bStartInfo.input_info.input_confirm_times == 0)
    {
        bStartInfo.input_info.input_middle_state = l_bit_start_in;
    }
    if(  bStartInfo.input_info.input_middle_state == l_bit_start_in
      && bStartInfo.input_info.input_middle_state != bStartInfo.input_info.input_state)
    {
        bStartInfo.input_info.input_confirm_times++;
        if(bStartInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
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
    //���������ť�����ź�
    if(  bHSpeedInfo.input_info.input_state != l_bit_h_speed_in
      && bHSpeedInfo.input_info.input_confirm_times == 0)
    {
        bHSpeedInfo.input_info.input_middle_state = l_bit_h_speed_in;
    }
    if(  bHSpeedInfo.input_info.input_middle_state == l_bit_h_speed_in
      && bHSpeedInfo.input_info.input_middle_state != bHSpeedInfo.input_info.input_state)
    {
        bHSpeedInfo.input_info.input_confirm_times++;
        if(bHSpeedInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
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
    //���������ť�����ź�
    if(  bLSpeedInfo.input_info.input_state != l_bit_l_speed_in
      && bLSpeedInfo.input_info.input_confirm_times == 0)
    {
        bLSpeedInfo.input_info.input_middle_state = l_bit_l_speed_in;
    }
    if(  bLSpeedInfo.input_info.input_middle_state == l_bit_l_speed_in
      && bLSpeedInfo.input_info.input_middle_state != bLSpeedInfo.input_info.input_state)
    {
        bLSpeedInfo.input_info.input_confirm_times++;
        if(bLSpeedInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
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
    //�����˹���ͣ�����ź�
    if(  bPauseInfo.input_info.input_state != l_bit_pause_in
      && bPauseInfo.input_info.input_confirm_times == 0)
    {
        bPauseInfo.input_info.input_middle_state = l_bit_pause_in;
    }
    if(  bPauseInfo.input_info.input_middle_state == l_bit_pause_in
      && bPauseInfo.input_info.input_middle_state != bPauseInfo.input_info.input_state)
    {
        bPauseInfo.input_info.input_confirm_times++;
        if(bPauseInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
        {
            bPauseInfo.input_info.input_state = bPauseInfo.input_info.input_middle_state;
            bPauseInfo.input_info.input_confirm_times = 0;
            if(bPauseInfo.input_info.input_state == 1)
            {
                if(bModeInfo.button_state == 1)//Զ��ģʽ��Ч
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

    //����Զ����ͣ�����ź�
    if (bremoteInfo.input_info.input_state != l_bit_remote_start_in
        && bremoteInfo.input_info.input_confirm_times == 0)
    {
        bremoteInfo.input_info.input_middle_state = l_bit_remote_start_in;
    }
    if (bremoteInfo.input_info.input_middle_state == l_bit_remote_start_in
        && bremoteInfo.input_info.input_middle_state != bremoteInfo.input_info.input_state)
    {
        bremoteInfo.input_info.input_confirm_times++;
        if (bremoteInfo.input_info.input_confirm_times > 50)//��ť����ʱ��50ms
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

    //���������ź�
    if((user_paras_local.Func_Select_Switch>>3)&0x1)//��������
    {
        g_link_down_stream_status = B_STREAM_IN_STATE;
        if (user_paras_local.Down_Stream_No == 0)//û����������վ��
        {
            g_link_down_stream_status = 1;
        }
    }
    else//CAN����
    {
        if(user_paras_local.Down_Stream_No == 0)//û����������վ��
        {
            g_link_down_stream_status = 1;
        }
    }
}
//������ͣ�����߼�
void Logic_Ctrl_Process(void)
{
    if(bModeInfo.button_state == 0)//����ģʽ
    {
        g_set_start_status = bStartInfo.button_state;
        g_set_speed_status = (bLSpeedInfo.button_state == 1)?0:2;
        if(g_alarm_type)//�б���
        {
            g_set_start_status = 0;
        }
    }
    else//Զ��ģʽ
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
   
    if(bPauseInfo.button_state == 1)//�˹���ͣ
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
    if(g_read_start_status != g_set_start_status)//��ͣ״̬�ı�
    {
        g_read_start_status = g_set_start_status;
        can_bus_send_module_status();
    }
    //��������
    if(  ((user_paras_local.Func_Select_Switch>>1)&0x1 == 1)
      && (g_remote_start_status == 1) )//�������ܿ������Ҵ���Զ������״̬
    {
         if(g_read_start_status == 1)//����Ƥ������״̬
         {
             if(g_link_down_stream_status == 0 && B_PHOTO_1_IN_STATE == 1)//���β���������ҹ�类����
             {
                 B_START_OUT_(Bit_RESET);//ֹͣ
             }
         }
         else//����Ƥ��ֹͣ״̬
         {
             if((g_link_down_stream_status == 1) && (g_alarm_type == 0))//�������������û�б���
             {
                 B_START_OUT_(Bit_SET);//����
             }
         }
    }
    //�°����
    if(((user_paras_local.Func_Select_Switch>>2)&0x1 == 1)&&(g_read_start_status == 1))//�°���⿪��,����״̬
    {
        if(B_PHOTO_1_IN_STATE == 1)//��类�ڵ�
        {
            block_check_time_cnt++;
            if(block_check_time_cnt >= user_paras_local.Block_Check_Time)
            {
                B_START_OUT_(Bit_RESET);//ֹͣ
                L_ALARM_OUT_(Bit_SET);//�°�����ָʾ�����
                g_alarm_type |= (0x1<<1);//�°�����
//                L_NOTI_START_OUT_(Bit_SET);//���ϱ�����ʾ
            }
            
        }
        else
        {
            block_check_time_cnt = 0;
           // L_ALARM_OUT_(Bit_RESET);//�°�����ָʾ�����
           // g_alarm_type &= ~(0x1<<1);//�°�����

        }
    }
    //�°���λ
    if((g_alarm_type & 0x2) != 0 && B_RESET_IN_STATE == 1 && B_PHOTO_1_IN_STATE == 0)
    {
        g_alarm_type &= ~(0x1<<1);//����°�����
        L_ALARM_OUT_(Bit_RESET);//�°�����ָʾ�����
    }


    //������ϼ��
    if(B_ALARM_IN_STATE == 1)
    {
        B_START_OUT_(Bit_RESET);//ֹͣ
        g_alarm_type |= 0x1;//�������
//        L_NOTI_START_OUT_(Bit_SET);//���ϱ�����ʾ
        L_ALARM_OUT_(Bit_SET);//�°�����ָʾ�����
    }
    //������ϸ�λ
    if((g_alarm_type & 0x1) != 0 && B_ALARM_IN_STATE == 0)
    {
        g_alarm_type &= ~(0x1);//����������
//        L_NOTI_START_OUT_(Bit_RESET);//������ϱ�����ʾ
        L_ALARM_OUT_(Bit_RESET);//�°�����ָʾ�����
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

    if(g_remote_start_flag == 1)//��ʱ�����߼� 
    {
        if((g_link_down_stream_status == 1)&&(g_alarm_type == 0)) //�������������û�б���
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
    else if(g_remote_start_flag == 2)//��ʱֹͣ�߼�
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
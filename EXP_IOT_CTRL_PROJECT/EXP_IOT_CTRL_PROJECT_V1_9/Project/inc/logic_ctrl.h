#ifndef _LIGIC_H
#define _LOGIC_H

#define VALUE 1
#define INVALUE 0

#define DEFAULT_TIME   5000
#define FLASH_TIME     500


#define  B_MODE_IN_STATE       IN1_0_STATE   //����/Զ��ģʽ�л�����
#define  B_START_IN_STATE      IN1_1_STATE   //�����ź�����
#define  B_ALARM_IN_STATE      IN1_2_STATE   //�����ź�����
#define  B_H_SPEED_IN_STATE    IN1_3_STATE   //������ť����
#define  B_L_SPEED_IN_STATE    IN1_4_STATE   //������ť����
#define  B_RESET_IN_STATE      IN1_5_STATE   //�°���λ��ť����
#define  B_PAUSE_IN_STATE      IN1_6_STATE   //�˹���ͣ��ť����
#define  B_STREAM_IN_STATE     IN2_0_STATE   //����׼���ź�����(����Ϊ����)
#define  B_PHOTO_1_IN_STATE    IN2_1_STATE   //���1����
#define  B_REMOTE_START_IN     IN2_5_STATE   //Զ��IO��ͣ�ź�

#define  L_START_OUT_(BitVal)      OUT_1_0_(BitVal)   //����ָʾ�����
#define  L_ALARM_OUT_(BitVal)      OUT_1_6_(BitVal)   //����ָʾ�����
#define  B_START_OUT_(BitVal)      OUT_1_2_(BitVal)   //Զ��ģʽ�°忨����Ƶ��������/ֹͣ�ź�
#define  B_RESET_OUT_(BitVal)      OUT_1_1_(BitVal)   //���ϸ�λ���
#define  B_STREAM_OUT_(BitVal)     OUT_1_3_(BitVal)   //����׼���ź����(����Ϊ����)
#define  L_NOTI_START_OUT_(BitVal) OUT_1_5_(BitVal)   //����Ƥ����������



#define  INPUT_TRIG_NULL    0
#define  INPUT_TRIG_UP      1
#define  INPUT_TRIG_DOWN    2

typedef struct {
    u8  input_state;
    u8  input_middle_state;
    u8  input_confirm_times;
    u8  input_trig_mode; //null,up,down
}sInput_Info;

typedef struct {
    sInput_Info  input_info;
    u16          button_state; //��ť��û�б�����
    u16          button_hold_time; //��ť��ס��ʱ���ʱ(ms)
}sButton_Info;

#define USER_PARA_DATA_LEN    15

typedef struct {
    u16 Station_No;         //ģ��վ��
    u16 Version_No_L;       //ģ��汾��(����)
    u16 Version_No_H;       //ģ��汾��(����)
    u16 Speed_Factor;       //�ٶȱ���ϵ��(���ͱ�ʾ������,��:123��ʾ1.23)
    u16 Motor_Type;         //�������(��Ƶ��0/�ŷ�1)
    u16 Motor_Model;        //����ͺ�
    u16 Func_Select_Switch; //����ѡ�񿪹�(bit0:�Ƿ������Զ�����;bit1:ǰ����������;bit2:�°���⹦��;bit3:�����ź�ѡ��(���ź�CAN/��������))
    u16 High_Speed_Target;  //����Ŀ���ٶ�(���ͱ�ʾ������,��:123��ʾ1.23m/s)
    u16 High_Speed_Freq;    //����Ƶ��ֵ/ת��(Ƶ��ֵ���ͱ�ʾ������,��:1234��ʾ12.34hz)
    u16 Middle_Speed_Target;//����Ŀ���ٶ�(���ͱ�ʾ������,��:123��ʾ1.23m/s)
    u16 Middle_Speed_Freq;  //����Ƶ��ֵ/ת��(Ƶ��ֵ���ͱ�ʾ������,��:1234��ʾ12.34hz)
    u16 Low_Speed_Target;   //����Ŀ���ٶ�(���ͱ�ʾ������,��:123��ʾ1.23m/s)
    u16 Low_Speed_Freq;     //����Ƶ��ֵ/ת��(Ƶ��ֵ���ͱ�ʾ������,��:1234��ʾ12.34hz)
    u16 Start_Delay_Time;   //������ʱʱ��
    u16 Stop_Delay_Time;    //ֹͣ��ʱʱ��
    u16 Block_Check_Time;   //�°����ʱ��
    u16 Up_Stream_No;       //����վ��
    u16 Down_Stream_No;     //����վ��
}USER_PARAS_T;

typedef struct {
    u8  flag;
    u8  status;
    u8  alarm;
    u16 cur_speed;
    u16 inverter_hz;
}MODULE_STATUS_T;


typedef struct {
    u16 start_cnt;
    u16 stop_cnt;
}srun_ctrl;

typedef struct {
    u8 runstatus;
    u8 runspeed;
}srun_state;

typedef struct{
    u16 flash_reg;
    u16 flash_flag;
}sstart_flash;


extern sButton_Info  bModeInfo;
extern sButton_Info  bStartInfo;
extern sButton_Info  bHSpeedInfo;
extern sButton_Info  bLSpeedInfo;
extern sButton_Info  bPauseInfo;
extern sButton_Info  bremoteInfo;

extern u8  g_remote_start_flag;
extern u8  g_remote_start_status;
extern u8  g_remote_speed_status;
extern u8  g_link_up_stream_status;
extern u8  g_link_down_stream_status;
extern u8  g_set_start_status;
extern u8  g_set_speed_status;
extern u8  g_read_start_status;
extern u8  g_alarm_type;

extern u16 start_delay_time_cnt;
extern u16 stop_delay_time_cnt;
extern u16 NotiLED_time_cnt;

extern USER_PARAS_T  user_paras_local;
extern USER_PARAS_T  user_paras_slave;
extern USER_PARAS_T  user_paras_temp;

extern MODULE_STATUS_T  module_status_buffer[];

extern srun_ctrl runctrl;
extern srun_state runstate;
extern sstart_flash startflash;

void read_user_paras(void);
void write_user_paras(u16* para);
void InputScanProc();
void Logic_Ctrl_Process();
void Motor_DelayAction(void);
void logit_start_flash(void);

#endif
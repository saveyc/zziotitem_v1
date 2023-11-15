#ifndef _LOGIC_H
#define _LOGIC_H

#define  B_START_IN_STATE      IN1_0_STATE   //�����ź�
#define  B_STOP_IN_STATE       IN1_1_STATE   //ֹͣ�ź�
#define  B_EMERGENCY_IN_STATE  IN1_2_STATE   //��ͣ�ź�

#define  B_PHOTO_1_IN_STATE    IN2_0_STATE   //���1����
#define  B_PHOTO_2_IN_STATE    IN2_1_STATE   //���2����
#define  B_PHOTO_3_IN_STATE    IN2_2_STATE   //���3����
#define  B_PHOTO_4_IN_STATE    IN2_3_STATE   //���4����
#define  B_PHOTO_5_IN_STATE    IN2_4_STATE   //���5����
#define  B_RESET_IN_STATE      IN2_5_STATE   //��λ��ť����
#define  B_STREAM_IN_STATE     IN2_6_STATE   //����׼���ź�����

#define  B_RUN_1_OUT_(BitVal)   OUT_1_0_(BitVal)   //�������1
#define  B_RUN_2_OUT_(BitVal)   OUT_1_1_(BitVal)   //�������2
#define  B_RUN_3_OUT_(BitVal)   OUT_1_2_(BitVal)   //�������3
#define  B_RUN_4_OUT_(BitVal)   OUT_1_3_(BitVal)   //�������4
#define  B_RUN_5_OUT_(BitVal)   OUT_1_4_(BitVal)   //�������5
#define  B_STREAM_OUT_(BitVal)  OUT_1_5_(BitVal)   //�������
#define  L_ALARM_OUT_(BitVal)   OUT_1_6_(BitVal)   //����ָʾ�����
#define  B_RUN_STA_OUT_(BitVal) OUT_1_7_(BitVal)   //��������״̬���

#define  INPUT_TRIG_NULL    0
#define  INPUT_TRIG_UP      1
#define  INPUT_TRIG_DOWN    2

#define  BELT_NUMMAX        10
#define  STOPSTATUS_MAX     2200

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
    u16          trig_cnt;
    u16          blocktrig_flag;       //������ ���ڶ°��ж���
}sButton_Info;

#define USER_PARA_DATA_LEN    113

typedef struct {
    u16 Func_Select_Switch;    //����ѡ�񿪹�(bit0:�Ƿ������Զ�����;bit1:ǰ����������;bit2:�°���⹦��;bit3:���Ź���(ʹ�ù��) bit4(���ⲿ׼���ź�)) bit5 ����ͬ��ֹͣ
    u16 Gear_1_Speed_Freq;     //��һ���ٶ�/Ƶ��(�ٶ����ͱ�ʾ������,��:12��ʾ1.2m/s)(Ƶ��ֵ���ͱ�ʾ������,��:1234��ʾ12.34hz)
    u16 Gear_2_Speed_Freq;     //�ڶ����ٶ�/Ƶ��
    u16 Gear_3_Speed_Freq;     //�������ٶ�/Ƶ��
    u16 Gear_4_Speed_Freq;     //���ĵ��ٶ�/Ƶ��
    u16 Gear_5_Speed_Freq;     //���嵵�ٶ�/Ƶ��
    u16 Sleep_Mode_Speed_Freq; //����ģʽ�ٶ�/Ƶ��
    u16 Start_Delay_Time;      //������ʱʱ��(ms)
    u16 Stop_Delay_Time;       //ֹͣ��ʱʱ��(ms)
    u16 Block_Check_Time;      //�°����ʱ��(s)
    u16 Sleep_Check_Time;      //���߼��ʱ��(s)
}BELT_PARAS_T;

typedef struct {
    u16 Version_No_L;       //ģ��汾��(����)
    u16 Version_No_H;       //ģ��汾��(����)
    u16 Station_No;         //ģ��վ��
    u16 Up_Stream_No;       //����վ��
    u16 Down_Stream_No;     //����վ��
    u16 Belt_Number;        //Ƥ��������(װ�������5,ж�������10)
    BELT_PARAS_T belt_para[10];
}USER_PARAS_T;

typedef struct {
    u16 fault_code;       //bit15~8(��Ƶ��������),bit5(��ͣ״̬),bit4:(����״̬),bit3(������״̬),bit2(�������״̬),bit1(�°�����),bit0(485ͨѶ����)
    u16 input_status;     //bit7(���һ�Ƿ񴥷���) bit6(����),bit5(���һ),bit4~0(��Ƶ��DI����״̬(bit0:�����ź�bit1:Զ���ź�bit4:����������))
    u16 line_speed;       //���ٶ�ʵʱ����
    u16 electric_current; //��Ƶ��ʵʱ����
    u16 inverter_freq;    //��Ƶ�����Ƶ��
}INVERTER_STATUS_T;

typedef struct {
    u16 station_no;
    u16 belt_number;
    INVERTER_STATUS_T inverter_status[10];
}MODULE_STATUS_T;

typedef struct {
    MODULE_STATUS_T *queue; /* ָ��洢���е�����ռ� */
    u16 front, rear, len; /* ����ָ�루�±꣩����βָ�루�±꣩�����г��ȱ��� */
    u16 maxSize; /* queue���鳤�� */
}MODULE_STATUS_QUEUE;

typedef struct {
    u16 rw_flag;         //��д���
    u16 inverter_no;     //��Ƶ��վ��
    u16 speed_gear;      //д���ٶȵĵ�λ
    u16 comm_interval;   //ͨѶ���ʱ��
    u16 comm_retry;      //ͨѶ���Դ���
    u16 value;           //�Ƿ���Ч
}COMM_NODE_T;

typedef struct {
    COMM_NODE_T *queue; /* ָ��洢���е�����ռ� */
    u16 front, rear, len; /* ����ָ�루�±꣩����βָ�루�±꣩�����г��ȱ��� */
    u16 maxSize; /* queue���鳤�� */
}COMM_SEND_QUEUE;

extern sButton_Info  bModeInfo;
extern sButton_Info  bStartInfo;
extern sButton_Info  bHSpeedInfo;
extern sButton_Info  bLSpeedInfo;
extern sButton_Info  bPauseInfo;
extern sButton_Info  bStreamInfo;
extern sButton_Info  bEmergencyInfo;

extern u8  g_remote_start_flag;
extern u8  g_remote_start_status;
extern u8  g_remote_speed_status;
extern u8  g_link_up_stream_status;
extern u8  g_link_down_stream_status;
extern u8  g_set_start_status;
extern u8  g_set_speed_status;
extern u8  g_read_start_status;
extern u8  g_block_disable_flag;
extern u8  g_speed_gear_status; //��ǰ���ٶȵ�λ(0:ֹͣ 1~5:�嵵�ٶ�)

extern u8  g_link_down_phototrig_status;      //���εĹ���Ƿ񴥷���

extern u16 start_delay_time_cnt;
extern u16 stop_delay_time_cnt;

extern u16 reset_start_time_cnt;
extern u8  reset_start_flag;

// can ���յ��ļ�ͣ�ź�
extern u8  g_emergency_stop;

extern USER_PARAS_T  user_paras_local;
extern USER_PARAS_T  user_paras_slave;
extern USER_PARAS_T  user_paras_temp;

extern MODULE_STATUS_T  module_status_buffer[];
extern INVERTER_STATUS_T  inverter_status_buffer[];

extern u16 logic_upload_stopStatus[];
extern u16 logic_upload_lastRunStatus[];

extern COMM_NODE_T comm_node;
extern u8  comm_busy_flag;
extern u8  polling_num;

extern u16 stopspeed_default[];
extern u16 freq_check_cnt;

void read_user_paras(void);
void write_user_paras(u16* para);
void InputScanProc();
void Speed_Ctrl_Process(u8 speed_gear);
void Linkage_Stop_Photo_Ctrl_Handle(u8 photo_index);
void Linkage_Stream_Ctrl_Handle(u8 inverter_index,u8 start_flag);
void Block_Check_Ctrl_Handle(void);
void Reset_Ctrl_Handle(void);
void Reset_Start_Inverter_Handle(void);

void InitUartSendQueue(void);
void AddUartSendData2Queue(COMM_NODE_T x);
COMM_NODE_T* GetUartSendDataFromQueue(void);
u8 IsUartSendQueueFree(void);
void InitModuleStatusQueue(void);
void AddModuleStatusData2Queue(MODULE_STATUS_T x);
MODULE_STATUS_T* GetModuleStatusDataFromQueue(void);
u8 IsModuleStatusQueueFree(void);
void Linkage_stream_extra_signal(u8 inverter_index, u8 start_flag);
void logic_upstream_io_allow_output(void);

u8 get_inverter_fault_status(INVERTER_STATUS_T inverter_status);

void logic_uarttmp_init(void);
void logic_cycle_decrease(void);

#endif
#ifndef _LIGIC_H
#define _LOGIC_H

#define VALUE 1
#define INVALUE 0

#define DEFAULT_TIME   5000
#define FLASH_TIME     500


#define  B_MODE_IN_STATE       IN1_0_STATE   //本地/远程模式切换输入
#define  B_START_IN_STATE      IN1_1_STATE   //运行信号输入
#define  B_ALARM_IN_STATE      IN1_2_STATE   //报警信号输入
#define  B_H_SPEED_IN_STATE    IN1_3_STATE   //高速旋钮输入
#define  B_L_SPEED_IN_STATE    IN1_4_STATE   //低速旋钮输入
#define  B_RESET_IN_STATE      IN1_5_STATE   //堵包复位按钮输入
#define  B_PAUSE_IN_STATE      IN1_6_STATE   //人工暂停按钮输入
#define  B_STREAM_IN_STATE     IN2_0_STATE   //下游准入信号输入(本机为上游)
#define  B_PHOTO_1_IN_STATE    IN2_1_STATE   //光电1输入
#define  B_REMOTE_START_IN     IN2_5_STATE   //远程IO启停信号

#define  L_START_OUT_(BitVal)      OUT_1_0_(BitVal)   //运行指示灯输出
#define  L_ALARM_OUT_(BitVal)      OUT_1_6_(BitVal)   //报警指示灯输出
#define  B_START_OUT_(BitVal)      OUT_1_2_(BitVal)   //远程模式下板卡给变频器的启动/停止信号
#define  B_RESET_OUT_(BitVal)      OUT_1_1_(BitVal)   //故障复位输出
#define  B_STREAM_OUT_(BitVal)     OUT_1_3_(BitVal)   //下游准入信号输出(本机为下游)
#define  L_NOTI_START_OUT_(BitVal) OUT_1_5_(BitVal)   //本节皮带启动提醒



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
    u16          button_state; //按钮有没有被触发
    u16          button_hold_time; //按钮按住的时间计时(ms)
}sButton_Info;

#define USER_PARA_DATA_LEN    15

typedef struct {
    u16 Station_No;         //模块站号
    u16 Version_No_L;       //模块版本号(低字)
    u16 Version_No_H;       //模块版本号(高字)
    u16 Speed_Factor;       //速度比例系数(整型表示浮点数,如:123表示1.23)
    u16 Motor_Type;         //电机类型(变频器0/伺服1)
    u16 Motor_Model;        //电机型号
    u16 Func_Select_Switch; //功能选择开关(bit0:是否启用自动调速;bit1:前后联动功能;bit2:堵包检测功能;bit3:联动信号选择(软信号CAN/端子输入))
    u16 High_Speed_Target;  //高速目标速度(整型表示浮点数,如:123表示1.23m/s)
    u16 High_Speed_Freq;    //高速频率值/转速(频率值整型表示浮点数,如:1234表示12.34hz)
    u16 Middle_Speed_Target;//中速目标速度(整型表示浮点数,如:123表示1.23m/s)
    u16 Middle_Speed_Freq;  //中速频率值/转速(频率值整型表示浮点数,如:1234表示12.34hz)
    u16 Low_Speed_Target;   //低速目标速度(整型表示浮点数,如:123表示1.23m/s)
    u16 Low_Speed_Freq;     //低速频率值/转速(频率值整型表示浮点数,如:1234表示12.34hz)
    u16 Start_Delay_Time;   //启动延时时间
    u16 Stop_Delay_Time;    //停止延时时间
    u16 Block_Check_Time;   //堵包检测时间
    u16 Up_Stream_No;       //上游站号
    u16 Down_Stream_No;     //下游站号
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
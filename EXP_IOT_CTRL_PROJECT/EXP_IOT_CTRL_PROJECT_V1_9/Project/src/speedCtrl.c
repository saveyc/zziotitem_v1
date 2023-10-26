#include "main.h"
#include "math.h"
#include "stdlib.h"
#include "stdio.h"

#define SPEED_FACTOR     0.52    //����������ֵ��ʵ��Ƥ���ߵľ���ı���ϵ��
#define CONST_TIME_INTERVAL  100
#define ENCODER_MAX_VALUE    50000

s16 encodeIncValue;
s16 encodeValuePre;
u16 speed_time_cnt;
s32 speed_encode_cnt;
float rt_speed;//��ǰ��ʵʱ�ٶ�ֵ(���)

#define fwOutHzMax    5000     //��Ƶ�����Ƶ�����
#define fwErrIMax     0.25     //����������
float fwKp = 1400, fwKi = 20, fwKd = 0;
float fwErr[2], fwErrD = 0,  fwErrI = 0;
float fwbV;
float speed_target = 0;//���õ�Ŀ���ٶ�ֵ(����)
int   fwHz;

char debug_send_buff[100];

extern u16 Forbid_ReadHz_cnt ;
extern u16 ReadHz_flag ;



void Udp_Send_Speed(void)
{
    u16 send_len = 0;
    
    send_len = sprintf(debug_send_buff, "encode:%d," , speed_encode_cnt);
    send_len += sprintf(debug_send_buff+send_len, "hz:%d," , fwHz);
    send_len += sprintf(debug_send_buff+send_len, "speed:%f\n" , rt_speed);
    
    DEBUG_process((u8*)debug_send_buff, send_len);
}

//����ʵʱ�ٶ�
void calculate_rt_speed(void)
{
    s16 encodeValueNow = TIM_GetCounter(TIM1);
    
    encodeIncValue = encodeValuePre - encodeValueNow;
/*    
    if(encodeIncValue>0)
      encodeIncValue = encodeValuePre-(ENCODER_MAX_VALUE + encodeValueNow);
*/      
    encodeValuePre = encodeValueNow;
    speed_time_cnt++;
    speed_encode_cnt += encodeIncValue;
    if(speed_time_cnt >= CONST_TIME_INTERVAL)
    {
        rt_speed = abs(speed_encode_cnt) * SPEED_FACTOR / CONST_TIME_INTERVAL;
        //Udp_Send_Speed();
        speed_time_cnt = 0;
        speed_encode_cnt = 0;
    }
}

void RS485_OUT(u16 hz)
{
    ReadHz_flag = 0;
    Forbid_ReadHz_cnt = 50;
    //Modbus_RTU_Write_Single_Reg_Cmd(8502,hz/10);//ʩ�͵�ATV310
    Modbus_RTU_Write_Single_Reg_Cmd(0x2001,hz);//̨��//0x2001
}

void fwErrCalculate(float speed_now)
{
    fwbV = speed_now;

    //�������
    fwErr[0] = fwErr[1];//err[0]:��һ�ε����ֵ��err[1]:��ǰ���ֵ
    fwErr[1] = speed_target - fwbV;

#ifdef D_CTRL_OPEN
    //΢�����
    //fwErrD = (fwErr[1]-fwErr[0])/ctrlF;
    fwErrD = (fwErr[1] - fwErr[0]);
    if (fabs(fwErrD) > 0.1) //����ļ��ٶȲ����������
        fwErrD = 0;
    if (fwErr[0] == 0 || fwErr[0] == fwErr[1])//�����δ���������������ʱ���ٹ��� ,������΢�ֿ���
        fwErrD = 0;
#endif
    //�������
    if (fabs(fwErr[1]) <= fwErrIMax) //ֻ�н���PID�����ſ�ʼ����
    {
        if ((fabs(fwErrI) >= fabs(fwErrIMax)) && (fwErrI * fwErr[1] > 0)) //������������
        {
            return;
        }

        fwErrI += fwErr[1];

        if (fabs(fwErrI) >= fwErrIMax) // ���ܳ������ֵ
        {
            fwErrI = fwErrI > 0 ?  fwErrIMax : (-fwErrIMax);
        }
    }
    else
        fwErrI = 0;


    if(fwErr[0]*fwErr[1] < 0) //�ٶȴ�С���趨�ٶ������޷�����Ծ
    {
        fwErrI = 0;
    }
}
void fwPIDctrl(void)
{
    fwErrCalculate(rt_speed);
    
    fwHz = (int)( fwHz + (float)fwKp * fwErr[1] + (float)fwKi * fwErrI + (float)fwKd * fwErrD );
    
    if(fabs(fwErr[1]) <= 0.001 && fabs(fwErrD) <= 1) //������ɣ���������� ǧ��֮һ����
    {
        fwErrI = 0;
    }
    
    if(fwHz > fwOutHzMax)
    {
        fwHz = fwOutHzMax;
    }
    
    if(fwHz < 0)
    {
        fwHz =  0;
    }
    
    RS485_OUT(fwHz);//485�������Ƶ��
}
void speed_ctrl_process(void)
{
    if(  user_paras_local.Motor_Type == 0 //�������Ϊ��Ƶ��
      && user_paras_local.Func_Select_Switch &0x1 == 1 ) //�����Զ�����
    {
        if(g_set_speed_status == 0)
        {
            speed_target = user_paras_local.Low_Speed_Target / 100.0;
        }
        else if(g_set_speed_status == 1)
        {
            speed_target = user_paras_local.Middle_Speed_Target / 100.0;
        }
        else if(g_set_speed_status == 2)
        {
            speed_target = user_paras_local.High_Speed_Target / 100.0;
        }
        if(g_set_start_status == 0)
        {
            speed_target = 0;
        }
        fwPIDctrl();
    }
    else
    {
        if(g_set_speed_status == 0)
        {
            fwHz = user_paras_local.Low_Speed_Freq;
        }
        else if(g_set_speed_status == 1)
        {
            fwHz = user_paras_local.Middle_Speed_Freq;
        }
        else if(g_set_speed_status == 2)
        {
            fwHz = user_paras_local.High_Speed_Freq;
        }
        if(g_set_start_status == 0)
        {
            fwHz = 0;
        }
        RS485_OUT(fwHz);
    }
}

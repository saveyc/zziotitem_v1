#ifndef _FUN_H
#define _FUN_H

/********�����**********/
#define		MSG_NULL_TYPE			        0x0000
/**********��������***********/
#define         HEART_MSG_TYPE                          0x1150
/**********������������*******/
#define		SEND_MSG_MODULE_STATUS_TYPE             0x1601
#define		SEND_MSG_ALARM_TYPE                     0x1602
/**********��������**********/
#define         RECV_MSG_READ_PARA_CMD_TYPE             0x1603
#define         RECV_MSG_WRITE_PARA_CMD_TYPE            0x1604
#define         RECV_MSG_START_CMD_TYPE                 0x1610
#define         RECV_MSG_RESET_CMD_TYPE                 0x1611
#define         RECV_MSG_FUNC_SELECT_CMD_TYPE           0x1612
#define         RECV_MSG_BOOT_CMD_TYPE                  0x1F01
/**********�ظ���������******/
#define		REPLY_RECV_MSG_READ_PARA_CMD_TYPE       0x9603
#define         REPLY_RECV_MSG_WRITE_PARA_CMD_TYPE      0x9604
#define         REPLY_RECV_MSG_START_CMD_TYPE           0x9610

#define         HEART_DELAY    5

#pragma pack (1) 
typedef struct {
	u8  MSG_TAG[2];
        u32 MSG_ID;
        u16 MSG_LENGTH;
	u8  MSG_CRC;
	u16 MSG_TYPE;
}MSG_HEAD_DATA;
#pragma pack () 

typedef struct {
        u16 *queue; /* ָ��洢���е�����ռ� */
        u16 front, rear, len; /* ����ָ�루�±꣩����βָ�루�±꣩�����г��ȱ��� */
        u16 maxSize; /* queue���鳤�� */
}MSG_SEND_QUEUE;

void InitSendMsgQueue(void);
void AddSendMsgToQueue(u16 msg);
u16 GetSendMsgFromQueue(void);
void recv_message_from_server(u8 *point,u16 *len);
void send_message_to_server(void);

#endif

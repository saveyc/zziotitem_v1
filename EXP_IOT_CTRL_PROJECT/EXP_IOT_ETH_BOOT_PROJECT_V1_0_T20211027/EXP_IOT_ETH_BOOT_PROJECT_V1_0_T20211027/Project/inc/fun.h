
/********命令定义**********/
#define		RECV_MSG_BOOT_MODE_TYPE			        0x1F01
#define		RECV_MSG_DLOAD_PARA_TYPE		        0x1F02
#define		RECV_MSG_USER_MODE_TYPE			        0x1F03
#define		REPLY_RECV_MSG_BOOT_MODE_TYPE			0x9F01
#define		REPLY_RECV_MSG_DLOAD_PARA_TYPE		        0x9F02
#define		REPLY_RECV_MSG_USER_MODE_TYPE			0x9F03
#define		SEND_MSG_PRT_MESSAGE_TYPE			0x1F04

typedef enum {DLOAD_INIT = 0, DLOAD_PARA, DLOAD_TRAN_INIT, DLOAD_TRAN_ERASE, DLOAD_TRAN_PROG} DLOAD_STATUS;

#define   DLOAD_MODE_LOCAL       0x00
#define   DLOAD_MODE_TRANS       0x01

#define   UART_CMD_NULL          0x00
#define   UART_CMD_ENTER_BOOT    0xB0
#define   UART_CMD_ERASE_FLASH   0xB1
#define   UART_CMD_TRAN_DATA     0xB2

#pragma pack (1) 

typedef struct {
	u8  MSG_TAG[2];
        u32 MSG_ID;
        u16 MSG_LENGTH;
	u8  MSG_CRC;
	u16 MSG_TYPE;
}MSG_HEAD_DATA;

typedef struct
{
    u8 head[11];
    u8 mode; //烧写模式（本机:0/转发:1）
    u8 st_addr; //转发模式时从机站号起始值
    u8 addr_num; //转发模式时要烧写的从机数
}DLOAD_PARA_DATA;

#pragma pack () 

typedef struct {
        u16 *queue; /* 指向存储队列的数组空间 */
        u16 front, rear, len; /* 队首指针（下标），队尾指针（下标），队列长度变量 */
        u16 maxSize; /* queue数组长度 */
}MSG_SEND_QUEUE;

void InitSendMsgQueue(void);
void AddSendMsgToQueue(u16 msg);
u16 GetSendMsgFromQueue(void);
void recv_message_from_sever(u8 *point,u16 *len);
void send_message_to_sever(void);
void udp_send_message(u8 *p_data, u16 len);
u8 recv_msg_check(u8 *point,u16 len);
extern DLOAD_PARA_DATA sDLoad_Para_Data;
extern DLOAD_STATUS g_download_status;
extern u32 file_tot_bytes;
extern u8  print_msg_buff[];
extern u16 print_msg_len;

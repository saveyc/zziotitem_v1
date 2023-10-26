#ifndef __CAN_BUS_H
#define __CAN_BUS_H
#pragma pack (1)

enum
{
    CAN_FUNC_ID_BOOT_MODE  = 0xF
};

enum
{
    CAN_SEG_POLO_NONE   = 0,
    CAN_SEG_POLO_FIRST  = 1,
    CAN_SEG_POLO_MIDDLE = 2,
    CAN_SEG_POLO_FINAL  = 3
};
/*
|    28~22    |    21~20      |    19~12     |     11~8     |    7~0      |    ExtID
|  dst_id(7)  |  seg_polo(2)  |  seg_num(8)  |  func_id(4)  |  src_id(8)  |
*/
typedef struct
{
    u8 seg_polo;
    u8 seg_num;
    u8 func_id;
    u8 src_id;
    u8 dst_id;
} sCanFrameExtID;

typedef struct
{
    sCanFrameExtID extId;
    u8 data_len;
    u8 data[8];
} sCanFrameExt;

#pragma pack ()

#define CAN_PACK_DATA_LEN       8
#define CAN_SEND_DATA_PK_SIZE   256

#define CAN_RX_DATA_PK_SIZE     256
#define CAN_RX_BUFF_SIZE        300

#define DLOAD_CMD_NULL          0x00
#define DLOAD_CMD_ENTER_BOOT    0xB0
#define DLOAD_CMD_ERASE_FLASH   0xB1
#define DLOAD_CMD_TRAN_DATA     0xB2

void can_recv_timeout();
void master_can_bus_frame_receive(CanRxMsg rxMsg);
void slave_can_bus_frame_receive(CanRxMsg rxMsg);
void can_send_frame_process();
void host_transmit_process();
void can_bus_reply_process(void);

#endif
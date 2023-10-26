																			   /**
  ******************************************************************************
  * @file    main.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   This file contains all the functions prototypes for the main.c 
  *          file.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f107.h"
#include "fun.h"
#include "can_bus.h"
#include <string.h>

/* Exported function prototypes ----------------------------------------------*/
void Time_Update(void);
void Delay(uint32_t nCount);

/* ETHERNET errors */
#define  ETH_ERROR              ((uint32_t)0)
#define  ETH_SUCCESS            ((uint32_t)1)
#define  DP83848_PHY_ADDRESS       0x01

#define	UDP_RECV_BUFF_SIZE		100
#define UDP_SEND_BUFF_SIZE              100
#define	CLIENT_DIS_CONNECT		0
#define	CLIENT_CONNECT_OK		1
#define	CLIENT_CONNECT_RECV		2
#define	CLIENT_RE_CONNECT		3

#define DEST_IP_ADDR0   192
#define DEST_IP_ADDR1   168
#define DEST_IP_ADDR2   2
#define DEST_IP_ADDR3   100
#define DEST_PORT       3000

#define LOCAL_UDP_PORT  DEST_PORT
/* MAC ADDRESS: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */
#define MAC_ADDR0   mac_addr[0]
#define MAC_ADDR1   mac_addr[1]
#define MAC_ADDR2   mac_addr[2]
#define MAC_ADDR3   mac_addr[3]
#define MAC_ADDR4   mac_addr[4]
#define MAC_ADDR5   mac_addr[5]
/*Static IP ADDRESS: IP_ADDR0.IP_ADDR1.IP_ADDR2.IP_ADDR3 */
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   2
#define IP_ADDR3   130
/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0
/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   2
#define GW_ADDR3   1

#define	 LED_STATE		GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_4)
#define	 LED_ON			GPIO_ResetBits(GPIOD,GPIO_Pin_4)
#define	 LED_OFF		GPIO_SetBits(GPIOD,GPIO_Pin_4)

//#define  DIP1_STATE             GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_3)
//#define  DIP2_STATE             GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_2)
//#define  DIP3_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_12)
//#define  DIP4_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_11)
//#define  DIP5_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_10)
//#define  DIP6_STATE             GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)
//#define  DIP7_STATE             GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12)
//#define  DIP8_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_9)

#define  DIP1_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_9)
#define  DIP2_STATE             GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_12)
#define  DIP3_STATE             GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)
#define  DIP4_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_10)
#define  DIP5_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_11)
#define  DIP6_STATE             GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_12)
#define  DIP7_STATE             GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_2)
#define  DIP8_STATE             GPIO_ReadInputDataBit(GPIOD,GPIO_Pin_3)

extern u8  mac_addr[6];

extern u8  Local_Station;
extern u8  cur_tran_station;
extern u16 pk_tot_num;
extern u16 pk_cur_num;
extern u8  trans_flag;


u16 readFlash_uint(uint32_t address);


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/


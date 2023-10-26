/**
  ******************************************************************************
  * @file    stm32f107.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   This file contains all the functions prototypes for the STM32F107 
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
#ifndef __STM32F107_H
#define __STM32F107_H

#ifdef __cplusplus
 extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
// boot区64K,用户程序区96K(包含2K参数区),临时存储区96K
#define ApplicationAddress                0x8010000  //用户程序运行起始地址
#define AppTempAddress                    0x8028000  //转发从机程序的临时空间起始地址
#define PAGE_SIZE                         (0x800)    /* 2 Kbyte */
#define FLASH_SIZE                        (0x40000)  /* 256 KBytes */
#define UserAppEnFlagAddress              0x800F800  //ApplicationAddress - PAGE_SIZE
#define UserParaStartAddress              0x8027800  //用户参数区起始地址(占用2 Kbyte)

void System_Setup(void);
void Ethernet_Configuration(void);
void check_ETH_link(void);
uint32_t FLASH_PagesMask(__IO uint32_t Size);

extern __IO uint32_t  EthInitStatus;
#ifdef __cplusplus
}
#endif

#endif /* __STM32F10F107_H */


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

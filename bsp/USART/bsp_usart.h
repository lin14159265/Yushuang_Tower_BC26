#ifndef __BSP__USART_H
#define __BSP__USART_H
/***********************************************************************************************************************************
 ** 【代码编写】  魔女开发板团队
 ** 【代码更新】  Q群文件夹       1126717453
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 ** 【文件名称】  bsp_usart.h
 **
 ** 【文件功能】  定义引脚、定义全局结构体、声明全局函数
 **               本文件简化的USART的初始化，及完善了收、发功能函数;初始化后调用函数即可使用;
 **
 ** 【适用平台】  STM32F103 + 标准库v3.5 + keil5
 **
 ** 【移植说明】  1- 复制本工程的USART文件夹，到目标工程文件夹中;
 **               2- 把c文件添加到工程管理器;
 **               3- 添加文件存放路径;
 **               4- 在需要串口功能的文件中，#include "usart_f103.h＂;
 **     
 ** 【硬件重点】  调用各USARTx_Init()函数时，将按以下默认引脚进行初始化：
 **               1- USART1 PA9, PA10
 **               2- USART2 PA2, PA3
 **               3- USART3 PB10,PB11
 **               4- UART4  PC10,PC11
 **               5- UART5  PC12,PD2
 **
 ** 【代码说明】  本文件的收发机制, 经多次修改, 已比较完善. 
 **               初始化: 只需调用：USARTx_Init(波特率), 函数内已做好引脚及时钟配置; 
 **               发 送 : 方法1_发送任意长度字符串: USARTx_SendString (char* stringTemp); 
 **                       方法2_发送指定长度数据  : USARTx_SendData (uint8_t* buf, uint8_t cnt);
 **               接 收 : 方式1_通过全局函数: USARTx_GetBuffer (uint8_t* buffer, uint8_t* cnt);　// 当有数据时，返回1; 本函数已清晰地示例了接收机制; 
 **                       方式2_通过判断xUSART.USARTxReceivedNum>0;
 **                              如在while中不断轮询，或在任何一个需要的地方判断这个接收字节长度变量值．示例：
 **                              while(1){
 **                                  if(xUSART1.USART1ReceivedNum>0)
 **                                  {
 **                                      printf((char*)xUSART.USART1ReceivedBuffer);          // 示例1: 如何输出成字符串
 **                                      uint16_t GPS_Value = xUSART.USART1ReceivedBuffer[1]; // 示例2: 如何读写其中某个成员的数据
 **                                      xUSART1.USART1ReceivedNum= 0;                     // 重要：处理完数据后, 必须把接收到的字节数量值清0
 **                                  }  
 **                              }     
 **   
 ** 【更新记录】
 **              2021-12-16  完善接收机制：取消接收标志，判断接收字节数>0即为接收到新数据
 **              2021-11-03  完善接收函数返回值处理
 **              2021-08-14  增加宏定义：接收缓存区大小设定值，使空间占用更可控;
 **              2021-08-14  修改发送函数名称：USARTx_Printf()，修改为：USARTx_SendString();
 **              2020-09-02  文件建立、USART1接收中断、空闲中断、发送中断、DMA收发
 **              2021-06-04  USART1、2、3及UART4、5 的收发完善代码
 **              2021-06-09  完善文件格式、注释  
 **              2021-06-22  完善注释说明
 **    
************************************************************************************************************************************/
#include <stm32f10x.h>  
#include <stdio.h>
#include "string.h"       // C标准库头文件: 字符数组常用：strcpy()、strncpy()、strcmp()、strlen()、strnset()


// 定义一个标志位，当接收到一行完整的指令时，此标志位置1
volatile uint8_t g_usart1_new_line_received = 0;


/*****************************************************************************
 ** 移植配置
****************************************************************************/
// 用哪个串口与上位机通信，可自行
#define USARTx_DEBUG            USART2              // 用于重定向printf, 使printf通过USARTx发送数据
// 数据接收缓冲区大小，可自行修改
#define U1_RX_BUF_SIZE            1024              // 配置每个USARTx接收缓冲区的大小(字节数)，包括中断里的缓存大小，和xUSART结构体里的缓存区大小
#define U2_RX_BUF_SIZE            1024              // --- 当每帧接收到的数据字节数，小于此值时，数据正常;剩余空间数据为: 0
#define U3_RX_BUF_SIZE            1024              // --- 当每帧接收到的数据字节数，超过此值时，超出部分，将在中断中直接弃舍，直到接收结束(发生空闲中断); 
#define U4_RX_BUF_SIZE            1024              // --- 有效区域：仅在各个USARTx的中断服务函数中工作;
#define U5_RX_BUF_SIZE            1024              // --- 主要作用:  1:配合空闲中断接收数据帧;  2:灵活配置缓存大小;  3:防止数据溢出!!!!

#define DEBUG_USART   USART2            // 用于调试的串口，可自行修改


/*****************************************************************************
 ** 全局变量 (无要修改)
****************************************************************************/
typedef struct 
{
    uint8_t   USART1InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART1ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART1ReceivedBuffer[U1_RX_BUF_SIZE]; // 接收到数据的缓存
    
    uint8_t   USART2InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART2ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART2ReceivedBuffer[U2_RX_BUF_SIZE]; // 接收到数据的缓存
    
    uint8_t   USART3InitFlag;                       // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  USART3ReceivedNum;                    // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   USART3ReceivedBuffer[U3_RX_BUF_SIZE]; // 接收到数据的缓存 
    
    uint8_t   UART4InitFlag;                        // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  UART4ReceivedNum;                     // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   UART4ReceivedBuffer[U4_RX_BUF_SIZE];  // 接收到数据的缓存
    
    uint8_t   UART5InitFlag;                        // 初始化标记; 0=未初始化, 1=已初始化
    uint16_t  UART5ReceivedNum;                     // 接收到多少个字节数据; 当等于0时，表示没有接收到数据; 当大于0时，表示已收到一帧新数据
    uint8_t   UART5ReceivedBuffer[U5_RX_BUF_SIZE];  // 接收到数据的缓存
    
    uint16_t  testCNT;                              // 仅用于测试
    
}xUSATR_TypeDef;

extern xUSATR_TypeDef  xUSART;                      // 声明为全局变量,方便记录信息、状态
    




/*****************************************************************************
 ** 声明全局函数 (无需修改)
****************************************************************************/
// USART1
void    USART1_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART1_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART1_SendData (uint8_t* buf, uint8_t cnt);          // 通过中断发送数据，适合各种数据
void    USART1_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在4096个长度内的
void    USART1_printfForDMA (char* stringTemp) ;              // 通过DMA发送数据，适合一次过发送数据量特别大的字符串，省了占用中断的时间
// USART2
void    USART2_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART2_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART2_SendData (uint8_t* buf, uint8_t cnt);          // 通过中断发送数据，适合各种数据
void    USART2_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在256个长度内的
// USART3
void    USART3_Init (uint32_t baudrate);                      // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t USART3_GetBuffer (uint8_t* buffer, uint8_t* cnt);     // 获取接收到的数据
void    USART3_SendData (uint8_t* buf, uint8_t cnt);          // 通过中断发送数据，适合各种数据
void    USART3_SendString (char* stringTemp);                 // 通过中断发送字符串，适合字符串，长度在256个长度内的
// USART4
void    UART4_Init (uint32_t baudrate);                       // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t UART4_GetBuffer (uint8_t* buffer, uint8_t* cnt);      // 获取接收到的数据
void    UART4_SendData (uint8_t* buf, uint8_t cnt);           // 通过中断发送数据，适合各种数据
void    UART4_SendString (char* stringTemp);                  // 通过中断发送字符串，适合字符串，长度在256个长度内的
// USART5
void    UART5_Init (uint32_t baudrate);                       // 初始化串口的GPIO、通信参数配置、中断优先级; (波特率可设、8位数据、无校验、1个停止位)
uint8_t UART5_GetBuffer (uint8_t* buffer, uint8_t* cnt);      // 获取接收到的数据
void    UART5_SendData (uint8_t* buf, uint8_t cnt);           // 通过中断发送数据，适合各种数据
void    UART5_SendString (char* stringTemp);                  // 通过中断发送字符串，适合字符串，长度在256个长度内的


#endif


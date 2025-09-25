//#ifndef __BSP__LED_H
//#define __BSP__LED_H
///***********************************************************************************************************************************
// ** 【文件名称】  led.h
// ** 【编写人员】  魔女开发板团队
// ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
// ***********************************************************************************************************************************
// ** 【文件功能】  简化常用的系统函数、初始化函数
// **                
// ** 【硬件重点】  1- 注意LED是代电平还是高电平点亮，可查原理图
// **               2- 
// **
// ** 【代码重点】  1- 引脚工作模式：推挽输出
// **
// ** 【移植说明】  
// **
// ** 【更新记录】  
// **
//***********************************************************************************************************************************/  
//#include <stm32f10x.h>  
//#include <stdio.h>





////移植参数区 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//#define  LED_RED_GPIO      GPIOC
//#define  LED_RED_PIN       GPIO_Pin_5   
//#define  LED_RED_CLK       RCC_APB2Periph_GPIOA   // 如果使用固件库就需要定义

//#define  LED_BLUE_GPIO     GPIOB
//#define  LED_BLUE_PIN      GPIO_Pin_2
//#define  LED_BLUE_CLK      RCC_APB2Periph_GPIOD   // 如果使用固件库就需要定义
////END 移植 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



//// 按色简化功能, 移植时不用修改
//// red
//#define LED_RED_ON         (LED_RED_GPIO->BSRR |= LED_RED_PIN <<16)    // 点亮，置低电平
//#define LED_RED_OFF        (LED_RED_GPIO->BSRR |= LED_RED_PIN)         // 熄灭，置高电平
//#define LED_RED_TOGGLE     (LED_RED_GPIO->ODR  ^= LED_RED_PIN)         // 反转，电平取反
//// blue
//#define LED_BLUE_ON        (LED_BLUE_GPIO->BSRR  |= LED_BLUE_PIN <<16) // 点亮，置低电平
//#define LED_BLUE_OFF       (LED_BLUE_GPIO->BSRR  |= LED_BLUE_PIN)      // 熄灭，置高电平
//#define LED_BLUE_TOGGLE    (LED_BLUE_GPIO->ODR   ^= LED_BLUE_PIN)      // 反转，电平取反



///*****************************************************************************
// ** 声明全局函数
//****************************************************************************/
//void Led_Init(void);



//#endif



#ifndef __BSP__LED_H
#define __BSP__LED_H

#include <stm32f10x.h>  
#include <stdio.h>

//==============================================================================
// (1) 移植参数区：在这里配置您所有的LED引脚
//==============================================================================

// --- LED 1 ---
#define  LED1_GPIO      GPIOB       // 连接在 GPIOB
#define  LED1_PIN       GPIO_Pin_1  // 连接在 PB1

// --- LED 2 ---
#define  LED2_GPIO      GPIOC       // 连接在 GPIOC
#define  LED2_PIN       GPIO_Pin_13 // 连接在 PC13

// --- LED 3 ---
#define  LED3_GPIO      GPIOB       // 连接在 GPIOB
#define  LED3_PIN       GPIO_Pin_8  // 连接在 PB8

// --- LED 4 ---
#define  LED4_GPIO      GPIOB       // 连接在 GPIOB
#define  LED4_PIN       GPIO_Pin_9  // 连接在 PB9


//==============================================================================
// (2) 功能宏定义区：为每个LED定义一套操作指令
//==============================================================================

/* --- LED 1 的操作指令 --- */
#define LED1_ON         (LED1_GPIO->BRR = LED1_PIN)         // 点亮 (置低电平)
#define LED1_OFF        (LED1_GPIO->BSRR = LED1_PIN)        // 熄灭 (置高电平)
#define LED1_TOGGLE     (LED1_GPIO->ODR  ^= LED1_PIN)       // 翻转

/* --- LED 2 的操作指令 --- */
#define LED2_ON         (LED2_GPIO->BRR = LED2_PIN)
#define LED2_OFF        (LED2_GPIO->BSRR = LED2_PIN)
#define LED2_TOGGLE     (LED2_GPIO->ODR  ^= LED2_PIN)

/* --- LED 3 的操作指令 --- */
#define LED3_ON         (LED3_GPIO->BRR = LED3_PIN)
#define LED3_OFF        (LED3_GPIO->BSRR = LED3_PIN)
#define LED3_TOGGLE     (LED3_GPIO->ODR  ^= LED3_PIN)

/* --- LED 4 的操作指令 --- */
#define LED4_ON         (LED4_GPIO->BRR = LED4_PIN)
#define LED4_OFF        (LED4_GPIO->BSRR = LED4_PIN)
#define LED4_TOGGLE     (LED4_GPIO->ODR  ^= LED4_PIN)


//==============================================================================
// (3) 函数声明区
//==============================================================================
void Led_Init(void);

#endif

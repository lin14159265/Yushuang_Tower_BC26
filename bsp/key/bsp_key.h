#ifndef _BSP_KEY_H
#define _BSP_KEY_H
#include <stm32f10x.h>  



/*****************************************************************************
 ** 移植时修改
****************************************************************************/
// KEY_1_WKUP, 闲时下拉，按下时被置高电平
#define KEY_1_GPIO                GPIOA                  // 所用端口
#define KEY_1_PIN                 GPIO_Pin_0             // 引脚编号
#define KEY_1_MODE                GPIO_Mode_IPD          // 引脚工作模式
#define KEY_1_TRIM                EXTI_Trigger_Rising    // 下降沿触发:EXTI_Trigger_Falling;   上升沿触发:EXTI_Trigger_Rising
#define KEY_1_IRQN                EXTI0_IRQn             // 中断向量编号
#define KEY_1_IRQHANDLER          EXTI0_IRQHandler       // 中断服务函数名称
#define KEY_1_EXTI_LINE           EXTI_Line0             // 中断线编号
#define KEY_1_GPIOSOURCE          GPIO_PortSourceGPIOA   // 中断线端口
#define KEY_1_PINSOURCE           GPIO_PinSource0    
// KEY_2, 闲时上拉，按下时被置低电平
#define KEY_2_GPIO                GPIOA                  // 所用端口
#define KEY_2_PIN                 GPIO_Pin_1             // 引脚编号
#define KEY_2_MODE                GPIO_Mode_IPU          // 引脚工作模式
#define KEY_2_TRIM                EXTI_Trigger_Falling   // 下降沿触发:EXTI_Trigger_Falling;   上升沿触发:EXTI_Trigger_Rising
#define KEY_2_IRQN                EXTI1_IRQn             // 中断向量编号
#define KEY_2_IRQHANDLER          EXTI1_IRQHandler       // 中断服务函数
#define KEY_2_GPIOSOURCE          GPIO_PortSourceGPIOA   // 中断线端口
#define KEY_2_EXTI_LINE           EXTI_Line1             // 中断线编号
#define KEY_2_PINSOURCE           GPIO_PinSource1   
// KEY_2, 闲时上拉，按下时被置低电平
#define KEY_3_GPIO                GPIOA                  // 所用端口
#define KEY_3_PIN                 GPIO_Pin_4             // 引脚编号
#define KEY_3_MODE                GPIO_Mode_IPU          // 引脚工作模式
#define KEY_3_TRIM                EXTI_Trigger_Falling   // 下降沿触发:EXTI_Trigger_Falling;   上升沿触发:EXTI_Trigger_Rising
#define KEY_3_IRQN                EXTI4_IRQn             // 中断向量编号
#define KEY_3_IRQHANDLER          EXTI4_IRQHandler       // 中断服务函数名称
#define KEY_3_GPIOSOURCE          GPIO_PortSourceGPIOA   // 中断线端口
#define KEY_3_EXTI_LINE           EXTI_Line4             // 中断线编号
#define KEY_3_PINSOURCE           GPIO_PinSource4    



/*****************************************************************************
 ** 声明全局函数
****************************************************************************/
void Key_Init(void);



#endif


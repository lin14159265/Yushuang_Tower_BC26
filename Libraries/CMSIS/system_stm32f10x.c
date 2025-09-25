/******************************************************************************
  * @file    system_stm32f10x.c
  * @author  魔女开发板团队
  * @version 
  * @date    2020年4月21日
  * @note    原来的文件又长又啰嗦，看不过眼， 重写了
******************************************************************************/
#include "stm32f10x.h"



// 中断向量表偏移
#define VECT_TAB_OFFSET    0x0                                                       // 不能修改！ 除非你已很熟悉STM32的启动和中断
// 分频预设数值对比
__I uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};    // 不能修改！ 用于检测系统时钟频率
// 系统时钟频率
uint32_t SystemCoreClock = 5201314;                                                  // 可先设随意数值，以便检验启动后时钟是否成功设置成: 72'000'000Hz



/*****************************************************************************
*函  数： system_ClockInit
*功  能： 系统时钟配置
*参  数： 无
*返回值： 无
*备  注： 注释_魔女开发板 
*****************************************************************************/
void SystemInit(void)
{
    // 复位时钟至默认状态
    RCC->CR   |= (u32)0x00000001;                                // 开启内部时钟
    RCC->CFGR &= (u32)0xF8FF0000;                                // 复位 SW, HPRE, PPRE1, PPRE2, ADCPRE and MCO bits
    RCC->CFGR &= (u32)0xFF80FFFF;                                // 复位 PLL和分频的配置
    RCC->CIR   = 0x009F0000;                                     // 关闭中断，清理中断位

    // 配置时钟 HCLK, PCLK, PCLK1, PCLK2, FLASH 
    __IO u32 StartUpCounter = 0, HSEStatus = 0;      
    
    RCC->CR |= ((u32)RCC_CR_HSEON);                              // 使能 HSE  
    do{                                                          // 等待HSE就绪
        HSEStatus = RCC->CR & RCC_CR_HSERDY;
        StartUpCounter++;  
      } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));   // 0x0500

    if ((RCC->CR & RCC_CR_HSERDY) != RESET)  {  
        FLASH->ACR |= FLASH_ACR_PRFTBE;                          // Enable Prefetch Buffer */    
        FLASH->ACR &= (u32)((u32)~FLASH_ACR_LATENCY);            // Flash 2 wait state */
        FLASH->ACR |= (u32)FLASH_ACR_LATENCY_2;    
 
        RCC->CFGR  |= (u32)RCC_CFGR_HPRE_DIV1;                   // [7:4]   AHB  预分频, HCLK = SYSCLK/1  不分频     
        RCC->CFGR  |= (u32)RCC_CFGR_PPRE2_DIV1;                  // [13:11] APB2 预分频, APB2 = HCLK/1,   不分频
        RCC->CFGR  |= (u32)RCC_CFGR_PPRE1_DIV2;                  // [10: 8] APB1 预分频, APB1 = HCLK/2,    2分频

        RCC->CFGR  &= (u32)(~( 1<<16 | 0x01<<17 | 0xF<<18));     // 清零
        RCC->CFGR  |= (u32)(0x01<<16 | 0x07<<18);                // PLL 时钟源,低频因子，倍频系数， 使PLLCK= HSE * 9= 72MHz   
        RCC->CR    |= (u32)(0x01<<24);                           // 使能 PLL    
        while((RCC->CR & RCC_CR_PLLRDY) == 0) {  }               // 等待PLL就绪
    
        RCC->CFGR &= (u32)((u32)~(0x3<<0));                      // 清0
        RCC->CFGR |= (u32)(0x1 << 1);                            // 切换系统时钟源为：PLLCLOCK    
        while ((RCC->CFGR & (u32)RCC_CFGR_SWS) != (u32)0x08) { } // 等待系统时钟切换完成
    }
    else
    { 
        // 重要！！！
        // 时钟初始化失败 
        // 处理位置
    }    
    // 重定向中断向量表位置
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
}



/*****************************************************************************
*函  数： SystemCoreClockUpdate
*功  能： 更新实际时钟频率，保存在：SystemCoreClock
*参  数：
*返回值：
*备  注： 注释_魔女开发板
*****************************************************************************/
void SystemCoreClockUpdate(void)
{
    u32 tmp = 0, pllmull = 0, pllsource = 0;    

    tmp = RCC->CFGR & RCC_CFGR_SWS;                                 // 获取时钟源

    switch (tmp)  {
        case 0x00:                                                  // HSI 内部高速晶振 被选为系统时钟源
            SystemCoreClock = HSI_VALUE;
            break;
    
        case 0x04:                                                  // HSE 外部高速晶振 被选为系统时钟源
            SystemCoreClock = HSE_VALUE;
            break;
    
        case 0x08:                                                  // PLL 被先为系统时钟源     
            pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;                 // PLL 时钟源及倍频系数
            pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;       
            pllmull = ( pllmull >> 18) + 2;      
            if (pllsource == 0x00)
            {        
                SystemCoreClock = (HSI_VALUE >> 1) * pllmull;       // HSI振荡器时钟2分频作为PLL时钟输入
            }
            else
            {                                                       // HSE作为PLL时钟输入                              
                if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != (u32)RESET)
                {
                    SystemCoreClock = (HSE_VALUE >> 1) * pllmull;   // HSE 2分频
                }
                else
                {
                    SystemCoreClock = HSE_VALUE * pllmull;
                }
            }
            break;

        default:
            SystemCoreClock = HSI_VALUE;
            break;
    } 
    tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];  
    SystemCoreClock >>= tmp;    
}


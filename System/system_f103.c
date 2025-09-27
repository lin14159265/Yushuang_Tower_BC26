/***********************************************************************************************************************************
 ** 【文件名称】  system_f103.c
 ** 【编写人员】  魔女开发板团队
 ** 【更新分享】  Q群文件夹       1126717453
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 ** 【 功  能 】  简化常用的系统函数、初始化函数
 **
 ** 【 c 文件 】  1- USART1  部分
 **               2- SysTick 部分
 **               3- 常用初始化函数 部分
 **               4- 辅助功能函数   部分
 **                 
 ** 【移植说明】  
 **
 ** 【更新记录】 
 **               2021-11-25  完善System_MCO1Init()注释
 **               2021-11-25  取消System_SysTickInit()函数内部的SWD引脚配置代码
 **               2021-09-07  增加内部FLASH数据存取函数
 **               2021-06-12  移除USART1功能函数，使其成为独立文件
 **               2021-04-23  SysTick配置函数，更新时钟源为HCLK
 **
***********************************************************************************************************************************/  
#include "system_f103.h"



/*****************************************************************************
 ** 本地变量声明
 *****************************************************************************/
u64 sysTickCnt = 0;                    // 运行时长，单位：ms
_flag xFlag;                           // 全局状态标志





/*****************************************************************************
 * 函  数： SysTick_Init
 * 功  能： 配置systick定时器， 1ms中断一次， 用于任务调度器、System_DelayMS()、System_DelayUS()
 * 参  数：
 * 返回值： 
 * 重  要： SysTick 的时钟源自 HCLK 的 8 分频
*****************************************************************************/
void System_SysTickInit(void)
{       
    SystemCoreClock = 5201314;             // 用于存放系统时钟频率，先随便设个值
    SystemCoreClockUpdate();               // 获取当前时钟频率， 更新全局变量 SystemCoreClock值 
    //printf("系统运行时钟          %d Hz\r", SystemCoreClock);  // 系统时钟频率信息 , SystemCoreClock在system_stm32f4xx.c中定义   
    
    u32 msTick= SystemCoreClock /1000;     // 计算重载值，全局变量SystemCoreClock的值 ， 定义在system_stm32f10x.c    
    SysTick -> LOAD  = msTick -1;          // 自动重载
    SysTick -> VAL   = 0;                  // 清空计数器
    SysTick -> CTRL  = 0;                  // 清0
    SysTick -> CTRL |= 1<<2;               // 0: 时钟=HCLK/8, 1:时钟=HCLK
    SysTick -> CTRL |= 1<<1;               // 使能中断
    SysTick -> CTRL |= 1<<0;               // 使能SysTick    
    
    printf("SysTick时钟配置       1ms中断1次\r");
} 

/*****************************************************************************
 * 函  数：SysTick_Handler
 * 功  能：SysTick中断函数，必须注释掉stm32f10x_it.c中的SysTick_Handler()
 * 参  数：
 * 返回值：
*****************************************************************************/
void SysTick_Handler(void)
{
    sysTickCnt++;             // 1ms 加1次    
    
    
    #ifdef __SCHEDULER_H     // 任务调度器;如果引用了scheduler.h文件，就调用下面的函数
        Scheduler_TickCnt();      
    #endif
}

/*****************************************************************************
 * 函  数： System_GetRunTimes
 * 功  能： 获取当前的运行时间，单位：毫秒
 * 参  数：
 * 返回值： 
*****************************************************************************/
u64 System_GetTimeMs(void)
{    
    return sysTickCnt  ;
}

/*****************************************************************************
 * 函  数： System_GetTimeUs
 * 功  能： 获取系统上电后运行时间数：us
 * 参  数：
 * 返回值： u32 us
*****************************************************************************/
u32 System_GetTimeUs(void)
{
    u32 ms;
    u32 us;
    do{
        ms = System_GetTimeMs() ;
        us = (float)ms *1000 + (SysTick ->LOAD - SysTick ->VAL )*1000/SysTick->LOAD ;
    }while(ms != System_GetTimeMs() );
    return us;        
}

/*****************************************************************************
 * 函  数： System_DelayMS
 * 功  能： 毫秒延时
 * 使  用： 在调用System_SysTickInit()函数后，即可使用 
 * 参  数： u32 ms : 需要延时的毫秒数
 * 返回值： 
*****************************************************************************/
void System_DelayMS(u32 ms)
{    
    static u64 _startTime=0;
    
    _startTime = System_GetTimeMs() ;
    while( System_GetTimeMs() - _startTime < ms );            
} 

/*****************************************************************************
 * 函  数： System_DelayUS
 * 功  能： 微秒延时; 
 * 使  用： 在调用System_SysTickInit()函数后，即可使用 
 * 参  数： u32 us : 需要延时的微秒数
 * 返回值：
*****************************************************************************/
void System_DelayUS(u32 us)
{
    u64 nowUs = System_GetTimeUs ();
    while(System_GetTimeUs() - nowUs < us);    
}

/*****************************************************************************
 * 函  数： System_GetTimeInterval
 * 功  能： 获取时间间隔，用于测试代码片段运行时间
 * 参  数： 
 * 返回值： 第一次调用返回0, 之后每次调用返回与上次调用的间隔时间(单位:us)
 *****************************************************************************/ 
u32 System_GetTimeInterval(void)
{
    static u32  lastTime=0, nowTime=0;
    
    lastTime = nowTime ;        
    nowTime = System_GetTimeUs ();               
    
    if(lastTime !=0 )                      // 不是第一次调用 
        return (nowTime-lastTime) ;          
   
    return 0;                              // 第1次调用   
}

/*****************************************************************************
 * 函  数： System_TestRunTimes
 * 功  能： 调试时使用，获取代码段的运行时长，并输出到串口 
 * 参  数： 
 * 返回值：
 *****************************************************************************/ 
void System_TestRunTimes(void)
{
    static u8 CNT=0;
    
    u32 intervalTimes = System_GetTimeInterval ();
    if(intervalTimes == 0)
    {
        printf("【运行时长-测试原点放置】\r");
        CNT++;
        System_GetTimeInterval ();
        return;
    }
    
    printf("【运行时长-监察点-%d:%9lu us】\r", CNT++, intervalTimes);
    System_GetTimeInterval ();
} 



/******************************************************************************
 * 函  数： GPIOSet
 * 功  能： 使能相应时钟、配置引脚
 *          外设时钟使能，必须在GPIO配置前，否则会出现问题
 * 参  数：
 * 返回值： 
******************************************************************************/
void System_GPIOSet(GPIO_TypeDef* GPIOx, u32 allPin, u8 mode, u8 speed)
{
    u32 reg = 0; 
    u32 nowPin=0;    
    
    if(GPIOx==GPIOA )   RCC->APB2ENR |= RCC_APB2ENR_GPIOAEN ;
    if(GPIOx==GPIOB )   RCC->APB2ENR |= RCC_APB2ENR_GPIOBEN ;
    if(GPIOx==GPIOC )   RCC->APB2ENR |= RCC_APB2ENR_GPIOCEN ;
    if(GPIOx==GPIOD )   RCC->APB2ENR |= RCC_APB2ENR_GPIODEN ;
    if(GPIOx==GPIOE )   RCC->APB2ENR |= RCC_APB2ENR_GPIOEEN ;    
     
// 配合F4标准库风格
//    // 普通输入    
//    if(mode == GPIO_Mode_IN) {   
//        if(pupd==0)  reg |= 0x01<<2;
//        else         reg |= 0x02<<2;
//    }
//    
//    if((ospeed &0x03)==0) ospeed= 0x03;       // 输出速度，
//    // 普通输出
//    if(mode==GPIO_Mode_OUT){
//        reg = ospeed & 0x03;                  // 引脚速度
//        reg |= (otype & 0x01)<<2;             // 普通推挽、开漏
//    }    
//    // 复用输出
//    if(mode ==GPIO_Mode_AF){
//        reg = ospeed & 0x03;                  // 引脚速度
//        reg |= ((otype | 0x02) & 0x03) <<2;   // 复用推挽、开漏
//    }      


    speed = speed & 0x03;             // 格式化
    if(speed == 0)                    // 输出模式下，不能为0
        speed = 0x03;    
    
    // 8种输入模式,配合标准库的方式
    switch(mode)
    {
        // 4种输入
        case GPIO_Mode_AIN:            // 1 模拟输入模式
            reg = 0x00;     
        break;   
        
        case GPIO_Mode_IN_FLOATING:    // 2 浮空输入模式(复位后的状态)
            reg = 0x04;  
        break;  
        
        case GPIO_Mode_IPD:            // 3 下拉输入模式
            reg = 0x08;
        break;
        
        case GPIO_Mode_IPU:            // 4 上拉输入模式
            reg = 0x08;
        break;
        
        // 4种输出
        case GPIO_Mode_Out_OD:         // 5 开漏输出模式
            reg = 0x04 | speed;
        break;
        
        case GPIO_Mode_Out_PP:         // 6 推挽输出模式
            reg = 0x00 | speed;
        break;
        
        case GPIO_Mode_AF_OD:          // 7 复用开漏输出模式
            reg = 0x0c | speed;
        break;
        
        case GPIO_Mode_AF_PP:          // 8 利用推挽输出模式
            reg = 0x08 | speed;    
        break;        
   
        default:                       // 无法判断，配置为模拟输入模式
            reg = 0x00;
        break;
    }       
    
    // 配置寄存器CHL, 即PIN 0~7          
    for(u32 i=0; i<8; i++) 
    {      
        nowPin = (u32) 0x01 << i;         // 当前要判断的引脚号     
        if((allPin & nowPin) != 0)        // 当前引脚要配置
        {
           GPIOx->CRL &= ~(0x0F<<(i*4));  // 清0
           GPIOx->CRL |= reg<<(i*4);      // 写入新配置     
        }          
    }          
    
    // 配置寄存器CRH, 即PIN 8~15         
    for(u32 i=0; i<8; i++)    
    {      
        nowPin = (u32) 0x01 << (i+8);     // 当前要判断的引脚号     
        if((allPin & nowPin) != 0)        // 当前引脚要配置
        {
           GPIOx->CRH &= ~(0x0F<<(i*4));  // 清0
           GPIOx->CRH |= reg<<(i*4);      // 写入新配置                
        }          
    }             
    
    if(mode == GPIO_Mode_IPU )   GPIOx->BSRR |= allPin ;
    if(mode == GPIO_Mode_IPD)    GPIOx->BSRR |= allPin << 16;         
}



/******************************************************************************
 * 函  数： NVICSet
 * 功  能： 优先级设置，为方便管理及使用FreeRTOS，统一使用4位抢占级(16级),0位子优先级(0级)
 *         直接调用即可，不用提前配置
 * 参  数： 
 * 返回值： 
******************************************************************************/
void System_NVICSet(u8 NVIC_Channel, u8 Preemption)
{    
    static u8 setGrouped=0;
    if(setGrouped ==0){
        // 全局分级设置,统一为组4, 值0b11,即：NVIC->IPx中高4位:主级4位(16级), 子级0位(0级）  
        SCB->AIRCR = ((u32)0x05FA0000)|(0x03<<8);   // 优先级分组设置, 已查,是3， F103和F429寄存器通用        
        setGrouped =1;
    }
    
    // 通道中断优先级设置
    NVIC->IP[NVIC_Channel] &= ~(0xF<<4);                     // 清空          
    NVIC->IP[NVIC_Channel]  =  (Preemption&0xF)<<4;          // 写入抢占级\优先级
    // 通道中断使能
    NVIC->ISER[NVIC_Channel/32] |= 1 << (NVIC_Channel % 32); // 使能中断通道        
    //NVIC->ICER[];                                            // 中断失能, 很少用到       
}



/***************************************************************************** 
 * 函  数： EXTISet
 * 功  能： 外部中断配置函数
 *         重要: 一次只能配置1个IO口,  2020-2-26
 *         只针对GPIOA~G;不包括PVD,RTC和USB唤醒这三个
 *         该函数会自动开启对应中断,以及屏蔽线  
 *         
 * 参  数： 【GPIOx】:GPIOA~G, 代表GPIOA~G
 *          【BITx】:GPIO_Pin_0~15, 需要使能的位;
 *          【TRIM】:触发模式, EXTI_FTIR/1:下降沿;  EXTI_RTIR/2:上升沿; 3:任意电平触发
 * 返  回： 
*****************************************************************************/
void System_EXTISet(GPIO_TypeDef* GPIOx, u16 PINx, u8 TRIM)
{
    u8 gpioNum = 0;
    u8 pinNum  = 0;
    
    // 转换GPIOx为数字
    if(GPIOx==GPIOA )  gpioNum=0;
    if(GPIOx==GPIOB )  gpioNum=1;
    if(GPIOx==GPIOC )  gpioNum=2;
    if(GPIOx==GPIOD )  gpioNum=3; 
    if(GPIOx==GPIOE )  gpioNum=4; 
    if(GPIOx==GPIOF )  gpioNum=5; 
    if(GPIOx==GPIOG )  gpioNum=6; 
    
    // 转换PINx为数字
    for(u8 i=0; i<16; i++){
        if( PINx== ((u32)1<<i)){
            pinNum=i;
            break;
        }          
    }    
    
    u8 offSet   = (pinNum%4)*4;                    // 寄存器内偏移
    RCC->APB2ENR |=0x01;                           // 使能io复用时钟             
    AFIO->EXTICR[pinNum/4] &=~(0x000F << offSet);  // 清0
    AFIO->EXTICR[pinNum/4] |=  gpioNum << offSet;  // EXTI.BITx映射到GPIOx.BITx 
    // 使能line BITx上的中断, 1:使能  0:屏蔽
    EXTI->IMR |= PINx ;                            
    //EXTI->EMR|=1<<BITx;                          // 不屏蔽line BITx上的事件 (如果不屏蔽这句,在硬件上是可以的,但是在软件仿真的时候无法进入中断!)
    // 触发沿
    if(TRIM & 0x01)  EXTI->FTSR |= PINx ;          // line BITx上事件下降沿触发
    if(TRIM & 0x02)  EXTI->RTSR |= PINx ;          // line BITx上事件上升降沿触发
}



/******************************************************************************
 * 函  数： System_SwdMode
 * 功  能： 设置芯片调试方式(SWD)
 *          关闭JTAG-DP,启用SW-DP，可释放引脚PB3、PB4、PA15，只需PA13、PA14
 * 参  数：
 * 返回值：     
*****************************************************************************/
void  System_SwdMode(void)                            
{
    RCC->APB2ENR|=1<<0;           // 开启辅助时钟       
    AFIO->MAPR &= 0XF8FFFFFF;     // 清0MAPR的[26:24]
    AFIO->MAPR |= 0x2<<24;        // 设置模式  000:全开   010：只开SWD   100:全关 
}



/*****************************************************************************
 * 函  数： GetSystemClock
 * 功  能： 获取系统时钟频率，
 * 参  数：
 * 返回值： u32 当前系统时钟频率
*****************************************************************************/
uint32_t System_GetSystemClock(void) 
{    
    SystemCoreClockUpdate();      // 获取系统时钟频率，并更新全局变量 SystemCoreClock的值 
    return SystemCoreClock ;
}



/*****************************************************************************
 * 函  数： System_Reset
 * 功  能： 系统软复位(F103和F429通用) 
 * 参  数：
 * 返回值：
*****************************************************************************/   
void System_Reset(void)
{   
    SCB->AIRCR =0X05FA0000|(u32)0x04;      
}     



// 采用如下方法实现执行汇编指令WFI  
void WFI_SET(void)
{
    __ASM volatile("wfi");          
}

// 关闭所有中断
void System_IntxDisable(void)
{          
    __ASM volatile("cpsid i");
}

// 开启所有中断
void System_IntxEnable(void)
{
    __ASM volatile("cpsie i");          
} 

// 进入待机模式      
void System_Standby(void)
{
    SCB->SCR|=1<<2;          // 使能SLEEPDEEP位 (SYS->CTRL)       
    RCC->APB1ENR|=1<<28;     // 使能电源时钟        
    PWR->CSR|=1<<8;          // 设置WKUP用于唤醒
    PWR->CR|=1<<2;           // 清除Wake-up 标志
    PWR->CR|=1<<1;           // PDDS置位          
    WFI_SET();               // 执行WFI指令         
}      


/******************************************************************************
 * 函  数： System_MCO1Init
 * 功  能： 把时钟输出引脚, 方便使用示波器检查时钟是否正确 
 *          输出时，请保证输出时钟频率不超过50MHz,因为此为IO口最高频率
 * 参  数： 0x00-没有时钟输出
 *          0x04-系统时钟SYSCLK输出
 *          0x05-内部8MHz的RC振荡器时钟输出
 *          0x06-外部3~25MHz振荡器时钟输出
 *          0x07-PLL时钟2分频后输出
 *          0x08-PLL2时钟输出
 *          0x09-PLL时钟3分频后输出
 *          0x0A-XT1外部3~25MHz振荡器时钟输出(为以太网)
 *          0x0B-PLL3时钟输出(为以太网)
 * 返回值： 无
 ******************************************************************************/  
void System_MCO1Init (uint32_t source)
{
    RCC->CFGR &= ~(0x0f << 24);
    RCC->CFGR |= (source & 0x0f)<<24;
}



// 作用：内部FLASH写入时，等待空闲，BSY位标志:0闲1忙
static uint8_t waitForFlashBSY(uint32_t timeOut)
{                                                        
    while((FLASH->SR & 0x01) && (timeOut-- != 0x00)) ; // 等待BSY标志空闲
    if(timeOut ==0)   
        return 1;    // 失败，返回1, 等待超时;　
    
    return 0;        // 正常，返回０                
}

// 下面这三个定义，用于存取内部FLASH数据，F103系列都不用修改
#ifdef   STM32F10X_HD
#define  STM32_FLASH_SECTOR_SIZE       2048                    // 内部FLASH页大小, 单位：bytes 　(注：STM32F10xx系列下，小中容量存储器扇区为1K, 大容量存储器扇区为2K）
#else
#define  STM32_FLASH_SECTOR_SIZE       1024                    // 内部FLASH页大小, 单位：bytes 　(注：STM32F10xx系列下，小中容量存储器扇区为1K, 大容量存储器扇区为2K）
#endif

#define  STM32_FLASH_ADDR_BASE   0x08000000                    // 芯片内部FLASH基址(这个基本不用修改）
static   uint8_t sectorbufferTemp[STM32_FLASH_SECTOR_SIZE];    // 开辟一段内存空间，作用：在内部FLASH写入时作数据缓冲

/******************************************************************************
 * 函　数:  System_WriteInteriorFlash
 * 功  能： 在芯片的内部FLASH里，写入指定长度数据
 * 参  数： uint32_t  writeAddr        重要：写入的目标地址必须是偶数!!!
 *          uint8_t  *writeToBuffer
 *          uint16_t  numToWrite       重要: 写入的数量, 必须是偶数!!!因为内部FLASH每次是16位写入.如果目标数据量是单数,侧追加缓存位置的后一字节补位存入.
 *
 * 返回值：0_成功，
 *         1_失败，地址范围不正确
 *         2_失败，FLASH->SR:BSY忙超时
 *         3_失败，擦除超时
 * 备  注： 
 ******************************************************************************/  
uint8_t System_WriteInteriorFlash(uint32_t writeAddr, uint8_t *writeToBuffer, uint16_t numToWrite)               
{
    uint16_t flashSize = *(uint16_t*)(0x1FFFF7E0);                // 读取芯片FLASH大小;本寄存器值为芯片出厂前写入的FLASH大小，只读，单位：KByte
                    
    uint32_t addrOff    = writeAddr - STM32_FLASH_ADDR_BASE;      // 去掉0x08000000后的实际偏移地址
    uint32_t secPos     = addrOff / STM32_FLASH_SECTOR_SIZE;;     // 扇区地址,即起始地址在第几个扇区
    uint16_t secOff     = addrOff%STM32_FLASH_SECTOR_SIZE ;       // 开始地始偏移字节数: 数据在扇区的第几字节存放
    uint16_t secRemain  = STM32_FLASH_SECTOR_SIZE - secOff;       // 本扇区需要写入的字节数 ,用于判断够不够存放余下的数据
    
    // 判断地址有效性   
    if(writeAddr < STM32_FLASH_ADDR_BASE)    return 1;                     // 如果读的地址，小于FLASH的最小地址，则退出，返回1_地址失败
    if(writeAddr > (STM32_FLASH_ADDR_BASE+(flashSize*1024)))    return 1;  // 如果读的地址，超出FLASH的最大地址，则退出, 返回1_地址失败
               
    // 0_解锁FLASH
    FLASH->KEYR = ((uint32_t)0x45670123);
    FLASH->KEYR = ((uint32_t)0xCDEF89AB);
    
    if(numToWrite <= secRemain)    secRemain=numToWrite;  
    while(1){    
        // 1_读取当前页的数据
        if(waitForFlashBSY(0x00888888))   return 2;                         // 失败，返回:2, 失败原因：FLASH->SR:BSY忙超时                    
        System_ReadInteriorFlash ( secPos*STM32_FLASH_SECTOR_SIZE+STM32_FLASH_ADDR_BASE , sectorbufferTemp, STM32_FLASH_SECTOR_SIZE );   // 读取扇区内容到缓存
                  
        // 2_擦险指定页(扇区)                           
        if(waitForFlashBSY(0x00888888))   return 2;                         // 失败，返回:2, 失败原因：FLASH->SR:BSY忙超时  
        FLASH->CR|= 1<<1;                                                   // PER:选择页擦除;位2MER为全擦除
        FLASH->AR = STM32_FLASH_ADDR_BASE + secPos*STM32_FLASH_SECTOR_SIZE; // 填写要擦除的页地址
        FLASH->CR|= 0x40;                                                   // STRT:写1时触发一次擦除运作　
        if(waitForFlashBSY(0x00888888))   return 2;                         // 失败，返回:３, 失败原因：擦除超时
        FLASH->CR &= ((uint32_t)0x00001FFD);                                // 关闭页擦除功能   
                                          
        for(uint16_t i=0; i<secRemain ; i++)                                // 原始数据写入缓存
            sectorbufferTemp[secOff+i] = writeToBuffer[i];
       
        for(uint16_t i=0; i<STM32_FLASH_SECTOR_SIZE/2 ; i++){               // 缓存数据写入芯片FLASH                      
            if(waitForFlashBSY(0x00888888))   return 2;                     // 失败，返回:2, 失败原因：FLASH->SR:BSY忙超时
            FLASH->CR |= 0x01<<0;                                           // PG: 编程                             
            *(uint16_t*)(STM32_FLASH_ADDR_BASE + secPos*STM32_FLASH_SECTOR_SIZE +i*2) = (sectorbufferTemp[i*2+1]<<8) | sectorbufferTemp[i*2] ; // 缓存数据写入设备
                                                        
            if(waitForFlashBSY(0x00888888))   return 2;                     // 失败，返回:2, 失败原因：FLASH->SR:BSY忙超时
            FLASH->CR &= ((uint32_t)0x00001FFE) ;                           // 关闭编程
        }
        
        if(secRemain == numToWrite){                          
            break;                                                          // 已全部写入
        }                        
        else{                                                               // 未写完                                                                          
            writeToBuffer += secRemain ;                                    // 原始数据指针偏移
            secPos ++;                                                      // 新扇区
            secOff =0;                                                      // 新偏移位,扇区内数据起始地址            
            numToWrite -= secRemain ;                                       // 剩余未写字节数            
            secRemain = (numToWrite>STM32_FLASH_SECTOR_SIZE)?(STM32_FLASH_SECTOR_SIZE):numToWrite;  // 计算新扇区写入字节数                  
        }              
    } 
    FLASH->CR |= 1<<7 ;                                                     // LOCK:重新上锁    
 
    return 0;                
}

/******************************************************************************
 * 函　数:  System_ReadInteriorFlash
 * 功  能： 在芯片的内部FLASH里，读取指定长度数据
 * 参  数： uint32_t  readAddr     数据地址
 *          uint8_t  *pBuffer      读出后存放位置
 *          uint16_t  numToRead    读取的字节数量
 * 返回值： 0_成功
 *          1_失败，地址小于FLASH基址
 *          2_失败，地址大于FLASH最大值
 * 备  注： 
 ******************************************************************************/  
uint8_t  System_ReadInteriorFlash (uint32_t readAddr, uint8_t *pBuffer, uint16_t numToRead) 
{    
    // 获取芯片FLASH大小            
    uint16_t flashSize = *(uint16_t*)(0x1FFFF7E0);                        // 读取芯片FLASH大小;本寄存器值为芯片出厂前写入的FLASH大小，只读单位：KByte
                
    // 判断地址有效性                
    if(readAddr < STM32_FLASH_ADDR_BASE)    return 1;                     // 如果读的地址，小于FLASH的最小地址，则退出
    if(readAddr > (STM32_FLASH_ADDR_BASE+(flashSize*1024)))  return 2;    // 如果读的地址，超出FLASH的最大地址，则退出
              
    // 开始复制                
    while(numToRead--)
    {
        *pBuffer = *(__IO uint8_t*)readAddr;
        pBuffer++;        // 指针后移一个数据长度
        readAddr++;       // 偏移一个数据位
    }      
    
    return 0;             // 成功，返回0;    
}

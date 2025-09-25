#include "bsp_key.h"
#include "bsp_led.h"





void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;    // 作用：配置引脚工作模式
    NVIC_InitTypeDef NVIC_InitStruct ;      // 作用：配置优先级
    EXTI_InitTypeDef EXTI_InitStruct ;      // 作用：配置引脚中断方式

    // 使能AFIO时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE) ;      // EXTI的时钟要设置AFIO寄存器

    // 使能GPIO时钟, 为减少调试过程忘了使能时钟而出错，把相关GPIO端口时钟，都使能了；
    RCC->APB2ENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF ;

    // KEY_1
    // 配置引脚工作模式
    GPIO_InitStructure.GPIO_Pin  = KEY_1_PIN;                  // 引脚KEY_1, 闲时下拉(重要)， 按下时被置高电平
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD ;             // 引脚工作模式; 闲时电平状态(使用芯片内部电阻进行电平上下拉)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz ;         // 输出电平反转速度；在输入状态时无效，但GPIO_Init函数需要用到；
    GPIO_Init(KEY_1_GPIO, &GPIO_InitStructure);                // 初始化，调用引脚工作模式配置函数
    // 配置中断的优先级
    NVIC_InitStruct.NVIC_IRQChannel = KEY_1_IRQN ;             // 中断号
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1 ;    // 配置抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0 ;           // 配置子优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE ;              // 使能中断通道
    NVIC_Init(&NVIC_InitStruct) ;                              // 初始化，调用优先级配置函数
    // 配置中断的方式
    GPIO_EXTILineConfig(KEY_1_GPIOSOURCE, KEY_1_PINSOURCE);    // 选择作为EXTI线的GPIO引脚
    EXTI_InitStruct.EXTI_Line    = KEY_1_EXTI_LINE ;           // 配置中断or事件线
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt ;       // 模式：中断：EXTI_Mode_Interrupt、事件：EXTI_Mode_Event
    EXTI_InitStruct.EXTI_Trigger = KEY_1_TRIM ;                // 边沿触发： 上升：EXTI_Trigger_Rising 、下降：EXTI_Trigger_Falling 、浮空：EXTI_Trigger_Rising_Falling
    EXTI_InitStruct.EXTI_LineCmd = ENABLE ;                    // 使能EXTI线
    EXTI_Init(&EXTI_InitStruct) ;                              // 初始化，调用中断线配置函数

    // KEY_2
    // 配置引脚工作模式
    GPIO_InitStructure.GPIO_Pin  = KEY_2_PIN;                  // 引脚KEY_1, 闲时下拉(重要)， 按下时被置高电平
    GPIO_InitStructure.GPIO_Mode = KEY_2_MODE ;                // 引脚工作模式; 闲时电平状态(使用芯片内部电阻进行电平上下拉)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz ;         // 输出电平反转速度；在输入状态时无效，但GPIO_Init函数需要用到；
    GPIO_Init(KEY_2_GPIO, &GPIO_InitStructure);                // 初始化，调用引脚工作模式配置函数
    // 配置中断的优先级
    NVIC_InitStruct.NVIC_IRQChannel = KEY_2_IRQN ;             // 中断号
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1 ;    // 配置抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0 ;           // 配置子优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE ;              // 使能中断通道
    NVIC_Init(&NVIC_InitStruct) ;                              // 初始化，调用优先级配置函数
    // 配置中断的方式
    GPIO_EXTILineConfig(KEY_2_GPIOSOURCE, KEY_2_PINSOURCE);    // 选择作为EXTI线的GPIO引脚
    EXTI_InitStruct.EXTI_Line    = KEY_2_EXTI_LINE ;           // 配置中断or事件线
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt ;       // 模式：中断：EXTI_Mode_Interrupt、事件：EXTI_Mode_Event
    EXTI_InitStruct.EXTI_Trigger = KEY_2_TRIM ;                // 边沿触发： 上升：EXTI_Trigger_Rising 、下降：EXTI_Trigger_Falling 、浮空：EXTI_Trigger_Rising_Falling
    EXTI_InitStruct.EXTI_LineCmd = ENABLE ;                    // 使能EXTI线
    EXTI_Init(&EXTI_InitStruct) ;                              // 初始化，调用中断线配置函数

    // KEY_3
    // 配置引脚工作模式
    GPIO_InitStructure.GPIO_Pin  = KEY_3_PIN;                  // 引脚KEY_1, 闲时下拉(重要)， 按下时被置高电平
    GPIO_InitStructure.GPIO_Mode = KEY_3_MODE ;                // 引脚工作模式; 闲时电平状态(使用芯片内部电阻进行电平上下拉)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz ;         // 输出电平反转速度；在输入状态时无效，但GPIO_Init函数需要用到；
    GPIO_Init(KEY_3_GPIO, &GPIO_InitStructure);                // 初始化，调用引脚工作模式配置函数
    // 配置中断的优先级
    NVIC_InitStruct.NVIC_IRQChannel = KEY_3_IRQN ;             // 中断号
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1 ;    // 配置抢占优先级
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0 ;           // 配置子优先级
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE ;              // 使能中断通道
    NVIC_Init(&NVIC_InitStruct) ;                              // 初始化，调用优先级配置函数
    // 配置中断的方式
    GPIO_EXTILineConfig(KEY_3_GPIOSOURCE, KEY_3_PINSOURCE);    // 选择作为EXTI线的GPIO引脚
    EXTI_InitStruct.EXTI_Line    = KEY_3_EXTI_LINE ;           // 配置中断or事件线
    EXTI_InitStruct.EXTI_Mode    = EXTI_Mode_Interrupt ;       // 模式：中断：EXTI_Mode_Interrupt、事件：EXTI_Mode_Event
    EXTI_InitStruct.EXTI_Trigger = KEY_3_TRIM ;                // 边沿触发： 上升：EXTI_Trigger_Rising 、下降：EXTI_Trigger_Falling 、浮空：EXTI_Trigger_Rising_Falling
    EXTI_InitStruct.EXTI_LineCmd = ENABLE ;                    // 使能EXTI线
    EXTI_Init(&EXTI_InitStruct) ;                              // 初始化，调用中断线配置函数

    printf("按键 初始化           配置完成\r");
}



// KEY_1 中断服务函数
void KEY_1_IRQHANDLER(void)                        // 提示：这个函数名，是h文件中的宏定义，在编译过程中，会被替换成宏定义的值
{
    if (EXTI->PR & KEY_1_PIN)                      // 板子上的按键已使用电容作简单的硬件消抖,无需再使用软件延时消抖
    {
        EXTI->PR |= KEY_1_PIN  ;                   // 清理中断标示
        printf("第 1 个按键被按下, 蓝灯反转\r");   // 重要提示：printf是不可重入函数，中断服务函数中使用，可能会产生不可预测的错误。这里使用printf，只用代码测试使用！！
        
    }
}



// KEY_2 中断服务函数
void KEY_2_IRQHANDLER(void)                        // 提示：这个函数名，是h文件中的宏定义，在编译过程中，会被替换成宏定义的值
{
    if (EXTI->PR & KEY_2_PIN)                      // 板子上的按键已使用电容作简单的硬件消抖,无需再使用软件延时消抖
    {
        EXTI->PR |= KEY_2_PIN  ;                   // 清理中断标示
        printf("第 2 个按键被按下, 蓝灯反转\r");   // 重要提示：printf是不可重入函数，中断服务函数中使用，可能会产生不可预测的错误。这里使用printf，只用代码测试使用！！      // 魔女开发板的按键使用电容进行硬件消抖,无需再使用软件延时消抖
   
    }
}



// KEY_3 中断服务函数
void KEY_3_IRQHANDLER(void)                        // 提示：这个函数名，是h文件中的宏定义，在编译过程中，会被替换成宏定义的值
{
    if (EXTI->PR & KEY_3_PIN)                      // 板子上的按键已使用电容作简单的硬件消抖,无需再使用软件延时消抖
    {
        EXTI->PR |= KEY_3_PIN  ;                   // 清理中断标示
        printf("第 3 个按键被按下, 蓝灯反转\r");   // 重要提示：printf是不可重入函数，中断服务函数中使用，可能会产生不可预测的错误。这里使用printf，只用代码测试使用！！      // 魔女开发板的按键使用电容进行硬件消抖,无需再使用软件延时消抖
    }
}





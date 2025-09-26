/***********************************************************************************************************************************
 ** 【代码编写】  魔女开发板团队
 ** 【代码更新】
 ** 【淘    宝】  魔女开发板      https://demoboard.taobao.com
 ***********************************************************************************************************************************
 **【文件名称】  bsp_usart.c
 **
 **【文件功能】  各USART的GPIO配置、通信协议配置、中断配置，及功能函数实现
 **
 **【适用平台】  STM32F103 + 标准库v3.5 + keil5
 **
 ** 【代码说明】  本文件的收发机制, 经多次修改, 已比较完善.
 **               初始化: 只需调用：USARTx_Init(波特率), 函数内已做好引脚及时钟配置；
 **               发 送 : 两个函数, 字符串: USARTx_SendString (char* stringTemp)、 数据: USARTx_SendData (uint8_t* buf, uint8_t cnt);
 **               接 收 : 方式1：通过全局函数USARTx_GetBuffer (uint8_t* buffer, uint8_t* cnt);　本函数已清晰地示例了接收机制；
 **                       方式2：通过判断: xUSART.USARTxReceivedNum > 0;
 **                              如在while中不断轮询，或在任何一个需要的地方判断接收到的字节数是否大于0．示例：
 **                              while(1){
 **                                  if(xUSART1.USART1ReceivedNum>0){
 **                                      printf((char*)xUSART.USART1ReceivedBuffer);          // 示例1: 如何输出成字符串
 **                                      uint16_t GPS_Value = xUSART.USART1ReceivedBuffer[1]; // 示例2: 如何读写其中某个成员的数据
 **                                      xUSART1.USART1ReceivedNum = 0;                     // 重要：处理完数据后
 **                                  }
 **                              }
 **
 **【更新记录】
 **              2021-12-16  完善接收机制：取消接收标志，判断接收字节数>0即为接收到新数据
 **              2021-12-15  完善接收机制，新帧数据包，可覆盖旧数据
 **              2021-11-03  完善接收函数返回值处理
 **              2021-08-31  修改: 接收中断，增加已接收包处理判断，如果旧包未处理，则放弃新包的数据，避免新包覆盖旧包；
 **              2021-08-14  修正: 接收中断，uint8_t cnt, 修改为uint16_t; 并增加已接收字节数判断，避免所接收的帧数据大于缓存空间时，后来数据覆盖起始数据;
 **              2021-06-22  完善注释说明;
 **              2021-06-12  完善文件结构;
 **              2021-06-09  完善文件格式、注释
 **              2021-06-08  USART1、2、3及UART4、5 的收发完善代码
 **              2020-09-02  文件建立、USART1接收中断、空闲中断、发送中断、DMA收发
 **
************************************************************************************************************************************/
#include "bsp_usart.h"



xUSATR_TypeDef  xUSART;         // 声明为全局变量,方便记录信息、状态




//////////////////////////////////////////////////////////////   USART-1   //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************
 * 函  数： vUSART1_Init
 * 功  能： 初始化USART1的GPIO、通信参数配置、中断优先级
 *          (8位数据、无校验、1个停止位)
 * 参  数： uint32_t baudrate  通信波特率
 * 返回值： 无
 ******************************************************************************/
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;                           // 使能USART1时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;                             // 使能GPIOA时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    USART_DeInit(USART1);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_Init(USART1, &USART_InitStructure);                       // 初始化串口

    USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                  // 使能接受中断
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);                  // 使能空闲中断

    USART_Cmd(USART1, ENABLE);                                      // 使能串口, 开始工作

    USART1->SR = ~(0x00F0);                                         // 清理中断

    xUSART.USART1InitFlag = 1;                                      // 标记初始化标志
    xUSART.USART1ReceivedNum = 0;                                   // 接收字节数清零

    printf("\r\r\r=========== 魔女开发板 STM32F103 外设初始报告 ===========\r");
    printf("USART1初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： USART1_IRQHandler
 * 功  能： USART1的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 *
******************************************************************************/
static uint8_t U1TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U1TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U1TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

// 文件: bsp_usart.c

void USART1_IRQHandler(void)
{
    // static uint16_t cnt = 0;  //<-- 不再需要这个静态的cnt
    // static uint8_t  RxTemp[U1_RX_BUF_SIZE]; //<-- 也不再需要这个临时数组

    // 接收中断
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 检查缓冲区是否已满，防止溢出
        if(xUSART.USART1ReceivedNum < U1_RX_BUF_SIZE)
        {
            // 直接将接收到的数据追加到全局缓冲区的末尾
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum++] = USART_ReceiveData(USART1);
        }
        else
        {
            // 缓冲区满了，丢弃数据，但要读一下DR来清除中断标志
            USART_ReceiveData(USART1);
        }
        // 注意：RXNE标志在读取DR后会自动清除
    }

    // 空闲中断, 在这种模式下，空闲中断可以什么都不做，或者只用于一些特殊逻辑
    // 为了兼容您原来的设计，我们保留空闲中断的清除操作
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        // 空闲中断的清除方法是先读SR再读DR
        // 因为上面的RXNE中断可能没发生，所以这里需要安全地读一下
        volatile uint16_t temp;
        temp = USART1->SR;
        temp = USART1->DR;
    }

    // 发送中断 (这部分逻辑不变)
    if ((USART1->SR & 1 << 7) && (USART1->CR1 & 1 << 7))
    {
        USART1->DR = U1TxBuffer[U1TxCounter++];
        if (U1TxCounter == U1TxCount)
            USART1->CR1 &= ~(1 << 7);
    }
}

/******************************************************************************
 * 函  数： vUSART1_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t USART1_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.USART1ReceivedNum > 0)                                           // 判断是否有新数据
    {
        memcpy(buffer, xUSART.USART1ReceivedBuffer, xUSART.USART1ReceivedNum);  // 把新数据复制到指定位置
        *cnt = xUSART.USART1ReceivedNum;                                        // 把新数据的字节数，存放指定变量
        xUSART.USART1ReceivedNum = 0;                                           // 接收标记置0
        return *cnt;                                                            // 返回所接收到新数据的字节数
    }
    return 0;                                                                   // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUSART1_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void USART1_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U1TxBuffer[U1TxCount++] = buf[i];

    if ((USART1->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        USART1->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUSART1_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void USART1_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    USART1_SendData((u8 *)stringTemp, num);      // 注意调用函数所需要的真实数据长度; 如果目标需要以0作结尾判断，需num+1：字符串以0结尾，即多发一个:0
}

/******************************************************************************
 * 函  数： vUSART1_SendStringForDMA
 * 功  能： UART通过DMA发送数据，省了占用中断的时间
 *         【适合场景】字符串，字节数非常多，
 *         【不 适 合】1:只适合发送字符串，不适合发送可能含0的数值类数据; 2-时间间隔要足够
 * 参  数： char strintTemp  要发送的字符串首地址
 * 返回值： 无
 ******************************************************************************/
void USART1_SendStringForDMA(char *stringTemp)
{
    static u8 Flag_DmaTxInit = 0;                // 用于标记是否已配置DMA发送
    u32   num = 0;                               // 发送的数量，注意发送的单位不是必须8位的
    char *t = stringTemp ;                       // 用于配合计算发送的数量

    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位

    while (DMA1_Channel4->CNDTR > 0);            // 重要：如果DMA还在进行上次发送，就等待; 得进完成中断清标志，F4不用这么麻烦，发送完后EN自动清零
    if (Flag_DmaTxInit == 0)                     // 是否已进行过USAART_TX的DMA传输配置
    {
        Flag_DmaTxInit  = 1;                     // 设置标记，下次调用本函数就不再进行配置了
        USART1 ->CR3   |= 1 << 7;                // 使能DMA发送
        RCC->AHBENR    |= 1 << 0;                // 开启DMA1时钟  [0]DMA1   [1]DMA2

        DMA1_Channel4->CCR   = 0;                // 失能， 清0整个寄存器, DMA必须失能才能配置
        DMA1_Channel4->CNDTR = num;              // 传输数据量
        DMA1_Channel4->CMAR  = (u32)stringTemp;  // 存储器地址
        DMA1_Channel4->CPAR  = (u32)&USART1->DR; // 外设地址

        DMA1_Channel4->CCR |= 1 << 4;            // 数据传输方向   0:从外设读   1:从存储器读
        DMA1_Channel4->CCR |= 0 << 5;            // 循环模式       0:不循环     1：循环
        DMA1_Channel4->CCR |= 0 << 6;            // 外设地址非增量模式
        DMA1_Channel4->CCR |= 1 << 7;            // 存储器增量模式
        DMA1_Channel4->CCR |= 0 << 8;            // 外设数据宽度为8位
        DMA1_Channel4->CCR |= 0 << 10;           // 存储器数据宽度8位
        DMA1_Channel4->CCR |= 0 << 12;           // 中等优先级
        DMA1_Channel4->CCR |= 0 << 14;           // 非存储器到存储器模式
    }
    DMA1_Channel4->CCR  &= ~((u32)(1 << 0));     // 失能，DMA必须失能才能配置
    DMA1_Channel4->CNDTR = num;                  // 传输数据量
    DMA1_Channel4->CMAR  = (u32)stringTemp;      // 存储器地址
    DMA1_Channel4->CCR  |= 1 << 0;               // 开启DMA传输
}






//////////////////////////////////////////////////////////////   USART-2   //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************
 * 函  数： vUSART2_Init
 * 功  能： 初始化USART的GPIO、通信参数配置、中断优先级
 *          (8位数据、无校验、1个停止位)
 * 参  数： uint32_t baudrate  通信波特率
 * 返回值： 无
 ******************************************************************************/
void USART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;                           // 使能USART2时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;                             // 使能GPIOA时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    //USART_DeInit(USART2);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_Init(USART2, &USART_InitStructure);                       // 初始化串口

    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);                   // 使能发送中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);                  // 使能接受中断
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);                  // 使能空闲中断

    USART_Cmd(USART2, ENABLE);                                      // 使能串口, 开始工作

    USART2->SR = ~(0x00F0);                                         // 清理中断

    xUSART.USART2InitFlag = 1;                                      // 标记初始化标志
    xUSART.USART2ReceivedNum = 0;                                   // 接收字节数清零

    printf("\rUSART2初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： USART2_IRQHandler
 * 功  能： USART2的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
static uint8_t U2TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U2TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U2TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

void USART2_IRQHandler(void)
{
    static uint16_t cnt = 0;                                         // 接收字节数累计：每一帧数据已接收到的字节数
    static uint8_t  RxTemp[U2_RX_BUF_SIZE];                          // 接收数据缓存数组：每新接收１个字节，先顺序存放到这里，当一帧接收完(发生空闲中断), 再转存到全局变量：xUSART.USARTxReceivedBuffer[xx]中；

    // 接收中断
    if (USART2->SR & (1 << 5))                                       // 检查RXNE(读数据寄存器非空标志位); RXNE中断清理方法：读DR时自动清理；
    {
        if ((cnt >= U2_RX_BUF_SIZE))//||xUSART.USART2ReceivedFlag==1 // 判断1: 当前帧已接收到的数据量，已满(缓存区), 为避免溢出，本包后面接收到的数据直接舍弃．
        {
            // 判断2: 如果之前接收好的数据包还没处理，就放弃新数据，即，新数据帧不能覆盖旧数据帧，直至旧数据帧被处理．缺点：数据传输过快于处理速度时会掉包；好处：机制清晰，易于调试
            USART2->DR;                                              // 读取数据寄存器的数据，但不保存．主要作用：读DR时自动清理接收中断标志；
            return;
        }
        RxTemp[cnt++] = USART2->DR ;                                 // 把新收到的字节数据，顺序存放到RXTemp数组中；注意：读取DR时自动清零中断位；
    }

    // 空闲中断, 用于配合接收中断，以判断一帧数据的接收完成
    if (USART2->SR & (1 << 4))                                       // 检查IDLE(空闲中断标志位); IDLE中断标志清理方法：序列清零，USART1 ->SR;  USART1 ->DR;
    {
        xUSART.USART2ReceivedNum  = 0;                               // 把接收到的数据字节数清0
        memcpy(xUSART.USART2ReceivedBuffer, RxTemp, U2_RX_BUF_SIZE); // 把本帧接收到的数据，存放到全局变量xUSART.USARTxReceivedBuffer中, 等待处理; 注意：复制的是整个数组，包括0值，以方便字符串数据
        xUSART.USART2ReceivedNum  = cnt;                             // 把接收到的字节数，存放到全局变量xUSART.USARTxReceivedCNT中；
        cnt = 0;                                                     // 接收字节数累计器，清零; 准备下一次的接收
        memset(RxTemp, 0, U2_RX_BUF_SIZE);                           // 接收数据缓存数组，清零; 准备下一次的接收
        USART2 ->SR;
        USART2 ->DR;                                 // 清零IDLE中断标志位!! 序列清零，顺序不能错!!
    }

    // 发送中断
    if ((USART2->SR & 1 << 7) && (USART2->CR1 & 1 << 7))             // 检查TXE(发送数据寄存器空)、TXEIE(发送缓冲区空中断使能)
    {
        USART2->DR = U2TxBuffer[U2TxCounter++];                      // 读取数据寄存器值；注意：读取DR时自动清零中断位；
        if (U2TxCounter == U2TxCount)
            USART2->CR1 &= ~(1 << 7);                                // 已发送完成，关闭发送缓冲区空置中断 TXEIE
    }
}

/******************************************************************************
 * 函  数： vUSART2_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t USART2_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.USART2ReceivedNum > 0)                                          // 判断是否有新数据
    {
        memcpy(buffer, xUSART.USART2ReceivedBuffer, xUSART.USART2ReceivedNum); // 把新数据复制到指定位置
        *cnt = xUSART.USART2ReceivedNum;                                       // 把新数据的字节数，存放指定变量
        xUSART.USART2ReceivedNum = 0;                                          // 接收标记置0
        return *cnt;                                                           // 返回所接收到新数据的字节数
    }
    return 0;                                                                  // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUSART2_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void USART2_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U2TxBuffer[U2TxCount++] = buf[i];

    if ((USART2->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        USART2->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUSART2_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void USART2_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    USART2_SendData((u8 *)stringTemp, num);      // 注意调用函数所需要的真实数据长度; 如果目标需要以0作结尾判断，需num+1：字符串以0结尾，即多发一个:0
}





//////////////////////////////////////////////////////////////   USART-3   //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************
 * 函  数： vUSART3_Init
 * 功  能： 初始化USART的GPIO、通信参数配置、中断优先级
 *          (8位数据、无校验、1个停止位)
 * 参  数： uint32_t baudrate  通信波特率
 * 返回值： 无
 ******************************************************************************/
void USART3_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;                           // 使能USART3时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;                             // 使能GPIOB时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    USART_DeInit(USART3);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_Init(USART3, &USART_InitStructure);                       // 初始化串口

    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);                  // 使能接受中断
    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);                  // 使能空闲中断

    USART_Cmd(USART3, ENABLE);  
    
    USART3->SR = ~(0x00F0);                                         // 清理中断

    xUSART.USART3InitFlag = 1;                                      // 标记初始化标志
    xUSART.USART3ReceivedNum = 0;                                   // 接收字节数清零

    printf("\rUSART3初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： USART3_IRQHandler
 * 功  能： USART的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
static uint8_t U3TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U3TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U3TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

void USART3_IRQHandler(void)
{
    static uint16_t cnt = 0;                                         // 接收字节数累计：每一帧数据已接收到的字节数
    static uint8_t  RxTemp[U3_RX_BUF_SIZE];                          // 接收数据缓存数组：每新接收１个字节，先顺序存放到这里，当一帧接收完(发生空闲中断), 再转存到全局变量：xUSART.USARTxReceivedBuffer[xx]中；
   
    // 接收中断
    if (USART3->SR & (1 << 5))                                       // 检查RXNE(读数据寄存器非空标志位); RXNE中断清理方法：读DR时自动清理；
    {
        if ((cnt >= U3_RX_BUF_SIZE))//||xUSART.USART3ReceivedFlag==1 // 判断1: 当前帧已接收到的数据量，已满(缓存区), 为避免溢出，本包后面接收到的数据直接舍弃．
        {
            // 判断2: 如果之前接收好的数据包还没处理，就放弃新数据，即，新数据帧不能覆盖旧数据帧，直至旧数据帧被处理．缺点：数据传输过快于处理速度时会掉包；好处：机制清晰，易于调试
            USART3->DR;                                              // 读取数据寄存器的数据，但不保存．主要作用：读DR时自动清理接收中断标志；
            return;
        }
        RxTemp[cnt++] = USART3->DR ;                                 // 把新收到的字节数据，顺序存放到RXTemp数组中；注意：读取DR时自动清零中断位；
    }

    // 空闲中断, 用于配合接收中断，以判断一帧数据的接收完成
    if (USART3->SR & (1 << 4))                                       // 检查IDLE(空闲中断标志位); IDLE中断标志清理方法：序列清零，USART1 ->SR;  USART1 ->DR;
    {
        xUSART.USART3ReceivedNum  = 0;                               // 把接收到的数据字节数清0
        memcpy(xUSART.USART3ReceivedBuffer, RxTemp, U3_RX_BUF_SIZE); // 把本帧接收到的数据，存放到全局变量xUSART.USARTxReceivedBuffer中, 等待处理; 注意：复制的是整个数组，包括0值，以方便字符串数据
        xUSART.USART3ReceivedNum  = cnt;                             // 把接收到的字节数，存放到全局变量xUSART.USARTxReceivedCNT中；
        cnt = 0;                                                     // 接收字节数累计器，清零; 准备下一次的接收
        memset(RxTemp, 0, U3_RX_BUF_SIZE);                           // 接收数据缓存数组，清零; 准备下一次的接收
        USART3 ->SR;
        USART3 ->DR;                                                 // 清零IDLE中断标志位!! 序列清零，顺序不能错!!
    }

    // 发送中断
    if ((USART3->SR & 1 << 7) && (USART3->CR1 & 1 << 7))             // 检查TXE(发送数据寄存器空)、TXEIE(发送缓冲区空中断使能)
    {
        USART3->DR = U3TxBuffer[U3TxCounter++];                      // 读取数据寄存器值；注意：读取DR时自动清零中断位；
        if (U3TxCounter == U3TxCount)
            USART3->CR1 &= ~(1 << 7);                                // 已发送完成，关闭发送缓冲区空置中断 TXEIE
    }
}

/******************************************************************************
 * 函  数： vUSART3_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t USART3_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.USART3ReceivedNum > 0)                                          // 判断是否有新数据
    {
        memcpy(buffer, xUSART.USART3ReceivedBuffer, xUSART.USART3ReceivedNum); // 把新数据复制到指定位置
        *cnt = xUSART.USART3ReceivedNum;                                       // 把新数据的字节数，存放指定变量
        xUSART.USART3ReceivedNum = 0;                                          // 接收标记置0
        return *cnt;                                                           // 返回所接收到新数据的字节数
    }
    return 0;                                                                  // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUSART3_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void USART3_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U3TxBuffer[U3TxCount++] = buf[i];

    if ((USART3->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        USART3->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUSART3_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void USART3_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    USART3_SendData((u8 *)stringTemp, num);      // 注意调用函数所需要的真实数据长度; 如果目标需要以0作结尾判断，需num+1：字符串以0结尾，即多发一个:0
}




#ifdef STM32F10X_HD  // STM32F103R，及以上，才有UART4和UART5

//////////////////////////////////////////////////////////////   UART-4   //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************
 * 函  数： vUART4_Init
 * 功  能： 初始化USART的GPIO、通信参数配置、中断优先级
 *          (8位数据、无校验、1个停止位)
 * 参  数： uint32_t baudrate  通信波特率
 * 返回值： 无
 ******************************************************************************/
void UART4_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB1ENR |= RCC_APB1ENR_UART4EN;                            // 使能UART4时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;                             // 使能GPIOC时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    USART_DeInit(UART4);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_Init(UART4, &USART_InitStructure);                        // 初始化串口

    USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);                   // 使能接受中断
    USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);                   // 使能空闲中断
    
    USART_Cmd(UART4, ENABLE);                                       // 使能串口, 开始工作

    UART4->SR = ~(0x00F0);                                          // 清理中断

    xUSART.UART4InitFlag = 1;                                       // 标记初始化标志
    xUSART.UART4ReceivedNum = 0;                                    // 接收字节数清零

    printf("\rUART4 初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： UART4_IRQHandler
 * 功  能： USART2的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
static uint8_t U4TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U4TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U4TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

void UART4_IRQHandler(void)
{
    static uint16_t cnt = 0;                                        // 接收字节数累计：每一帧数据已接收到的字节数
    static uint8_t  RxTemp[U4_RX_BUF_SIZE];                         // 接收数据缓存数组：每新接收１个字节，先顺序存放到这里，当一帧接收完(发生空闲中断), 再转存到全局变量：xUSART.USARTxReceivedBuffer[xx]中；

    // 接收中断
    if (UART4->SR & (1 << 5))                                       // 检查RXNE(读数据寄存器非空标志位); RXNE中断清理方法：读DR时自动清理；
    {
        if ((cnt >= U4_RX_BUF_SIZE))//||xUSART.UART4ReceivedFlag==1 // 判断1: 当前帧已接收到的数据量，已满(缓存区), 为避免溢出，本包后面接收到的数据直接舍弃．
        {
            // 判断2: 如果之前接收好的数据包还没处理，就放弃新数据，即，新数据帧不能覆盖旧数据帧，直至旧数据帧被处理．缺点：数据传输过快于处理速度时会掉包；好处：机制清晰，易于调试
            UART4->DR;                                              // 读取数据寄存器的数据，但不保存．主要作用：读DR时自动清理接收中断标志；
            return;
        }
        RxTemp[cnt++] = UART4->DR ;                                 // 把新收到的字节数据，顺序存放到RXTemp数组中；注意：读取DR时自动清零中断位；
    }

    // 空闲中断, 用于配合接收中断，以判断一帧数据的接收完成
    if (UART4->SR & (1 << 4))                                       // 检查IDLE(空闲中断标志位); IDLE中断标志清理方法：序列清零，USART1 ->SR;  USART1 ->DR;
    {
        xUSART.UART4ReceivedNum  = 0;                               // 把接收到的数据字节数清0
        memcpy(xUSART.UART4ReceivedBuffer, RxTemp, U4_RX_BUF_SIZE); // 把本帧接收到的数据，存放到全局变量xUSART.USARTxReceivedBuffer中, 等待处理; 注意：复制的是整个数组，包括0值，以方便字符串数据
        xUSART.UART4ReceivedNum  = cnt;                             // 把接收到的字节数，存放到全局变量xUSART.USARTxReceivedCNT中；
        cnt = 0;                                                    // 接收字节数累计器，清零; 准备下一次的接收
        memset(RxTemp, 0, U4_RX_BUF_SIZE);                          // 接收数据缓存数组，清零; 准备下一次的接收
        UART4 ->SR;
        UART4 ->DR;                                  // 清零IDLE中断标志位!! 序列清零，顺序不能错!!
    }

    // 发送中断
    if ((UART4->SR & 1 << 7) && (UART4->CR1 & 1 << 7))              // 检查TXE(发送数据寄存器空)、TXEIE(发送缓冲区空中断使能)
    {
        UART4->DR = U4TxBuffer[U4TxCounter++];                      // 读取数据寄存器值；注意：读取DR时自动清零中断位；
        if (U4TxCounter == U4TxCount)
            UART4->CR1 &= ~(1 << 7);                                // 已发送完成，关闭发送缓冲区空置中断 TXEIE
    }
}

/******************************************************************************
 * 函  数： vUART4_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t UART4_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.UART4ReceivedNum > 0)                                         // 判断是否有新数据
    {
        memcpy(buffer, xUSART.UART4ReceivedBuffer, xUSART.UART4ReceivedNum); // 把新数据复制到指定位置
        *cnt = xUSART.UART4ReceivedNum;                                      // 把新数据的字节数，存放指定变量
        xUSART.UART4ReceivedNum = 0;                                         // 接收标记置0
        return *cnt;                                                         // 返回所接收到新数据的字节数
    }
    return 0;                                                                // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUART4_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void UART4_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U4TxBuffer[U4TxCount++] = buf[i];

    if ((UART4->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        UART4->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUART4_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void UART4_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    UART4_SendData((u8 *)stringTemp, num);       // 调用函数完成发送，num+1：字符串以0结尾，需多发一个:0
}




//////////////////////////////////////////////////////////////   UART-4   //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/******************************************************************************
 * 函  数： vUART5_Init
 * 功  能： 初始化USART的GPIO、通信参数配置、中断优先级
 *          (8位数据、无校验、1个停止位)
 * 参  数： uint32_t baudrate  通信波特率
 * 返回值： 无
 ******************************************************************************/
void UART5_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    // 时钟使能
    RCC->APB1ENR |= RCC_APB1ENR_UART5EN;                            // 使能UART5时钟
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPCEN;        // 使能GPIO时钟

    // GPIO_TX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;                // TX引脚工作模式：复用推挽
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    // GPIO_RX引脚配置
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;                  // RX引脚工作模式：上拉输入; 如果使用浮空输入，引脚空置时可能产生误输入; 当电路上为一主多从电路时，可以使用复用开漏模式
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 中断配置
    NVIC_InitStructure .NVIC_IRQChannel = UART5_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 2 ;     // 抢占优先级
    NVIC_InitStructure .NVIC_IRQChannelSubPriority = 2;             // 子优先级
    NVIC_InitStructure .NVIC_IRQChannelCmd = ENABLE;                // IRQ通道使能
    NVIC_Init(&NVIC_InitStructure);

    //USART 初始化设置
    USART_DeInit(UART5);
    USART_InitStructure.USART_BaudRate   = baudrate;                // 串口波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长为8位数据格式
    USART_InitStructure.USART_StopBits   = USART_StopBits_1;        // 一个停止位
    USART_InitStructure.USART_Parity     = USART_Parity_No;         // 无奇偶校验位
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; // 使能收、发模式
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(UART5, &USART_InitStructure);                        // 初始化串口

    USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
    USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);                   // 使能接受中断
    USART_ITConfig(UART5, USART_IT_IDLE, ENABLE);                   // 使能空闲中断

    USART_Cmd(UART5, ENABLE);                                       // 使能串口, 开始工作

    UART5->SR = ~(0x00F0);                                          // 清理中断

    xUSART.UART5InitFlag = 1;                                       // 标记初始化标志
    xUSART.UART5ReceivedNum = 0;                                    // 接收字节数清零

    printf("\rUART5 初始化配置      接收中断、空闲中断, 发送中断\r");
}

/******************************************************************************
 * 函  数： UART5_IRQHandler
 * 功  能： USART2的接收中断、空闲中断、发送中断
 * 参  数： 无
 * 返回值： 无
 ******************************************************************************/
static uint8_t U5TxBuffer[256] ;    // 用于中断发送：环形缓冲区，256个字节
static uint8_t U5TxCounter = 0 ;    // 用于中断发送：标记已发送的字节数(环形)
static uint8_t U5TxCount   = 0 ;    // 用于中断发送：标记将要发送的字节数(环形)

void UART5_IRQHandler(void)
{
    static uint16_t cnt = 0;                                        // 接收字节数累计：每一帧数据已接收到的字节数
    static uint8_t  RxTemp[U5_RX_BUF_SIZE];                         // 接收数据缓存数组：每新接收１个字节，先顺序存放到这里，当一帧接收完(发生空闲中断), 再转存到全局变量：xUSART.USARTxReceivedBuffer[xx]中；

    // 接收中断
    if (UART5->SR & (1 << 5))                                       // 检查RXNE(读数据寄存器非空标志位); RXNE中断清理方法：读DR时自动清理；
    {
        if ((cnt >= U5_RX_BUF_SIZE))//||xUSART.UART5ReceivedFlag==1 // 判断1: 当前帧已接收到的数据量，已满(缓存区), 为避免溢出，本包后面接收到的数据直接舍弃．
        {
            // 判断2: 如果之前接收好的数据包还没处理，就放弃新数据，即，新数据帧不能覆盖旧数据帧，直至旧数据帧被处理．缺点：数据传输过快于处理速度时会掉包；好处：机制清晰，易于调试
            UART5->DR;                                              // 读取数据寄存器的数据，但不保存．主要作用：读DR时自动清理接收中断标志；
            return;
        }
        RxTemp[cnt++] = UART5->DR ;                                 // 把新收到的字节数据，顺序存放到RXTemp数组中；注意：读取DR时自动清零中断位；
    }

    // 空闲中断, 用于配合接收中断，以判断一帧数据的接收完成
    if (UART5->SR & (1 << 4))                                       // 检查IDLE(空闲中断标志位); IDLE中断标志清理方法：序列清零，USART1 ->SR;  USART1 ->DR;
    {
        xUSART.UART5ReceivedNum  = 0;                               // 把接收到的数据字节数清0
        memcpy(xUSART.UART5ReceivedBuffer, RxTemp, U5_RX_BUF_SIZE); // 把本帧接收到的数据，存放到全局变量xUSART.USARTxReceivedBuffer中, 等待处理; 注意：复制的是整个数组，包括0值，以方便字符串数据
        xUSART.UART5ReceivedNum  = cnt;                             // 把接收到的字节数，存放到全局变量xUSART.USARTxReceivedCNT中；
        cnt = 0;                                                    // 接收字节数累计器，清零; 准备下一次的接收
        memset(RxTemp, 0, U5_RX_BUF_SIZE);                          // 接收数据缓存数组，清零; 准备下一次的接收
        UART5 ->SR;
        UART5 ->DR;                                  // 清零IDLE中断标志位!! 序列清零，顺序不能错!!
    }

    // 发送中断
    if ((UART5->SR & 1 << 7) && (UART5->CR1 & 1 << 7))              // 检查TXE(发送数据寄存器空)、TXEIE(发送缓冲区空中断使能)
    {
        UART5->DR = U5TxBuffer[U5TxCounter++];                      // 读取数据寄存器值；注意：读取DR时自动清零中断位；
        if (U5TxCounter == U5TxCount)
            UART5->CR1 &= ~(1 << 7);                                // 已发送完成，关闭发送缓冲区空置中断 TXEIE
    }
}

/******************************************************************************
 * 函  数： vUART5_GetBuffer
 * 功  能： 获取UART所接收到的数据
 * 参  数： uint8_t* buffer   数据存放缓存地址
 *          uint8_t* cnt      接收到的字节数
 * 返回值： 0_没有接收到新数据， 非0_所接收到新数据的字节数
 ******************************************************************************/
uint8_t UART5_GetBuffer(uint8_t *buffer, uint8_t *cnt)
{
    if (xUSART.UART5ReceivedNum > 0)                                         // 判断是否有新数据
    {
        memcpy(buffer, xUSART.UART5ReceivedBuffer, xUSART.UART5ReceivedNum); // 把新数据复制到指定位置
        *cnt = xUSART.UART5ReceivedNum;                                      // 把新数据的字节数，存放指定变量
        xUSART.UART5ReceivedNum = 0;                                         // 接收标记置0
        return *cnt;                                                         // 返回所接收到新数据的字节数
    }
    return 0;                                                                // 返回0, 表示没有接收到新数据
}

/******************************************************************************
 * 函  数： vUART5_SendData
 * 功  能： UART通过中断发送数据,适合各种数据类型
 *         【适合场景】本函数可发送各种数据，而不限于字符串，如int,char
 *         【不 适 合】注意环形缓冲区容量256字节，如果发送频率太高，注意波特率
 * 参  数： uint8_t* buffer   需发送数据的首地址
 *          uint8_t  cnt      发送的字节数 ，限于中断发送的缓存区大小，不能大于256个字节
 * 返回值：
 ******************************************************************************/
void UART5_SendData(uint8_t *buf, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++)
        U5TxBuffer[U5TxCount++] = buf[i];

    if ((UART5->CR1 & 1 << 7) == 0)       // 检查发送缓冲区空置中断(TXEIE)是否已打开
        UART5->CR1 |= 1 << 7;
}

/******************************************************************************
 * 函  数： vUART5_SendString
 * 功  能： UART通过中断发送输出字符串,无需输入数据长度
 *         【适合场景】字符串，长度<=256字节
 *         【不 适 合】int,float等数据类型
 * 参  数： char* stringTemp   需发送数据的缓存首地址
 * 返回值： 元
 ******************************************************************************/
void UART5_SendString(char *stringTemp)
{
    u16 num = 0;                                 // 字符串长度
    char *t = stringTemp ;                       // 用于配合计算发送的数量
    while (*t++ != 0)  num++;                    // 计算要发送的数目，这步比较耗时，测试发现每多6个字节，增加1us，单位：8位
    UART5_SendData((u8 *)stringTemp, num);       // 注意调用函数所需要的真实数据长度; 如果目标需要以0作结尾判断，需num+1：字符串以0结尾，即多发一个:0
}

#endif





//////////////////////////////////////////////////////////////  printf   //////////////////////////////////////////////////////////////
/******************************************************************************
 * 功  能： printf函数支持代码
 *         【特别注意】加入以下代码, 使用printf函数时, 不再需要选择use MicroLIB
 * 参  数：
 * 返回值：
 * 备  注： 最后修改_2020年07月15日
 ******************************************************************************/
//加入以下代码,支持printf函数,而不需要选择use MicroLIB
#pragma import(__use_no_semihosting)
struct __FILE
{
    int handle;
};                     // 标准库需要的支持函数
FILE __stdout;         // FILE 在stdio.h文件
void _sys_exit(int x)
{
    x = x;             // 定义_sys_exit()以避免使用半主机模式
}



int fputc(int ch, FILE *f)                   // 重定向fputc函数，使printf的输出，由fputc输出到UART,  这里使用串口2(USART2)
{
#if 0                                        // 方式1-使用常用的poll方式发送数据，比较容易理解，但等待耗时大
    while ((USART2->SR & 0X40) == 0);        // 等待上一次串口数据发送完成
    USART2->DR = (u8) ch;                    // 写DR,串口2将发送数据
    return ch;
#else                                        // 方式2-使用queue+中断方式发送数据; 无需像方式1那样等待耗时，但要借助已写好的函数、环形缓冲
    uint8_t c[1] = {(uint8_t)ch};
    if (USARTx_DEBUG == USART1)    USART1_SendData(c, 1);
    if (USARTx_DEBUG == USART2)    USART2_SendData(c, 1);
    if (USARTx_DEBUG == USART3)    USART3_SendData(c, 1);
    if (USARTx_DEBUG == UART4)     UART4_SendData(c, 1);
    if (USARTx_DEBUG == UART5)     UART5_SendData(c, 1);
    return ch;
#endif
}


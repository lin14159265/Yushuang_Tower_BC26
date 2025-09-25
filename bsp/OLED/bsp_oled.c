#include "bsp_oled.h"
#include "oledfont.h"
#include "bmp.h"



/*****************************************************************************
 ** 宏定义
 *****************************************************************************/
#define SCL_0   (OLED_SCL_GPIO->BRR   = OLED_SCL_PIN)  // SCL
#define SCL_1   (OLED_SCL_GPIO->BSRR  = OLED_SCL_PIN)

#define SDA_0   (OLED_SDA_GPIO ->BRR  = OLED_SDA_PIN)  // SDA
#define SDA_1   (OLED_SDA_GPIO ->BSRR = OLED_SDA_PIN)



static char strTemp[30];



/*****************************************************************************
 ** 内部函数
****************************************************************************/
// 简单的时序内部延时
static void delay(void)
{
    uint8_t i;
    for (i = 0; i < 3; i++);    // 循环次数为10时，SCL频率 = 205KHz
}

// 延时ms
static void delay_ms(uint32_t ms)           // 定义一个ms延时函数，减少移植时对外部文件依赖; 本函数仅作粗略延时使用，而并非精准延时;
{
    ms = ms * 11993;                        // 注意：此参考值运行条件：打勾 Options/ c++ / One ELF Section per Function
    for (uint32_t i = 0; i < ms; i++);      // 72MHz系统时钟下，大约多少个空循环耗时1ms
}

// 开始信号
static void start(void)
{
    SCL_0 ;   //
    SDA_1;
    delay();

    SCL_1;     // SCL高电平期间，SDA向下跳变
    SDA_0;

    delay();
    SCL_0;
    delay();
}

// 停止信号
static void stop(void)
{
    SCL_0 ;
    SDA_0 ;
    delay();

    SCL_1 ;   // SCL高电平期间，SDA向上跳变
    SDA_1;
    delay();
}

// 应答信号 
static void waitAck(void)
{
    SCL_1 ;
    delay();
    SCL_0;
    delay();
}

// 总线产生1字节数据时序
static void sendByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        SCL_0 ;
        delay();
        if (data & 0x80)
            SDA_1 ;
        else
            SDA_0 ;
        data <<= 1;
        delay();
        SCL_1 ;
        delay();   // 重要，这里要加一个延时，不然容易通信误
        SCL_0 ;
    }
}

// 写命令
static void writeCmd(uint8_t IIC_Command)
{
    start();
    sendByte(0x78);            // 设备地址
    waitAck();
    sendByte(0x00);            // 命令
    waitAck();
    sendByte(IIC_Command);
    waitAck();
    stop();
}

// 写数据
static void writeData(uint8_t IIC_Data)
{
    start();
    sendByte(0x78);           //D/C#=0; R/W#=0
    waitAck();
    sendByte(0x40);           //write data
    waitAck();
    sendByte(IIC_Data);
    waitAck();
    stop();
}

// 设置坐标，x:横向像素点，y:纵向第几页(行)
static void OLED_SetPos(uint8_t x, uint8_t y)
{
    writeCmd(0xb0 + y);
    writeCmd(((x & 0xf0) >> 4) | 0x10);
    writeCmd((x & 0x0f));
}



/*****************************************************************************
* 函  数： OLED_Init
* 功  能： 初始化OLED
* 参  数: 
* 返回值： 无
* 备  注： 
*****************************************************************************/
void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;   // 定义一个GPIO_InitTypeDef类型的结构体

    // 使能LED_RED所用引脚端口时钟;使用端口判断的方法使能时钟, 以使代码移植更方便
    if (OLED_SCL_GPIO == GPIOA)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    if (OLED_SCL_GPIO == GPIOB)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    if (OLED_SCL_GPIO == GPIOC)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    if (OLED_SCL_GPIO == GPIOD)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    if (OLED_SCL_GPIO == GPIOE)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    if (OLED_SCL_GPIO == GPIOF)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
    if (OLED_SCL_GPIO == GPIOG)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
    // 使能LED_RED所用引脚端口时钟;使用端口判断的方法使能时钟, 以使代码移植更方便
    if (OLED_SDA_GPIO == GPIOA)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    if (OLED_SDA_GPIO == GPIOB)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    if (OLED_SDA_GPIO == GPIOC)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    if (OLED_SDA_GPIO == GPIOD)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    if (OLED_SDA_GPIO == GPIOE)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    if (OLED_SDA_GPIO == GPIOF)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
    if (OLED_SDA_GPIO == GPIOG)  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

    // 配置LED_RED引脚工作模式
    GPIO_InitStructure.GPIO_Pin   = OLED_SCL_PIN;           // 选择要控制的GPIO引脚
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;       // 设置引脚模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       // 设置引脚速率为50MHz
    GPIO_Init(OLED_SCL_GPIO, &GPIO_InitStructure);          // 调用库函数，初始化GPIO

    // 配置LED_RED引脚工作模式
    GPIO_InitStructure.GPIO_Pin   = OLED_SDA_PIN;           // 选择要控制的GPIO引脚
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;       // 设置引脚模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       // 设置引脚速率为50MHz
    GPIO_Init(OLED_SDA_GPIO, &GPIO_InitStructure);          // 调用库函数，初始化GPIO

    stop();

    delay_ms(200);  // 重要，不要删

    writeCmd(0xAE); // OLED休眠
    writeCmd(0x00); // 设置低列地址
    writeCmd(0x10); // 设置高列地址
    writeCmd(0x40); // 设置起始地址线
    writeCmd(0xB0); // set page address
    writeCmd(0x81); // 设置对比度
    writeCmd(0xFF); // 128
    writeCmd(0xA1); // 0xa0左右反置 0xa1正常
    writeCmd(0xA6); // normal / reverse
    writeCmd(0xA8); // 设置多路复用(1 to 64)
    writeCmd(0x3F); // 1/32 duty
    writeCmd(0xC8); // Com scan direction
    writeCmd(0xD3); // 设置显示的偏移映射内存计数器
    writeCmd(0x00); //

    writeCmd(0xD5); // 设置显示时钟分频比/振荡器频率
    writeCmd(0x80); // 设置分频比例，时钟设置为100帧/秒

    writeCmd(0xD8); // set area color mode off
    writeCmd(0x05); //

    writeCmd(0xD9); // 预充电时间
    writeCmd(0xF1); // 预充电为15个脉冲，释放为1个脉冲

    writeCmd(0xDA); // 引脚设置硬件配置
    writeCmd(0x12); //

    writeCmd(0xDB); // 设置VCOM电平
    writeCmd(0x30); //

    OLED_On();      // 唤醒
}



/*****************************************************************************
* 函  数： OLED_Fill
* 功  能： 清屏函数, 即填充整个屏幕为0
* 参  数: 
* 返回值： 无
* 备  注： 
*****************************************************************************/
void OLED_Fill(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        writeCmd(0xb0 + i);  //设置页地址（0~7）
        writeCmd(0x00);      //设置显示位置—列低地址
        writeCmd(0x10);      //设置显示位置—列高地址
        for (uint8_t n = 0; n < 128; n++)
            writeData(0);   // 要填充的数据
    }
}



/*****************************************************************************
* 函  数： OLED_On
* 功  能： 从休眠中唤醒
* 参  数: 
* 返回值： 无
* 备  注： 
*****************************************************************************/
void OLED_On(void)
{
    writeCmd(0X8D); // 设置电荷泵
    writeCmd(0X14); // 开启电荷泵
    writeCmd(0XAF); // OLED唤醒
}



/*****************************************************************************
* 函  数： OLED_Off
* 功  能： 休眠模式下,OLED功耗不到10uA
* 参  数: 
* 返回值： 无
* 备  注： 
*****************************************************************************/
void OLED_Off(void)
{
    writeCmd(0X8D); // 设置电荷泵
    writeCmd(0X10); // 关闭电荷泵
    writeCmd(0XAE); // OLED休眠
}



/*****************************************************************************
* 函  数： OLED_ShowString
* 功  能： 显示oledfont.h中的ASCII字符
* 参  数:  x, y : 起始点坐标(x0:0~127, y0:0~7)
*          chr* : 要显示的字符串;
*          size : 字符大小(可选：8, 16)
* 返回值： 无
* 备  注： 
*****************************************************************************/
void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t size)
{
    uint8_t i = 0;
    uint8_t c = 0;

    if (size == 16)
    {
        while (chr[i] != '\0')
        {
            c = chr[i++] - ' ';
            OLED_SetPos(x, y);
            for (uint8_t j = 0; j < 8; j++)
                writeData(F8X16[c * 16 + j]);
            OLED_SetPos(x, y + 1);
            for (uint8_t j = 0; j < 8; j++)
                writeData(F8X16[c * 16 + j + 8]);

            x += 8;
            if (x > 120)
            {
                x = 0;
                y += 2;
            }
        }
    }
    else
    {
        while (chr[i] != '\0')
        {
            c = chr[i++] - ' ';
            OLED_SetPos(x, y);
            for (uint8_t j = 0; j < 6; j++)
                writeData(F6x8[c][j]);

            x += 8;
            if (x > 120)
            {
                x = 0;
                y += 1;
            }
        }
    }
}

/*****************************************************************************
* 函  数： OLED_ShowCHinese
* 功  能： 显示 oledfont.h 中的汉字,16*16点阵
* 参  数:  x,y: 起始点坐标(x:0~127, y:0~7)
*          N: 汉字在 oledfont.h 数组中的索引
* 返回值： 无
* 备  注： 字模软件：PCtoLCD2002, 参数：阴码、列行式、16*16、逆向(具体有图解)
*          调用展示：
*          OLED_ShowCHinese( 0,0,0);  // 温
*          OLED_ShowCHinese(18,0,2);  // 度
*****************************************************************************/
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t N)
{
    uint8_t wm = 0;
    unsigned int  adder = 32 * N;

    OLED_SetPos(x, y);
    for (wm = 0; wm < 16; wm++)
    {
        writeData(CH16[adder]);
        adder += 1;
    }

    OLED_SetPos(x, y + 1);
    for (wm = 0; wm < 16; wm++)
    {
        writeData(CH16[adder]);
        adder += 1;
    }
}

static  

/*****************************************************************************
* 函  数： OLED_DrawBMP
* 功  能： 显示BMP位图
* 参  数:  x0,y0 :起始点坐标(x0:0~127, y0:0~7)
*          x1,y1 : 起点对角线(结束点)的坐标(x1:1~128,y1:1~8)
* 返回值： 无
* 备  注： 图片生成的字表较大，会占用较多空间，注意FLASH空间是否能容纳图片大小
*          调用展示： 
*          OLED_DrawBMP(0, 0, 128, 8, BMP1);
*****************************************************************************/
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[])
{
    uint32_t j = 0;
    uint8_t x, y;

    if (y1 % 8 == 0) y = y1 / 8;
    else y = y1 / 8 + 1;

    for (y = y0; y < y1; y++)
    {
        OLED_SetPos(x0, y);
        for (x = x0; x < x1; x++)
        {
            writeData(BMP[j++]);
        }
    }
}



void OLED_GUI(void)
{
    // 显示标题
    OLED_ShowCHinese(20, 0, 0);                   // 温, 在字模数组中的第0个字模
    OLED_ShowCHinese(38, 0, 1);                   // 湿, 在字模数组中的第1个字模
    OLED_ShowCHinese(56, 0, 2);                   // 度, 在字模数组中的第2个字模
    OLED_ShowCHinese(74, 0, 3);                   // 采, 在字模数组中的第3个字模
    OLED_ShowCHinese(92, 0, 4);                   // 集, 在字模数组中的第4个字模
    // 温度采集显示
    OLED_ShowCHinese(2,  3, 0);                   // 温, 在字模数组中的第0个字模
    OLED_ShowCHinese(20, 3, 2);                   // 度, 在字模数组中的第2字模
    OLED_ShowString(38, 3, ":", 16);              // :, 注意，这个是英文字符
    float temperature = 0.0;                      //
    sprintf(strTemp, "%2.1f", temperature);       // 重要：很多人咨询, 如何把各种数据类型，转换为字符串，再显示到显示屏. 请百度 sprintf() 的具体用法，
    OLED_ShowString(54, 3, strTemp, 16);          // 显示数值到显示屏上
    OLED_ShowCHinese(92, 3, 5);                   // ℃
    // 湿度采集显示
    OLED_ShowCHinese(2,  6, 1);                   // 湿, 在字模数组中的第n个字模
    OLED_ShowCHinese(20, 6, 2);                   // 度
    OLED_ShowString(38, 6, ":", 16);              // :, 注意，这个是英文字符
    float humidity = 0.0;                         //
    sprintf(strTemp, "%2.1f", humidity);          // 重要：很多人咨询, 如何把各种数据类型，转换为字符串，再显示到显示屏. 请百度 sprintf() 的具体用法，
    OLED_ShowString(54, 6, strTemp, 16);          // 假设值，只作演示如何显示数值到显示屏上
    OLED_ShowString(92, 6, "%", 16);              // %
}





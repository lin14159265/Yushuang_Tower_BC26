#include "hardware.h"



/*****************************************************************************
*函  HardWare_NVICManage
*功  能：外部中断优先级管理安排， 都放这里， 以清晰优先级关系 
*参  数：
*返回值：
*****************************************************************************/
void HardWare_NVICManage(void)
{
    
}



/*****************************************************************************
*函  Hardware_Init
*功  能：初始化硬件 
*参  数：
*返回值：
*****************************************************************************/
u8 Hardware_Init(void)
{    
    USART1_Init(115200);                              // 串口初始化：USART1(115200-N-8-1), 且工程已把printf重定向至USART1输出
    System_SwdMode ();                                // 设置芯片调试方式(SWD); 关闭JTAG只保留SWD; 目的:释放PB3、PB4、PA15，只需PA13、PA14
    System_SysTickInit();                             // 初始化systick时钟，1ms中断1次，作用：使System_DelayMS()、System_DelayUS()正常工作
    printf("芯片内部FLASH         ");                 // 读取本芯片FLASH大小，
    printf("%d Kbytes\n",*(uint16_t*)(0x1FFFF7E0));   // 本地址段，存放着芯片出厂时编写的FLASH大小;　本行另一个作用：示例使用指针读取数据
      
    Led_Init();                                       // LED 初始化
    LED_BLUE_ON ;
    LED_RED_ON;
            
    Key_Init();                                       // 按键 初始化

    W25qx_Init();                                     // 设备W25Q128： 引脚初始化，SPI初始化，测试连接, 测试是否存在字库
    
    LCD_Init();                                       // 显示屏初始化                 
    XPT2046_Init(xLCD.width, xLCD.height, xLCD.dir ); // 触摸屏初始化, 宽度像素, 高度像素, 方向:0_竖屏, 1_横屏   
    //XPT2046_ReCalibration();                        // 重新校准触摸坐标
    XPT2046_Cmd(ENABLE);                              // 打开触摸检测   
    LCD_GUI();                                        // 绘制展示界面
         
    CAN1_Config();
    
    // 串口、ESP8266
    UART4_Init(115200); 
    

    
    return 0;
}


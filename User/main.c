/**==================================================================================================================
 **【文件名称】  main.c
 **【功能测试】  STM32F103驱动物联网模块 (纯USART2调试版)
 **==================================================================================================================
 **【实验平台】  STM32F103RC + KEIL5.27 + BC26物联网模块
 **
 **【实验操作】  1-接线
 **                BC26_TXD --- PA10 (USART1_RX)
 **                BC26_RXD --- PA9  (USART1_TX)
 **                USB-TTL_TX --- PA3 (USART2_RX)
 **                USB-TTL_RX --- PA2 (USART2_TX)
 **              2-烧录代码到开发板
 **              3-打开串口助手(连接TTL模块对应的COM口)，波特率115200，即可观察AT指令交互日志
 **
 **【移植说明】  本代码已将用户AT指令交互逻辑，移植到"魔女开发板"的工程模板中。
 **              所有OLED代码已被移除，所有调试信息通过printf重定向到USART2输出。
 **
====================================================================================================================*/

#include <stm32f10x.h>
#include "stm32f10x_conf.h"
#include "system_f103.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"  // 🔧 【新增】支持rand()函数
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_usart.h"



/*
 ===============================================================================
                            AT指令宏定义
 ===============================================================================
 * - 字符串已根据C语言语法要求，对内部的双引号 " 进行了转义 \"
 * - 每条指令末尾都包含了您要求的 \r\n
*/

#define CMD_AT                      "AT\r\n"
#define CMD_GET_CIMI                "AT+CIMI\r\n"
#define CMD_ENABLE_GATT             "AT+CGATT=1\r\n"
#define CMD_QUERY_GATT              "AT+CGATT?\r\n"
#define CMD_SET_MQTT_VERSION        "AT+QMTCFG=\"version\",0,4\r\n"

/*
 * 注意: 您提供的指令中使用 mqtts 主机名但端口为 1883。
 *      通常 MQTTS (SSL/TLS加密) 连接使用的标准端口是 8883。
 *      此处完全遵照您的原始指令，未做任何修改。
 */
#define CMD_OPEN_MQTT_NETWORK       "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n"

/*
 * 连接到MQTT服务器的指令
 * 内部包含的多个双引号均已正确转义
 */
#define CMD_CONNECT_MQTT_BROKER     "AT+QMTCONN=0,\"test\",\"d4J8Spo9uo\",\"version=2018-10-31&res=products%2Fd4J8Spo9uo%2Fdevices%2Ftest&et=1790584042&method=md5&sign=EaWtOdD9uj7fXkgmkswN3A%3D%3D\"\r\n"



/* ================== 用户代码: 全局函数和延时 START ================== */

static void delay_ms(uint32_t ms)
{
    ms = ms * 11993;
    for (uint32_t i = 0; i < ms; i++);
}
/********************************************************************************
 * @brief  示例函数：按顺序发送AT指令以建立MQTT连接
 * @param  None
 * @retval None
 * @note   在实际应用中，每发送一条指令后，都应等待模块的响应，
 *         并根据响应结果决定是否继续执行下一条。
 *         这里的延时仅为简单示例，实际延时时间需根据模块手册和网络状况调整。
 ********************************************************************************/
void Initialize_And_Connect_MQTT(void)
{
    // 发送 AT, 测试模块是否就绪
    USART1_SendString(CMD_AT);
    delay_ms(500); // 等待 "OK"

    // 获取SIM卡信息
    USART1_SendString(CMD_GET_CIMI);
    delay_ms(1000); // 等待 CIMI 号码

    // 附着GPRS网络
    USART1_SendString(CMD_ENABLE_GATT);
    delay_ms(1000);

    // 查询网络附着状态
    USART1_SendString(CMD_QUERY_GATT);
    delay_ms(2000); // 等待 "+CGATT: 1"

    // 配置MQTT协议版本为 3.1.1 (对应参数 4)
    USART1_SendString(CMD_SET_MQTT_VERSION);
    delay_ms(500);

    // 打开一个MQTT网络连接
    USART1_SendString(CMD_OPEN_MQTT_NETWORK);
    delay_ms(4000); // 等待 "+QMTOPEN: 0,0"

    // 连接到MQTT Broker
    USART1_SendString(CMD_CONNECT_MQTT_BROKER);
    delay_ms(5000); // 等待 "+QMTCONN: 0,0,0"
}


// 主函数
int main(void)
{
    char cmd_buffer[512];
    char json_buffer[256];
    long message_id = 100;

    // 1. 系统核心初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    System_SwdMode();

    // 2. 外设初始化
    USART1_Init(115200); // 初始化USART1，用于和BC26模块通信
    USART2_Init(115200); // 初始化USART2，用于调试信息输出
    Led_Init();

    Initialize_And_Connect_MQTT();

    

    while (1)
    {
        

     }
}


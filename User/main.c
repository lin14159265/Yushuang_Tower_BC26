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
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_usart.h"

/* ================== 用户代码: 物联网平台信息 START ================== */

// --- 1. OneNET 连接信息 ---
#define DEVICE_NAME  "test"
#define PRODUCT_ID   "nZ4v9G1iDK"
#define AUTH_INFO    "version=2018-10-31&res=products%%2FnZ4v9G1iDK%%2Fdevices%%2Ftest&et=1798497693&method=md5&sign=ZmzDSu0enWpLqIS8rHDjXw%%D%%D"

// --- OneNET 主题信息 ---
#define PUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/dp/post/json"
#define SUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/cmd/request/+" 

/* ================== 用户代码: 物联网平台信息 END ==================== */


/* ================== 用户代码: 全局函数和延时 START ================== */

static void delay_ms(uint32_t ms)
{
    ms = ms * 11993;
    for (uint32_t i = 0; i < ms; i++);
}

/**
 * @brief 发送AT指令并等待预期响应（使用 bsp_usart 驱动）
 * @param cmd 要发送的指令
 * @param expected_rsp 期待的响应字符串，例如 "OK"
 * @param timeout_ms 超时时间
 * @return 0: 成功, 1: 失败/超时
 */
int send_cmd(const char* cmd, const char* expected_rsp, uint32_t timeout_ms)
{   
    xUSART.USART1ReceivedNum = 0; 
    USART1_SendString((char*)cmd);
    
    // 通过USART2打印出发送的指令
    printf(">> Send to Module: %s", cmd);

    uint32_t time_start = 0;
    while(time_start < timeout_ms)
    {
        if (xUSART.USART1ReceivedNum > 0)
		{
			// 注意：在中断里我们已经加了'\0'，这里可以不加，但为了保险起见加上也无妨。
			xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
			
			// 通过USART2打印出接收到的响应，便于调试
			printf("<< Recv from Module: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
			
			// 检查是否包含我们期望的响应
			if (strstr((const char*)xUSART.USART1ReceivedBuffer, expected_rsp) != NULL)
			{
				// 找到了！清除标志并返回成功
				memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE); // 清空缓冲区是个好习惯
				xUSART.USART1ReceivedNum = 0;
				return 0; // 成功
			}
			
			// 检查是否包含ERROR
			if (strstr((const char*)xUSART.USART1ReceivedBuffer, "ERROR") != NULL)
			{
				// 收到错误！清除标志并返回失败
				memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
				xUSART.USART1ReceivedNum = 0;
				return 1; // 错误
			}
			
			// 如果执行到这里，说明我们收到了数据，但不是我们想要的最终结果 (OK 或 ERROR)。
			// 这可能是模块的回显或其他提示信息。我们不应该立刻返回失败。
			// 我们要做的只是把缓冲区清零，然后让 while 循环继续，等待下一条信息的到来。
			xUSART.USART1ReceivedNum = 0;
		}
        delay_ms(1);
        time_start++;
    }
    
    printf("!! Timeout for cmd: %s\r\n", cmd);
    return 1; // 超时
}

/**
 * @brief 解析服务器下发的命令
 * @param buffer 包含+QMTRECV的串口接收缓冲区内容
 */
void parse_command(const char* buffer)
{
    char* payload_start = strstr(buffer, ",\"");
    if (!payload_start) return;
    payload_start = strstr(payload_start + 2, ",\"");
    if (!payload_start) return;
    payload_start = strstr(payload_start + 2, ",\"");
    if (!payload_start) return;
    payload_start += 2;

    char* payload_end = strrchr(buffer, '\"');

    if (payload_start && payload_end && payload_end > payload_start)
    {
        char command[128] = {0};
        int len = payload_end - payload_start -1;
        if(len > 0 && len < 128)
        {
             strncpy(command, payload_start+1, len);
             printf("## Received command from Cloud: %s\r\n", command);

             if (strcmp(command, "LED_ON") == 0)
             {
                 
                 printf("## Action: Turn LED ON\r\n");
             }
             else if (strcmp(command, "LED_OFF") == 0)
             {
                 
                 printf("## Action: Turn LED OFF\r\n");
             }
        }
    }
}

/* ================== 用户代码: 全局函数和延时 END ==================== */


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

    // 3. 开机状态指示

    printf("\r\n\r\n==================================\r\n");
    printf("IoT Module Program Start...\r\n");
    printf("==================================\r\n");

    // 4. 【核心】物联网模块初始化流程
    printf("\r\n--- Initializing Module ---\r\n");
    while(send_cmd("AT\r\n", "OK", 1000))
    {
        printf("AT command failed, retrying...\r\n");
        delay_ms(1000);
    }
    
    send_cmd("ATE0\r\n", "OK", 1000);

    printf("\r\n--- Attaching to Network ---\r\n");
    while(send_cmd("AT+CGATT?\r\n", "+CGATT: 1", 2000))
    {
        printf("Waiting for network attachment...\r\n");
        delay_ms(2000);
    }
    
    printf("\r\n--- Connecting to MQTT Broker ---\r\n");
    send_cmd("AT+QMTCFG=\"version\",0,4\r\n", "OK", 3000);
    send_cmd("AT+QMTOPEN=0,\"mqtt.heclouds.com\",1883\r\n", "+QMTOPEN: 0,0", 8000);
    
    sprintf(cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", DEVICE_NAME, PRODUCT_ID, AUTH_INFO);
    if(send_cmd(cmd_buffer, "+QMTCONN: 0,0,0", 8000) != 0)
    {
        printf("\r\n!! MQTT Connect Failed! Program Halted. !!\r\n");
        while(1); // 连接失败，卡死在这里
    }

    sprintf(cmd_buffer, "AT+QMTSUB=0,1,\"%s\",1\r\n", SUB_TOPIC);
    send_cmd(cmd_buffer, "+QMTSUB: 0,1,0", 5000);

    printf("\r\n==================================\r\n");
    printf("Initialization Complete. Running...\r\n");
    printf("==================================\r\n\r\n");

    // 5. 主循环
    while (1)
    {
        // --- 检查是否有服务器下发的命令 ---
        if(xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            printf("<< Recv from Module (URC): %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            if(strstr((const char*)xUSART.USART1ReceivedBuffer, "+QMTRECV:"))
            {
                parse_command((const char*)xUSART.USART1ReceivedBuffer);
            }
            xUSART.USART1ReceivedNum = 0;
        }

        // --- 定时上报数据 ---
        float temperature = 25.8;
        message_id++;

        sprintf(json_buffer, "{\"id\":\"%ld\",\"dp\":{\"temp\":[{\"v\":%.1f}]}}",
                message_id, temperature);

        sprintf(cmd_buffer, "AT+QMTPUB=0,0,0,0,\"%s\",%d\r\n", PUB_TOPIC, strlen(json_buffer));
        
        printf("\r\n-> Publishing data...\r\n");
        if(send_cmd(cmd_buffer, ">", 2000) == 0)
        {
            send_cmd(json_buffer, "OK", 5000);
        }
        
 
        
        delay_ms(15000);
    }
}

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

volatile uint8_t g_is_waiting_for_rsp = 0;

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
 * @brief 发送AT指令并等待预期响应（改进版，兼容标准库）
 * @param cmd 要发送的指令
 * @param expected_rsp 期待的响应字符串，例如 "OK"
 * @param timeout_ms 超时时间
 * @return 0: 成功, 1: 失败/超时
 */
int send_cmd(const char* cmd, const char* expected_rsp, uint32_t timeout_ms)
{
    char debug_buffer[256];
    
    // ---> 设置标志，表示我们开始等待响应 <---
    g_is_waiting_for_rsp = 1;

    memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
    xUSART.USART1ReceivedNum = 0;
    
    USART1_SendString((char*)cmd);

    sprintf(debug_buffer, ">> Send to Module: %s", cmd);
    USART2_SendString(debug_buffer);

    uint32_t time_start = 0;
    int result = 1; // 默认返回失败/超时

    while(time_start < timeout_ms)
    {
        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            
            if (strstr((const char*)xUSART.USART1ReceivedBuffer, expected_rsp) != NULL)
            {
                result = 0; // 成功
                break; // 跳出循环
            }
            
            if (strstr((const char*)xUSART.USART1ReceivedBuffer, "ERROR") != NULL)
            {
                result = 1; // 错误
                break; // 跳出循环
            }
        }
        
        delay_ms(1);
        time_start++;
    }
    
    if (result == 1) // 如果循环是因为超时而结束
    {
        sprintf(debug_buffer, "!! Timeout for cmd: %s\r\n", cmd);
        USART2_SendString(debug_buffer);
    }
    
    // ---> 清除标志，将串口数据处理权还给主循环 <---
    g_is_waiting_for_rsp = 0;

    if (result == 1) // 如果循环是因为超时而结束
    {
        sprintf(debug_buffer, "!! Timeout for cmd: %s\r\n", cmd);
        USART2_SendString(debug_buffer);
        // ---> 新增打印 <---
        sprintf(debug_buffer, "!! Buffer content on timeout: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
        USART2_SendString(debug_buffer);
    }
    
    return result;
}




/**
 * @brief 仅等待预期响应（send_cmd的简化版）
 * @param expected_rsp 期待的响应字符串
 * @param timeout_ms 超时时间
 * @return 0: 成功, 1: 失败/超时
 */
int wait_for_rsp(const char* expected_rsp, uint32_t timeout_ms)
{
    char debug_buffer[256];
    
    // ---> 设置标志，表示我们开始等待响应 <---
    g_is_waiting_for_rsp = 1;

    uint32_t time_start = 0;
    int result = 1; // 默认返回失败/超时

    while(time_start < timeout_ms)
    {
        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            
            if (strstr((const char*)xUSART.USART1ReceivedBuffer, expected_rsp) != NULL)
            {
                result = 0; // 成功
                break;
            }
            if (strstr((const char*)xUSART.USART1ReceivedBuffer, "ERROR") != NULL)
            {
                result = 1; // 错误
                break;
            }
        }
        delay_ms(1);
        time_start++;
    }

    if (result == 1) // 如果是因为超时而结束
    {
        sprintf(debug_buffer, "!! Timeout while waiting for: %s\r\n", expected_rsp);
        USART2_SendString(debug_buffer);
        sprintf(debug_buffer, "!! Buffer content on timeout: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
        USART2_SendString(debug_buffer);
    }
    
    // ---> 清除标志，将串口数据处理权还给主循环 <---
    g_is_waiting_for_rsp = 0;
    
    return result;
}



/**
 * @brief 解析服务器下发的命令
 * @param buffer 包含+QMTRECV的串口接收缓冲区内容
 */
void parse_command(const char* buffer)
{
    char debug_buffer[256];
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
             sprintf(debug_buffer, "## Received command from Cloud: %s\r\n", command);
             USART2_SendString(debug_buffer);

             if (strcmp(command, "LED_ON") == 0)
             {
                 
                 USART2_SendString("## Action: Turn LED ON\r\n");
             }
             else if (strcmp(command, "LED_OFF") == 0)
             {
                 
                 USART2_SendString("## Action: Turn LED OFF\r\n");
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

    USART2_SendString("\r\n\r\n==================================\r\n");
    USART2_SendString("IoT Module Program Start...\r\n");
    USART2_SendString("==================================\r\n");

    // 4. 【核心】物联网模块初始化流程
    USART2_SendString("\r\n--- Initializing Module ---\r\n");

    while(send_cmd("AT\r\n", "OK", 1000))
    {
        USART2_SendString("AT command failed, retrying...\r\n");
        delay_ms(1000);
    }
    
    send_cmd("ATE0\r\n", "OK", 1000);

    USART2_SendString("\r\n--- Attaching to Network ---\r\n");
    // 必须严格等待 "+CGATT: 1"
    while(send_cmd("AT+CGATT?\r\n", "+CGATT: 1", 5000)) 
    {
        USART2_SendString("Waiting for network attachment...\r\n");


        // --- 新增调试代码 START ---
        if (retry_count % 3 == 0) // 每循环3次，就查询一下信号质量
        {
            USART2_SendString("Checking signal quality...\r\n");
            send_cmd("AT+CSQ\r\n", "OK", 2000); // 发送CSQ指令
            
            char debug_buffer[256];
            sprintf(debug_buffer, "!! CSQ Response: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer); // 打印CSQ的返回结果
        }
        retry_count++;
        // --- 新增调试代码 END ---



        // 这里可以增加打印接收到的内容，方便调试
        char debug_buffer[256];
        sprintf(debug_buffer, "!! Current buffer: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
        USART2_SendString(debug_buffer);
        delay_ms(3000); // 给模块更长的搜索网络时间
    }
    USART2_SendString("## Network Attached Successfully! ##\r\n"); // 成功后打印一下，给自己信心
    
    USART2_SendString("\r\n--- Connecting to MQTT Broker ---\r\n");
    send_cmd("AT+QMTCFG=\"version\",0,4\r\n", "OK", 3000);
    send_cmd("AT+QMTOPEN=0,\"mqtt.heclouds.com\",1883\r\n", "+QMTOPEN: 0,0", 8000);
    
    sprintf(cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", DEVICE_NAME, PRODUCT_ID, AUTH_INFO);
    if(send_cmd(cmd_buffer, "+QMTCONN: 0,0,0", 8000) != 0)
    {
        USART2_SendString("\r\n!! MQTT Connect Failed! Program Halted. !!\r\n");
        while(1); // 连接失败，卡死在这里
    }

    sprintf(cmd_buffer, "AT+QMTSUB=0,1,\"%s\",1\r\n", SUB_TOPIC);
    send_cmd(cmd_buffer, "+QMTSUB: 0,1,0", 5000);

    USART2_SendString("\r\n==================================\r\n");
    USART2_SendString("Initialization Complete. Running...\r\n");
    USART2_SendString("==================================\r\n\r\n");
    


    // 5. 主循环
    while (1)
    {
        // --- 检查是否有服务器下发的命令 ---
        if(g_is_waiting_for_rsp == 0 && xUSART.USART1ReceivedNum > 0)
        {
            // ... 这部分处理URC的代码保持不变 ...
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            char debug_buffer[256];
            sprintf(debug_buffer, "<< Recv from Module (URC): %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer);
            if(strstr((const char*)xUSART.USART1ReceivedBuffer, "+QMTRECV:"))
            {
                parse_command((const char*)xUSART.USART1ReceivedBuffer);
            }
            
            memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
            xUSART.USART1ReceivedNum = 0;
        }

        // --- 定时上报数据 (已修改为正确的两阶段流程) ---
        float temperature = 25.8;
        message_id++;

        // 1. 准备JSON数据和AT指令
        sprintf(json_buffer, "{\"id\":\"%ld\",\"dp\":{\"temp\":[{\"v\":%.1f}]}}",
                message_id, temperature);

        sprintf(cmd_buffer, "AT+QMTPUB=0,0,0,0,\"%s\",%d\r\n", PUB_TOPIC, strlen(json_buffer));
        
        USART2_SendString("\r\n-> Publishing data step 1/2: Send command...\r\n");

        // 2. 发送第一阶段指令，并等待 '>' 符号
        if(send_cmd(cmd_buffer, ">", 2000) == 0)
        {
            USART2_SendString("-> Publishing data step 2/2: Send payload...\r\n");
            
            // 3. (关键) 收到'>'后，直接发送JSON数据负载，而不是用send_cmd
            //    在发送前最好也清空一下接收缓冲区，以防'>'的残留影响后续判断
            memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
            xUSART.USART1ReceivedNum = 0;
            
            USART1_SendString(json_buffer); // 直接发送
            
            // 4. (关键) 调用新的辅助函数等待最终的 "OK" 或 "+QMTPUB" 响应
            if(wait_for_rsp("OK", 5000) == 0)
            {
                USART2_SendString("## Publish Success! ##\r\n");
            }
            else
            {
                USART2_SendString("!! Publish Failed after sending payload. !!\r\n");
            }
        }
        else
        {
            USART2_SendString("!! Publish Failed: Did not receive '>'. !!\r\n");
        }
        
        delay_ms(15000);
    }
}


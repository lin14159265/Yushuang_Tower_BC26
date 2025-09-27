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
    int retry_count = 0;  // 🔧 【新增】添加retry_count变量声明，修复编译错误

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

    USART2_SendString("\r\n--- Network Registration Process ---\r\n");

    // 🔧 【改进】完整的网络注册流程
    int network_ready = 0;
    int max_network_retries = 30; // 最多重试30次，约90秒

    for(int retry = 0; retry < max_network_retries && !network_ready; retry++)
    {
        USART2_SendString("========================================\r\n");
        char status_buffer[256];
        sprintf(status_buffer, "Network Check Attempt %d/%d\r\n", retry + 1, max_network_retries);
        USART2_SendString(status_buffer);

        // 1. 检查信号质量
        USART2_SendString("1. Checking signal quality...\r\n");
        memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
        xUSART.USART1ReceivedNum = 0;
        USART1_SendString("AT+CSQ\r\n");
        delay_ms(1000);

        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            char debug_buffer[256];
            sprintf(debug_buffer, "   CSQ Response: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer);
        }

        // 2. 检查网络注册状态
        USART2_SendString("2. Checking network registration...\r\n");
        memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
        xUSART.USART1ReceivedNum = 0;
        USART1_SendString("AT+CEREG?\r\n");
        delay_ms(1000);

        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            char debug_buffer[256];
            sprintf(debug_buffer, "   CEREG Response: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer);
        }

        // 3. 检查GPRS附着状态
        USART2_SendString("3. Checking GPRS attachment...\r\n");
        memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
        xUSART.USART1ReceivedNum = 0;
        USART1_SendString("AT+CGATT?\r\n");
        delay_ms(1000);

        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            char debug_buffer[512];
            sprintf(debug_buffer, "   CGATT Response: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer);

            // 检查是否已附着
            if (strstr((const char*)xUSART.USART1ReceivedBuffer, "+CGATT: 1") != NULL)
            {
                USART2_SendString("## ✅ Network Ready! ##\r\n");
                network_ready = 1;
                break;
            }
            else
            {
                USART2_SendString("   ❌ Network not ready yet\r\n");
            }
        }

        // 4. 如果还没有准备好，等待更长时间
        if (!network_ready)
        {
            USART2_SendString("4. Waiting 5 seconds before next check...\r\n");
            delay_ms(5000);
        }
    }

    if (!network_ready)
    {
        USART2_SendString("!! ERROR: Network not ready after maximum retries!\r\n");
        USART2_SendString("!! Troubleshooting:\r\n");
        USART2_SendString("!! 1. Check BC26 module power (red LED should be on)\r\n");
        USART2_SendString("!! 2. Check antenna connection\r\n");
        USART2_SendString("!! 3. Check SIM card (should be inserted correctly)\r\n");
        USART2_SendString("!! 4. Check network coverage in your area\r\n");
        USART2_SendString("!! 5. Try restarting the module\r\n");
        USART2_SendString("!! Program will continue but MQTT may fail!\r\n\r\n");
    }
    
    // 只有在网络准备好的情况下才尝试MQTT连接
    if (network_ready)
    {
        USART2_SendString("\r\n--- Connecting to MQTT Broker ---\r\n");

        // 配置MQTT版本
        if(send_cmd("AT+QMTCFG=\"version\",0,4\r\n", "OK", 3000) != 0)
        {
            USART2_SendString("!! MQTT Configuration Failed!\r\n");
        }

        // 打开MQTT连接
        if(send_cmd("AT+QMTOPEN=0,\"mqtt.heclouds.com\",1883\r\n", "+QMTOPEN: 0,0", 8000) != 0)
        {
            USART2_SendString("!! MQTT Open Connection Failed!\r\n");
            USART2_SendString("!! Buffer content: ");
            USART2_SendString((char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString("\r\n");
        }
        else
        {
            USART2_SendString("✅ MQTT Connection Opened Successfully!\r\n");

            // 连接到MQTT服务器
            sprintf(cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", DEVICE_NAME, PRODUCT_ID, AUTH_INFO);
            if(send_cmd(cmd_buffer, "+QMTCONN: 0,0,0", 8000) != 0)
            {
                USART2_SendString("!! MQTT Authentication Failed!\r\n");
                USART2_SendString("!! Buffer content: ");
                USART2_SendString((char*)xUSART.USART1ReceivedBuffer);
                USART2_SendString("\r\n");
            }
            else
            {
                USART2_SendString("✅ MQTT Authentication Successful!\r\n");

                // 订阅主题
                sprintf(cmd_buffer, "AT+QMTSUB=0,1,\"%s\",1\r\n", SUB_TOPIC);
                if(send_cmd(cmd_buffer, "+QMTSUB: 0,1,0", 5000) == 0)
                {
                    USART2_SendString("✅ MQTT Topic Subscription Successful!\r\n");
                }
                else
                {
                    USART2_SendString("!! MQTT Topic Subscription Failed!\r\n");
                }
            }
        }
    }
    else
    {
        USART2_SendString("!! Skipping MQTT connection due to network issues!\r\n");
        USART2_SendString("!! Please fix network problems and restart the device.\r\n");
    }

    USART2_SendString("\r\n==================================\r\n");
    if (network_ready)
    {
        USART2_SendString("✅ Initialization Complete - IoT Ready!\r\n");
    }
    else
    {
        USART2_SendString("⚠️  Initialization Complete - Network Issues!\r\n");
    }
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

        // 🔧 【改进】只有在网络和MQTT都连接成功的情况下才发送数据
        if (network_ready)
        {
            // --- 定时上报数据 (参考示例代码，增加湿度数据，保持temp字段名) ---
            float temperature = 25.8;
            float humidity = 65.0;
            message_id++;

            // 1. 准备JSON数据和AT指令 (保持temp字段名，增加湿度数据)
            sprintf(json_buffer, "{\"id\":\"%ld\",\"dp\":{\"temp\":[{\"v\":%.1f}],\"Humidity\":[{\"v\":%.1f}]}}",
                    message_id, temperature, humidity);

            sprintf(cmd_buffer, "AT+QMTPUB=0,0,0,0,\"%s\",%d\r\n", PUB_TOPIC, strlen(json_buffer));

            USART2_SendString("\r\n-> Publishing data step 1/2: Send command...\r\n");

            // 2. 发送第一阶段指令，并等待 '>' 符号
            if(send_cmd(cmd_buffer, ">", 2000) == 0)
            {
                USART2_SendString("-> Publishing data step 2/2: Send payload...\r\n");

                // 3. 收到'>'后，直接发送JSON数据负载
                memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
                xUSART.USART1ReceivedNum = 0;

                USART1_SendString(json_buffer);

                // 4. 等待最终的 "OK" 响应
                if(wait_for_rsp("OK", 5000) == 0)
                {
                    USART2_SendString("## ✅ Publish Success! ##\r\n");
                }
                else
                {
                    USART2_SendString("!! ❌ Publish Failed after sending payload. !!\r\n");
                }
            }
            else
            {
                USART2_SendString("!! ❌ Publish Failed: Did not receive '>'. !!\r\n");
            }
        }
        else
        {
            // 网络未连接时，显示状态信息
            static int status_counter = 0;
            if (status_counter++ % 20 == 0) // 每20个循环（约5分钟）显示一次状态
            {
                USART2_SendString("\r\n⚠️  Network not ready - skipping data publish\r\n");
                USART2_SendString("   Please check module status and restart if needed\r\n");
            }
        }
        
        delay_ms(15000);
    }
}


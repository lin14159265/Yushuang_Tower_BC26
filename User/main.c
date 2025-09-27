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

volatile uint8_t g_is_waiting_for_rsp = 0;

// 🔧 【新增】MQTT连接状态变量
volatile int mqtt_connected = 0;

/* ================== 用户代码: 物联网平台信息 START ================== */

// --- 1. OneNET 连接信息 ---
#define DEVICE_NAME  "test"
#define PRODUCT_ID   "nZ4v9G1iDK"
#define AUTH_INFO    "version=2018-10-31&res=products%%2FnZ4v9G1iDK%%2Fdevices%%2Ftest&et=1798497693&method=md5&sign=ZmzDSu0enWpLqIS8rHDjXw%%D%%D"

// 🔧 【修复】修正MQTT服务器地址和主题（使用普通MQTT，避免SSL问题）
#define MQTT_SERVER  "mqtt.heclouds.com"   // 使用普通MQTT（非SSL）
#define PUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/dp/post/json"
#define SUB_TOPIC    "$sys/"PRODUCT_ID"/"DEVICE_NAME"/dp/post/json/accepted" 

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

    uint32_t last_data_time = 0;
    while(time_start < timeout_ms)
    {
        if (xUSART.USART1ReceivedNum > 0)
        {
            last_data_time = time_start;

            // 🔧 【修复】创建临时缓冲区进行字符串匹配，避免干扰原始数据
            char temp_buffer[U1_RX_BUF_SIZE + 1];
            memcpy(temp_buffer, xUSART.USART1ReceivedBuffer, xUSART.USART1ReceivedNum);
            temp_buffer[xUSART.USART1ReceivedNum] = '\0';

            if (strstr(temp_buffer, expected_rsp) != NULL)
            {
                result = 0; // 成功
                break; // 跳出循环
            }

            if (strstr(temp_buffer, "ERROR") != NULL)
            {
                result = 1; // 错误
                break; // 跳出循环
            }
        }

        // 🔧 【新增】如果500ms内没有新数据，可以提前退出（响应已完成）
        if(time_start - last_data_time > 500 && last_data_time > 0)
        {
            // 再等待一下确保数据完整
            delay_ms(200);
            break;
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

        // 🔧 【改进】更详细的超时调试信息
        if (xUSART.USART1ReceivedNum > 0)
        {
            char temp_buffer[U1_RX_BUF_SIZE + 1];
            memcpy(temp_buffer, xUSART.USART1ReceivedBuffer, xUSART.USART1ReceivedNum);
            temp_buffer[xUSART.USART1ReceivedNum] = '\0';

            sprintf(debug_buffer, "!! Buffer content on timeout (len=%d): %s\r\n", xUSART.USART1ReceivedNum, temp_buffer);
            USART2_SendString(debug_buffer);

            // 🔧 【调试】检查是否包含期望的响应
            if (strstr(temp_buffer, expected_rsp) != NULL)
            {
                sprintf(debug_buffer, "!! ERROR: Expected response '%s' found in buffer but not detected!\r\n", expected_rsp);
                USART2_SendString(debug_buffer);
            }
        }
        else
        {
            USART2_SendString("!! No data received during timeout period\r\n");
        }
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

    // 🔧 【新增】给模块更多时间初始化和搜索网络
    USART2_SendString("\r\n--- Waiting for Module Initialization ---\r\n");
    USART2_SendString("Giving module 60 seconds to initialize and search for network...\r\n");
    for(int i = 60; i > 0; i--)
    {
        char wait_buffer[64];
        sprintf(wait_buffer, "Waiting... %d seconds\r\n", i);
        USART2_SendString(wait_buffer);
        delay_ms(1000);
    }

    // 🔧 【新增】检查SIM卡状态
    USART2_SendString("\r\n--- Checking SIM Card Status ---\r\n");
    if(send_cmd("AT+CPIN?\r\n", "+CPIN: READY", 5000) != 0)
    {
        USART2_SendString("!! ERROR: SIM card not ready or not inserted!\r\n");
        USART2_SendString("!! Please check SIM card insertion and restart device.\r\n");
    }
    else
    {
        USART2_SendString("✅ SIM card is ready!\r\n");
    }

    // 🔧 【新增】检查网络注册状态
    USART2_SendString("\r\n--- Checking Network Registration ---\r\n");
    if(send_cmd("AT+COPS?\r\n", "OK", 5000) != 0)
    {
        USART2_SendString("!! WARNING: Cannot get network operator info!\r\n");
    }

    USART2_SendString("\r\n--- Network Registration Process ---\r\n");

    // 🔧 【改进】完整的网络注册流程
    int network_ready = 0;
    int max_network_retries = 60; // 最多重试60次，约300秒（5分钟）

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

        // 2. 检查EPS网络注册状态 (NB-IoT使用CEREG)
        USART2_SendString("2. Checking EPS network registration...\r\n");
        memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
        xUSART.USART1ReceivedNum = 0;
        USART1_SendString("AT+CEREG?\r\n");
        delay_ms(2000);

        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
            char debug_buffer[256];
            sprintf(debug_buffer, "   CEREG Response: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
            USART2_SendString(debug_buffer);
        }

        // 3. 尝试主动附着GPRS网络
        USART2_SendString("3. Attempting GPRS attachment...\r\n");
        if(send_cmd("AT+CGATT=1\r\n", "OK", 10000) != 0)
        {
            USART2_SendString("   ❌ GPRS attachment command failed\r\n");
        }
        else
        {
            USART2_SendString("   ✅ GPRS attachment command sent\r\n");
        }

        // 4. 检查GPRS附着状态 - 重新设计检测逻辑
        USART2_SendString("4. Checking GPRS attachment status...\r\n");

        // 🔧 【重写】更智能的CGATT检测函数
        int check_cgatt_status_improved(void)
        {
            USART2_SendString("   Sending AT+CGATT?...\r\n");
            memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
            xUSART.USART1ReceivedNum = 0;
            USART1_SendString("AT+CGATT?\r\n");

            // 等待完整响应（BC26通常需要时间返回完整响应）
            uint32_t total_wait = 0;
            uint32_t last_data_time = 0;
            int found_cgatt = 0;
            int found_ok = 0;

            while(total_wait < 5000) // 总共等待5秒
            {
                delay_ms(50); // 每50ms检查一次
                total_wait += 50;

                // 检查是否有新数据到达
                if(xUSART.USART1ReceivedNum > 0)
                {
                    last_data_time = total_wait;

                    // 创建临时缓冲区检查
                    char temp_buffer[U1_RX_BUF_SIZE + 1];
                    memcpy(temp_buffer, xUSART.USART1ReceivedBuffer, xUSART.USART1ReceivedNum);
                    temp_buffer[xUSART.USART1ReceivedNum] = '\0';

                    // 检查是否包含期望的响应
                    if(strstr(temp_buffer, "+CGATT: 1") != NULL)
                    {
                        found_cgatt = 1;
                        USART2_SendString("   Found +CGATT: 1 in buffer\r\n");
                    }

                    if(strstr(temp_buffer, "OK") != NULL)
                    {
                        found_ok = 1;
                        USART2_SendString("   Found OK in buffer\r\n");
                    }

                    // 🔧 【调试】显示当前缓冲区内容
                    char debug_buffer[512];
                    sprintf(debug_buffer, "   Buffer (len=%d): %s\r\n", xUSART.USART1ReceivedNum, temp_buffer);
                    USART2_SendString(debug_buffer);
                }

                // 如果2秒内没有新数据，认为响应完成
                if(total_wait - last_data_time > 2000 && last_data_time > 0)
                {
                    USART2_SendString("   Response complete (no new data for 2s)\r\n");
                    break;
                }

                // 如果已经找到了两个条件，可以提前退出
                if(found_cgatt && found_ok)
                {
                    USART2_SendString("   Both conditions met, early exit\r\n");
                    break;
                }
            }

            // 最终结果判断
            if(found_cgatt)
            {
                USART2_SendString("   ✅ Final: CGATT check PASSED\r\n");
                return 1;
            }
            else
            {
                USART2_SendString("   ❌ Final: CGATT check FAILED\r\n");
                return 0;
            }
        }

        if(check_cgatt_status_improved())
        {
            USART2_SendString("## ✅ GPRS Attached! Network Ready! ##\r\n");
            network_ready = 1;
            break;
        }
        else
        {
            USART2_SendString("   ❌ GPRS not attached yet\r\n");
        }

        // 5. 如果还没有准备好，等待更长时间
        if (!network_ready)
        {
            USART2_SendString("5. Waiting 10 seconds before next check...\r\n");
            delay_ms(10000);
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

        // 🔧 【改进】配置MQTT参数
        USART2_SendString("1. Configuring MQTT parameters...\r\n");
        if(send_cmd("AT+QMTCFG=\"version\",0,4\r\n", "OK", 3000) != 0)
        {
            USART2_SendString("!! MQTT Version Configuration Failed!\r\n");
        }
        else
        {
            USART2_SendString("✅ MQTT Version Configured!\r\n");
        }

        // 🔧 【改进】设置keepalive参数
        if(send_cmd("AT+QMTCFG=\"keepalive\",0,60\r\n", "OK", 3000) != 0)
        {
            USART2_SendString("!! MQTT Keepalive Configuration Failed!\r\n");
        }
        else
        {
            USART2_SendString("✅ MQTT Keepalive Configured!\r\n");
        }

        // 🔧 【改进】重试机制打开MQTT连接
        int mqtt_connect_retry = 0;
        int mqtt_connected = 0;
        int max_mqtt_retries = 5;

        while(mqtt_connect_retry < max_mqtt_retries && !mqtt_connected)
        {
            USART2_SendString("========================================\r\n");
            char mqtt_status_buffer[256];
            sprintf(mqtt_status_buffer, "MQTT Connection Attempt %d/%d\r\n", mqtt_connect_retry + 1, max_mqtt_retries);
            USART2_SendString(mqtt_status_buffer);

            // 2. 打开MQTT连接
            USART2_SendString("2. Opening MQTT connection...\r\n");
            sprintf(cmd_buffer, "AT+QMTOPEN=0,\"%s\",1883\r\n", MQTT_SERVER);
            if(send_cmd(cmd_buffer, "+QMTOPEN: 0,0", 10000) != 0)
            {
                USART2_SendString("!! MQTT Open Connection Failed!\r\n");
                USART2_SendString("!! Buffer content: ");
                USART2_SendString((char*)xUSART.USART1ReceivedBuffer);
                USART2_SendString("\r\n");
            }
            else
            {
                USART2_SendString("✅ MQTT Connection Opened Successfully!\r\n");

                // 3. 连接到MQTT服务器
                USART2_SendString("3. Authenticating with MQTT server...\r\n");
                sprintf(cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", DEVICE_NAME, PRODUCT_ID, AUTH_INFO);
                if(send_cmd(cmd_buffer, "+QMTCONN: 0,0,0", 10000) != 0)
                {
                    USART2_SendString("!! MQTT Authentication Failed!\r\n");
                    USART2_SendString("!! Buffer content: ");
                    USART2_SendString((char*)xUSART.USART1ReceivedBuffer);
                    USART2_SendString("\r\n");

                    // 如果认证失败，关闭连接以便重试
                    send_cmd("AT+QMTCLOSE=0\r\n", "OK", 3000);
                }
                else
                {
                    USART2_SendString("✅ MQTT Authentication Successful!\r\n");
                    mqtt_connected = 1;

                    // 4. 订阅主题
                    USART2_SendString("4. Subscribing to topic...\r\n");
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

            if(!mqtt_connected)
            {
                mqtt_connect_retry++;
                if(mqtt_connect_retry < max_mqtt_retries)
                {
                    USART2_SendString("Waiting 10 seconds before retry...\r\n");
                    delay_ms(10000);
                }
            }
        }

        if(!mqtt_connected)
        {
            USART2_SendString("!! ERROR: MQTT connection failed after maximum retries!\r\n");
            network_ready = 0; // 标记为未连接，防止后续操作
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
    int publish_error_count = 0;
    int max_publish_errors = 3;
    int mqtt_connection_check_counter = 0;

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

            // 🔧 【改进】检查MQTT连接状态
            if(strstr((const char*)xUSART.USART1ReceivedBuffer, "+QMTSTAT:"))
            {
                USART2_SendString("!! WARNING: MQTT connection status received, checking connection...\r\n");
                mqtt_connected = 0; // 标记MQTT连接可能有问题
            }

            if(strstr((const char*)xUSART.USART1ReceivedBuffer, "+QMTRECV:"))
            {
                parse_command((const char*)xUSART.USART1ReceivedBuffer);
            }

            memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
            xUSART.USART1ReceivedNum = 0;
        }

        // 🔧 【改进】定期检查MQTT连接状态
        mqtt_connection_check_counter++;
        if(mqtt_connection_check_counter >= 120) // 每30分钟检查一次MQTT连接
        {
            mqtt_connection_check_counter = 0;
            if(network_ready && mqtt_connected)
            {
                USART2_SendString("\r\n--- Periodic MQTT Connection Check ---\r\n");
                // 发送心跳包检查连接
                if(send_cmd("AT+QMTSTAT=0\r\n", "OK", 3000) != 0)
                {
                    USART2_SendString("!! MQTT connection may be lost, marking for reconnection\r\n");
                    mqtt_connected = 0;
                }
            }
        }

        // 🔧 【改进】只有在网络和MQTT都连接成功的情况下才发送数据
        if (network_ready && mqtt_connected)
        {
            // --- 定时上报传感器数据到OneNET ---
            USART2_SendString("\r\n--- Preparing Sensor Data ---\r\n");

            // 🔧 【改进】模拟多种传感器数据
            float temperature = 25.8f + (rand() % 100 - 50) * 0.1f;  // 温度：20.8~30.8℃
            float humidity = 65.0f + (rand() % 200 - 100) * 0.1f;      // 湿度：45.0~85.0%
            int signal_strength = 85 + (rand() % 15);                   // 信号强度：85~99
            float battery_voltage = 3.7f + (rand() % 50) * 0.01f;       // 电池电压：3.7~4.2V
            int device_status = 1;                                       // 设备状态：1-正常

            message_id++;

            // 1. 准备JSON数据 - 符合OneNET平台格式
            sprintf(json_buffer,
                "{\"id\":\"%ld\",\"version\":\"1.0\",\"sys\":{\"net\":{\"signal\":%d,\"attach\":1},\"dev\":{\"status\":%d,\"battery\":%.2f}},\"dp\":{\"temperature\":[{\"v\":%.1f}],\"humidity\":[{\"v\":%.1f}],\"light\":[{\"v\":%d}],\"location\":{\"lon\":116.3974,\"lat\":39.9093}}}",
                message_id, signal_strength, device_status, battery_voltage, temperature, humidity, signal_strength);

            sprintf(cmd_buffer, "AT+QMTPUB=0,0,0,0,\"%s\",%d\r\n", PUB_TOPIC, strlen(json_buffer));

            // 显示要发送的数据
            char data_info[256];
            sprintf(data_info, "Data: T=%.1f℃ H=%.1f%% Signal=%d Battery=%.2fV\r\n",
                   temperature, humidity, signal_strength, battery_voltage);
            USART2_SendString(data_info);
            sprintf(data_info, "JSON Length: %d bytes\r\n", strlen(json_buffer));
            USART2_SendString(data_info);

            USART2_SendString("-> Publishing data step 1/2: Send command...\r\n");

            // 2. 发送第一阶段指令，并等待 '>' 符号
            if(send_cmd(cmd_buffer, ">", 3000) == 0)
            {
                USART2_SendString("-> Publishing data step 2/2: Send payload...\r\n");

                // 3. 收到'>'后，直接发送JSON数据负载
                memset(xUSART.USART1ReceivedBuffer, 0, U1_RX_BUF_SIZE);
                xUSART.USART1ReceivedNum = 0;

                USART1_SendString(json_buffer);

                // 4. 等待最终的 "OK" 响应
                if(wait_for_rsp("OK", 8000) == 0)
                {
                    USART2_SendString("## ✅ Publish Success! ##\r\n");
                    publish_error_count = 0; // 重置错误计数

                    // 🔧 【新增】显示成功统计信息
                    static int success_count = 0;
                    success_count++;
                    sprintf(data_info, "Total successful publishes: %d\r\n", success_count);
                    USART2_SendString(data_info);
                }
                else
                {
                    USART2_SendString("!! ❌ Publish Failed after sending payload. !!\r\n");
                    publish_error_count++;

                    // 🔧 【改进】错误计数和恢复机制
                    if(publish_error_count >= max_publish_errors)
                    {
                        USART2_SendString("!! Too many publish errors, checking connections...\r\n");

                        // 检查网络状态
                        if(send_cmd("AT+CGATT?\r\n", "+CGATT: 1", 5000) != 0)
                        {
                            USART2_SendString("!! Network lost! Marking for reconnection...\r\n");
                            network_ready = 0;
                            mqtt_connected = 0;
                        }
                        else
                        {
                            // 检查MQTT连接状态
                            if(send_cmd("AT+QMTSTAT=0\r\n", "OK", 3000) != 0)
                            {
                                USART2_SendString("!! MQTT connection lost! Marking for reconnection...\r\n");
                                mqtt_connected = 0;
                            }
                            else
                            {
                                USART2_SendString("!! Network and MQTT OK, but publish failed\r\n");
                            }
                        }
                        publish_error_count = 0;
                    }
                }
            }
            else
            {
                USART2_SendString("!! ❌ Publish Failed: Did not receive '>'. !!\r\n");
                publish_error_count++;

                if(publish_error_count >= max_publish_errors)
                {
                    USART2_SendString("!! Too many command errors, resetting connection...\r\n");
                    network_ready = 0;
                    mqtt_connected = 0;
                    publish_error_count = 0;
                }
            }
        }
        else
        {
            // 🔧 【改进】网络或MQTT未连接时的恢复机制
            static int reconnect_counter = 0;
            if (reconnect_counter++ % 12 == 0) // 每12个循环（约3分钟）尝试一次重连
            {
                USART2_SendString("\r\n--- Connection Recovery Attempt ---\r\n");

                // 1. 首先检查网络状态
                USART2_SendString("1. Checking network status...\r\n");
                if(send_cmd("AT+CGATT?\r\n", "+CGATT: 1", 5000) == 0)
                {
                    USART2_SendString("✅ Network is ready!\r\n");
                    network_ready = 1;

                    // 2. 如果网络正常，尝试重新建立MQTT连接
                    USART2_SendString("2. Attempting MQTT reconnection...\r\n");

                    // 关闭现有连接
                    send_cmd("AT+QMTCLOSE=0\r\n", "OK", 3000);
                    delay_ms(1000);

                    // 重新打开连接
                    sprintf(cmd_buffer, "AT+QMTOPEN=0,\"%s\",1883\r\n", MQTT_SERVER);
                    if(send_cmd(cmd_buffer, "+QMTOPEN: 0,0", 10000) == 0)
                    {
                        USART2_SendString("✅ MQTT reopened successfully!\r\n");

                        // 重新认证
                        sprintf(cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", DEVICE_NAME, PRODUCT_ID, AUTH_INFO);
                        if(send_cmd(cmd_buffer, "+QMTCONN: 0,0,0", 10000) == 0)
                        {
                            USART2_SendString("✅ MQTT reconnected successfully!\r\n");
                            mqtt_connected = 1;
                            reconnect_counter = 0;
                        }
                        else
                        {
                            USART2_SendString("❌ MQTT re-authentication failed\r\n");
                        }
                    }
                    else
                    {
                        USART2_SendString("❌ MQTT reopen failed\r\n");
                    }
                }
                else
                {
                    USART2_SendString("❌ Network still not ready\r\n");
                    network_ready = 0;

                    // 尝试重新附着网络
                    USART2_SendString("Attempting network re-attachment...\r\n");
                    send_cmd("AT+CGATT=1\r\n", "OK", 10000);
                }
            }
            else
            {
                // 显示当前状态
                static int status_display_counter = 0;
                if (status_display_counter++ % 4 == 0) // 每分钟显示一次状态
                {
                    USART2_SendString("\r\n⚠️  System Status: ");
                    if (!network_ready)
                    {
                        USART2_SendString("Network Down ");
                    }
                    if (!mqtt_connected)
                    {
                        USART2_SendString("MQTT Down ");
                    }
                    USART2_SendString("- Next recovery attempt in 3 minutes\r\n");
                }
            }
        }

        delay_ms(15000);
    }
}


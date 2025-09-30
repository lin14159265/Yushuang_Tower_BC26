#include "bsp_MQTT.h"
#include "stm32f10x.h"      // STM32核心头文件
#include "bsp_usart.h"      // 您的USART驱动头文件
#include <stdio.h>          // 用于 sprintf
#include <string.h>         // 用于字符串操作

/*
 ===============================================================================
                            核心配置区: 设备三元组
 ===============================================================================
 * 当需要更换设备或更新密码时，您只需要修改下面的三个宏定义即可。
*/

// --- 1. 设备所属的产品ID ---
#define MQTT_PRODUCT_ID  "d4J8Spo9uo"

// --- 2. 设备的名称 (也将用作 ClientID) ---
#define MQTT_DEVICE_NAME "test"

// --- 3. 设备的连接鉴权签名 (密码) ---
#define MQTT_PASSWORD_SIGNATURE "version=2018-10-31&res=products%2Fd4J8Spo9uo%2Fdevices%2Ftest&et=1790584042&method=md5&sign=EaWtOdD9uj7fXkgmkswN3A%3D%3D"


/*
 ===============================================================================
                            模块内部变量
 ===============================================================================
*/
static char g_cmd_buffer[512];      // 用于构建AT指令的全局缓冲区
static unsigned int g_message_id = 0; // 用于数据上报的、自增的消息ID


/*
 ===============================================================================
                            静态辅助函数
 ===============================================================================
 * 'static' 关键字使这些函数仅在当前文件 (mqtt_handler.c) 内可见。
*/

/**
 * @brief  毫秒级延时函数
 * @param  ms: 要延时的毫秒数
 */
static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 11993; // 计数值 (根据您的系统时钟调整)
    while(count--);
}


/*
 ===============================================================================
                            公开函数实现
 ===============================================================================
 * 以下是 mqtt_handler.h 中声明的所有函数的具体实现。
*/

/**
 * @brief  初始化AT指令并连接到MQTT服务器
 */
void Initialize_And_Connect_MQTT(void)
{
    // 基础AT指令，检查模块是否就绪
    USART1_SendString("AT\r\n");
    delay_ms(50);
    USART1_SendString("AT+CIMI\r\n");
    delay_ms(100);
    
    // 网络附着
    USART1_SendString("AT+CGATT=1\r\n");
    delay_ms(100);
    USART1_SendString("AT+CGATT?\r\n");
    delay_ms(200);

    // 配置MQTT版本为 3.1.1 (v4)
    USART1_SendString("AT+QMTCFG=\"version\",0,4\r\n");
    delay_ms(500);

    // 打开MQTT网络
    sprintf(g_cmd_buffer, "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n");
    USART1_SendString(g_cmd_buffer);
    delay_ms(400);

    // 连接到MQTT Broker
    sprintf(g_cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n",
            MQTT_DEVICE_NAME,
            MQTT_PRODUCT_ID,
            MQTT_PASSWORD_SIGNATURE);
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}


/**
 * @brief 发布温湿度数据到云平台
 */
void MQTT_Publish_Data(int temperature, int humidity)
{
    char json_payload[256]; 
    
    g_message_id++; // 消息ID自增

    // 构建JSON格式的数据负载
    sprintf(json_payload, 
            "{\"id\":%u,\"dp\":{\"humi\":[{\"v\":%d.%d}],\"temp\":[{\"v\":%d.%d}]}}",
            g_message_id,
            humidity / 10, humidity % 10,
            temperature / 10, temperature % 10);

    // 构建最终的AT发布指令
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/dp/post/json\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}

/**
 * @brief 发布命令响应，回复给云平台
 * @param cmdId         从收到的命令主题中解析出来的命令ID字符串
 * @param response_msg  要回复给云平台的消息体 (例如 "OK" 或 JSON 字符串)
 */
void MQTT_Publish_Command_Response(const char* cmdId, const char* response_msg)
{
    char topic_buffer[128];
    char response_payload[128];

    // 1. 构建响应的 JSON 负载。
    //    OneNET 推荐的格式是 {"msg":"your_message"}
    sprintf(response_payload, "{\"msg\":\"%s\"}", response_msg);

    // 2. 构建完整的响应主题，把收到的 cmdId 包含进去
    sprintf(topic_buffer, "$sys/%s/%s/cmd/response/%s",
            MQTT_PRODUCT_ID,
            MQTT_DEVICE_NAME,
            cmdId);

    // 3. 构建最终的 AT 发布指令
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"%s\",\"%s\"\r\n",
            topic_buffer, 
            response_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}




/**
 * @brief 订阅云端下发命令的主题
 */
void MQTT_Subscribe_Command_Topic(void)
{
    // 主题格式: $sys/{product_id}/{device_name}/cmd/request/+
    // 最后的 '+' 是通配符，表示我们愿意接收任何cmdId的命令
    // QoS 等级设置为 1，确保命令至少送达一次
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/cmd/request/+\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
            
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}
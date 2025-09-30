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





/**
 * @brief [最终版] 上报所有属性 (使用标准的 'params' 格式)
 * @note  此函数生成的报文与OneNET设备调试模拟器的日志完全一致。
 *        确保 g_cmd_buffer 和 json_payload 缓冲区足够大 (例如 1024 字节)。
 */
void MQTT_Publish_All_Properties(
        int ambient_temp, int humidity, int pressure, int wind_speed,
        int temp1, int temp2, int temp3, int temp4,
        int crop_stage, int intervention_status,
        bool sprinklers_available, bool fans_available, bool heaters_available
    )
    {
        // C语言中，bool转为字符串 "true" 或 "false"
        const char* str_sprinklers = sprinklers_available ? "true" : "false";
        const char* str_fans = fans_available ? "true" : "false";
        const char* str_heaters = heaters_available ? "true" : "false";
    
        char json_payload[1024]; 
        g_message_id++;
    
        // 构建与日志完全一致的 'params' JSON 负载
        sprintf(json_payload, 
            "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":{"
            "\\\"ambient_temp\\\":{\\\"value\\\":%d},"
            "\\\"humidity\\\":{\\\"value\\\":%d},"
            "\\\"pressure\\\":{\\\"value\\\":%d},"
            "\\\"wind_speed\\\":{\\\"value\\\":%d},"
            "\\\"temp1\\\":{\\\"value\\\":%d},"
            "\\\"temp2\\\":{\\\"value\\\":%d},"
            "\\\"temp3\\\":{\\\"value\\\":%d},"
            "\\\"temp4\\\":{\\\"value\\\":%d},"
            "\\\"crop_stage\\\":{\\\"value\\\":%d},"
            "\\\"intervention_status\\\":{\\\"value\\\":%d},"
            "\\\"sprinklers_available\\\":{\\\"value\\\":%s},"
            "\\\"fans_available\\\":{\\\"value\\\":%s},"
            "\\\"heaters_available\\\":{\\\"value\\\":%s}"
            "}}",
            g_message_id,
            ambient_temp, humidity, pressure, wind_speed,
            temp1, temp2, temp3, temp4,
            crop_stage, intervention_status,
            str_sprinklers, str_fans, str_heaters
        );
    
        // Topic 必须使用 'thing/property/post'
        sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
                MQTT_PRODUCT_ID, 
                MQTT_DEVICE_NAME,
                json_payload);
    
        USART1_SendString(g_cmd_buffer);
        delay_ms(2000); // 报文很长，建议增加延时确保发送完整
    }


/**
 * @brief 上报霜冻风险警告事件
 * @param current_temp: 触发告警时的当前温度
 * @note  此函数生成的报文与OneNET设备调试模拟器的“事件上报”日志完全一致。
 */
void MQTT_Post_Frost_Alert_Event(float current_temp)
{
    char json_payload[256]; 
    g_message_id++;

    // 构建与日志完全一致的 'event post' JSON 负载
    // %.1f 表示将浮点数格式化为保留一位小数
    sprintf(json_payload, 
        "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":{"
        "\\\"frost_alert\\\":{\\\"value\\\":{\\\"current_temp\\\":%.1f}}"
        "}}",
        g_message_id,
        current_temp
    );

    // Topic 必须使用 'thing/event/post'
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/event/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}



/**
 * @brief 主动获取云端存储的“作物生长时期”属性的期望值
 * @note  此函数仅负责发送“获取请求”。云平台的结果会通过一条新的 +QMTRECV 消息返回，
 *        其 Topic 为 "$sys/.../thing/property/desired/get/reply"。
 *        您需要在 Process_MQTT_Message 函数中添加对这个 reply 主题的处理逻辑。
 */
void MQTT_Get_Desired_Crop_Stage(void)
{
    char json_payload[256]; 
    g_message_id++;

    // 构建与日志完全一致的 'desired/get' JSON 负载
    // params 是一个只包含字符串 "crop_stage" 的数组
    sprintf(json_payload, 
        "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":[\\\"crop_stage\\\"]}",
        g_message_id
    );

    // Topic 必须使用 'thing/property/desired/get'
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/desired/get\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload);

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}

/*

#### **如何使用及处理后续逻辑？**

1.  **调用时机**: 可以在设备初始化完成、MQTT连接成功后调用一次，以确保开机时与云端状态同步。

    ```c
    // 在 main() 函数中
    // ...
    Initialize_And_Connect_MQTT();
    MQTT_Subscribe_Topics(); // 假设您有一个订阅所有主题的函数
    
    // 开机后，主动向云平台查询一次期望的作物时期
    printf("Getting desired crop stage from cloud...\r\n");
    MQTT_Get_Desired_Crop_Stage();
    // ...
    ```

2.  **处理云端回复 (非常重要！)**:
    调用这个函数后，您不会马上得到结果。结果会在稍后由云平台通过一条下行消息发给您。您必须在 `Process_MQTT_Message` 函数中添加相应的处理代码来接收这个结果。

    ```c
    void Process_MQTT_Message(const char* buffer)
    {
        // ... 原有的其他 if-else if 判断 ...

        // [新增] 处理“获取期望值”的回复
        else if (strstr(buffer, "/thing/property/desired/get/reply") != NULL)
        {
            printf("Received 'desired value get' reply.\r\n");
            char* p_start = strstr(buffer, "\"crop_stage\\\":");
            if (p_start)
            {
                p_start += strlen("\"crop_stage\\\":");
                int desired_stage = atoi(p_start);
                printf("Desired crop stage is: %d\r\n", desired_stage);
                // 在这里根据获取到的 desired_stage 更新您的设备状态
                // CROP_SetStage(desired_stage);
            }
        }
    }


*/

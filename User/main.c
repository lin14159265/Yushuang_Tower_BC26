#include <stm32f10x.h>
#include "stm32f10x_conf.h"
#include "system_f103.h"
#include "stdio.h"
#include "string.h"
#include "bsp_led.h"
#include "stdlib.h"
#include "bsp_usart.h"
#include "stdbool.h" // 引入布尔类型头文件
#include <ctype.h>   // [新增] 包含此头文件以使用 isspace() 函数

// 将 g_cmd_buffer 的大小从 1024 增加到 2048
static char g_cmd_buffer[4096];
static char g_json_payload[4096]; 
static unsigned int g_message_id = 0;

// --- [新增] 定义全局变量来存储从云端下发的状态 ---
int g_crop_stage = 0;           // 作物生长时期 (默认为0)
int g_intervention_status = 0;  // 人工干预状态 (默认为0)


// --- 1. 设备所属的产品ID ---
#define MQTT_PRODUCT_ID  "30w1g93kaf"
// --- 2. 设备的名称 (也将用作 ClientID) ---
#define MQTT_DEVICE_NAME "Yushuang_Tower_007"
// --- 3. 设备的连接鉴权签名 (密码) ---
#define MQTT_PASSWORD_SIGNATURE "version=2018-10-31&res=products%2F30w1g93kaf%2Fdevices%2FYushuang_Tower_007&et=1790671501&method=md5&sign=F48CON9W%2FTkD6dPXA%2FKxgQ%3D%3D"




/*
 ===============================================================================
                            模块内部变量
 ===============================================================================
*/

// 将 g_cmd_buffer 的大小从 1024 增加到 2048
#define CMD_BUFFER_SIZE 4096
#define JSON_PAYLOAD_SIZE 4096

/*
 ===============================================================================
                            静态辅助函数
 ===============================================================================
*/
/**
 * @brief  [已更新] 毫秒级延时函数 (使用SysTick硬件定时器)
 * @param  ms: 要延时的毫秒数
 */
static void delay_ms(uint32_t ms)
{
    // 直接调用您工程中提供的、基于SysTick的精确延时函数
    System_DelayMS(ms);
}

/*
 ===============================================================================
                            公开函数实现
 ===============================================================================
*/


/**
 * @brief  [升级版] 发送AT指令并等待响应 (使用SysTick实现精确超时)
 * @param  cmd: 要发送的AT指令字符串。
 * @param  expected_response: 期望在模块的回复中找到的关键字字符串。
 * @param  timeout_ms: 等待响应的超时时间，单位毫秒。
 * @return bool: true 代表成功，false 代表失败。
 */
bool MQTT_Send_AT_Command(const char* cmd, const char* expected_response, uint32_t timeout_ms)
{
    // 步骤1：清空串口接收缓冲区
    memset(xUSART.USART1ReceivedBuffer, 0, sizeof(xUSART.USART1ReceivedBuffer));
    xUSART.USART1ReceivedNum = 0;

    // 步骤2：通过串口发送AT指令
    printf("SEND: %s", cmd);
    USART1_SendString((char*)cmd);

    // 步骤3：[核心升级] 使用SysTick获取当前时间作为超时判断的起点
    u64 start_time = System_GetTimeMs();

    // 步骤4：在超时时间内循环等待
    while ((System_GetTimeMs() - start_time) < timeout_ms)
    {
        // 检查串口是否收到了数据
        if (xUSART.USART1ReceivedNum > 0)
        {
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';

            // 检查收到的数据中是否包含期望的响应
            if (strstr((char*)xUSART.USART1ReceivedBuffer, expected_response) != NULL)
            {
                printf("SUCCESS: Found response '%s'\r\n\r\n", expected_response);
                return true; // 成功！
            }
        }
        
        // 短暂延时，避免CPU空转，让出CPU给中断等其他任务
        // 这里的 delay_ms() 已经是我们更新后的精确延时了
        delay_ms(10); 
    }

    // 如果循环结束，说明已经超过了指定的 timeout_ms
    printf("FAIL: Timeout. Did not receive '%s' in %lu ms.\r\n\r\n", expected_response, timeout_ms);
    printf("Last received data: %s\r\n", (char*)xUSART.USART1ReceivedBuffer);
    return false; // 失败！
}



/**
 * @brief [改造版] 使用同步发送-确认机制，可靠地初始化模块并连接到MQTT服务器
 * @return bool: true 代表所有步骤都成功，false 代表有任何一步失败。
 */
bool Robust_Initialize_And_Connect_MQTT(void)
{
    // 1. 检查AT指令是否响应
    if (!MQTT_Send_AT_Command("AT\r\n", "OK", 500)) 
        return false;
    
    // 2. 获取SIM卡信息 (IMSI)
    if (!MQTT_Send_AT_Command("AT+CIMI\r\n", "OK", 1000)) 
        return false;

    // 3. 设置GPRS附着
    if (!MQTT_Send_AT_Command("AT+CGATT=1\r\n", "OK", 1000)) 
        return false;

    // 4. 查询GPRS附着状态，期望回复是 "+CGATT: 1"
    if (!MQTT_Send_AT_Command("AT+CGATT?\r\n", "+CGATT: 1", 3000)) 
        return false;

    // 5. 配置MQTT协议版本为 3.1.1
    if (!MQTT_Send_AT_Command("AT+QMTCFG=\"version\",0,4\r\n", "OK", 1000)) 
        return false;

    // 6. 打开MQTT网络 (连接到OneNET服务器)
    //    注意：网络操作需要更长的超时时间
    sprintf(g_cmd_buffer, "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n");
    if (!MQTT_Send_AT_Command(g_cmd_buffer, "+QMTOPEN: 0,0", 5000)) 
        return false; 

    // 7. 使用三元组连接MQTT Broker
    //    注意：这是最关键的网络认证步骤，也需要较长超时
    sprintf(g_cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", MQTT_DEVICE_NAME, MQTT_PRODUCT_ID, MQTT_PASSWORD_SIGNATURE);
    if (!MQTT_Send_AT_Command(g_cmd_buffer, "+QMTCONN: 0,0,0", 5000)) 
        return false; 
    return true; // 所有步骤都成功了！
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
    g_message_id++;
    // 构建与日志完全一致的 'params' JSON 负载
    sprintf(g_json_payload, 
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
        g_crop_stage, g_intervention_status,
        str_sprinklers, str_fans, str_heaters
    );
    // Topic 必须使用 'thing/property/post'
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);
            LED3_ON;
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


/**
 * @brief [健壮版] 订阅云端下发命令的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
bool MQTT_Subscribe_Command_Topic(void)
{
    // 目标Topic: $sys/{product_id}/{device_name}/cmd/request/+
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/cmd/request/+\",1\r\n", 
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME);
            
    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}


/**
 * @brief [健壮版] 订阅“属性设置”的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
bool MQTT_Subscribe_Property_Set_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/property/set
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/set\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}

/**
 * @brief [健壮版] 订阅“服务调用”的主题，并等待确认
 * @return bool: true 代表订阅成功, false 代表失败
 */
bool MQTT_Subscribe_Service_Invoke_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/service/invoke
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/service/invoke\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);

    // 发送指令并等待模块返回 "+QMTSUB: 0,1,0" 表示成功
    return MQTT_Send_AT_Command(g_cmd_buffer, "+QMTSUB: 0,1,0", 3000);
}


/**
 * @brief [健壮版] 一次性订阅所有需要接收消息的主题，并检查每一步的结果
 * @return bool: true 代表所有主题都订阅成功, false 代表有任何一个失败
 */
bool MQTT_Subscribe_All_Topics(void)
{
    printf("INFO: Subscribing to all topics...\r\n");

    if (!MQTT_Subscribe_Command_Topic()) {
        printf("ERROR: Failed to subscribe to Command Topic.\r\n");
        return false;
    }
    
    if (!MQTT_Subscribe_Property_Set_Topic()) {
        printf("ERROR: Failed to subscribe to Property Set Topic.\r\n");
        return false;
    }
    
    if (!MQTT_Subscribe_Service_Invoke_Topic()) {
        printf("ERROR: Failed to subscribe to Service Invoke Topic.\r\n");
        return false;
    }
    
    // 如果未来有更多需要订阅的主题，继续在这里添加...
    // if (!MQTT_Subscribe_Another_Topic()) {
    //     return false;
    // }

    printf("INFO: All topics subscribed successfully.\r\n\r\n");
    return true; // 所有订阅都成功了
}

/**
 * @brief  生成一组随机的模拟传感器数据，并上报到云平台
 * @note   此函数用于测试。在实际产品中，请替换为真实的传感器读取。
 */
void MQTT_Publish_All_Properties_Random(void)
{
    int ambient_temp, humidity, pressure, wind_speed;
    int temp1, temp2, temp3, temp4;
    
    bool sprinklers_available = true;
    bool fans_available = true;
    bool heaters_available = true;

    // --- 生成随机数据 ---
    // rand() % N 会生成一个 0 到 N-1 之间的随机整数

    // 环境温度: 在 15 到 25 之间波动
    ambient_temp = 15 + (rand() % 11); 
    
    // 相对湿度: 在 50 到 70 之间波动
    humidity = 50 + (rand() % 21);

    // 大气压: 在 1000 到 1020 之间波动
    pressure = 1000 + (rand() % 21);

    // 风速: 在 0 到 5 之间波动
    wind_speed = rand() % 6;

    // 四个监测点温度: 在环境温度的基础上，加上 -2 到 +2 的随机偏差
    temp1 = ambient_temp + (rand() % 5) - 2; 
    temp2 = ambient_temp + (rand() % 5) - 2;
    temp3 = ambient_temp + (rand() % 5) - 2;
    temp4 = ambient_temp + (rand() % 5) - 2;
    // --- 调用原始的上报函数发送数据 ---
    MQTT_Publish_All_Properties(
        ambient_temp, humidity, pressure, wind_speed,
        temp1, temp2, temp3, temp4,
        g_crop_stage, g_intervention_status,
        sprinklers_available, fans_available, heaters_available
    );
}



/**
 * @brief [安全修正版] 仅上报 temp1 和 temp2 两个温度属性
 * @note  使用 snprintf 替代 sprintf，从根本上防止缓冲区溢出风险。
 */
void MQTT_Publish_Temp1_Temp2(int temp1, int temp2)
{
    // 每次调用都增加消息ID
    g_message_id++;

    // 1. [修改] 使用 snprintf 安全地构建 JSON 负载
    snprintf(g_json_payload, JSON_PAYLOAD_SIZE,
        "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":{"
        "\\\"temp1\\\":{\\\"value\\\":%d},"
        "\\\"temp2\\\":{\\\"value\\\":%d}"
        "}}",
        g_message_id,
        temp1, temp2
    );

    // 2. [修改] 使用 snprintf 安全地构建 AT+QMTPUB 指令
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
            "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理 (提示：在正式产品中，建议使用非阻塞方式)
    delay_ms(1000); 
}

/**
 * @brief [修正版] 生成随机数据并调用“上报两个温度”的函数
 * @note  此函数逻辑不变，但它现在调用的是上面那个更安全的版本。
 */
void MQTT_Publish_Temp1_Temp2_Random(void)
{
    int temp1, temp2;
    
    // 生成 15 到 25 度之间的随机基准温度
    int base_temp = 15 + (rand() % 11); 

    // 两个监测点温度
    temp1 = base_temp + (rand() % 5) - 2; 
    temp2 = base_temp + (rand() % 5) - 2;

    // --- 调用只上报两个温度的函数 ---
    MQTT_Publish_Temp1_Temp2(temp1, temp2);
}


/**
 * @brief [新增] 仅上报 temp3 和 temp4 两个温度属性
 * @note  使用 snprintf 防止缓冲区溢出。
 */
void MQTT_Publish_Temp3_Temp4(int temp3, int temp4)
{
    // 每次调用都增加消息ID
    g_message_id++;

    // 1. 构建只包含 temp3 和 temp4 属性的 JSON 负载
    snprintf(g_json_payload, JSON_PAYLOAD_SIZE,
        "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":{"
        "\\\"temp3\\\":{\\\"value\\\":%d},"
        "\\\"temp4\\\":{\\\"value\\\":%d}"
        "}}",
        g_message_id,
        temp3, temp4
    );

    // 2. 构建 AT+QMTPUB 指令
    snprintf(g_cmd_buffer, CMD_BUFFER_SIZE, 
            "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理
    delay_ms(1000); 
}

/**
 * @brief [新增] 生成随机数据并调用“上报temp3和temp4”的函数
 */
void MQTT_Publish_Temp3_Temp4_Random(void)
{
    int temp3, temp4;
    
    // 生成 30 到 40 度之间的随机基准温度 (假设这是另一组传感器)
    int base_temp = 30 + (rand() % 11); 

    // 两个监测点温度
    temp3 = base_temp + (rand() % 6) - 3; // 波动范围稍大一些
    temp4 = base_temp + (rand() % 6) - 3;

    // --- 调用只上报两个温度的函数 ---
    MQTT_Publish_Temp3_Temp4(temp3, temp4);
}



/**
 * @brief 从一个JSON格式的字符串中查找指定的键(key)，并解析其对应的整数值。
 * @param buffer: 包含JSON内容的字符串。
 * @param key:    要查找的JSON键名。
 * @param result: 如果解析成功，整数值将被存放在这个指针指向的地址。
 * @return int:   如果成功找到并解析了整数，返回1；否则返回0。
 * @note   这是一个不依赖任何JSON库的安全解析实现。
 *         它能处理键和值周围的空格，并能验证值的合法性。
 */
int find_and_parse_json_int(const char* buffer, const char* key, int* result)
{
    char search_key[64];
    // 1. 构造要搜索的键格式，例如: "crop_stage"
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    // 2. 查找键的位置
    char* p_key = strstr(buffer, search_key);
    if (p_key == NULL) {
        return 0; // 没找到键
    }

    // 3. 移动指针到键的末尾
    char* p_val = p_key + strlen(search_key);

    // 4. 跳过键和冒号之间的所有空格
    while (*p_val && isspace((unsigned char)*p_val)) {
        p_val++;
    }

    // 5. 检查后面是否是冒号
    if (*p_val != ':') {
        return 0; // 格式错误，键后面不是冒号
    }
    p_val++; // 跳过冒号

    // 6. 使用strtol进行安全的字符串到长整型转换
    char* end_ptr;
    long parsed_value = strtol(p_val, &end_ptr, 10);

    // 7. 验证转换是否成功
    // 如果p_val和end_ptr指向同一个地址，说明冒号后面第一个字符就不是数字，转换失败。
    if (p_val == end_ptr) {
        return 0; // 转换失败
    }

    // 8. 如果成功，将结果存入result指针
    *result = (int)parsed_value;
    return 1; // 成功
}


/**
 * @brief [健壮版] 统一处理所有从云平台接收到的MQTT消息
 * @param buffer: 指向串口接收缓冲区的指针
 * @note  此版本不使用外部库，但通过一个辅助函数实现了更安全的JSON解析。
 */
void Process_MQTT_Message_Robust(const char* buffer)
{
    printf("RECV: %s\r\n", buffer);
    int parsed_value;

    // --- 1. 判断是否为“属性设置” (`/thing/property/set`) ---
    if (strstr(buffer, "/thing/property/set") != NULL)
    {
        printf("DEBUG: Received a 'Property Set' command.\r\n");
        if (find_and_parse_json_int(buffer, "crop_stage", &parsed_value))
        {
            g_crop_stage = parsed_value;
            printf("ACTION: Cloud set 'crop_stage' to %d\r\n\r\n", g_crop_stage);
        }
        else
        {
            printf("WARN: Failed to parse 'crop_stage' from property set message.\r\n\r\n");
        }
    }
    // --- 2. 判断是否为“服务调用” (`/thing/service/invoke`) ---
    else if (strstr(buffer, "/thing/service/invoke") != NULL)
    {
        printf("DEBUG: Received a 'Service Invoke' command.\r\n");
        // 可以先判断是哪个服务，再解析参数
        if (strstr(buffer, "\"method\":\"set_intervention\""))
        {
            if (find_and_parse_json_int(buffer, "status", &parsed_value))
            {
                g_intervention_status = parsed_value;
                printf("ACTION: Cloud invoked 'set_intervention' with status %d\r\n\r\n", g_intervention_status);
            }
            else
            {
                printf("WARN: Failed to parse 'status' from service invoke message.\r\n\r\n");
            }
        }
    }
    // --- 3. 判断是否为“获取期望值”的回复 (`/desired/get/reply`) ---
    else if (strstr(buffer, "/thing/property/desired/get/reply") != NULL)
    {
        printf("DEBUG: Received a 'Desired Value Get' reply.\r\n");
        if (find_and_parse_json_int(buffer, "crop_stage", &parsed_value))
        {
            g_crop_stage = parsed_value;
            printf("ACTION: Synced desired 'crop_stage' from cloud: %d\r\n\r\n", g_crop_stage);
        }
        else
        {
            printf("WARN: Failed to parse desired 'crop_stage' from reply.\r\n\r\n");
        }
    }
    // --- 4. 判断是否为“旧版命令” (`/cmd/request/`) ---
    else if (strstr(buffer, "/cmd/request/") != NULL)
    {
        printf("DEBUG: Received a legacy 'Command Request'.\r\n");
    }
}

/**
 * @brief [新增] 仅上报四个温度属性
 * @note  此函数用于分包发送数据，以避免单条AT指令过长导致的问题。
 */
void MQTT_Publish_Only_Temperatures(int temp1, int temp2, int temp3, int temp4)
{
    // 每次调用都增加消息ID，确保与云端同步
    g_message_id++;

    // 1. 构建只包含四个温度属性的 'params' JSON 负载
    sprintf(g_json_payload, 
        "{\\\"id\\\":\\\"%u\\\",\\\"version\\\":\\\"1.0\\\",\\\"params\\\":{"
        "\\\"temp1\\\":{\\\"value\\\":%d},"
        "\\\"temp2\\\":{\\\"value\\\":%d},"
        "\\\"temp3\\\":{\\\"value\\\":%d},"
        "\\\"temp4\\\":{\\\"value\\\":%d}"
        "}}",
        g_message_id,
        temp1, temp2, temp3, temp4
    );

    // 2. 构建 AT+QMTPUB 指令，Topic 保持不变
    //    "$sys/{product_id}/{device_name}/thing/property/post"
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/thing/property/post\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            g_json_payload);

    // 3. 通过串口发送指令
    USART1_SendString(g_cmd_buffer);

    // 4. 等待模块处理和发送
    delay_ms(1000); // 因为报文变短了，延时可以适当缩短
}

/**
 * @brief [新增] 生成随机的模拟温度数据，并调用新函数上报到云平台
 */
void MQTT_Publish_Temperatures_Random(void)
{
    int temp1, temp2, temp3, temp4;
    
    // 生成 15 到 25 度之间的随机基准温度
    int base_temp = 15 + (rand() % 11); 

    // 四个监测点温度: 在基准温度的基础上，加上 -2 到 +2 的随机偏差
    temp1 = base_temp + (rand() % 5) - 2; 
    temp2 = base_temp + (rand() % 5) - 2;
    temp3 = base_temp + (rand() % 5) - 2;
    temp4 = base_temp + (rand() % 5) - 2;

    // --- 调用只上报温度的函数 ---
    MQTT_Publish_Only_Temperatures(temp1, temp2, temp3, temp4);
}

/**
 * @brief 主函数
 */
/**
 * @brief 主函数
 */
int main(void)
{
    // 1. 系统核心初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    System_SwdMode();

    // 2. 外设初始化
    System_SysTickInit();
    USART1_Init(115200);
    USART2_Init(115200); // 假设 printf 从 USART2 输出
    Led_Init();

    printf("System Initialized. Trying to connect to MQTT server...\r\n");

    if (Robust_Initialize_And_Connect_MQTT())
    {
        printf("SUCCESS: MQTT Connected.\r\n");
        LED1_ON; // 用常亮灯表示连接成功

        // [修改点] 连接成功后，订阅所有主题，并检查结果
        if (MQTT_Subscribe_All_Topics())
        {
            // 订阅成功，可以进入主循环
            printf("Entering main loop...\r\n");
            
            // --- [新增] 开机后，主动向云平台查询一次期望的作物时期 ---
            MQTT_Get_Desired_Crop_Stage();
            
            while (1)
            {
                // --- 1. 检查并处理从云平台下发的数据 ---
                if (xUSART.USART1ReceivedNum > 0)
                {
                    xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';
                    Process_MQTT_Message_Robust((char*)xUSART.USART1ReceivedBuffer);                
                    xUSART.USART1ReceivedNum = 0;
                }
                
                // --- 2. 周期性上报数据 (示例) ---
                MQTT_Publish_Temperatures_Random();
                delay_ms(10000); // 例如每10秒上报一次
            }
        }
        else
        {
            // [新增] 订阅失败处理
            printf("FATAL ERROR: Failed to subscribe to topics. Halting.\r\n");
            // 在这里可以闪烁一个错误LED灯，然后进入死循环
            while(1)
            {
                LED1_TOGGLE; // 假设 LED1 用于错误指示
                delay_ms(200);
            }
        }
    }
    else
    {
        // 连接失败处理
        printf("FATAL ERROR: Failed to connect to MQTT server. Halting.\r\n");
        while(1)
        {
            LED2_TOGGLE; // 假设 LED2 用于连接失败指示
            delay_ms(500);
        }
    }
}


#include <stm32f10x.h>
#include "stm32f10x_conf.h"
#include "system_f103.h"
#include "stdio.h"
#include "string.h"
#include "bsp_led.h"
#include "stdlib.h"
#include "bsp_usart.h"
#include "stdbool.h" // 引入布尔类型头文件

// 缓冲区加倍，因为新的JSON报文很长
static char g_cmd_buffer[1024];      
static char g_json_payload[1024];
static unsigned int g_message_id = 0; 



// --- 1. 设备所属的产品ID ---
#define MQTT_PRODUCT_ID  "30w1g93kaf"

// --- 2. 设备的名称 (也将用作 ClientID) ---
#define MQTT_DEVICE_NAME "Yushuang_Tower_007"

// --- 3. 设备的连接鉴权签名 (密码) ---
#define MQTT_PASSWORD_SIGNATURE "version=2018-10-31&res=products%2F30w1g93kaf%2Fdevices%2FYushuang_Tower_007&et=1790736737&method=md5&sign=QnmELX6Y%2BaGxhsPnvzreMQ%3D%3D"




/*
 ===============================================================================
                            模块内部变量
 ===============================================================================
*/



/*
 ===============================================================================
                            静态辅助函数
 ===============================================================================
*/
static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 11993;
    while(count--);
}


/*
 ===============================================================================
                            公开函数实现
 ===============================================================================
*/

/**
 * @brief  初始化AT指令并连接到MQTT服务器
 */
void Initialize_And_Connect_MQTT(void)
{
    USART1_SendString("AT\r\n");
    delay_ms(50);
    USART1_SendString("AT+CIMI\r\n");
    delay_ms(100);
    USART1_SendString("AT+CGATT=1\r\n");
    delay_ms(100);
    USART1_SendString("AT+CGATT?\r\n");
    delay_ms(200);
    USART1_SendString("AT+QMTCFG=\"version\",0,4\r\n");
    delay_ms(500);
    sprintf(g_cmd_buffer, "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n");
    USART1_SendString(g_cmd_buffer);
    delay_ms(400);
    sprintf(g_cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n", MQTT_DEVICE_NAME, MQTT_PRODUCT_ID, MQTT_PASSWORD_SIGNATURE);
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
 /*
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
}```

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
    ```

现在您已经拥有了与模拟器功能完全对应的三个核心上行操作的STM32代码。
*/


/**
 * @brief 订阅云端下发命令的主题
 * @note  这个函数让设备告诉OneNET平台：“我准备好接收命令了”。
 */
void MQTT_Subscribe_Command_Topic(void)
{
    // 准备构建AT指令
    // AT+QMTSUB=<client_idx>,<msg_id>,"<topic>",<qos>
    // client_idx: 客户端索引，我们一直用0
    // msg_id: 消息ID，对于订阅可以随便写，比如1
    // topic: 订阅的主题，这是核心
    // qos: 服务质量等级，通常用1，表示至少送达一次

    // 目标Topic: $sys/{product_id}/{device_name}/cmd/request/+
    // 最后的 '+' 是一个通配符，表示我们愿意接收任何cmdId的命令。
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/cmd/request/+\",1\r\n", 
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME);
            
    // 通过串口将AT指令发送给模块
    USART1_SendString(g_cmd_buffer);
    delay_ms(500); // 延时等待模块的响应
}


// 订阅“属性设置”的主题
void MQTT_Subscribe_Property_Set_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/property/set
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/property/set\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}

// 订阅“服务调用”的主题
void MQTT_Subscribe_Service_Invoke_Topic(void)
{
    // Topic: $sys/{product_id}/{device_name}/thing/service/invoke
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/thing/service/invoke\",1\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}


/**
 * @brief [推荐] 一次性订阅所有需要接收消息的主题
 */
void MQTT_Subscribe_All_Topics(void)
{
    printf("Subscribing to all topics...\r\n");

    MQTT_Subscribe_Command_Topic();
    printf("-> Subscribed to Command Topic.\r\n");

    MQTT_Subscribe_Property_Set_Topic();
    printf("-> Subscribed to Property Set Topic.\r\n");
    
    MQTT_Subscribe_Service_Invoke_Topic();
    printf("-> Subscribed to Service Invoke Topic.\r\n");
    
    // 如果未来有更多需要订阅的主题，继续在这里添加...
}

/**
 * @brief  生成一组随机的模拟传感器数据，并上报到云平台
 * @note   此函数用于测试。在实际产品中，请替换为真实的传感器读取。
 */
void MQTT_Publish_All_Properties_Random(void)
{
    int ambient_temp, humidity, pressure, wind_speed;
    int temp1, temp2, temp3, temp4;
    // 下面这些通常是固定的或者由逻辑控制的，但也暂时给个初始值
    int crop_stage = 0;
    int intervention_status = 0;
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
        crop_stage, intervention_status,
        sprinklers_available, fans_available, heaters_available
    );
}




/**
 * @brief 主函数
 */
int main(void)
{

    // 1. 系统核心初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    System_SwdMode();

    // 2. 外设初始化
    USART1_Init(115200);
    Led_Init();

    

    // 3. 执行模块初始化和连接云平台
    Initialize_And_Connect_MQTT();
    

    MQTT_Subscribe_All_Topics();

    


    
    

    
    

    while (1)
    {
        // 每隔一段时间 (例如 5 秒) 上报一次随机数据
        MQTT_Publish_All_Properties_Random();
        
        // 这里用 delay_ms 做演示。在实际项目中，请使用定时器或操作系统的延时
        delay_ms(5000);

    }
}
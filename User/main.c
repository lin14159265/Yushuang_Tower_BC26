#include <stm32f10x.h>
#include "stm32f10x_conf.h"
#include "system_f103.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "bsp_usart.h"

static char g_cmd_buffer[512];

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



/* ================== 延时函数 (来自您的代码) ================== */
/*
static void delay_ms(uint32_t ms)
{
    ms = ms * 11993;
    for (uint32_t i = 0; i < ms; i++);
}
*/

static void delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms * 11993; // 将计数值存入一个 volatile 变量
    while(count--); // 使用 while 循环递减
}


/*
 ===============================================================================
                            AT指令功能函数封装
 ===============================================================================
 * - 所有指令参数均基于上面的核心配置宏动态生成或使用常量。
*/

/********************************************************************************
 * @brief  按顺序发送AT指令以建立MQTT连接
 ********************************************************************************/
void Initialize_And_Connect_MQTT(void)
{
    

    USART1_SendString("AT\r\n");
    delay_ms(500);
    USART1_SendString("AT+CIMI\r\n");
    delay_ms(1000);
    USART1_SendString("AT+CGATT=1\r\n");
    delay_ms(1000);
    USART1_SendString("AT+CGATT?\r\n");
    delay_ms(2000);


    USART1_SendString("AT+QMTCFG=\"version\",0,4\r\n");
    delay_ms(500);

    sprintf(g_cmd_buffer, "AT+QMTOPEN=0,\"mqtts.heclouds.com\",1883\r\n");
    USART1_SendString(g_cmd_buffer);
    delay_ms(4000);

    sprintf(g_cmd_buffer, "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"\r\n",
            MQTT_DEVICE_NAME,          // 参数1: ClientID
            MQTT_PRODUCT_ID,           // 参数2: Username
            MQTT_PASSWORD_SIGNATURE);  // 参数3: Password
    USART1_SendString(g_cmd_buffer);
    delay_ms(5000);
}

/**
 * @brief 订阅主题
 */
void MQTT_Subscribe_Topic(void)
{
    
    // 在函数内部根据核心宏动态生成订阅主题
    sprintf(g_cmd_buffer, "AT+QMTSUB=0,1,\"$sys/%s/%s/dp/post/json/accepted\",0\r\n", 
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}

/**
 * @brief 发布温湿度数据到云平台
 * @param temperature 整数形式的温度值 (例如 253 代表 25.3 度)
 * @param humidity    整数形式的湿度值 (例如 605 代表 60.5 %)
 */
// 【修正后】的 MQTT_Publish_Data 函数
void MQTT_Publish_Data(int temperature, int humidity)
{
     char json_payload[256]; 
    sprintf(json_payload, 
            "{\\\"id\\\":%d,\\\"dp\\\":{\\\"Humidity\\\":[{\\\"v\\\":%d.%d}],\\\"Temperature\\\":[{\\\"v\\\":%d.%d}]}}",
            (int)rand(), // 使用 %d 和 (int) 强制类型转换，嵌入式系统兼容性更好
            humidity / 10, humidity % 10,
            temperature / 10, temperature % 10);

    // 第2步：将JSON负载作为 %s 参数，构建最终的AT指令。
    // 这样做，sprintf会自动处理好所有必要的引号，代码清晰且不会出错。
    sprintf(g_cmd_buffer, "AT+QMTPUB=0,0,0,0,\"$sys/%s/%s/dp/post/json\",\"%s\"\r\n",
            MQTT_PRODUCT_ID, 
            MQTT_DEVICE_NAME,
            json_payload); // 将构建好的json字符串作为参数传入

    USART1_SendString(g_cmd_buffer);
    delay_ms(1000);
}

/**
 * @brief 取消订阅主题
 */
void MQTT_Unsubscribe_Topic(void)
{
    
    // 同样在函数内部动态生成主题
    sprintf(g_cmd_buffer, "AT+QMTUNS=0,1,\"$sys/%s/%s/dp/post/json/accepted\"\r\n",
            MQTT_PRODUCT_ID, MQTT_DEVICE_NAME);
    USART1_SendString(g_cmd_buffer);
    delay_ms(500);
}

/**
 * @brief 断开MQTT连接
 */
void MQTT_Disconnect(void)
{
    USART1_SendString("AT+QMTDISC=0\r\n");
    delay_ms(500);
}


// 主函数 (保持不变)
int main(void)
{
    int fake_temp, fake_humi;
    
    // 1. 系统核心初始化
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    System_SwdMode();

    // 2. 外设初始化
    USART1_Init(115200); // 与通信模块交互
    

    // 3. 执行模块初始化和连接云平台
    Initialize_And_Connect_MQTT();

    delay_ms(8000);
    
    
    // 4. 连接成功后，订阅主题
    //MQTT_Subscribe_Topic();


    while (1)
    {
        
        // 生成模拟的温湿度数据用于测试
        fake_temp = 200 + (rand() % 100); 
        fake_humi = 500 + (rand() % 300);

        

        // 调用函数，发布数据
        MQTT_Publish_Data(fake_temp, fake_humi);
        
        // 每10秒上报一次
        delay_ms(10000); 
        
        
    }
}
#include <stm32f10x.h>
#include "stm32f10x_conf.h"
#include "system_f103.h"
#include "stdio.h"
#include "string.h"
#include "bsp_led.h"
#include "stdlib.h"
#include "bsp_usart.h"
#include "../bsp/MQTT/bsp_MQTT.h"




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

    
    
    // 4. 连接成功后，订阅主题
    MQTT_Subscribe_Topic();


    // 5. 【新增】订阅云端下发命令的主题
    MQTT_Subscribe_Command_Topic();


    while (1)
    {
       

        // 检查您的驱动提供的“新数据”标志/计数器
        if (xUSART.USART1ReceivedNum > 0)
        {
            // 【关键步骤】: 在使用 strstr 这类字符串函数之前，
            // 必须手动在数据末尾添加一个字符串结束符 '\0'，
            // 这样 C 语言才能知道字符串在哪里结束，防止内存越界读取。
            xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';

            // 现在可以安全地使用字符串函数来查找我们关心的内容了
            // 注意：所有的 g_usart1_rx_buffer 都被替换成了 (char*)xUSART.USART1ReceivedBuffer
            if (strstr((char*)xUSART.USART1ReceivedBuffer, "+QMTRECV:") != NULL && 
                strstr((char*)xUSART.USART1ReceivedBuffer, "/cmd/request/") != NULL)
            {
                char* p_start;
                char* p_end;
                char cmdId[32] = {0};

                // a. 解析 cmdId
                p_start = strstr((char*)xUSART.USART1ReceivedBuffer, "/cmd/request/");
                if (p_start)
                {
                    p_start += strlen("/cmd/request/");
                    p_end = strchr(p_start, '\"');
                    if (p_end)
                    {
                        strncpy(cmdId, p_start, p_end - p_start);
                    }
                }

                // b. 解析命令内容并执行动作
                if (strstr((char*)xUSART.USART1ReceivedBuffer, "\"LED_ON\""))
                {
                    LED1_ON;
                    if (strlen(cmdId) > 0)
                    {
                        MQTT_Publish_Command_Response(cmdId, "LED has been turned ON");
                    }
                }
                else if (strstr((char*)xUSART.USART1ReceivedBuffer, "\"LED_OFF\""))
                {
                    LED1_OFF;
                    if (strlen(cmdId) > 0)
                    {
                        MQTT_Publish_Command_Response(cmdId, "LED has been turned OFF");
                    }
                }
                else
                {
                    if (strlen(cmdId) > 0)
                    {
                        MQTT_Publish_Command_Response(cmdId, "Unknown command");
                    }
                }
            }
            
            // 【重要】: 数据处理完毕后，必须按照驱动的设计，将接收字节数清零，
            // 这样驱动才能在下次接收新数据时从头开始填充缓冲区。
            xUSART.USART1ReceivedNum = 0;
        }

        
    }
}
#ifndef __MQTT_H
#define __MQTT_H

#include <stdint.h> // 为了在头文件中使用 uint32_t 类型

/*
 ===============================================================================
                            函数原型声明
 ===============================================================================
 * 以下是本模块对外提供的所有功能函数。
 * 外部文件（如 main.c）只需要 #include "mqtt_handler.h" 就可以调用它们。
*/

/**
 * @brief  初始化AT指令并连接到MQTT服务器
 * @note   该函数会按顺序发送一系列AT指令来建立网络附着和MQTT连接。
 */
void Initialize_And_Connect_MQTT(void);

/**
 * @brief 订阅数据点响应主题
 * @note  用于接收云平台对数据上报的确认响应。
 */
void MQTT_Subscribe_Topic(void);

/**
 * @brief 发布温湿度数据到云平台
 * @param temperature 整数形式的温度值 (例如 253 代表 25.3 度)
 * @param humidity    整数形式的湿度值 (例如 605 代表 60.5 %)
 */
void MQTT_Publish_Data(int temperature, int humidity);

/**
 * @brief 订阅云端下发命令的主题
 * @note  这是接收云平台指令的关键，必须在连接成功后调用。
 */
void MQTT_Subscribe_Command_Topic(void);
void MQTT_Publish_Command_Response(const char* cmdId, const char* response_msg);

#endif /* __MQTT_HANDLER_H */
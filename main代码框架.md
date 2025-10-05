该程序整体框架遵循一个典型的物联网设备工作流程：

1.  **初始化**：
    *   **系统初始化**：配置微控制器（MCU）的核心时钟、中断、调试接口等。
    *   **外设初始化**：初始化用于通信的串口（USART1 用于和蜂窝模块通信，USART2 可能用于调试）和用于状态指示的 LED。
    *   **网络和MQTT初始化**：通过 `Robust_Initialize_And_Connect_MQTT` 函数，使用 AT 指令序列来配置蜂窝模块，使其附着到 GPRS 网络，并最终连接到 OneNET 的 MQTT 服务器。

2.  **订阅主题**：
    *   连接成功后，通过 `MQTT_Subscribe_All_Topics` 函数订阅 OneNET 平台定义的多个 MQTT 主题，用于接收云端下发的指令，如属性设置、服务调用等。

3.  **主循环 (Super Loop)**：
    *   程序进入一个无限循环 `while(1)`。
    *   **接收数据处理**：在循环中，不断检查串口接收缓冲区，看是否有来自云平台通过蜂窝模块转发过来的消息。如果有，则调用 `Process_MQTT_Message` 函数进行解析和处理。
    *   **发送数据**：在循环中，周期性地调用数据上报函数（如 `MQTT_Publish_Two_Temps_Random`）来模拟传感器数据并将其发送到云平台。

### **全局变量和宏定义**

在深入函数之前，先了解一下关键的全局配置：

*   **`g_cmd_buffer[4096]`**: 一个静态字符数组，用作缓冲区，用于构建要发送给蜂窝模块的 AT 指令字符串。
*   **`g_json_payload[4096]`**: 一个静态字符数组，用于构建要作为 MQTT 消息负载（Payload）的 JSON 格式数据。
*   **`g_message_id`**: 一个无符号整型变量，作为 MQTT 消息的唯一标识符。每次发送消息时会递增，有助于追踪消息。
*   **`g_crop_stage` / `g_intervention_status`**: 全局变量，用于存储从云端同步下来的设备状态，如“作物生长时期”和“人工干预状态”。
*   **`MQTT_PRODUCT_ID` / `MQTT_DEVICE_NAME` / `MQTT_PASSWORD_SIGNATURE`**: MQTT 连接三元组。这是设备连接到 OneNET 平台的身份凭证，是代码中最关键的配置信息。

---

### **函数功能详解**

#### **1. 辅助与底层函数**

*   **`static void delay_ms(uint32_t ms)`**
    *   **作用**：提供一个毫秒级的延时。它封装了底层的 `System_DelayMS` 函数，该函数通常基于 SysTick 定时器实现，比简单的软件循环延时更精确。

*   **`bool MQTT_Send_AT_Command(const char* cmd, const char* expected_response, uint32_t timeout_ms)`**
    *   **作用**：这是整个程序与蜂窝模块通信的核心函数。它负责发送一条 AT 指令，并在指定的超时时间内等待模块返回一个预期的响应。
    *   **参数**：
        *   `cmd`: 要发送的 AT 指令字符串。
        *   `expected_response`: 期望在模块的回复中找到的关键字，用于判断指令是否执行成功（例如 "OK"）。
        *   `timeout_ms`: 等待响应的超时时间（毫秒）。
    *   **返回值**：`true` 表示在超时时间内收到了预期的响应，指令成功；`false` 表示超时或未收到预期响应，指令失败。
    *   **实现逻辑**：先清空串口接收缓冲区，然后发送指令，并在一个 `while` 循环中检查是否收到数据以及数据中是否包含期望的响应字符串。循环的退出条件是时间超过 `timeout_ms`。

#### **2. 初始化与连接函数**

*   **`bool Robust_Initialize_And_Connect_MQTT(void)`**
    *   **作用**：执行一整套 AT 指令序列，完成从模块上电后到成功连接上 OneNET MQTT 服务器的全部流程。这是一个关键的初始化步骤。
    *   **执行步骤**：
        1.  `AT`：测试模块是否正常响应。
        2.  `AT+CIMI`：获取 SIM 卡信息，检查 SIM 卡是否在位。
        3.  `AT+CGATT=1`：设置模块附着到 GPRS 网络。
        4.  `AT+CGATT?`：查询 GPRS 附着状态，确保成功。
        5.  `AT+QMTCFG="version",0,4`：配置 MQTT 协议版本为 3.1.1。
        6.  `AT+QMTOPEN`：建立一个到 OneNET MQTT 服务器的 TCP 连接。
        7.  `AT+QMTCONN`：使用预设的 MQTT 三元组进行身份验证，正式连接到 MQTT Broker。
    *   **返回值**：`true` 表示所有步骤都成功执行；`false` 表示其中任何一步失败，连接过程终止。

#### **3. 数据上报（发布）函数**

这些函数负责将本地数据打包成 JSON 格式，并通过 MQTT `PUBLISH` 指令发送到云平台。

*   **`void MQTT_Publish_All_Properties(...)`**
    *   **作用**：将多达13个不同的属性值（环境温湿度、监测点温度、设备状态等）打包成一个符合 OneNET 物模型标准的 JSON 字符串，并发布到属性上报主题（`$sys/.../thing/property/post`）。

*   **`void MQTT_Post_Frost_Alert_Event(float current_temp)`**
    *   **作用**：当检测到霜冻风险时，调用此函数上报一个“霜冻告警”事件。它会构建一个特定的 JSON 负载，并发布到事件上报主题（`$sys/.../thing/event/post`）。

*   **`void MQTT_Publish_Temp1_Temp2(int temp1, int temp2)`**
    *   **作用**：仅上报 `temp1` 和 `temp2` 两个属性。这个函数是 `MQTT_Publish_All_Properties` 的一个精简版，用于演示部分属性上报。它使用了 `snprintf` 来构建字符串，比 `sprintf` 更安全，可以防止缓冲区溢出。

*   **`void MQTT_Publish_Temp3_Temp4(int temp3, int temp4)`**
    *   **作用**：与上一个函数类似，仅上报 `temp3` 和 `temp4` 两个属性。

*   **`void MQTT_Publish_Only_Temperatures(int temp1, int temp2, int temp3, int temp4)`**
    *   **作用**：仅上报四个监测点的温度属性。这可能是为了解决一次性上报所有属性导致 AT 指令过长的问题，通过分包发送来提高可靠性。

*   **`..._Random()` 系列函数 (`MQTT_Publish_All_Properties_Random`, `MQTT_Publish_Temp1_Temp2_Random`, etc.)**
    *   **作用**：这些都是测试函数。它们内部使用 `rand()` 函数生成随机的模拟传感器数据，然后调用对应的上报函数将这些数据发送出去。在实际产品中，这些函数会被替换为从真实传感器读取数据的代码。

#### **4. 消息订阅函数**

这些函数让设备“告诉”云平台它关心哪些消息，当云平台有相关消息时，就会推送给设备。

*   **`void MQTT_Subscribe_Command_Topic(void)`**: 订阅旧版命令下发主题 (`.../cmd/request/+`)。
*   **`void MQTT_Subscribe_Property_Set_Topic(void)`**: 订阅物模型中的“属性设置”主题 (`.../thing/property/set`)。
*   **`void MQTT_Subscribe_Service_Invoke_Topic(void)`**: 订阅物模型中的“服务调用”主题 (`.../thing/service/invoke`)。
*   **`void MQTT_Subscribe_All_Topics(void)`**
    *   **作用**：一个集合函数，调用上述所有独立的订阅函数，一次性完成所有必要主题的订阅。这简化了主函数中的逻辑。

#### **5. 云端交互与数据解析函数**

*   **`void MQTT_Get_Desired_Crop_Stage(void)`**
    *   **作用**：主动向云平台发送一个请求，以获取之前在云端为设备设置的“作物生长时期 (`crop_stage`)”的期望值。这常用于设备开机时与云端状态同步。
    *   **注意**：这个函数只负责发送请求，真正的结果会通过一条新的下行 MQTT 消息返回，需要在消息处理函数中进行解析。

*   **`int find_and_parse_json_int(const char* buffer, const char* key, int* result)`**
    *   **作用**：一个非常实用的辅助函数。它在一个包含 JSON 格式的字符串中，安全地查找一个指定的键（key），并解析出其对应的整数值。
    *   **特点**：它不依赖任何外部 JSON 解析库，通过字符串查找 (`strstr`) 和标准库函数 (`strtol`) 实现，代码健壮，能处理一些格式上的小问题（如多余的空格）。

*   **`void Process_MQTT_Message_Robust(const char* buffer)`**
    *   **作用**：这是处理所有从云端接收到的 MQTT 消息的统一入口。它会分析接收到的数据属于哪种类型（属性设置、服务调用、获取期望值回复等），然后调用 `find_and_parse_json_int` 解析出关键参数，并根据这些参数更新全局变量或执行相应操作。

#### **6. 主函数**

*   **`int main(void)`**
    *   **作用**：程序的入口点和主控制流程。
    *   **执行流程**：
        1.  执行系统和外设的初始化。
        2.  调用 `Robust_Initialize_And_Connect_MQTT()` 尝试连接到云平台。
        3.  如果连接成功：
            *   点亮 LED1 作为成功指示。
            *   调用 `MQTT_Subscribe_All_Topics()` 订阅所有主题。
            *   调用 `MQTT_Get_Desired_Crop_Stage()` 主动同步一次云端状态。
            *   进入 `while(1)` 主循环。
        4.  在主循环中：
            *   不断检查串口是否接收到来自云端的数据。如果收到，则调用 `Process_MQTT_Message` 处理。
            *   周期性地调用 `MQTT_Publish_Two_Temps_Random()` 上报模拟数据。
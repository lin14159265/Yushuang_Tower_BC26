# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32F103RC IoT project that connects to the OneNET cloud platform using a BC26 NB-IoT module. The main functionality includes:
- Hardware initialization (LED, USART, etc.)
- IoT module communication via AT commands with MQTT protocol
- Data publishing to OneNET cloud platform
- Command reception and processing from the cloud
- Real-time sensor data simulation and transmission

## Development Environment

- **IDE**: Keil MDK-ARM (µVision)
- **Target**: STM32F103RC microcontroller (Cortex-M3, 256KB Flash, 48KB RAM)
- **IoT Module**: BC26 NB-IoT module
- **Cloud Platform**: OneNET IoT platform
- **Debugger**: J-Link or ST-Link

## Build Commands

### Building the Project
1. Open `Project/STM32F103_OLED.uvprojx` in Keil µVision
2. Build using F7 or the Build button
3. Flash to device using F8 or Download button

### Cleaning Build Files
Run the batch file `删除编译文件减少体积(不影响原代码).bat` to remove all build artifacts:
```bash
# This script removes:
# - Object files (*.o, *.obj)
# - Listing files (*.lst, *.map)
# - Executable files (*.axf)
# - Debugger logs (JLinkLog.txt)
# - Temporary files (*.tmp, *.bak)
```

## Architecture Overview

### Core Components

1. **Main Application** (`User/main.c`):
   - IoT module initialization and MQTT connection via `Initialize_And_Connect_MQTT()`
   - Real-time command processing from cloud with command ID tracking
   - LED control through cloud commands ("LED_ON", "LED_OFF")
   - Buffer management for USART communication

2. **MQTT Handler** (`bsp/MQTT/bsp_MQTT.c`):
   - OneNET platform integration with device authentication
   - AT command sequence for network attachment and MQTT connection
   - Topic subscription for data points and command responses
   - Data publishing with integer scaling (temperature/humidity)

3. **USART Driver** (`bsp/USART/bsp_usart.c`):
   - Multi-USART support (USART1-5, UART4-5)
   - Interrupt-driven reception with idle line detection
   - Global structure `xUSART` for buffer management
   - Configurable buffer sizes and automatic null-termination

4. **System Layer** (`System/system_f103.h`):
   - GPIO, NVIC, EXTI initialization helpers
   - Hardware abstraction with bit-band operations
   - SysTick-based delay functions (ms/us precision)
   - Internal flash data access functions

## Hardware Connections

- **BC26 Module**: USART1 (PA9/TX, PA10/RX)
- **Debug Output**: USART2 (PA2/TX, PA3/RX)
- **Status LED**: PC13 (active low)
- **Additional Pins**: Customizable through BSP modules

## Key Implementation Details

### USART Communication Pattern
The project uses a sophisticated USART handling pattern:
```c
// Check for new data
if (xUSART.USART1ReceivedNum > 0) {
    // Null-terminate the buffer for string operations
    xUSART.USART1ReceivedBuffer[xUSART.USART1ReceivedNum] = '\0';

    // Process MQTT messages or commands
    if (strstr((char*)xUSART.USART1ReceivedBuffer, "+QMTRECV:") != NULL) {
        // Handle incoming MQTT messages
    }

    // Clear the buffer after processing
    xUSART.USART1ReceivedNum = 0;
}
```

### MQTT Command Processing
Cloud commands are processed with command ID tracking for response correlation:
```c
// Extract command ID from MQTT topic
p_start = strstr((char*)xUSART.USART1ReceivedBuffer, "/cmd/request/");
if (p_start) {
    p_start += strlen("/cmd/request/");
    p_end = strchr(p_start, '\"');
    if (p_end) {
        strncpy(cmdId, p_start, p_end - p_start);
    }
}
```

## Development Workflow

### Adding New Sensors
1. Create sensor driver in `bsp/` following existing pattern
2. Add include in `User/hardware.h`
3. Initialize in `main()` function
4. Integrate data publishing in main loop using `MQTT_Publish_Data()`

### Modifying Cloud Connection
Update MQTT connection parameters in `bsp/MQTT/bsp_MQTT.c`:
- Device credentials and authentication
- MQTT topics and QoS settings
- AT command timeout values

### Debugging AT Commands
Monitor USART2 output for AT command responses:
```c
// Enable debug output to see AT command flow
USART2_Init(115200);
printf("AT Command: %s\r\n", at_command);
```

## Code Conventions

### Naming Patterns
- **Functions**: PascalCase for public functions (e.g., `Initialize_And_Connect_MQTT`)
- **Variables**: camelCase for local variables, snake_case for globals
- **Constants**: UPPER_CASE for macros and constants
- **Files**: snake_case with module prefix (e.g., `bsp_usart.c`)

### Error Handling
- All functions have appropriate return value checking
- AT commands include timeout mechanisms
- USART operations include buffer overflow protection
- Hardware initialization uses standard STM32 error codes

### Memory Management
- Fixed-size buffers for USART communication
- Stack-allocated variables where possible
- Dynamic allocation avoided in real-time sections
- Buffer sizes defined as macros for easy modification
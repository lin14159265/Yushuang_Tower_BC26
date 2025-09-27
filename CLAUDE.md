# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a professional STM32F103RC IoT project implementing BC26 NB-IoT module connectivity to the OneNET cloud platform. The architecture follows a layered design with comprehensive hardware abstraction, robust communication protocols, and production-ready error handling.

## Build System

**Primary Tool**: Keil MDK-ARM (µVision) 5.x
- **Project File**: `Project/STM32F103_OLED.uvprojx`
- **Build Output**: `Project/Output/` directory
- **Target**: STM32F103RC (Cortex-M3, 256KB Flash, 48KB RAM)
- **Toolchain**: ARMCC with C99 standard, Level 1 optimization

**Build Commands**:
- Open project in Keil µVision and build (F7)
- Clean build files: Run `删除编译文件减少体积(不影响原代码).bat`

## Architecture

### Layered Structure
- **Application Layer** (`User/`): IoT business logic, MQTT client, data publishing
- **BSP Layer** (`bsp/`): Hardware abstraction for LEDs, USART, sensors
- **System Layer** (`System/`): Core utilities, GPIO/NVIC helpers, delay functions
- **HAL Layer** (`Libraries/`): STM32 Standard Peripheral Library + CMSIS

### Key Design Patterns
- **Interrupt-driven USART** with 1024-byte buffers and idle detection
- **Timeout-managed AT command execution** with retry logic
- **Frame-based communication** using global `xUSART` structure
- **Modular peripheral drivers** with consistent initialization patterns

## Core Components

### 1. IoT Communication Stack (`User/main.c`)
- **AT Command Engine**: `send_cmd()` with timeout/retry, `wait_for_rsp()` for response handling
- **MQTT Client**: OneNET platform integration with JSON data formatting
- **Network Management**: Automatic attachment monitoring with CSQ signal quality checks
- **Bidirectional Communication**: 15-second data publishing + cloud command processing

**Critical Functions**:
- `send_cmd(cmd, expected_rsp, timeout)` - Reliable AT command execution
- `wait_for_rsp(expected_rsp, timeout)` - Response waiting mechanism
- `parse_command(buffer)` - Cloud-to-device command parsing

### 2. USART Driver (`bsp/USART/`)
- **Multi-UART Support**: USART1-3, UART4-5 with individual buffers
- **Interrupt Architecture**: Idle line detection for frame boundaries
- **Global State**: `xUSART` structure tracks buffer status and received data
- **Configuration**: 115200 baud, 8N1, configurable buffer sizes (default 1024 bytes)

### 3. System Utilities (`System/system_f103.h`)
- **GPIO Abstraction**: `System_GPIOSet()` for standardized pin configuration
- **Bit-band Operations**: 51-style direct access (PAout(n), PBin(n), etc.)
- **Delay Functions**: `System_DelayMS()`, `System_DelayUS()` using SysTick
- **Type System**: Standardized types (u8, u16, s32, etc.)

## Hardware Configuration

**Pin Assignments**:
- **USART1 (BC26)**: PA9 (TX), PA10 (RX)
- **USART2 (Debug)**: PA2 (TX), PA3 (RX)
- **LEDs**: PB1, PB8, PB9, PC13
- **SWD Debug**: PA13, PA14

**Memory Map**:
- **Flash**: 0x08000000 - 0x08040000 (256KB)
- **RAM**: 0x20000000 - 0x2000C000 (48KB)

## Development Workflow

### Building and Flashing
1. Open `Project/STM32F103_OLED.uvprojx` in Keil µVision
2. Build project (F7)
3. Flash to target via ST-Link/J-Link
4. Monitor debug output on USART2 (115200 baud, 8N1)

### IoT Configuration
**Cloud Parameters** (in `User/main.c`):
- `DEVICE_NAME`: "test"
- `PRODUCT_ID`: "nZ4v9G1iDK"
- `AUTH_INFO`: OneNET authentication token
- `PUB_TOPIC`/`SUB_TOPIC`: OneNET standard MQTT topics

### Adding New Sensors/Peripherals
1. Create BSP module in `bsp/` following existing pattern
2. Implement initialization and data acquisition functions
3. Add to `User/hardware.h` includes
4. Initialize in `main()` and integrate into data publishing loop

### AT Command Testing
- Use `send_cmd()` function for reliable command execution
- Monitor responses via USART2 debug output
- Leverage existing timeout and retry mechanisms
- Test network attachment with `AT+CGATT?` and signal quality with `AT+CSQ`

## Communication Protocols

### AT Command Flow
1. Clear receive buffer and set waiting flag
2. Send command via USART1
3. Wait for expected response or timeout
4. Handle errors and retries automatically
5. Clear waiting flag for main loop URC processing

### MQTT Data Publishing
1. Format JSON data with incrementing message ID
2. Send `AT+QMTPUB` command and wait for '>'
3. Transmit JSON payload directly
4. Wait for "OK" confirmation
5. 15-second publishing cycle with error handling

## Error Handling and Debugging

### Built-in Diagnostics
- **Timeout Detection**: All AT commands have configurable timeouts
- **Error Response Handling**: Automatic detection of "ERROR" responses
- **Buffer Monitoring**: Debug output of receive buffer contents on timeout
- **Network Status**: Continuous attachment monitoring with signal quality checks

### Debug Output
- USART2 provides comprehensive AT command interaction logging
- Buffer contents printed on timeout for debugging
- Network attachment progress and error reporting
- Cloud command processing feedback

## Maintenance

### Code Quality
- Consistent Chinese documentation throughout
- Modular architecture with clear layer separation
- Comprehensive error handling and retry logic
- Professional build configuration and output management

### Project Cleanup
- Use `删除编译文件减少体积(不影响原代码).bat` to clean build artifacts
- Removes object files, listings, debug files, and temporary build products
- Preserves source code and project configuration
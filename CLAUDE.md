# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32F103 IoT project that connects to the OneNET cloud platform using a BC26 NB-IoT module. The main functionality includes:
- Hardware initialization (LED, USART, etc.)
- IoT module communication via AT commands
- Data publishing to OneNET cloud platform
- Command reception and processing from the cloud

## Code Structure

- `User/` - Main application code including main.c with IoT logic
- `bsp/` - Board Support Packages for peripherals (LED, key, OLED, USART)
- `System/` - System-level functions and scheduler
- `Libraries/` - STM32 standard peripheral library
- `Project/` - Keil project files

## Key Components

1. **Main Application** (`User/main.c`):
   - IoT module initialization and connection
   - AT command handling with timeout mechanism
   - Periodic data publishing to cloud
   - Command processing from cloud

2. **USART Driver** (`bsp/USART/`):
   - Multi-USART support with interrupt-driven reception
   - Idle line detection for frame-based reception
   - Configurable buffer sizes

3. **System Functions** (`System/system_f103.h`):
   - GPIO, NVIC, EXTI initialization helpers
   - Delay functions using SysTick
   - Bit-band operations for direct pin access

## Development Environment

- **IDE**: Keil MDK-ARM (µVision)
- **Target**: STM32F103RC microcontroller
- **IoT Module**: BC26 NB-IoT module
- **Cloud Platform**: OneNET IoT platform

## Hardware Connections

- BC26 module connected via USART1 (PA9/PA10)
- Debug/monitor output via USART2 (PA2/PA3)
- LED indicators for status

## Common Development Tasks

### Building the Project
Open the project file `Project/STM32F103_OLED.uvprojx` in Keil µVision and build using the IDE.

### Modifying IoT Functionality
- Cloud connection parameters in `User/main.c` (DEVICE_NAME, PRODUCT_ID, AUTH_INFO)
- Data publishing format in the main loop
- Command handling in `parse_command()` function

### Adding New Peripherals
- Create new BSP modules following the existing pattern in `bsp/`
- Include header files in `User/hardware.h`
- Initialize in `main()` function

### Testing AT Commands
- Modify the main loop to send custom AT commands
- Use the `send_cmd()` function for reliable AT command execution
- Check responses via USART2 debug output
TARGET = main
DEBUG = 1
OPT = -Og

BUILD_DIR = build
PREFIX = arm-none-eabi-
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

C_SOURCES =  \
User/main.c\
User/stm32f10x_it.c\
bsp/LED/bsp_led.c\
bsp/USART/bsp_usart.c\
bsp/key/bsp_key.c\
System/system_f103.c\
Libraries/CMSIS/core_cm3.c\
Libraries/CMSIS/system_stm32f10x.c\
Libraries/FWlib/src/misc.c\
Libraries/FWlib/src/stm32f10x_adc.c\
Libraries/FWlib/src/stm32f10x_bkp.c\
Libraries/FWlib/src/stm32f10x_can.c\
Libraries/FWlib/src/stm32f10x_dac.c\
Libraries/FWlib/src/stm32f10x_dbgmcu.c\
Libraries/FWlib/src/stm32f10x_dma.c\
Libraries/FWlib/src/stm32f10x_exti.c\
Libraries/FWlib/src/stm32f10x_flash.c\
Libraries/FWlib/src/stm32f10x_gpio.c\
Libraries/FWlib/src/stm32f10x_i2c.c\
Libraries/FWlib/src/stm32f10x_iwdg.c\
Libraries/FWlib/src/stm32f10x_pwr.c\
Libraries/FWlib/src/stm32f10x_rcc.c\
Libraries/FWlib/src/stm32f10x_rtc.c\
Libraries/FWlib/src/stm32f10x_sdio.c\
Libraries/FWlib/src/stm32f10x_spi.c\
Libraries/FWlib/src/stm32f10x_tim.c\
Libraries/FWlib/src/stm32f10x_usart.c\
Libraries/FWlib/src/stm32f10x_wwdg.c\

ASM_SOURCES =  \
startup_stm32f103xe.s

CPU = -mcpu=cortex-m3

MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

AS_DEFS = 

C_DEFS =  \
-DARM_GCC \
-DSTM32F10X_HD \
-DSTM32F10X_HD\
-DUSE_STDPERIPH_DRIVER\

AS_INCLUDES =

C_INCLUDES =  \
-IUser\
-ISystem\
-ILibraries/CMSIS\
-ILibraries/CMSIS/startup\
-ILibraries/FWlib/inc\
-ILibraries/FWlib/src\
-Ibsp/w25qxx\
-Ibsp/CAN\
-Ibsp/key\
-Ibsp/LCD_2.8_ILI9341\
-Ibsp/LED\
-Ibsp/USART\
-Ibsp/XPT2046\
-Ibsp/ESP8266\
-Ibsp/RS485\
-Ibsp/USART2\

ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
CFLAGS+=-std=c11
endif

CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

LDSCRIPT = STM32F103RCTx_FLASH.ld

LIBS = -lc -lm -lnosys -u _printf_float -u _scanf_float
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo build $@
	@$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo build $@
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@echo build $@
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@

clean:
	-rm -fR $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

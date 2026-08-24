PROJECT := UART_To_LoRa
BUILD_DIR ?= build
ROOT := ../../../../..

CC := arm-none-eabi-gcc
AS := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
DEFINES := -DCORE_CM4 -DUSE_HAL_DRIVER -DSTM32WL55xx
INCLUDES := \
	-I./Core/Inc \
	-I./SubGHz_Phy/App \
	-I./SubGHz_Phy/Target \
	-I$(ROOT)/Utilities/trace/adv_trace \
	-I$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Inc \
	-I$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Inc/Legacy \
	-I$(ROOT)/Utilities/misc \
	-I$(ROOT)/Utilities/sequencer \
	-I$(ROOT)/Utilities/timer \
	-I$(ROOT)/Utilities/lpm/tiny_lpm \
	-I$(ROOT)/Drivers/CMSIS/Device/ST/STM32WLxx/Include \
	-I$(ROOT)/Middlewares/Third_Party/SubGHz_Phy \
	-I$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver \
	-I$(ROOT)/Drivers/CMSIS/Include \
	-I$(ROOT)/Drivers/BSP/STM32WLxx_Nucleo

CFLAGS := $(CPU_FLAGS) $(DEFINES) $(INCLUDES) -std=gnu11 -Os -g0 \
	-ffunction-sections -fdata-sections -Wall -Wextra -Werror=implicit-function-declaration
ASFLAGS := $(CPU_FLAGS) -x assembler-with-cpp
LDSCRIPT := $(ROOT)/Projects/NUCLEO-WL55JC/Applications/SubGHz_Phy/SubGHz_Phy_PingPong/STM32CubeIDE/STM32WL55JCIX_FLASH.ld
LDFLAGS := $(CPU_FLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs \
	-Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map -Wl,--cref

C_SOURCES := \
	Core/Src/main.c \
	Core/Src/gpio.c \
	Core/Src/dma.c \
	Core/Src/rtc.c \
	Core/Src/subghz.c \
	Core/Src/stm32_lpm_if.c \
	Core/Src/sys_app.c \
	Core/Src/sys_debug.c \
	Core/Src/timer_if.c \
	Core/Src/usart_if.c \
	Core/Src/usart.c \
	Core/Src/stm32wlxx_it.c \
	Core/Src/stm32wlxx_hal_msp.c \
	Core/Src/system_stm32wlxx.c \
	SubGHz_Phy/App/app_subghz_phy.c \
	SubGHz_Phy/App/subghz_phy_app.c \
	SubGHz_Phy/App/lora_transparent_at.c \
	SubGHz_Phy/App/subg_command.c \
	SubGHz_Phy/Target/radio_board_if.c \
	STM32CubeIDE/syscalls.c \
	STM32CubeIDE/sysmem.c \
	$(ROOT)/Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo.c \
	$(ROOT)/Drivers/BSP/STM32WLxx_Nucleo/stm32wlxx_nucleo_radio.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_cortex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_dma_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_exti.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_flash_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_gpio.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_pwr_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rcc_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rtc.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_rtc_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_subghz.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_tim.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_tim_ex.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart.c \
	$(ROOT)/Drivers/STM32WLxx_HAL_Driver/Src/stm32wlxx_hal_uart_ex.c \
	$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/lr_fhss_mac.c \
	$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio.c \
	$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_driver.c \
	$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/radio_fw.c \
	$(ROOT)/Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/wl_lr_fhss.c \
	$(ROOT)/Utilities/trace/adv_trace/stm32_adv_trace.c \
	$(ROOT)/Utilities/misc/stm32_mem.c \
	$(ROOT)/Utilities/misc/stm32_systime.c \
	$(ROOT)/Utilities/misc/stm32_tiny_sscanf.c \
	$(ROOT)/Utilities/misc/stm32_tiny_vsnprintf.c \
	$(ROOT)/Utilities/sequencer/stm32_seq.c \
	$(ROOT)/Utilities/timer/stm32_timer.c \
	$(ROOT)/Utilities/lpm/tiny_lpm/stm32_lpm.c

ASM_SOURCE := $(ROOT)/Drivers/CMSIS/Device/ST/STM32WLxx/Source/Templates/gcc/startup_stm32wl55xx_cm4.s
OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o))) $(BUILD_DIR)/startup_stm32wl55xx_cm4.o
VPATH := $(sort $(dir $(C_SOURCES))) $(dir $(ASM_SOURCE))

.PHONY: all clean
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin
	$(SIZE) $(BUILD_DIR)/$(PROJECT).elf

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/startup_stm32wl55xx_cm4.o: $(ASM_SOURCE) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJECTS:.o=.d)

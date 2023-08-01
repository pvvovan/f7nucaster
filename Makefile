TARGET = f7nucaster

# debug build?
DEBUG = 1

# Build path
BUILD_DIR = build

# C sources
C_SOURCES = \
Src/main.c \
Src/Helper.c \
Src/lwip.c \
Src/TcpServer.c \
Src/ClientQueue.c \
Src/ringbuffer_dma.c \
Src/system_stm32f7xx.c \
Src/ethernetif.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_iwdg.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_eth.c \
STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c \
LwIP/src/core/init.c \
LwIP/src/netif/ethernet.c \
LwIP/src/core/mem.c \
LwIP/src/core/pbuf.c \
LwIP/src/core/tcp.c \
LwIP/src/core/memp.c \
LwIP/src/core/netif.c \
LwIP/src/core/timeouts.c \
LwIP/src/core/udp.c \
LwIP/src/core/def.c \
LwIP/src/core/tcp_out.c \
LwIP/src/core/tcp_in.c \
LwIP/src/core/ip.c \
LwIP/src/core/ipv4/dhcp.c \
LwIP/src/core/ipv4/etharp.c \
LwIP/src/core/ipv4/ip4_addr.c \
LwIP/src/core/ipv4/ip4.c \
LwIP/src/core/ipv4/ip4_frag.c \
LwIP/src/core/ipv4/icmp.c

# ASM sources
ASM_SOURCES = \
startup_stm32f746xx.s


#######################################
# binaries
#######################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
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


#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m7

# fpu
FPU = -mfpu=fpv5-sp-d16

# float-abi
FLOAT-ABI = -mfloat-abi=hard

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS = \
-D USE_HAL_DRIVER \
-D STM32F746xx

# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES = \
-I Inc \
-I STM32F7xx_HAL_Driver/Inc \
-I CMSIS/Include \
-I CMSIS/Device/ST/STM32F7xx/Include \
-I LwIP/src/include \
-I LwIP/system

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) -Wall -Wextra -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) -Wall -Wextra -fdata-sections -ffunction-sections

ifeq ($(DEBUG), 1)
CFLAGS += -g3 -gdwarf-5 -O0
else
CFLAGS += -g0 -gdwarf-2 -O2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"


#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F746ZGTx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin


#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	mkdir $@		


#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)


#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***

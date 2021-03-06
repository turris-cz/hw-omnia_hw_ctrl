APP_NAME=omnia_hw_ctrl
BOOT_NAME=bootloader_mcu

################################################################################
#                   SETUP TOOLS                                                #
################################################################################

CROSS_COMPILE ?= armv6m-softfloat-eabi-

CC      = $(CROSS_COMPILE)gcc
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
AS      = $(CROSS_COMPILE)as
SIZE	= $(CROSS_COMPILE)size

##### Preprocessor options
FW_VERSION = "{ $(shell git rev-parse HEAD | sed 's/\(..\)/0x\1, /g' | sed -r 's/,\s+$$//') }"

#defines needed when working with the STM peripherals library
DEFS 	= -DSTM32F030R8T6
DEFS   += -DSTM32F030X8
DEFS   += -DUSE_STDPERIPH_DRIVER
DEFS   += -D__ASSEMBLY_
DEFS   += -DVERSION=$(FW_VERSION)

##### Assembler options

AFLAGS  = -mcpu=cortex-m0 
AFLAGS += -mthumb
AFLAGS += -mlittle-endian

##### Compiler options

CFLAGS  = -ggdb
CFLAGS += -O2
CFLAGS += -Wall -Wextra -Warray-bounds #-pedantic
CFLAGS += -fno-common -ffreestanding -fno-stack-protector -fno-stack-clash-protection -fno-pie
CFLAGS += $(AFLAGS)
CFLAGS += -nostdlib
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections

##### Linker options

# tell ld which linker file to use
# (this file is in the current directory)
#  -Wl,...:     tell GCC to pass this to linker.
#    -Map:      create map file
#    --cref:    add cross reference to  map file
LFLAGS  = -T$(LINKER_DIR)/STM32F0308_FLASH.ld
LFLAGS +="-Wl,-Map=$(APP_NAME).map",--cref
LFLAGS += -nostartfiles
LFLAGS += -Xlinker --gc-sections -no-pie

BOOT_LFLAGS  = -T$(BOOT_LINKER_DIR)/STM32F0308_FLASH.ld
BOOT_LFLAGS +="-Wl,-Map=$(BOOT_NAME).map",--cref
BOOT_LFLAGS += -nostartfiles
BOOT_LFLAGS += -Xlinker --gc-sections

# directories to be searched for header files
INCLUDE = $(addprefix -I,$(INC_DIRS))

BOOT_INCLUDE = $(addprefix -I,$(BOOT_INC_DIRS))

################################################################################
#                   SOURCE FILES DIRECTORIES                                   #
################################################################################
PROJ_ROOT_DIR	= src

#APPLICATION
APP_ROOT_DIR	= $(PROJ_ROOT_DIR)/application


STM_ROOT_LIB    = $(APP_ROOT_DIR)/stm_lib/stm32f0xx_stdperiph_driver
APP_SRC_DIR		= $(APP_ROOT_DIR)/app
STM_SRC_DIR     = $(STM_ROOT_LIB)/src
STM_CMSIS_DIR 	= $(APP_ROOT_DIR)/stm_lib/cmsis_boot
STM_STARTUP_DIR = $(STM_CMSIS_DIR)/startup

LINKER_DIR = $(APP_ROOT_DIR)/linker

vpath %.c $(APP_ROOT_DIR)
vpath %.c $(APP_SRC_DIR)
vpath %.c $(STM_SRC_DIR)
vpath %.c $(STM_CMSIS_DIR)
vpath %.s $(STM_STARTUP_DIR)

################################################################################
#                   HEADER FILES DIRECTORIES                                   #
################################################################################

# The header files we use are located here
INC_DIRS += $(STM_ROOT_LIB)/inc
INC_DIRS += $(STM_CMSIS_DIR)
INC_DIRS += $(APP_SRC_DIR)
INC_DIRS += $(APP_ROOT_DIR)/stm_lib/cmsis_core

################################################################################
#                   SOURCE FILES TO COMPILE                                    #
################################################################################
SRCS  += main.c
SRCS  += system_stm32f0xx.c
SRCS  += stm32f0xx_it.c
SRCS  += debounce.c
SRCS  += led_driver.c
SRCS  += delay.c
SRCS  += power_control.c
SRCS  += msata_pci.c
SRCS  += wan_lan_pci_status.c
SRCS  += slave_i2c_device.c
SRCS  += debug_serial.c
SRCS  += app.c
SRCS  += eeprom.c

################# STM LIB ##########################
SRCS  += stm32f0xx_rcc.c
SRCS  += stm32f0xx_gpio.c
SRCS  += stm32f0xx_tim.c
SRCS  += stm32f0xx_dma.c
SRCS  += stm32f0xx_dbgmcu.c
SRCS  += stm32f0xx_exti.c
SRCS  += stm32f0xx_i2c.c
SRCS  += stm32f0xx_syscfg.c
SRCS  += stm32f0xx_misc.c
SRCS  += stm32f0xx_spi.c
SRCS  += stm32f0xx_flash.c
SRCS  += stm32f0xx_usart.c

# startup file, calls main
ASRC  = startup_stm32f030x8.s

OBJS  = $(SRCS:.c=.o)
OBJS += $(ASRC:.s=.o)

#BOOTLOADER -------------------------------------------------------------------
BOOT_ROOT_DIR		= $(PROJ_ROOT_DIR)/bootloader
BOOT_LINKER_DIR		= $(BOOT_ROOT_DIR)/linker

BOOT_STM_ROOT_LIB  	= $(APP_ROOT_DIR)/stm_lib/stm32f0xx_stdperiph_driver
BOOT_STM_SRC_DIR    = $(BOOT_STM_ROOT_LIB)/src
BOOT_STM_CMSIS_DIR 	= $(APP_ROOT_DIR)/stm_lib/cmsis_boot

BOOT_STARTUP_DIR = $(BOOT_ROOT_DIR)/startup

vpath %.c $(BOOT_ROOT_DIR)
vpath %.c $(BOOT_STM_SRC_DIR)
vpath %.c $(BOOT_STM_CMSIS_DIR)
vpath %.s $(BOOT_STARTUP_DIR)

BOOT_INC_DIRS += $(BOOT_STM_ROOT_LIB)/inc
BOOT_INC_DIRS += $(BOOT_STM_CMSIS_DIR)
BOOT_INC_DIRS += $(BOOT_ROOT_DIR)
BOOT_INC_DIRS += $(APP_ROOT_DIR)/stm_lib/cmsis_core

BOOTSRCS  += boot_main.c
BOOTSRCS  += boot_i2c.c
BOOTSRCS  += flash.c
BOOTSRCS  += boot_stm32f0xx_it.c
BOOTSRCS  += system_stm32f0xx.c
BOOTSRCS  += boot_led_driver.c
BOOTSRCS  += delay.c
BOOTSRCS  += power_control.c
BOOTSRCS  += debug_serial.c
BOOTSRCS  += eeprom.c
BOOTSRCS  += bootloader.c

BOOTSRCS  += stm32f0xx_rcc.c
BOOTSRCS  += stm32f0xx_gpio.c
BOOTSRCS  += stm32f0xx_tim.c
BOOTSRCS  += stm32f0xx_dbgmcu.c
BOOTSRCS  += stm32f0xx_i2c.c
BOOTSRCS  += stm32f0xx_syscfg.c
BOOTSRCS  += stm32f0xx_misc.c
BOOTSRCS  += stm32f0xx_spi.c
BOOTSRCS  += stm32f0xx_flash.c
BOOTSRCS  += stm32f0xx_usart.c

BOOTASRC  = boot_startup_stm32f030x8.s

BOOT_OBJS  = $(BOOTSRCS:.c=.o)
BOOT_OBJS += $(BOOTASRC:.s=.o)


################################################################################
#                         SIZE OF OUTPUT                                       #
################################################################################
APP_ELFSIZE = $(SIZE) -d $(APP_NAME).elf

app_buildsize: $(APP_NAME).elf
	@echo Program Size: 
	$(APP_ELFSIZE)

BOOT_ELFSIZE = $(SIZE) -d $(BOOT_NAME).elf

boot_buildsize: $(BOOT_NAME).elf
	@echo Program Size: 
	$(BOOT_ELFSIZE)

################################################################################
#                         SETUP TARGETS                                        #
################################################################################

.PHONY: app

all: app boot

app: $(APP_NAME).elf app_buildsize

boot: $(BOOT_NAME).elf boot_buildsize

$(APP_NAME).elf: $(OBJS)
	@echo "[Linking    ]  $@"
	@$(CC) $(CFLAGS) $(LFLAGS) $(INCLUDE) $^ -o $@
	@$(OBJCOPY) -O ihex $(APP_NAME).elf   $(APP_NAME).hex
	@$(OBJCOPY) -O binary $(APP_NAME).elf $(APP_NAME).bin
	@$(OBJDUMP) -D -S $(APP_NAME).elf >$(APP_NAME).dis

$(BOOT_NAME).elf: $(BOOT_OBJS)
	@echo "[Linking    ]  $@"
	@$(CC) $(CFLAGS) $(BOOT_LFLAGS) $(BOOT_INCLUDE) $^ -o $@
	@$(OBJCOPY) -O ihex $(BOOT_NAME).elf   $(BOOT_NAME).hex
	@$(OBJCOPY) -O binary $(BOOT_NAME).elf $(BOOT_NAME).bin
	@$(OBJDUMP) -D -S $(BOOT_NAME).elf >$(BOOT_NAME).dis

%.o : %.c
	@echo "[Compiling  ]  $^"
	$(CC) -c $(DEFS) $(CFLAGS) $(INCLUDE) $(BOOT_INCLUDE) $< -o $@

%.o : %.s 
	@echo "[Assembling ]" $^
	@$(AS) $(AFLAGS) $< -o $@

clean: cleanapp cleanboot
	rm -rf *.o

cleanapp:
	rm -rf $(APP_NAME).elf $(APP_NAME).hex $(APP_NAME).bin $(APP_NAME).map $(APP_NAME).dis
cleanboot:
	rm -rf $(BOOT_NAME).elf $(BOOT_NAME).hex $(BOOT_NAME).bin $(BOOT_NAME).map $(BOOT_NAME).dis


#********************************
# generating of the dependencies
dep:
	$(CC) $(DEFS) $(CFLAGS) $(INCLUDE) -MM $(APP_ROOT_DIR)/*.c $(APP_SRC_DIR)/*.c > dep.list

## insert generated dependencies
-include dep.list 
#*******************************


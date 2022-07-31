SRCS_PLAT_stm32	= $(wildcard src/platform/*.c)
SRCS_PLAT_stm32	+= src/platform/stm_lib/cmsis_boot/system_stm32f0xx.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_rcc.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_gpio.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_tim.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_exti.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_i2c.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_syscfg.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_misc.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_spi.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_flash.c
SRCS_PLAT_stm32	+= src/platform/stm_lib/stm32f0xx_stdperiph_driver/src/stm32f0xx_usart.c

CPPFLAGS_stm32	= -DSTM32F030X8 -DUSE_STDPERIPH_DRIVER
CPPFLAGS_stm32	+= -Isrc/platform/stm_lib/cmsis_boot
CPPFLAGS_stm32	+= -Isrc/platform/stm_lib/cmsis_core
CPPFLAGS_stm32	+= -Isrc/platform/stm_lib/stm32f0xx_stdperiph_driver/inc

CFLAGS_stm32	= -mcpu=cortex-m0 -mthumb -mlittle-endian

APP_POS_stm32	= 0x5000
CSUM_POS_stm32	= 0xC0

$(eval $(call PlatBuild,stm32))

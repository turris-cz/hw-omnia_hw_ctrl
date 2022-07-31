CROSS_COMPILE	?= arm-none-eabi-

CC		= $(CROSS_COMPILE)gcc
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
SIZE		= $(CROSS_COMPILE)size

HOSTCC		= gcc
HOSTCFLAGS	= -O2

CFLAGS		= -ggdb -Os -fno-pie -ffreestanding -Wall -Wextra -Warray-bounds -nostdlib -ffunction-sections -fdata-sections
CPPFLAGS	= -DVERSION="{ $(shell git rev-parse HEAD | sed 's/\(..\)/0x\1, /g' | sed -r 's/,\s+$$//') }"
CPPFLAGS	+= -Isrc/include -Isrc/drivers -Isrc/platform -Isrc/application
CPPFLAGS_app	= -DBOOTLOADER_BUILD=0
CPPFLAGS_boot	= -DBOOTLOADER_BUILD=1

LDSCRIPT_app	= src/application/application.ld
LDSCRIPT_boot	= src/bootloader/bootloader.ld
LDFLAGS		= -T$(LDSCRIPT) -Wl,-Map=$(LDMAP),--cref -nostartfiles -Xlinker --gc-sections

SRCS_DRIVERS	= $(filter-out src/drivers/debug.c,$(wildcard src/drivers/*.c))
SRCS_APP	= $(wildcard src/application/*.c)
SRCS_BOOT	= $(wildcard src/bootloader/*.c)

HOSTTOOLS = crc32tool genflashimg

ifeq ($(DBG_ENABLE), 1)
	SRCS_DRIVERS += src/drivers/debug.c
	CPPFLAGS += -DDBG_ENABLE=1
	DEBUG_INFO_PRINT = \
		@echo -e "\n======================================================="; \
		echo "Built with debug output enabled on MCU's UART pins!"; \
		echo "MiniPCIe/mSATA card detection and PCIe1 PLED won't work"; \
		echo -e "=======================================================\n"
	ifdef DBG_BAUDRATE
		ifeq ($(shell echo "$(DBG_BAUDRATE)" | grep -qe "[^0-9]" && echo err),err)
			CPPFLAGS += $(error Wrong value for DBG_BAUDRATE: $(DBG_BAUDRATE))
		else
			CPPFLAGS += -DDBG_BAUDRATE=$(DBG_BAUDRATE)U
		endif
	else
		CPPFLAGS += -DDBG_BAUDRATE=115200U
	endif
else
	CPPFLAGS += -DDBG_ENABLE=0
	DEBUG_INFO_PRINT =
endif

ifdef OMNIA_BOARD_REVISION
	ifeq ($(shell echo "$(OMNIA_BOARD_REVISION)" | grep -qe "[^0-9]" && echo err),err)
		CPPFLAGS += $(error Wrong value for OMNIA_BOARD_REVISION: $(OMNIA_BOARD_REVISION))
	else
		CPPFLAGS += -DOMNIA_BOARD_REVISION=$(OMNIA_BOARD_REVISION)
	endif
else
	CPPFLAGS += $(error OMNIA_BOARD_REVISION not defined)
endif

ifeq ($(USER_REGULATOR_ENABLED), 1)
	CPPFLAGS += -DUSER_REGULATOR_ENABLED=1
else
	CPPFLAGS += -DUSER_REGULATOR_ENABLED=0
endif

.PHONY: all app boot clean

all: app boot

app:
	$(DEBUG_INFO_PRINT)
boot:
	$(DEBUG_INFO_PRINT)
clean:
	rm -rf $(addprefix tools/,$(HOSTTOOLS))

define HostToolRule
  tools/$(1): tools/$(1).c
	@echo "[HOSTCC  ]" $$<
	@$(HOSTCC) $(HOSTCFLAGS) -o $$@ $$<
endef

$(foreach tool,$(HOSTTOOLS),$(eval $(call HostToolRule,$(tool))))

define CompileRule
  build.$(1)/$(2)/$(3)%.o: $(3)%.c
	@echo "$(1): [CC      ]  $$^"
	@mkdir -p $$(@D)
	@$$(CC) -c $$(CPPFLAGS) $$(CPPFLAGS_$(1)) $$(CFLAGS) $$(CFLAGS_$(1)) $$< -o $$@
endef

define PlatBuild
  SRCS_APP_$(1) = $$(SRCS_DRIVERS) $$(SRCS_APP) $$(SRCS_PLAT_$(1))
  SRCS_BOOT_$(1) = $$(SRCS_DRIVERS) $$(SRCS_BOOT) $$(SRCS_PLAT_$(1))
  OBJS_APP_$(1) = $$(addprefix build.$(1)/app/,$$(SRCS_APP_$(1):.c=.o))
  OBJS_BOOT_$(1) = $$(addprefix build.$(1)/boot/,$$(SRCS_BOOT_$(1):.c=.o))

  .PHONY: $(1) app_$(1) boot_$(1) clean_$(1)

  $(1): $(1).flash.bin
  app_$(1): $(1).app.bin build.$(1)/app.hex build.$(1)/app.dis
  boot_$(1): $(1).boot.bin build.$(1)/boot.hex build.$(1)/boot.dis

  $(1).flash.bin: app_$(1) boot_$(1) tools/genflashimg
	@echo "$(1): [GENFLASH]  $$@"
	@tools/genflashimg $(1).boot.bin $(1).app.bin $$(APP_POS_$(1)) >$$@
	@chmod +x $$@

  clean_$(1):
	rm -rf build.$(1) $(1).flash.bin $(1).app.bin $(1).boot.bin

  build.$(1)/%.hex: $(1).%.bin
	@echo "$(1): [HEX     ]  $$@"
	@$$(OBJCOPY) -I binary -O ihex $$< $$@

  build.$(1)/%.dis: build.$(1)/%.elf
	@echo "$(1): [DISASM  ]  $$@"
	@$$(OBJDUMP) -D -S $$< >$$@

  $(1).app.bin: build.$(1)/app.bin.nocrc tools/crc32tool
	@echo "$(1): [CRC32   ]  $$@"
	@tools/crc32tool $$(CSUM_POS_stm32) $$< >$$@
	@chmod +x $$@

  build.$(1)/app.bin.nocrc: build.$(1)/app.elf
	@echo "$(1): [BIN     ]  $$@"
	@$$(OBJCOPY) -O binary $$< $$@

  build.$(1)/app.elf: CPPFLAGS += $$(CPPFLAGS_app)
  build.$(1)/app.elf: LDSCRIPT = $$(LDSCRIPT_app)
  build.$(1)/app.elf: LDMAP = build.$(1)/app.map
  build.$(1)/app.elf: $$(OBJS_APP_$(1)) $$(LDSCRIPT_app)
	@echo "$(1): [LD      ]  $$@"
	@$$(CC) $$(CFLAGS) $$(CFLAGS_$(1)) $$(LDFLAGS) $$(OBJS_APP_$(1)) -o $$@
	@$$(SIZE) -d $$@

  $(1).boot.bin: build.$(1)/boot.elf
	@echo "$(1): [BIN     ]  $$@"
	@$$(OBJCOPY) -O binary $$< $$@

  build.$(1)/boot.elf: CPPFLAGS += $$(CPPFLAGS_boot)
  build.$(1)/boot.elf: LDSCRIPT = $$(LDSCRIPT_boot)
  build.$(1)/boot.elf: LDMAP = build.$(1)/boot.map
  build.$(1)/boot.elf: $$(OBJS_BOOT_$(1)) $$(LDSCRIPT_boot)
	@echo "$(1): [LD      ]  $$@"
	@$$(CC) $$(CFLAGS_$(1)) $$(LDFLAGS) $$(OBJS_BOOT_$(1)) -o $$@
	@$$(SIZE) -d $$@

  $$(foreach dir,$$(sort $$(dir $$(SRCS_APP_$(1)))),$$(eval $$(call CompileRule,$(1),app,$$(dir))))
  $$(foreach dir,$$(sort $$(dir $$(SRCS_BOOT_$(1)))),$$(eval $$(call CompileRule,$(1),boot,$$(dir))))

  all: $(1)
  app: app_$(1)
  boot: boot_$(1)
  clean: clean_$(1)
endef

include Makedefs.*.mk

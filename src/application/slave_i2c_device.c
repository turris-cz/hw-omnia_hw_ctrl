/**
 ******************************************************************************
 * @file    slave_i2c_device.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    18-August-2015
 * @brief   Driver for IC2 communication with master device (main CPU).
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "compiler.h"
#include "stm32f0xx_conf.h"
#include "slave_i2c_device.h"
#include "debug.h"
#include "led_driver.h"
#include "wan_lan_pci_status.h"
#include "power_control.h"
#include "delay.h"
#include "debounce.h"
#include "eeprom.h"
#include "msata_pci.h"
#include "i2c_slave.h"

static const uint8_t version[] = VERSION;

#define I2C_PINS_ALT_FN		1
#define I2C_SCL_PIN		PIN(F, 6)
#define I2C_SDA_PIN		PIN(F, 7)

#define I2C_SLAVE_ADDRESS		0x2a
#define I2C_SLAVE_ADDRESS_EMULATOR	0x2b

#define NUMBER_OF_BYTES_VERSION         20
#define BOOTLOADER_VERSION_ADDR         0x080000C0

static const uint16_t slave_features_supported =
#if OMNIA_BOARD_REVISION >= 32
	FEAT_PERIPH_MCU |
#endif
	FEAT_EXT_CMDS;

enum boot_request_e {
    BOOTLOADER_REQ                      = 0xAA,
    FLASH_ERROR                         = 0x55,
    FLASH_OK                            = 0x88
};

struct st_i2c_status i2c_status;

/*******************************************************************************
  * @brief  This function reads data from flash, byte after byte
  * @param  flash_address: start of selected flash area to be read
  * @param  data: data from flash
  * @retval None.
  *****************************************************************************/
static void flash_read(volatile uint32_t *flash_address, uint8_t *data)
{
   *data = *(uint8_t*)*flash_address;
    (*flash_address)++;
}

/*******************************************************************************
  * @brief  This function reads version of bootloader (stored in flash)
  * @param  flash_address: start of selected flash area to be read
  * @param  data: data from flash
  * @retval None.
  *****************************************************************************/
static void read_bootloader_version(uint8_t buff[])
{
    uint8_t idx;
    uint32_t boot_version_addr = BOOTLOADER_VERSION_ADDR;

    for(idx = 0; idx < NUMBER_OF_BYTES_VERSION; idx++)
    {
        flash_read(&boot_version_addr, &(buff[idx]));
    }
}

/*******************************************************************************
  * @function   slave_i2c_check_control_byte
  * @brief      Decodes a control byte and perform suitable reaction.
  * @param      control_byte: control byte sent from master (CPU)
  * @param      bit_mask: 0 - dont care bit, 1 - write bit
  * @retval     None.
  *****************************************************************************/
static void slave_i2c_check_control_byte(uint8_t control_byte, uint8_t bit_mask)
{
    struct st_i2c_status *i2c_control = &i2c_status;
    struct button_def *button = &button_front;
    eeprom_var_t ee_var;

    i2c_control->state = SLAVE_I2C_OK;

    if ((control_byte & CTL_LIGHT_RST) && (bit_mask & CTL_LIGHT_RST))
    {
        /* confirm received byte of I2C and reset */
        i2c_slave_ack(SLAVE_I2C, 1);
        /* set CFG_CTRL pin to high state ASAP */
        gpio_write(CFG_CTRL_PIN, 1);
        /* reset of CPU */
        gpio_write(MANRES_PIN, 0);
        return;
    }

    if ((control_byte & CTL_HARD_RST) && (bit_mask & CTL_HARD_RST))
    {
        i2c_control->state = SLAVE_I2C_HARD_RST;
        return;
    }

    if (bit_mask & CTL_USB30_PWRON)
    {
        if (control_byte & CTL_USB30_PWRON)
        {
            power_control_usb(USB3_PORT0, USB_ON);
            i2c_control->status_word |= STS_USB30_PWRON;
        }
        else
        {
            power_control_usb(USB3_PORT0, USB_OFF);
            i2c_control->status_word &= (~STS_USB30_PWRON);
        }
    }

    if (bit_mask & CTL_USB31_PWRON)
    {
        if (control_byte & CTL_USB31_PWRON)
        {
            power_control_usb(USB3_PORT1, USB_ON);
            i2c_control->status_word |= STS_USB31_PWRON;
        }
        else
        {
            power_control_usb(USB3_PORT1, USB_OFF);
            i2c_control->status_word &= (~STS_USB31_PWRON);
        }
    }

#if USER_REGULATOR_ENABLED
    if (bit_mask & CTL_ENABLE_4V5)
    {
        if (control_byte & CTL_ENABLE_4V5)
        {
            i2c_control->state = SLAVE_I2C_PWR4V5_ENABLE;
        }
        else
        {
            gpio_write(ENABLE_4V5_PIN, 0);
            i2c_control->status_word &= (~STS_ENABLE_4V5);
        }
    }
#endif

    if (bit_mask & CTL_BUTTON_MODE)
    {
        if (control_byte & CTL_BUTTON_MODE)
        {
           button->button_mode = BUTTON_USER;
           i2c_control->status_word |= STS_BUTTON_MODE;
        }
        else
        {
           button->button_mode = BUTTON_DEFAULT;
           button->button_pressed_counter = 0;
           i2c_control->status_word &= (~STS_BUTTON_MODE);
        }
    }

    if (bit_mask & CTL_BOOTLOADER)
    {
        if (control_byte & CTL_BOOTLOADER)
        {
            ee_var = EE_WriteVariable(RESET_VIRT_ADDR, BOOTLOADER_REQ);

            switch(ee_var)
            {
                case VAR_FLASH_COMPLETE:    debug("RST: OK\n"); break;
                case VAR_PAGE_FULL:         debug("RST: Pg full\n"); break;
                case VAR_NO_VALID_PAGE:     debug("RST: No Pg\n"); break;
                default:
                    break;
            }

            i2c_control->state = SLAVE_I2C_GO_TO_BOOTLOADER;
        }
    }
}

static const struct {
    gpio_t pin;
    bool inv;
    uint16_t mask;
} ext_control_pins[] = {
#define ECTRL(n, i) \
    { n ## _PIN, i, EXT_CTL_ ## n }
    ECTRL(RES_MMC, 1),
    ECTRL(RES_LAN, 1),
    ECTRL(RES_PHY, 1),
    ECTRL(PERST0, 1),
    ECTRL(PERST1, 1),
    ECTRL(PERST2, 1),
    ECTRL(PHY_SFP, 0),
    ECTRL(VHV_CTRL, 1),
#undef ECTRL
};

/*******************************************************************************
  * @function   slave_i2c_ext_control
  * @brief      Decodes an extended control word and performs suitable reaction.
  * @param      ext_control_word: extended control word sent from master (CPU)
  * @param      bit_mask: 0 - dont care bit, 1 - write bit
  * @retval     None.
  *****************************************************************************/
void slave_i2c_ext_control(uint16_t ext_control_word, uint16_t bit_mask)
{
    /* don't do anything on pre-v32 boards */
    if (OMNIA_BOARD_REVISION < 32)
        return;

    /* save the requested value */
    ext_control_word = (i2c_status.ext_control_word & ~bit_mask) |
                       (ext_control_word & bit_mask);
    i2c_status.ext_control_word = ext_control_word;

    /*
     * PHY_SFP_AUTO isn't a GPIO, rather an internal setting.
     * If set, we let PHY_SFP to be set in app.c' input_manager() according to
     * value read from SFP_NDET, so we don't change it here.
     * If not set, we want to set PHY_SFP according to value in
     * ext_control_word.
     */
    if (ext_control_word & EXT_CTL_PHY_SFP_AUTO)
        bit_mask &= ~EXT_CTL_PHY_SFP;
    else
        bit_mask |= EXT_CTL_PHY_SFP;

    for_each_const(pin, ext_control_pins)
        if (bit_mask & pin->mask)
            gpio_write(pin->pin, !!(ext_control_word & pin->mask) ^ pin->inv);
}

/*******************************************************************************
  * @function   slave_i2c_get_ext_control_status
  * @brief      Get extended control status (peripheral's resets, ...).
  * @param      None.
  * @retval     ext_control_status.
  *****************************************************************************/
static uint16_t slave_i2c_get_ext_control_status(void)
{
    uint16_t ext_control_status = 0;

    if (OMNIA_BOARD_REVISION >= 32) {
        for_each_const(pin, ext_control_pins)
            if (gpio_read(pin->pin) ^ pin->inv)
                ext_control_status |= pin->mask;
    }

    /* PHY_SFP_AUTO isn't a GPIO, rather an internal setting about behavior */
    ext_control_status |= i2c_status.ext_control_word & EXT_CTL_PHY_SFP_AUTO;

    return ext_control_status;
}

typedef struct {
	uint8_t cmd[10];
	uint8_t reply[20];
	uint8_t cmd_len, reply_len, reply_idx;
} slave_i2c_state_t;

#define set_reply(x)						\
	__builtin_memcpy(state->reply, &(x), sizeof(x));	\
	state->reply_len = sizeof(x);

static int cmd_get_features(slave_i2c_state_t *state)
{
	debug("get_features\n");
	set_reply(slave_features_supported);

	return 0;
}

static int cmd_get_status(slave_i2c_state_t *state)
{
	debug("get_status\n");
	set_reply(i2c_status.status_word);

	return 0;
}

static int cmd_general_control(slave_i2c_state_t *state)
{
	debug("general_control\n");
	slave_i2c_check_control_byte(state->cmd[1], state->cmd[2]);

	return 0;
}

static int cmd_get_ext_status(slave_i2c_state_t *state)
{
	debug("get_ext_status\n");
	set_reply(i2c_status.ext_status_dword);

	return 0;
}

static int cmd_ext_control(slave_i2c_state_t *state)
{
	uint8_t *args = &state->cmd[1];

	debug("ext_control\n");
	slave_i2c_ext_control(args[0] | (args[1] << 8),
			      args[2] | (args[3] << 8));

	return 0;
}

static int cmd_get_ext_control_status(slave_i2c_state_t *state)
{
	uint16_t ext_ctrl = slave_i2c_get_ext_control_status();

	debug("get_ext_control_status\n");
	set_reply(ext_ctrl);

	return 0;
}

static int cmd_get_reset(slave_i2c_state_t *state)
{
	debug("get_reset\n");
	set_reply(i2c_status.reset_type);

	return 0;
}

#if USER_REGULATOR_ENABLED
static int cmd_user_voltage(slave_i2c_state_t *state)
{
	debug("user_voltage\n");
	power_control_set_voltage(state->cmd[1]);

	return 0;
}
#endif

static int cmd_led_mode(slave_i2c_state_t *state)
{
	uint8_t *args = &state->cmd[1];

	debug("led_mode\n");
	led_driver_set_led_mode(args[0] & 0x0F, !!(args[0] & 0x10));

	return 0;
}

static int cmd_led_state(slave_i2c_state_t *state)
{
	uint8_t *args = &state->cmd[1];

	debug("led_state\n");
	led_driver_set_led_state_user(args[0] & 0x0F, !!(args[0] & 0x10));

	return 0;
}

static int cmd_led_color(slave_i2c_state_t *state)
{
	uint8_t *args = &state->cmd[1];

	debug("led_color\n");
	led_driver_set_colour(args[0] & 0x0F,
			      (args[1] << 16) | (args[2] << 8) | args[3]);

	return 0;
}

static int cmd_set_brightness(slave_i2c_state_t *state)
{
	debug("set_brightness\n");
	led_driver_pwm_set_brightness(state->cmd[1]);

	return 0;
}

static int cmd_get_brightness(slave_i2c_state_t *state)
{
	uint8_t brightness = leds->brightness;

	debug("get_brightness\n");
	set_reply(brightness);

	return 0;
}

static int cmd_watchdog_state(slave_i2c_state_t *state)
{
	watchdog.watchdog_state = state->cmd[1];

	if (watchdog.watchdog_state) {
		debug("watchdog_state RUN\n");
	} else {
		debug("watchdog_state STOP\n");
	}

	return 0;
}

static int cmd_watchdog_status(slave_i2c_state_t *state)
{
	watchdog.watchdog_sts = state->cmd[1];

	debug("watchdog_status\n");
	switch (EE_WriteVariable(WDG_VIRT_ADDR, watchdog.watchdog_sts)) {
	case VAR_FLASH_COMPLETE:
		debug("WDT: OK\n");
		break;
	case VAR_PAGE_FULL:
		debug("WDT: Pg full\n");
		break;
	case VAR_NO_VALID_PAGE:
		debug("WDT: No Pg\n");
		break;
	default:
		break;
	}

	return 0;
}

static int cmd_get_watchdog_state(slave_i2c_state_t *state)
{
	uint8_t wdt_state = watchdog.watchdog_state;

	debug("get_watchdog_state\n");
	set_reply(wdt_state);

	return 0;
}

static int cmd_get_version(slave_i2c_state_t *state)
{
	state->reply_len = 20;

	debug("get_version\n");
	if (state->cmd[0] == CMD_GET_FW_VERSION_BOOT)
		read_bootloader_version(state->reply);
	else
		__builtin_memcpy(state->reply, version, 20);

	return 0;
}

typedef struct {
	uint8_t len;
	int (*handler)(slave_i2c_state_t *state);
	bool leds_only;
} cmdinfo_t;

static const cmdinfo_t commands[] = {
	/* control & status */
	[CMD_GET_FEATURES]		= { 1, cmd_get_features },
	[CMD_GET_STATUS_WORD]		= { 1, cmd_get_status },
	[CMD_GENERAL_CONTROL]		= { 3, cmd_general_control },
	[CMD_GET_EXT_STATUS_DWORD]	= { 1, cmd_get_ext_status },
	[CMD_EXT_CONTROL]		= { 5, cmd_ext_control },
	[CMD_GET_EXT_CONTROL_STATUS]	= { 1, cmd_get_ext_control_status },
	[CMD_GET_RESET]			= { 1, cmd_get_reset },
#if USER_REGULATOR_ENABLED
	[CMD_USER_VOLTAGE]		= { 2, cmd_user_voltage },
#endif

	/* LEDs */
	[CMD_LED_MODE]			= { 2, cmd_led_mode, 1 },
	[CMD_LED_STATE]			= { 2, cmd_led_state, 1 },
	[CMD_LED_COLOUR]		= { 5, cmd_led_color, 1 },
	[CMD_SET_BRIGHTNESS]		= { 2, cmd_set_brightness, 1 },
	[CMD_GET_BRIGHTNESS]		= { 1, cmd_get_brightness, 1 },

	/* watchdog */
	[CMD_WATCHDOG_STATE]		= { 2, cmd_watchdog_state },
	[CMD_WATCHDOG_STATUS]		= { 2, cmd_watchdog_status },
	[CMD_GET_WATCHDOG_STATE]	= { 1, cmd_get_watchdog_state },

	/* version info */
	[CMD_GET_FW_VERSION_APP]	= { 1, cmd_get_version },
	[CMD_GET_FW_VERSION_BOOT]	= { 1, cmd_get_version },
};

static int handle_cmd(uint8_t addr, slave_i2c_state_t *state)
{
	uint8_t cmdidx = state->cmd[0];
	const cmdinfo_t *cmd;

	if (cmdidx >= ARRAY_SIZE(commands))
		return -1;

	cmd = &commands[cmdidx];
	if (!cmd->handler ||
	    (addr == I2C_SLAVE_ADDRESS_EMULATOR && !cmd->leds_only))
		return -1;

	if (state->cmd_len < cmd->len)
		return 0;
	else if (state->cmd_len == cmd->len)
		return cmd->handler(state);
	else
		return -1;
}

static int slave_i2c_event_cb(void *priv, uint8_t addr, i2c_slave_event_t event,
			      uint8_t *val)
{
	slave_i2c_state_t *state = priv;

	switch (event) {
	case I2C_SLAVE_READ_PROCESSED:
		state->reply_idx++;
		fallthrough;

	case I2C_SLAVE_READ_REQUESTED:
		if (state->reply_idx >= state->reply_len)
			return -1;

		*val = state->reply[state->reply_idx];
		break;

	case I2C_SLAVE_WRITE_REQUESTED:
		break;

	case I2C_SLAVE_WRITE_RECEIVED:
		if (state->cmd_len < sizeof(state->cmd))
			state->cmd[state->cmd_len++] = *val;
		else
			return -1;

		return handle_cmd(addr, state);

	case I2C_SLAVE_STOP:
		/* delete button status and counter bit from status_word */
		if (state->cmd[0] == CMD_GET_STATUS_WORD) {
			i2c_status.status_word &= ~STS_BUTTON_PRESSED;

			/* decrease button counter */
			button_counter_decrease(FIELD_GET(STS_BUTTON_COUNTER_MASK, i2c_status.status_word));
		}

		state->cmd_len = 0;
		state->reply_len = 0;
		state->reply_idx = 0;
		break;
	}

	return 0;
}

static slave_i2c_state_t slave_i2c_state;

static i2c_slave_t i2c_slave = {
	.cb = slave_i2c_event_cb,
	.priv = &slave_i2c_state,
};

/*******************************************************************************
  * @function   slave_i2c_config
  * @brief      Configuration of I2C peripheral and its timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_config(void)
{
	/* configure pins */
	gpio_init_alts(I2C_PINS_ALT_FN, pin_opendrain, pin_spd_1, pin_nopull,
		       I2C_SCL_PIN, I2C_SDA_PIN);

	i2c_slave_init(SLAVE_I2C, &i2c_slave,
		       I2C_SLAVE_ADDRESS, I2C_SLAVE_ADDRESS_EMULATOR, 1);
}

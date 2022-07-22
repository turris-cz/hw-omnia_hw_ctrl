#ifndef GPIO_H
#define GPIO_H

#include <stdarg.h>
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_rcc.h"
#include "compiler.h"
#include "bits.h"

typedef enum {
	PORT_A = 0,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_E,
	PORT_F,
} port_t;

#define NPORTS 6

typedef uint8_t pin_alt_fn_t;

typedef GPIOMode_TypeDef pin_mode_t;
static const pin_mode_t pin_mode_in = GPIO_Mode_IN;
static const pin_mode_t pin_mode_out = GPIO_Mode_OUT;
static const pin_mode_t pin_mode_alt = GPIO_Mode_AF;

typedef GPIOOType_TypeDef pin_out_t;
static const pin_out_t pin_pushpull = GPIO_OType_PP;
static const pin_out_t pin_opendrain = GPIO_OType_OD;

typedef GPIOPuPd_TypeDef pin_pull_t;
static const pin_pull_t pin_nopull = GPIO_PuPd_NOPULL;
static const pin_pull_t pin_pullup = GPIO_PuPd_UP;
static const pin_pull_t pin_pulldown = GPIO_PuPd_DOWN;

typedef GPIOSpeed_TypeDef pin_spd_t;
static const pin_spd_t pin_spd_1 = GPIO_Speed_Level_1;
static const pin_spd_t pin_spd_2 = GPIO_Speed_Level_2;
static const pin_spd_t pin_spd_3 = GPIO_Speed_Level_3;

typedef uint8_t gpio_t;

static const uint8_t PIN_INVALID = 0xff;

#define PIN(_port, _pin)	((gpio_t)((PORT_ ## _port) << 4) | ((_pin) & 0xf))

static __force_inline port_t pin_port(gpio_t pin)
{
	return pin >> 4;
}

static __force_inline uint8_t pin_nr(gpio_t pin)
{
	return pin & 0xf;
}

static __force_inline uint32_t port_clk_bit(port_t port)
{
	switch (port) {
	case PORT_A: return RCC_AHBPeriph_GPIOA;
	case PORT_B: return RCC_AHBPeriph_GPIOB;
	case PORT_C: return RCC_AHBPeriph_GPIOC;
	case PORT_D: return RCC_AHBPeriph_GPIOD;
	case PORT_E: return RCC_AHBPeriph_GPIOE;
	case PORT_F: return RCC_AHBPeriph_GPIOF;
	default: unreachable();
	}
}

static __force_inline GPIO_TypeDef *port_to_plat(port_t port)
{
	switch (port) {
	case PORT_A: return GPIOA;
	case PORT_B: return GPIOB;
	case PORT_C: return GPIOC;
	case PORT_D: return GPIOD;
	case PORT_E: return GPIOE;
	case PORT_F: return GPIOF;
	default: unreachable();
	}
}

static __force_inline GPIO_TypeDef *pin_port_to_plat(gpio_t pin)
{
	return port_to_plat(pin_port(pin));
}

static __force_inline bool gpio_read(gpio_t pin)
{
	if (pin == PIN_INVALID)
		return 0;

	return !!(pin_port_to_plat(pin)->IDR & BIT(pin_nr(pin)));
}

static __force_inline uint16_t gpio_read_port(port_t port)
{
	return port_to_plat(port)->IDR;
}

static __force_inline bool gpio_read_output(gpio_t pin)
{
	if (pin == PIN_INVALID)
		return 0;

	return !!(pin_port_to_plat(pin)->ODR & BIT(pin_nr(pin)));
}

static __force_inline void gpio_write(gpio_t pin, bool value)
{
	if (pin == PIN_INVALID)
		return;

	if (value)
		pin_port_to_plat(pin)->BSRR = BIT(pin_nr(pin));
	else
		pin_port_to_plat(pin)->BRR = BIT(pin_nr(pin));
}

static inline void gpio_write_multi_list(bool value, unsigned int len,
					 const gpio_t *pins)
{
	uint16_t bits[NPORTS] = {};

	for (unsigned int i = 0; i < len; ++i)
		if (pins[i] != PIN_INVALID)
			bits[pin_port(pins[i])] |= BIT(pin_nr(pins[i]));

	for (unsigned int i = 0; i < NPORTS; ++i) {
		if (!bits[i])
			continue;

		if (value)
			port_to_plat(i)->BSRR = bits[i];
		else
			port_to_plat(i)->BRR = bits[i];
	}
}

#define gpio_write_multi(_value, ...)					\
	({								\
		gpio_t ___pins[] = { __VA_ARGS__ };			\
		gpio_write_multi_list((_value), ARRAY_SIZE(___pins),	\
				      ___pins);				\
	})

static inline void gpio_init_port_clks(unsigned int len, const gpio_t *pins)
{
	uint32_t clk_bits = 0;

	for (unsigned int i = 0; i < len; ++i)
		if (pins[i] != PIN_INVALID)
			clk_bits |= port_clk_bit(pin_port(pins[i]));

	RCC_AHBPeriphClockCmd(clk_bits, ENABLE);
}

static inline void gpio_init_list(pin_mode_t mode, uint8_t alt_fn,
				  pin_out_t otype, pin_spd_t spd,
				  pin_pull_t pull, bool init_val,
				  unsigned int len, const gpio_t *pins)
{
	GPIO_InitTypeDef init = {
		.GPIO_Mode = mode,
		.GPIO_OType = otype,
		.GPIO_Speed = spd,
		.GPIO_PuPd = pull,
	};

	/* initialize port clock */
	gpio_init_port_clks(len, pins);

	/* set initial output values if output mode */
	if (mode == pin_mode_out)
		gpio_write_multi_list(init_val, len, pins);

	/* initialize individual pins */
	for (unsigned int i = 0; i < len; ++i) {
		if (pins[i] != PIN_INVALID) {
			/* set alt func if alt mode */
			if (mode == pin_mode_alt)
				GPIO_PinAFConfig(pin_port_to_plat(pins[i]),
						 pin_nr(pins[i]), alt_fn);

			/* initialize pin */
			init.GPIO_Pin = BIT(pin_nr(pins[i]));
			GPIO_Init(pin_port_to_plat(pins[i]), &init);
		}
	}
}

#define gpio_init_inputs(_pull, ...)					\
	({								\
		gpio_t ___pins[] = { __VA_ARGS__ };			\
		gpio_init_list(pin_mode_in, 0, 0, 0, (_pull), 0,	\
			       ARRAY_SIZE(___pins), ___pins);		\
	})

#define gpio_init_outputs(_otype, _spd, _init_val, ...)			\
	({								\
		gpio_t ___pins[] = { __VA_ARGS__ };			\
		gpio_init_list(pin_mode_out, 0, (_otype), (_spd),	\
			       pin_pullup, (_init_val),			\
			       ARRAY_SIZE(___pins), ___pins);		\
	})

#define gpio_init_alts(_alt_fn, _otype, _spd, _pull, ...)		\
	({								\
		gpio_t ___pins[] = { __VA_ARGS__ };			\
		gpio_init_list(pin_mode_alt, (_alt_fn), (_otype),	\
			       (_spd), (_pull), 0, ARRAY_SIZE(___pins),	\
			       ___pins);				\
	})

#endif /* GPIO_H */

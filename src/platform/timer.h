#ifndef TIMER_H
#define TIMER_H

#include "stm32f0xx_tim.h"
#include "stm32f0xx_rcc.h"
#include "compiler.h"

typedef uint8_t timer_nr_t;

#define DEBOUNCE_TIMER		16
#define LED_TIMER		3
#define LED_EFFECT_TIMER	6
#define LED_PWM_TIMER		15
#define USB_TIMEOUT_TIMER	17

#define TIMER_PARENT_FREQ	48000000U

typedef enum {
	timer_interrupt,
	timer_pwm,
} timer_type_t;

static __force_inline void timer_rcc_config(timer_nr_t tim_nr, bool on)
{
	switch (tim_nr) {
#define _TIMER_RCC_CFG(_bus, _n)							\
	case _n:									\
		RCC_ ## _bus ## PeriphClockCmd(RCC_ ## _bus ## Periph_TIM ## _n, on);	\
		break;
	_TIMER_RCC_CFG(APB2, 15)
	_TIMER_RCC_CFG(APB2, 16)
	_TIMER_RCC_CFG(APB2, 17)
	_TIMER_RCC_CFG(APB1, 3)
	_TIMER_RCC_CFG(APB1, 6)
	_TIMER_RCC_CFG(APB1, 14)
#undef _TIMER_RCC_CFG
	default:
		unreachable();
	}
}

static __force_inline TIM_TypeDef *timer_to_plat(timer_nr_t tim_nr)
{
	switch (tim_nr) {
	case 3: return TIM3;
	case 6: return TIM6;
	case 14: return TIM14;
	case 15: return TIM15;
	case 16: return TIM16;
	case 17: return TIM17;
	default: unreachable();
	}
}

static __force_inline uint8_t timer_irqn(timer_nr_t tim_nr)
{
	switch (tim_nr) {
	case 3: return TIM3_IRQn;
	case 6: return TIM6_DAC_IRQn;
	case 14: return TIM14_IRQn;
	case 15: return TIM15_IRQn;
	case 16: return TIM16_IRQn;
	case 17: return TIM17_IRQn;
	default: unreachable();
	}
}

static __force_inline void timer_init(timer_nr_t tim_nr, timer_type_t type,
				      uint16_t period, uint32_t freq,
				      uint8_t irq_prio)
{
	TIM_TypeDef *tim = timer_to_plat(tim_nr);
	TIM_TimeBaseInitTypeDef init = {
		.TIM_Period = period - 1,
		.TIM_Prescaler = (TIMER_PARENT_FREQ / freq) - 1,
		.TIM_ClockDivision = TIM_CKD_DIV1,
		.TIM_CounterMode = TIM_CounterMode_Up,
	};

	compiletime_assert(TIMER_PARENT_FREQ % freq == 0,
			   "Requested frequency unachievable");

	timer_rcc_config(tim_nr, 0);
	TIM_DeInit(tim);
	timer_rcc_config(tim_nr, 1);

	TIM_TimeBaseInit(tim, &init);

	if (type == timer_pwm) {
		TIM_OCInitTypeDef ocinit = {
			.TIM_OCMode = TIM_OCMode_PWM1,
			.TIM_OutputState = TIM_OutputState_Enable,
			.TIM_Pulse = 0,
			.TIM_OCPolarity = TIM_OCPolarity_Low,
		};

		TIM_OC2Init(tim, &ocinit);
		TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
	}

	TIM_ARRPreloadConfig(tim, ENABLE);

	if (type == timer_interrupt) {
		NVIC_InitTypeDef nvinit = {
			.NVIC_IRQChannel = timer_irqn(tim_nr),
			.NVIC_IRQChannelPriority = irq_prio,
			.NVIC_IRQChannelCmd = ENABLE,
		};

		tim->DIER = TIM_IT_Update;
		NVIC_Init(&nvinit);
	}

	if (type == timer_pwm)
		TIM_CtrlPWMOutputs(tim, ENABLE);
}

static __force_inline void timer_deinit(timer_nr_t tim_nr)
{
	TIM_DeInit(timer_to_plat(tim_nr));
}

static __force_inline void timer_enable(timer_nr_t tim_nr, bool on)
{
	TIM_Cmd(timer_to_plat(tim_nr), on);
}

static __force_inline void timer_irq_clear(timer_nr_t tim_nr)
{
	timer_to_plat(tim_nr)->SR = ~(uint16_t)TIM_IT_Update;
}

static __force_inline void timer_set_counter(timer_nr_t tim_nr, uint32_t cnt)
{
	timer_to_plat(tim_nr)->CNT = cnt;
}

static __force_inline void timer_set_pulse(timer_nr_t tim_nr, uint32_t pulse)
{
	timer_to_plat(tim_nr)->CCR2 = pulse;
}

#endif /* TIMER_H */

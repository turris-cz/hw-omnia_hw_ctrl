#include <stdint.h>
#include "compiler.h"

extern uint32_t _stack_top, _sfdata, _sdata, _edata, _sbss, _ebss;

extern void __noreturn main(void);

void SystemInit(void);

void __noreturn __naked __section(".startup")
reset_handler(void)
{
	asm volatile(
		"ldr	r0, =_stack_top\n"
		"mov	sp, r0\n"
	);

	/* copy data */
	for (uint32_t *src = &_sfdata, *dst = &_sdata; dst < &_edata;)
		*dst++ = *src++;

	/* zero out bss */
	for (uint32_t *ptr = &_sbss; ptr < &_ebss; ++ptr)
		*(uint32_t *)ptr = 0;

	SystemInit();
	main();
}

static void __irq default_handler(void)
{
	while (1);
}

void nmi_handler(void) __weak_alias(default_handler);
void hardfault_handler(void) __weak_alias(default_handler);
void svc_handler(void) __weak_alias(default_handler);
void pendsv_handler(void) __weak_alias(default_handler);
void external_irq(void) __weak_alias(default_handler);

void systick_irq_handler(void) __weak_alias(default_handler);
void WWDG_IRQHandler(void) __weak_alias(default_handler);
void RTC_IRQHandler(void) __weak_alias(default_handler);
void FLASH_IRQHandler(void) __weak_alias(default_handler);
void RCC_IRQHandler(void) __weak_alias(default_handler);
void EXTI0_1_IRQHandler(void) __weak_alias(default_handler);
void EXTI2_3_IRQHandler(void) __weak_alias(default_handler);
void EXTI4_15_IRQHandler(void) __weak_alias(default_handler);
void DMA1_Channel1_IRQHandler(void) __weak_alias(default_handler);
void DMA1_Channel2_3_IRQHandler(void) __weak_alias(default_handler);
void DMA1_Channel4_5_IRQHandler(void) __weak_alias(default_handler);
void ADC1_IRQHandler(void) __weak_alias(default_handler);
void TIM1_BRK_UP_TRG_COM_IRQHandler(void) __weak_alias(default_handler);
void TIM1_CC_IRQHandler(void) __weak_alias(default_handler);
void led_driver_irq_handler(void) __weak_alias(default_handler);
void led_driver_effect_irq_handler(void) __weak_alias(default_handler);
void TIM14_IRQHandler(void) __weak_alias(default_handler);
void TIM15_IRQHandler(void) __weak_alias(default_handler);
void debounce_timer_irq_handler(void) __weak_alias(default_handler);
void power_control_usb_timeout_irq_handler(void) __weak_alias(default_handler);
void I2C1_IRQHandler(void) __weak_alias(default_handler);
void i2c_slave_irq_handler(void) __weak_alias(default_handler);
void SPI1_IRQHandler(void) __weak_alias(default_handler);
void SPI2_IRQHandler(void) __weak_alias(default_handler);
void USART1_IRQHandler(void) __weak_alias(default_handler);
void USART2_IRQHandler(void) __weak_alias(default_handler);

static __used __section(".isr_vector") void *isr_vector[] = {
	&_stack_top,
	reset_handler,
	nmi_handler,
	hardfault_handler,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	svc_handler,
	NULL,
	NULL,
	pendsv_handler,
	systick_irq_handler,
	WWDG_IRQHandler,		/* Window WatchDog */
	NULL,				/* Reserved */
	RTC_IRQHandler,			/* RTC through the EXTI line */
	FLASH_IRQHandler,		/* FLASH */
	RCC_IRQHandler,			/* RCC */
	EXTI0_1_IRQHandler,		/* EXTI Line 0 and 1 */
	EXTI2_3_IRQHandler,		/* EXTI Line 2 and 3 */
	EXTI4_15_IRQHandler,		/* EXTI Line 4 to 15 */
	NULL,				/* Reserved */
	DMA1_Channel1_IRQHandler,	/* DMA1 Channel 1 */
	DMA1_Channel2_3_IRQHandler,	/* DMA1 Channel 2 and Channel 3 */
	DMA1_Channel4_5_IRQHandler,	/* DMA1 Channel 4 and Channel 5 */
	ADC1_IRQHandler,		/* ADC1 */
	TIM1_BRK_UP_TRG_COM_IRQHandler,	/* TIM1 Break, Update, Trigger and Commutation */
	TIM1_CC_IRQHandler,		/* TIM1 Capture Compare */
	NULL,				/* Reserved */
	led_driver_irq_handler,		/* TIM3 */
	led_driver_effect_irq_handler,	/* TIM6 */
	NULL,				/* Reserved */
	TIM14_IRQHandler,		/* TIM14 */
	TIM15_IRQHandler,		/* TIM15 */
	debounce_timer_irq_handler,	/* TIM16 */
	power_control_usb_timeout_irq_handler, /* TIM17 */
	I2C1_IRQHandler,		/* I2C1 */
	i2c_slave_irq_handler,		/* I2C2 */
	SPI1_IRQHandler,		/* SPI1 */
	SPI2_IRQHandler,		/* SPI2 */
	USART1_IRQHandler,		/* USART1 */
	USART2_IRQHandler,		/* USART2 */
	NULL,				/* Reserved */
	NULL,				/* Reserved */
	NULL,				/* Reserved */
};

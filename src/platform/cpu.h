#ifndef CPU_H
#define CPU_H

#include "compiler.h"

static __force_inline void enable_irq(void)
{
	asm volatile("cpsie i\n" : : : "memory");
}

static __force_inline void disable_irq(void)
{
	asm volatile("cpsid i\n" : : : "memory");
}

static __force_inline void set_msp(uint32_t msp)
{
	asm volatile("msr msp, %0\n" : : "r" (msp));
}

static __force_inline void isb(void)
{
	asm volatile("isb\n");
}

static __force_inline void nop(void)
{
	asm("nop\n");
}

static __force_inline __noreturn void reset_to_address(uint32_t isr_vec_addr)
{
	__noreturn void (*new_reset_handler)(void);
	uint32_t sp;

	/* get stack pointer from ISR vector */
	sp = *(volatile uint32_t *)isr_vec_addr;

	new_reset_handler = (void *)*(volatile uint32_t *)(isr_vec_addr + 4);

	/* set stack pointer */
	set_msp(sp);

	/* instruction synchronization barrier to flush pipeline */
	isb();

	/* jump to new app */
	new_reset_handler();
}

#endif /* CPU_H */

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

#endif /* CPU_H */

#ifndef CRC32_H
#define CRC32_H

#include "stm32f0xx_rcc.h"
#include "compiler.h"

static inline bool crc32(uint32_t *res, uint32_t init,
			 const uint32_t *addr, uint16_t len)
{
	/* we don't support unaligned length yet */
	if (len % 4)
		return 0;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, 1);

	CRC->CR = CRC_CR_RESET;
	CRC->INIT = init;
	while (len > 0) {
		CRC->DR = *addr++;
		len -= 4;
	}

	*res = CRC->DR;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, 0);

	return 1;
}

#endif /* CRC32_H */

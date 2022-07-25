#ifndef SPI_H
#define SPI_H

#include "stm32f0xx_spi.h"
#include "stm32f0xx_rcc.h"
#include "compiler.h"

typedef uint8_t spi_nr_t;

#define LED_SPI		1

static __force_inline SPI_TypeDef *spi_to_plat(spi_nr_t spi_nr)
{
	switch (spi_nr) {
	case 1: return SPI1;
	case 2: return SPI2;
	default: unreachable();
	}
}

static __force_inline void spi_rcc_config(spi_nr_t spi_nr, bool on)
{
	switch (spi_nr) {
	case 1:
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, on);
		break;
	case 2:
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, on);
		break;
	default:
		unreachable();
	}
}

static inline void spi_init(spi_nr_t spi_nr)
{
	SPI_TypeDef *spi = spi_to_plat(spi_nr);
	SPI_InitTypeDef init = {
		.SPI_Direction = SPI_Direction_1Line_Tx,
		.SPI_Mode = SPI_Mode_Master,
		.SPI_DataSize = SPI_DataSize_16b,
		.SPI_CPOL = SPI_CPOL_Low,
		.SPI_CPHA = SPI_CPHA_1Edge,
		.SPI_NSS = SPI_NSS_Soft,
		.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2,
		.SPI_FirstBit = SPI_FirstBit_MSB,
	};

	spi_rcc_config(spi_nr, 0);
	SPI_I2S_DeInit(spi);
	spi_rcc_config(spi_nr, 1);

	SPI_I2S_DeInit(spi);
	SPI_Init(spi, &init);
	SPI_Cmd(spi, ENABLE);
}

static __force_inline void spi_send16(spi_nr_t spi_nr, uint16_t data)
{
	spi_to_plat(spi_nr)->DR = data;
}

static __force_inline bool spi_is_busy(spi_nr_t spi_nr)
{
	return !!(spi_to_plat(spi_nr)->SR & SPI_I2S_FLAG_BSY);
}

#endif /* SPI_H */

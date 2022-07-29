#include "i2c_slave.h"

i2c_slave_t *i2c_slave_ptr[2];

void i2c_slave_irq_handler(void)
{
	i2c_nr_t i2c_nr = i2c_nr_in_irq();
	i2c_slave_t *slave = i2c_slave_ptr[i2c_nr - 1];
	I2C_TypeDef *i2c = i2c_to_plat(i2c_nr);
	uint32_t isr;
	int ret;

	__disable_irq();

	isr = i2c->ISR;

	if (isr & (I2C_ISR_TIMEOUT | I2C_ISR_ARLO | I2C_ISR_BERR))
		/* TODO: reset of the i2c peripheral on error */
		i2c->ICR = I2C_ICR_TIMOUTCF | I2C_ICR_ARLOCF | I2C_ICR_BERRCF;

	/* Address matched */
	if (isr & I2C_ISR_ADDR) {
		slave->addr = (isr & I2C_ISR_ADDCODE) >> __bf_shf(I2C_ISR_ADDCODE);

		if (isr & I2C_ISR_DIR) {
			slave->state = I2C_SLAVE_READ_REQUESTED;

			/* disable slave byte control, enable TX interrupts */
			i2c->CR1 = (i2c->CR1 & ~I2C_CR1_SBC) | I2C_CR1_TXIE;

			/* flush TXDR */
			i2c->ISR |= I2C_ISR_TXE;
		} else {
			/* enable slave byte control and RX interrupts */
			i2c->CR1 |= I2C_CR1_SBC | I2C_CR1_RXIE;

			i2c_slave_ack(i2c_nr, 1);

			slave->state = I2C_SLAVE_WRITE_REQUESTED;
			slave->cb(slave->priv, slave->addr, slave->state,
				  &slave->val);
		}

		/* clear */
		i2c->ICR = I2C_ICR_ADDRCF;
	}

	/* Receive data register not empty */
	if (isr & I2C_ISR_RXNE) {
		slave->val = i2c->RXDR;
		slave->state = I2C_SLAVE_WRITE_RECEIVED;
		ret = slave->cb(slave->priv, slave->addr, slave->state, &slave->val);

		/* set NACK on error */
		i2c_slave_ack(i2c_nr, !ret);
	}

	/* Transmit interrupt status */
	if (isr & I2C_ISR_TXIS) {
		ret = slave->cb(slave->priv, slave->addr, slave->state,
				&slave->val);

		/* if no more reply bytes are available, write 0x00 to master
		 * TODO: do a reset of i2c peripheral instead */
		if (ret)
			i2c->TXDR = 0x00;
		else
			i2c->TXDR = slave->val;

		slave->state = I2C_SLAVE_READ_PROCESSED;
	}

	/* Stop detection flag */
	if (isr & I2C_ISR_STOPF) {
		slave->state = I2C_SLAVE_STOP;
		slave->cb(slave->priv, slave->addr, slave->state, &slave->val);

		/* disable RX & TX interrupts */
		i2c->CR1 &= ~(I2C_CR1_RXIE | I2C_CR1_TXIE);

		/* disable all interrupts if pause requested */
		if (slave->paused)
			i2c->CR1 &= ~(I2C_CR1_ADDRIE | I2C_CR1_ERRIE |
				      I2C_CR1_STOPIE);

		/* clear */
		i2c->ICR = I2C_ICR_STOPCF;
	}

	__enable_irq();
}

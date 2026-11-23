#ifndef SPI1_H
#define SPI1_H

#include <stdint.h>

void spi1_init(void);
void spi1_send(uint8_t data);
void spi1_write_fast(const uint8_t *data, uint16_t length);
void spi1_dma_enable(void);
void spi1_dma_disable(void);
void spi1_wait_idle(void);

#endif

#ifndef DMA_H
#define DMA_H

#include <stdint.h>

void dma2_init(void);
void dma2_spi1_tx_start(const uint8_t *buf, uint16_t len);
void dma2_spi1_tx_wait(void);

#endif

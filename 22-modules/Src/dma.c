#include "dma.h"
#include "regs.h"

void dma2_init(void) {
	RCC_AHB1ENR |= (1U << 22); 								// włączenie zegara dla DMA2
	DMA2_S3FCR = 0; 										// wyczyszczenie rejestru kontrolnego strumienia 3
}

static void dma2_clear_flags(void) {
	DMA2_LIFCR = (1U << 27) | (1U << 26) | (1U << 25) | (1U << 24) | (1U << 22); // wyczyszczenie flag TCIF3, HTIF3, TEIF3, DMEIF3, FEIF3
}

void dma2_spi1_tx_start(const uint8_t *buf, uint16_t len) {
	DMA2_S3CR &= ~(1U << 0);
	while (DMA2_S3CR & (1U << 0));
	dma2_clear_flags();
	DMA2_S3PAR  = (uint32_t)&SPI1_DR;
	DMA2_S3M0AR = (uint32_t)buf;
	DMA2_S3NDTR = len;
	DMA2_S3CR = (3U << 25) | (2U << 16) | (1U << 10) | (1U << 6);	// CH3, priorytet wysoki, MINC, mem->periph
	DMA2_S3CR |= (1U << 0);											// EN
}
void dma2_spi1_tx_wait(void) {
	while (!(DMA2_LISR & (1U << 27)));
	DMA2_LIFCR = (1U << 27);
}

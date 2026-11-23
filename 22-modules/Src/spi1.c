#include "spi1.h"
#include "regs.h"

#define OLED_SCK_PIN  3		// PB3
#define OLED_MOSI_PIN 5		// PB5

void spi1_init(void) {
	RCC_AHB1ENR |= (1U << 1);
	RCC_APB2ENR |= (1U << 12);		// włączenie zegara dla SPI1

	GPIOB_MODER &= ~((3U << (2 * OLED_SCK_PIN)) | (3U << (2 * OLED_MOSI_PIN)));	// ustawienie trybu alternatywnego dla pinów PB3 i PB5
	GPIOB_MODER |=  ((2U << (2 * OLED_SCK_PIN)) | (2U << (2 * OLED_MOSI_PIN)));	// ustawienie trybu alternatywnego dla pinów PB3 i PB5
	GPIOB_AFRL &= ~(0xFU << (4 * OLED_SCK_PIN));	// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB3
	GPIOB_AFRL |= (5U << (4 * OLED_SCK_PIN));		// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB3
	GPIOB_AFRL &= ~(0xFU << (4 * OLED_MOSI_PIN));	// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB5
	GPIOB_AFRL |= (5U << (4 * OLED_MOSI_PIN));		// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB5

	SPI1_CR1 = (1U << 2) | (2U << 3) | (1U << 9) | (1U << 8);	// master, f/8, SSM, SSI
	SPI1_CR1 |= (1U << 6);										// SPE na końcu
}

void spi1_send(uint8_t data) {
    while (!(SPI1_SR & (1U << 1)));		// TXE: bufor pusty
    SPI1_DR = data;						// wysłanie danych do rejestru DR
    while (!(SPI1_SR & (1U << 1)));		// TXE: bajt przeniesiony do rejestru przesuwnego
    while (SPI1_SR & (1U << 7));		// BSY = 0: transmisja zakończona
}

void spi1_write_fast(const uint8_t *data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		while (!(SPI1_SR & (1U << 1)));		// TXE: bufor pusty
		SPI1_DR = data[i];					// wysłanie danych do rejestru DR
	}
}

void spi1_dma_enable(void)  { SPI1_CR2 |=  (1U << 1); }
void spi1_dma_disable(void) { SPI1_CR2 &= ~(1U << 1); }
void spi1_wait_idle(void)   { while (!(SPI1_SR & (1U << 1))); while (SPI1_SR & (1U << 7)); }

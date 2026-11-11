#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400)

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB2ENR  (*(volatile uint32_t *)0x40023844)
#define SPI1_CR1     (*(volatile uint32_t *)0x40013000)
#define SPI1_SR      (*(volatile uint32_t *)0x40013008)
#define SPI1_DR      (*(volatile uint32_t *)0x4001300C)
#define GPIOB_AFRL   (*(volatile uint32_t *)0x40020420)
#define GPIOB_BSRR    (*(volatile uint32_t *)0x40020418)
#define GPIOA_BSRR    (*(volatile uint32_t *)0x40020018)

#define OLED_CS_PIN   10	// PA10
#define OLED_DC_PIN   4		// PB4
#define OLED_RES_PIN  10	// PB10
#define OLED_SCK_PIN  3		// PB3
#define OLED_MOSI_PIN 5		// PB5

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void delay_ms(uint32_t ms)
{
	uint32_t start_ticks = ms_ticks;
	while ((ms_ticks - start_ticks) < ms);
}

static void spi1_send(uint8_t data) {
    while (!(SPI1_SR & (1U << 1)));   // TXE: bufor pusty
    SPI1_DR = data;
    while (!(SPI1_SR & (1U << 1)));   // TXE: bajt przeniesiony do rejestru przesuwnego
    while (SPI1_SR & (1U << 7));      // BSY = 0: transmisja zakończona
}

static void oled_cmd(uint8_t cmd) {
	GPIOA_BSRR = (1U << (OLED_CS_PIN + 16));  // ustawienie pinu CS w stan niski (wybranie wyświetlacza)
	GPIOB_BSRR = (1U << (OLED_DC_PIN + 16));    // ustawienie pinu DC w stan niski (tryb komendy)
	spi1_send(cmd);            // wysłanie komendy przez SPI
	GPIOA_BSRR = (1U << OLED_CS_PIN);   // ustawienie pinu CS w stan wysoki (odznaczenie wyświetlacza)
}

static void oled_data(uint8_t data) {
	GPIOA_BSRR = (1U << (OLED_CS_PIN + 16)); // ustawienie pinu CS w stan niski (wybranie wyświetlacza)
	GPIOB_BSRR = (1U << OLED_DC_PIN);  // ustawienie pinu DC w stan wysoki (tryb danych)
	spi1_send(data);           // wysłanie danych przez SPI
	GPIOA_BSRR = (1U << OLED_CS_PIN);   // ustawienie pinu CS w stan wysoki (odznaczenie wyświetlacza)
}

static void oled_reset(void) {
	GPIOB_BSRR = (1U << (OLED_RES_PIN));  	   // ustawienie pinu RES w stan wysoki
	GPIOB_BSRR = (1U << (OLED_RES_PIN + 16));  // ustawienie pinu RES w stan niski (reset)
	delay_ms(10);               // odczekanie 10 ms
	GPIOB_BSRR = (1U << OLED_RES_PIN);  // ustawienie pinu RES w stan wysoki (koniec resetu)
	delay_ms(10);               // odczekanie 10 ms
}

void gpio_init(void) {
	RCC_AHB1ENR |= (1U << 0);
	RCC_AHB1ENR |= (1U << 1);

	RCC_APB2ENR |= (1U << 12); 								// włączenie zegara dla SPI1

	GPIOB_MODER &= ~((3U << (2 * OLED_DC_PIN)) | (3U << (2 * OLED_RES_PIN))); 	// ustawienie trybu wyjścia dla pinów PB4 i PB10
	GPIOB_MODER |=  ((1U << (2 * OLED_DC_PIN)) | (1U << (2 * OLED_RES_PIN)));
	GPIOA_MODER &= ~((3U << (2 * OLED_CS_PIN))); 								// ustawienie trybu wyjścia dla pinu PA10
	GPIOA_MODER |=  ((1U << (2 * OLED_CS_PIN)));
}

void systick_init(void) {
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);
}

void spi1_init(void) {
	GPIOB_MODER &= ~((3U << (2 * OLED_SCK_PIN)) | (3U << (2 * OLED_MOSI_PIN))); 	// ustawienie trybu alternatywnego dla pinów PB3 i PB5
	GPIOB_MODER |=  ((2U << (2 * OLED_SCK_PIN)) | (2U << (2 * OLED_MOSI_PIN)));
	GPIOB_AFRL &= ~(0xFU << (4 * OLED_SCK_PIN));
	GPIOB_AFRL |= (5U << (4 * OLED_SCK_PIN)); 							// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB3
	GPIOB_AFRL &= ~(0xFU << (4 * OLED_MOSI_PIN));
	GPIOB_AFRL |= (5U << (4 * OLED_MOSI_PIN)); 							// ustawienie funkcji alternatywnej AF5 (SPI1) dla pinu PB5

	SPI1_CR1 = (1U << 2) | (2U << 3) | (1U << 9) | (1U << 8);   // master, f/8, SSM, SSI
	SPI1_CR1 |= (1U << 6);                                      // SPE na końcu
}

void oled_init(void){
	oled_reset();
	GPIOA_BSRR = (1U << (OLED_CS_PIN)); 	// ustawienie pinu CS w stan wysoki (odznaczenie wyświetlacza)

	oled_cmd(0xAE);   // ekran wyłączony
	oled_cmd(0x8D);   // charge pump: ustawienie
	oled_cmd(0x14);   //   włączona
	oled_cmd(0xAF);   // ekran włączony
	oled_cmd(0xA4);   // wyświetlanie z pamięci
}


int main(void)
{
	systick_init();
	gpio_init();
	spi1_init();
	oled_init();

	for (;;) {
	}
}

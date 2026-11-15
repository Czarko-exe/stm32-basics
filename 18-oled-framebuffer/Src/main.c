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

static void spi1_write_fast(const uint8_t *data, uint16_t length) {
	for (uint16_t i = 0; i < length; i++) {
		while (!(SPI1_SR & (1U << 1)));   // TXE: bufor pusty
		SPI1_DR = data[i];
	}
}

static void oled_cmd(uint8_t cmd) {
	GPIOA_BSRR = (1U << (OLED_CS_PIN + 16));  // ustawienie pinu CS w stan niski (wybranie wyświetlacza)
	GPIOB_BSRR = (1U << (OLED_DC_PIN + 16));    // ustawienie pinu DC w stan niski (tryb komendy)
	spi1_send(cmd);            // wysłanie komendy przez SPI
	GPIOA_BSRR = (1U << OLED_CS_PIN);   // ustawienie pinu CS w stan wysoki (odznaczenie wyświetlacza)
}

static void oled_reset(void) {
	GPIOB_BSRR = (1U << (OLED_RES_PIN));  	   // ustawienie pinu RES w stan wysoki
	GPIOB_BSRR = (1U << (OLED_RES_PIN + 16));  // ustawienie pinu RES w stan niski (reset)
	delay_ms(10);               // odczekanie 10 ms
	GPIOB_BSRR = (1U << OLED_RES_PIN);  // ustawienie pinu RES w stan wysoki (koniec resetu)
	delay_ms(10);               // odczekanie 10 ms
}

static uint8_t fb[128 * 8];

void oled_update(void) {
	oled_cmd(0x21);   // ustawienie kolumn: 0-127
	oled_cmd(0x00);
	oled_cmd(0x7F);
	oled_cmd(0x22);   // ustawienie stron: 0-7
	oled_cmd(0x00);
	oled_cmd(0x07);

	GPIOA_BSRR = (1U << (OLED_CS_PIN + 16));
	GPIOB_BSRR = (1U << OLED_DC_PIN);
	spi1_write_fast(fb, sizeof(fb));
	while (SPI1_SR & (1U << 7));      // BSY = 0: transmisja zakończona
	GPIOA_BSRR = (1U << OLED_CS_PIN);
}

void oled_set_pixel(uint8_t x, uint8_t y, uint8_t color) {
	if (x >= 128 || y >= 64) return; // sprawdzenie zakresu
	uint16_t index = x + (y / 8) * 128;
	if (color) {
		fb[index] |= (1 << (y % 8)); // ustawienie bitu
	} else {
		fb[index] &= ~(1 << (y % 8)); // wyczyszczenie bitu
	}
}

void oled_checkerboard(uint8_t color) {
	uint8_t fill_color = color ? 0 : 1; // kolor wypełnienia przeciwny do koloru piksela
	for (uint8_t i = 0; i < 128; i++) {
		for (uint8_t j = 0; j < 64; j++) {
			if ((i/8 + j/8) % 2 == 0) {
				oled_set_pixel(i, j, color); // ustawienie pikseli w poziomie
			} else {
				oled_set_pixel(i, j, fill_color); // ustawienie pikseli w poziomie
			}
		}
	}
}

void oled_clear(void) {
	for (uint16_t i = 0; i < sizeof(fb); i++) {
		fb[i] = 0; // wyczyszczenie bufora
	}
}

void oled_hline(uint8_t x, uint8_t y, uint8_t length, uint8_t color) {
	for (uint8_t i = 0; i < length; i++) {
		oled_set_pixel(x + i, y, color); // ustawienie pikseli w poziomie
	}
}

void oled_vline(uint8_t x, uint8_t y, uint8_t length, uint8_t color) {
	for (uint8_t i = 0; i < length; i++) {
		oled_set_pixel(x, y + i, color); // ustawienie pikseli w pionie
	}
}

void oled_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
	oled_hline(x, y, width, color); // górna krawędź
	oled_hline(x, y + height - 1, width, color); // dolna krawędź
	oled_vline(x, y, height, color); // lewa krawędź
	oled_vline(x + width - 1, y, height, color); // prawa krawędź
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
	oled_cmd(0x20);   // ustawienie trybu adresowania: poziomego
	oled_cmd(0x00);   //   poziomego
	oled_cmd(0xAF);   // ekran włączony
}


int main(void)
{
	systick_init();
	gpio_init();
	spi1_init();
	oled_init();

	uint32_t last_tick = 0;
	uint8_t pattern = 0;
	oled_checkerboard(1);	// wyświetlenie szachownicy
	for (;;) {
		if (ms_ticks - last_tick >= 500) {
			last_tick = ms_ticks;
			pattern += 1;
			oled_clear();	// wyczyszczenie bufora
			switch (pattern % 3) {
				case 0:
					oled_checkerboard(1);	// wyświetlenie szachownicy
					break;
				case 1:
					oled_checkerboard(0);	// wyświetlenie szachownicy
					break;
				case 2:
					oled_rect(0, 0, 128, 64, 1);	// wyświetlenie prostokąta
					break;
			}
			oled_update();	// aktualizacja wyświetlacza
		}
	}
}

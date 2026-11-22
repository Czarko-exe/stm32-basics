#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400)
#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)
#define GPIOB_AFRH   (*(volatile uint32_t *)0x40020424)
#define GPIOB_OTYPER  (*(volatile uint32_t *)0x40020404)

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
#define SPI1_CR2     (*(volatile uint32_t *)0x40013004)

#define I2C1_CR1     (*(volatile uint32_t *)0x40005400)
#define I2C1_CR2     (*(volatile uint32_t *)0x40005404)
#define I2C1_DR      (*(volatile uint32_t *)0x40005410)
#define I2C1_SR1     (*(volatile uint32_t *)0x40005414)
#define I2C1_SR2     (*(volatile uint32_t *)0x40005418)
#define I2C1_CCR     (*(volatile uint32_t *)0x4000541C)
#define I2C1_TRISE   (*(volatile uint32_t *)0x40005420)

#define DMA2_S3CR    (*(volatile uint32_t *)0x40026458)
#define DMA2_S3NDTR  (*(volatile uint32_t *)0x4002645C)
#define DMA2_S3PAR   (*(volatile uint32_t *)0x40026460)
#define DMA2_S3M0AR  (*(volatile uint32_t *)0x40026464)
#define DMA2_S3FCR   (*(volatile uint32_t *)0x4002646C)
#define DMA2_LISR    (*(volatile uint32_t *)0x40026400)
#define DMA2_LIFCR   (*(volatile uint32_t *)0x40026408)

#define OLED_CS_PIN   10	// PA10
#define OLED_DC_PIN   4		// PB4
#define OLED_RES_PIN  10	// PB10
#define OLED_SCK_PIN  3		// PB3
#define OLED_MOSI_PIN 5		// PB5

#define I2C1_SCL_PIN 8 // PB8
#define I2C1_SDA_PIN 9 // PB9

static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;
static int32_t t_fine;

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static int wait_flag(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms)		// funkcja oczekująca na ustawienie flagi w rejestrze z timeoutem
{
    uint32_t start = ms_ticks;
    while (!(*reg & mask)) {
        if (ms_ticks - start >= timeout_ms) {
            return 0;
        }
    }
    return 1;
}
static int wait_flag_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms)	// funkcja oczekująca na wyczyszczenie flagi w rejestrze z timeoutem
{
	uint32_t start = ms_ticks;
	while (*reg & mask) {
		if (ms_ticks - start >= timeout_ms) {
			return 0;
		}
	}
	return 1;
}

int i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t val) {
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), 10)) return 0;		// czekanie na zakończenie poprzedniej transmisji
	I2C1_CR1 |= (1U << 8);											// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), 10)) goto fail;			// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);											// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), 10)) goto fail;			// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;													// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;													// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), 10)) goto fail;			// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;													// wysłanie adresu rejestru, do którego chcemy zapisać dane
	if (!wait_flag(&I2C1_SR1, (1U << 7), 10)) goto fail;			// czekanie na ustawienie flagi TXE
	I2C1_DR = val;													// wysłanie danych do zapisania w rejestrze
	if (!wait_flag(&I2C1_SR1, (1U << 2), 10)) goto fail;			// czekanie na ustawienie flagi BTF

	I2C1_CR1 |= (1U << 9);											// generowanie warunku STOP
	return 1;

	fail:															// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
	}

int i2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *out) {
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), 10)) return 0;		// czekanie na zakończenie poprzedniej transmisji
	I2C1_CR1 |= (1U << 8);											// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), 10)) goto fail;			// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);											// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), 10)) goto fail;			// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;													// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;													// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), 10)) goto fail;			// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;													// wysłanie adresu rejestru, z którego chcemy odczytać dane
	if (!wait_flag(&I2C1_SR1, (1U << 2), 10)) goto fail;			// czekanie na ustawienie flagi BTF
	I2C1_CR1 |= (1U << 8);											// generowanie warunku START ponownie
	if (!wait_flag(&I2C1_SR1, (1U << 0), 10)) goto fail;			// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1) | 1;										// wysłanie adresu urządzenia z bitem R/W = 1 (odczyt)
	if (!wait_flag(&I2C1_SR1, (1U << 1), 10)) goto fail;			// czekanie na ustawienie flagi ADDR

	I2C1_CR1 &= ~(1U << 10);										// wyłączenie potwierdzenia odbioru
	(void)I2C1_SR1;
	(void)I2C1_SR2;
	I2C1_CR1 |= (1U << 9);											// generowanie warunku STOP
	if (!wait_flag(&I2C1_SR1, (1U << 6), 10)) goto fail;			// czekanie na ustawienie flagi RXNE

	*out = I2C1_DR;													// odczytanie danych z rejestru DR
	return 1;

	fail:															// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
}

int i2c_read_burst(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t n){
	if (n < 3) return 0;													// funkcja obsługuje tylko odczyt co najmniej 3 bajtów
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), 10)) return 0;				// czekanie na zakończenie poprzedeniej transmisji
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), 10)) goto fail;					// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);													// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), 10)) goto fail;					// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;															// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;															// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), 10)) goto fail;					// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;															// wysłanie adresu rejestru, z którego chcemy odczytać dane
	if (!wait_flag(&I2C1_SR1, (1U << 2), 10)) goto fail;					// czekanie na ustawienie flagi BTF

	I2C1_CR1 |= (1U << 10);													// włącznie potwierdzenia odbioru
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START ponownie
	if (!wait_flag(&I2C1_SR1, (1U << 0), 10)) goto fail;					// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1) | 1;												// wysłanie adresu urządzenia z bitem R/W = 1 (odczyt)
	if (!wait_flag(&I2C1_SR1, (1U << 1), 10)) goto fail;					// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;
	(void)I2C1_SR2;
	for (uint8_t i=0; i < n - 3; i++) {
		if (!wait_flag(&I2C1_SR1, (1U << 6), 10)) goto fail;				// czekanie na ustawienie flagi RXNE
		buf[i] = I2C1_DR;													// odczytanie danych z rejestru DR
	}
	if (!wait_flag(&I2C1_SR1, (1U << 2), 10)) goto fail; 					// czekanie na ustawienie flagi BTF
	I2C1_CR1 &= ~(1U << 10);												// wyłączenie potwierdzenia odbioru
	buf[n - 3] = I2C1_DR;													// odczytanie danych z rejestru DR
	if (!wait_flag(&I2C1_SR1, (1U << 2), 10)) goto fail; 					// czekanie na ustawienie flagi BTF
	I2C1_CR1 |= (1U << 9);													// generowanie warunku STOP
	buf[n - 2] = I2C1_DR;													// odczytanie danych z rejestru DR
	buf[n - 1] = I2C1_DR;													// odczytanie danych z rejestru DR
	return 1;

	fail:																	// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
}

int32_t bme280_compensate_T_int32(int32_t adc_T) {
	int32_t var1, var2, T;
	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
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

static void dma2_clear_flags(void) {
	DMA2_LIFCR = (1U << 27) | (1U << 26) | (1U << 25) | (1U << 24) | (1U << 22); // wyczyszczenie flag TCIF3, HTIF3, TEIF3, DMEIF3, FEIF3
}


void oled_cmd(uint8_t cmd) {
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

void oled_clear(void) {
	for (uint16_t i = 0; i < sizeof(fb); i++) {
		fb[i] = 0; // wyczyszczenie bufora
	}
}

static const uint8_t font5x7[][5] = {
	{0x00, 0x00, 0x00, 0x00, 0x00}, // space (32)
	{0x00, 0x00, 0x5F, 0x00, 0x00}, // !
	{0x00, 0x07, 0x00, 0x07, 0x00}, // "
	{0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
	{0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
	{0x23, 0x13, 0x08, 0x64, 0x62}, // %
	{0x36, 0x49, 0x56, 0x20, 0x50}, // &
	{0x00, 0x05, 0x03, 0x00, 0x00}, // '
	{0x00, 0x1C, 0x22, 0x41, 0x00}, // (
	{0x00, 0x41, 0x22, 0x1C, 0x00}, // )
	{0x14, 0x08, 0x3E, 0x08, 0x14}, // *
	{0x08, 0x08, 0x3E, 0x08, 0x08}, // +
	{0x00, 0x50, 0x30, 0x00, 0x00}, // ,
	{0x08, 0x08, 0x08, 0x08, 0x08}, // -
	{0x00, 0x60, 0x60, 0x00, 0x00}, // .
	{0x20, 0x10, 0x08, 0x04, 0x02}, // slash
	{0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
	{0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
	{0x42, 0x61, 0x51, 0x49, 0x46}, // 2
	{0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
	{0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
	{0x27, 0x45, 0x45, 0x45, 0x39}, // 5
	{0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
	{0x01, 0x71, 0x09, 0x05, 0x03}, // 7
	{0x36, 0x49, 0x49, 0x49, 0x36}, // 8
	{0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
	{0x00, 0x36, 0x36, 0x00, 0x00}, // :
	{0x00, 0x56, 0x36, 0x00, 0x00}, // ;
	{0x08, 0x14, 0x22, 0x41, 0x00}, // <
	{0x14, 0x14, 0x14, 0x14, 0x14}, // =
	{0x00, 0x41, 0x22, 0x14, 0x08}, // >
	{0x02, 0x01, 0x51, 0x09, 0x06}, // question mark
	{0x32, 0x49, 0x79, 0x41, 0x3E}, // @
	{0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
	{0x7F, 0x49, 0x49, 0x49, 0x36}, // B
	{0x3E, 0x41, 0x41, 0x41, 0x22}, // C
	{0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
	{0x7F, 0x49, 0x49, 0x49, 0x41}, // E
	{0x7F, 0x09, 0x09, 0x09, 0x01}, // F
	{0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
	{0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
	{0x00, 0x41, 0x7F, 0x41, 0x00}, // I
	{0x20, 0x40, 0x41, 0x3F, 0x01}, // J
	{0x7F, 0x08, 0x14, 0x22, 0x41}, // K
	{0x7F, 0x40, 0x40, 0x40, 0x40}, // L
	{0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
	{0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
	{0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
	{0x7F, 0x09, 0x09, 0x09, 0x06}, // P
	{0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
	{0x7F, 0x09, 0x19, 0x29, 0x46}, // R
	{0x46, 0x49, 0x49, 0x49, 0x31}, // S
	{0x01, 0x01, 0x7F, 0x01, 0x01}, // T
	{0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
	{0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
	{0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
	{0x63, 0x14, 0x08, 0x14, 0x63}, // X
	{0x07, 0x08, 0x70, 0x08, 0x07}, // Y
	{0x61, 0x51, 0x49, 0x45, 0x43}, // Z
	{0x00, 0x7F, 0x41, 0x41, 0x00}, // [
	{0x02, 0x04, 0x08, 0x10, 0x20}, // backslash
	{0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
	{0x04, 0x02, 0x01, 0x02, 0x04}, // ^
	{0x40, 0x40, 0x40, 0x40, 0x40}, // _
	{0x00, 0x01, 0x02, 0x04, 0x00}, // `
	{0x20, 0x54, 0x54, 0x54, 0x78}, // a
	{0x7F, 0x48, 0x44, 0x44, 0x38}, // b
	{0x38, 0x44, 0x44, 0x44, 0x20}, // c
	{0x38, 0x44, 0x44, 0x48, 0x7F}, // d
	{0x38, 0x54, 0x54, 0x54, 0x18}, // e
	{0x08, 0x7E, 0x09, 0x01, 0x02}, // f
	{0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
	{0x7F, 0x08, 0x04, 0x04, 0x78}, // h
	{0x00, 0x44, 0x7D, 0x40, 0x00}, // i
	{0x20, 0x40, 0x44, 0x3D, 0x00}, // j
	{0x7F, 0x10, 0x28, 0x44, 0x00}, // k
	{0x00, 0x41, 0x7F, 0x40, 0x00}, // l
	{0x7C, 0x04, 0x18, 0x04, 0x78}, // m
	{0x7C, 0x08, 0x04, 0x04, 0x78}, // n
	{0x38, 0x44, 0x44, 0x44, 0x38}, // o
	{0x7C, 0x14, 0x14, 0x14, 0x08}, // p
	{0x08, 0x14, 0x14, 0x18, 0x7C}, // q
	{0x7C, 0x08, 0x04, 0x04, 0x08}, // r
	{0x48, 0x54, 0x54, 0x54, 0x20}, // s
	{0x04, 0x3F, 0x44, 0x40, 0x20}, // t
	{0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
	{0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
	{0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
	{0x44, 0x28, 0x10, 0x28, 0x44}, // x
	{0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
	{0x44, 0x64, 0x54, 0x4C, 0x44}, // z
	{0x00, 0x08, 0x36, 0x41, 0x00}, // {
	{0x00, 0x00, 0x7F, 0x00, 0x00}, // |
	{0x00, 0x41, 0x36, 0x08, 0x00}, // }
	{0x10, 0x08, 0x08, 0x10, 0x08}  // ~ (126)
};

void oled_draw_char(uint8_t x, uint8_t y, char c) {
	if (c < ' ' || c > '~') return;
	const uint8_t *char_bitmap = font5x7[c - ' '];
	for (uint8_t i = 0; i < 5; i++) {
		uint8_t column = char_bitmap[i];
		for (uint8_t j = 0; j < 7; j++) {
			uint8_t pixel_color = (column >> j) & 0x01;
			oled_set_pixel(x + i, y + j, pixel_color);
		}
	}
}

void oled_draw_string(uint8_t x, uint8_t y, const char *str) {
	while (*str) {
		if (x+5 > 128) break; // sprawdzenie, czy nie wyjdziemy poza ekran
		oled_draw_char(x, y, *str++);
		x += 6; // przesunięcie o szerokość znaku + 1 piksel odstępu
	}
}

static void oled_update_dma(void) {
	oled_cmd(0x21);   // ustawienie kolumn: 0-127
	oled_cmd(0x00);
	oled_cmd(0x7F);
	oled_cmd(0x22);   // ustawienie stron: 0-7
	oled_cmd(0x00);
	oled_cmd(0x07);

	DMA2_S3CR &= ~(1U << 0); // wyłączenie strumienia 3
	dma2_clear_flags();
	while (DMA2_S3CR & (1U << 0)); // oczekiwanie na wyłączenie strumienia 3
	DMA2_S3PAR = (uint32_t)&SPI1_DR; // ustawienie adresu rejestru danych SPI1 jako docelowego
	DMA2_S3M0AR = (uint32_t)fb; // ustawienie adresu bufora jako źródłowego
	DMA2_S3NDTR = sizeof(fb); // ustawienie liczby bajtów do przesłania
	DMA2_S3CR = (3U << 25) | (2U << 16) | (1U << 10) | (1U << 6);	// ustawienie priorytetu na wysoki, trybu pamięć->peryferia, włączenie przerwania po zakończeniu transferu, włączenie DMA

	GPIOA_BSRR = (1U << (OLED_CS_PIN + 16));  // ustawienie pinu CS w stan niski (wybranie wyświetlacza)
	GPIOB_BSRR = (1U << OLED_DC_PIN);          // ustawienie pinu DC w stan wysoki (tryb danych)

	SPI1_CR2 = (1U << 1);                                      // TXDMAEN
	DMA2_S3CR |= (1U << 0); // włączenie strumienia 3
}

static void oled_dma_finish(void) {
	while (!(DMA2_LISR & (1U << 27))); // oczekiwanie na ustawienie flagi TCIF3 (transfer zakończony)
	DMA2_LIFCR = (1U << 27);           // wyczyszczenie flagi TCIF3
	while (!(SPI1_SR & (1U << 1)));   // TXE: bufor pusty
	while (SPI1_SR & (1U << 7));      // BSY = 0: transmisja zakończona
	SPI1_CR2 &= ~(1U << 1);
	GPIOA_BSRR = (1U << OLED_CS_PIN);  // ustawienie pinu CS w stan wysoki (odznaczenie wyświetlacza)
}

void temperature_to_string(int32_t temp, char *str) {
	int i = 0;

	if (temp < 0) {
		str[i++] = '-';
		temp = -temp;
	}

	int32_t integer_part = temp / 100;
	int32_t fractional_part = temp % 100;
	if (integer_part >= 10) {
		str[i++] = '0' + (integer_part / 10);
	}
	str[i++] = '0' + (integer_part % 10);
	str[i++] = '.';
	str[i++] = '0' + (fractional_part / 10);
	str[i++] = '0' + (fractional_part % 10);
	str[i] = '\0';
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

void i2c1_init(void) {
	RCC_APB1ENR |= (1U << 21); 								// włączenie zegara dla I2C1

	GPIOB_MODER &= ~((3U << (2 * I2C1_SCL_PIN)) | (3U << (2 * I2C1_SDA_PIN))); 	// ustawienie trybu alternatywnego dla pinów PB8 i PB9
	GPIOB_MODER |=  ((2U << (2 * I2C1_SCL_PIN)) | (2U << (2 * I2C1_SDA_PIN)));
	GPIOB_AFRH &= ~(0xFU << (4 * (I2C1_SCL_PIN - 8)));
	GPIOB_AFRH |= (4U << (4 * (I2C1_SCL_PIN - 8))); 							// ustawienie funkcji alternatywnej AF4 (I2C1) dla pinu PB8
	GPIOB_AFRH &= ~(0xFU << (4 * (I2C1_SDA_PIN - 8)));
	GPIOB_AFRH |= (4U << (4 * (I2C1_SDA_PIN - 8))); 							// ustawienie funkcji alternatywnej AF4 (I2C1) dla pinu PB9
	GPIOB_OTYPER |= (1U << I2C1_SCL_PIN) | (1U << I2C1_SDA_PIN); 				// ustawienie typu wyjścia na open-drain dla pinów PB8 i PB9

	I2C1_CR2 = 16; 											// ustawienie częstotliwości zegara na 16 MHz
	I2C1_CCR = 80;											// ustawienie współczynnika podziału na 80, co daje prędkość transmisji 100 kHz
	I2C1_TRISE = 17;										// ustawienie maksymalnego czasu narastania na 1000 ns
	I2C1_CR1 |= (1U << 0);									// włączenie modułu I2C
}

void bme280_init(void) {
	i2c_write_reg(0x76, 0xF4, 0x27); 	// ustawienie trybu normalnego, oversampling x1 dla temperatury i ciśnienia, oversampling x1 dla wilgotności

	uint8_t calib_data[6];
	i2c_read_burst(0x76, 0x88, calib_data, 6); 	// odczytanie danych kalibracyjnych dla temperatury i ciśnienia
	dig_T1 = (uint16_t)((calib_data[1] << 8) | calib_data[0]);
	dig_T2 = (int16_t)((calib_data[3] << 8) | calib_data[2]);
	dig_T3 = (int16_t)((calib_data[5] << 8) | calib_data[4]);

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

void dma2_init(void) {
	RCC_AHB1ENR |= (1U << 22); 								// włączenie zegara dla DMA2
	DMA2_S3FCR = 0; 										// wyczyszczenie rejestru kontrolnego strumienia 3
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

volatile uint32_t cpu_work = 0;

int main(void)
{
	systick_init();
	gpio_init();
	spi1_init();
	i2c1_init();
	bme280_init();
	dma2_init();
	oled_init();

	uint32_t last_tick = 0;
	for (;;) {
		if (ms_ticks - last_tick >= 1000) {
			last_tick = ms_ticks;


			uint8_t temp_data[3];
			oled_clear();
			if (i2c_read_burst(0x76, 0xFA, temp_data, 3)) {
				int32_t adc_T = (int32_t)((temp_data[0] << 12) | (temp_data[1] << 4) | (temp_data[2] >> 4));
				int32_t temperature = bme280_compensate_T_int32(adc_T);

				char temperature_str[8];
				temperature_to_string(temperature, temperature_str);

				oled_draw_string(0, 0, "Temp: ");
				oled_draw_string(36, 0, temperature_str);
				oled_draw_string(72, 0, "C");
			}
			else {
				oled_draw_string(0, 0, "Temp: Error");
			}

			oled_update_dma();
			cpu_work = 0;
			while (!(DMA2_LISR & (1U << 27))) {
				cpu_work++;
			}
			oled_dma_finish();
		}
	}
}

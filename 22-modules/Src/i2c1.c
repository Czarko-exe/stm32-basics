#include "i2c1.h"
#include "regs.h"
#include "timebase.h"

#define I2C_TIMEOUT_MS 10
#define I2C1_SCL_PIN 8	// PB8
#define I2C1_SDA_PIN 9	// PB9

void i2c1_init(void) {
	RCC_AHB1ENR |= (1U << 1);
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
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), I2C_TIMEOUT_MS)) return 0;		// czekanie na zakończenie poprzedniej transmisji
	I2C1_CR1 |= (1U << 8);											// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), I2C_TIMEOUT_MS)) goto fail;			// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);											// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), I2C_TIMEOUT_MS)) goto fail;			// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;													// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;													// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), I2C_TIMEOUT_MS)) goto fail;			// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;													// wysłanie adresu rejestru, do którego chcemy zapisać dane
	if (!wait_flag(&I2C1_SR1, (1U << 7), I2C_TIMEOUT_MS)) goto fail;			// czekanie na ustawienie flagi TXE
	I2C1_DR = val;													// wysłanie danych do zapisania w rejestrze
	if (!wait_flag(&I2C1_SR1, (1U << 2), I2C_TIMEOUT_MS)) goto fail;			// czekanie na ustawienie flagi BTF

	I2C1_CR1 |= (1U << 9);											// generowanie warunku STOP
	return 1;

	fail:															// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
}

int i2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *out) {
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), I2C_TIMEOUT_MS)) return 0;	// czekanie na zakończenie poprzedniej transmisji
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);													// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;															// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;															// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;															// wysłanie adresu rejestru, z którego chcemy odczytać dane
	if (!wait_flag(&I2C1_SR1, (1U << 2), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi BTF
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START ponownie
	if (!wait_flag(&I2C1_SR1, (1U << 0), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1) | 1;												// wysłanie adresu urządzenia z bitem R/W = 1 (odczyt)
	if (!wait_flag(&I2C1_SR1, (1U << 1), I2C_TIMEOUT_MS)) goto fail;		// czekanie na ustawienie flagi ADDR

	I2C1_CR1 &= ~(1U << 10);												// wyłączenie potwierdzenia odbioru
	(void)I2C1_SR1;
	(void)I2C1_SR2;
	I2C1_CR1 |= (1U << 9);													// generowanie warunku STOP
	if (!wait_flag(&I2C1_SR1, (1U << 6), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi RXNE

	*out = I2C1_DR;															// odczytanie danych z rejestru DR
	return 1;

	fail:																	// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
}

int i2c_read_burst(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t n){
	if (n < 3) return 0;													// funkcja obsługuje tylko odczyt co najmniej 3 bajtów
	if (!wait_flag_clear(&I2C1_SR2, (1U << 1), I2C_TIMEOUT_MS)) return 0;				// czekanie na zakończenie poprzedeniej transmisji
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START
	if (!wait_flag(&I2C1_SR1, (1U << 0), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1);													// wysłanie adresu urządzenia z bitem R/W = 0 (zapis)
	if (!wait_flag(&I2C1_SR1, (1U << 1), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;															// odczytanie rejestru SR1 w celu wyczyszczenia flagi ADDR
	(void)I2C1_SR2;															// odczytanie rejestru SR2 w celu wyczyszczenia flagi ADDR
	if (!wait_flag(&I2C1_SR1, (1U << 7), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi TXE
	I2C1_DR = reg;															// wysłanie adresu rejestru, z którego chcemy odczytać dane
	if (!wait_flag(&I2C1_SR1, (1U << 2), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi BTF

	I2C1_CR1 |= (1U << 10);													// włącznie potwierdzenia odbioru
	I2C1_CR1 |= (1U << 8);													// generowanie warunku START ponownie
	if (!wait_flag(&I2C1_SR1, (1U << 0), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi SB
	I2C1_DR = (dev << 1) | 1;												// wysłanie adresu urządzenia z bitem R/W = 1 (odczyt)
	if (!wait_flag(&I2C1_SR1, (1U << 1), I2C_TIMEOUT_MS)) goto fail;					// czekanie na ustawienie flagi ADDR
	(void)I2C1_SR1;
	(void)I2C1_SR2;
	for (uint8_t i=0; i < n - 3; i++) {
		if (!wait_flag(&I2C1_SR1, (1U << 6), I2C_TIMEOUT_MS)) goto fail;				// czekanie na ustawienie flagi RXNE
		buf[i] = I2C1_DR;													// odczytanie danych z rejestru DR
	}
	if (!wait_flag(&I2C1_SR1, (1U << 2), I2C_TIMEOUT_MS)) goto fail; 					// czekanie na ustawienie flagi BTF
	I2C1_CR1 &= ~(1U << 10);												// wyłączenie potwierdzenia odbioru
	buf[n - 3] = I2C1_DR;													// odczytanie danych z rejestru DR
	if (!wait_flag(&I2C1_SR1, (1U << 2), I2C_TIMEOUT_MS)) goto fail; 					// czekanie na ustawienie flagi BTF
	I2C1_CR1 |= (1U << 9);													// generowanie warunku STOP
	buf[n - 2] = I2C1_DR;													// odczytanie danych z rejestru DR
	buf[n - 1] = I2C1_DR;													// odczytanie danych z rejestru DR
	return 1;

	fail:																	// w przypadku błędu transmisji generowany jest warunek STOP i funkcja zwraca 0
	    I2C1_SR1 &= ~(1U << 10);
	    I2C1_CR1 |= (1U << 9);
	    return 0;
}

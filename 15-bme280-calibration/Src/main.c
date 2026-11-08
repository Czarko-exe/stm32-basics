#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_AFRL  (*(volatile uint32_t *)0x40020020)
#define GPIOB_MODER  (*(volatile uint32_t *)0x40020400)
#define GPIOB_AFRH   (*(volatile uint32_t *)0x40020424)
#define GPIOB_OTYPER  (*(volatile uint32_t *)0x40020404)

#define I2C1_CR1     (*(volatile uint32_t *)0x40005400)
#define I2C1_CR2     (*(volatile uint32_t *)0x40005404)
#define I2C1_DR      (*(volatile uint32_t *)0x40005410)
#define I2C1_SR1     (*(volatile uint32_t *)0x40005414)
#define I2C1_SR2     (*(volatile uint32_t *)0x40005418)
#define I2C1_CCR     (*(volatile uint32_t *)0x4000541C)
#define I2C1_TRISE   (*(volatile uint32_t *)0x40005420)

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)

#define USART2_SR    (*(volatile uint32_t *)0x40004400)
#define USART2_DR    (*(volatile uint32_t *)0x40004404)
#define USART2_BRR   (*(volatile uint32_t *)0x40004408)
#define USART2_CR1   (*(volatile uint32_t *)0x4000440C)

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

void uart_send_char(char c){				// funkcja wysyłająca pojedynczy znak przez UART
	while (!(USART2_SR & (1U << 7)));
	USART2_DR = c;
}

void uart_send_string(const char *str){		// funkcja wysyłająca łańcuch znaków przez UART
	while (*str){
		uart_send_char(*str++);
	}
}

uint32_t length_of_number(uint32_t number) {
	uint32_t length = 0;
	do {
		length++;
		number /= 10;
	} while (number != 0);
	return length;
}

void uart_send_number(uint32_t number) {
	char buffer[12]; // enough to hold the maximum value of a 32-bit unsigned integer
	uint32_t count = length_of_number(number);
	for (uint32_t i = 0; i < count; i++) {
		buffer[i] = number % 10 + '0';
		number /= 10;
	}
	for (int i = count - 1; i >= 0; i--) {
		uart_send_char(buffer[i]);
	}
}

void uart_send_signed(int32_t number) {
	if (number < 0) {
		uart_send_char('-');
		number = -number;
	}
	uart_send_number((uint32_t)number);
}

void uart_send_hex(uint8_t data[], uint8_t n) {
		char hex_chars[] = "0123456789ABCDEF";
		uart_send_string("0x");
		for (int i = 0; i < n; i++) {
				uart_send_char(hex_chars[(data[i] >> 4) & 0x0F]);
				uart_send_char(hex_chars[data[i] & 0x0F]);
				uart_send_string(" ");
		}
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


int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

	RCC_AHB1ENR |= (1U << 0);
	RCC_AHB1ENR |= (1U << 1);
	RCC_APB1ENR |= (1U << 17);
	RCC_APB1ENR |= (1U << 21);

	GPIOA_MODER &= ~(3U << (2 * 2));
	GPIOA_MODER |= (2U << (2 * 2));
	GPIOA_AFRL &= ~(0xFU << (4 * 2));
	GPIOA_AFRL |= (7U << (4 * 2));

	GPIOB_MODER &= ~((3U << (2 * 8)) | (3U << (2 * 9)));
	GPIOB_MODER |=  ((2U << (2 * 8)) | (2U << (2 * 9)));
	GPIOB_AFRH |= (4U << 0) | (4U << 4);
	GPIOB_OTYPER |= (1U << 8) | (1U << 9);

	I2C1_CR2 = 16;											// ustawienie częstotliwości zegara I2C na 16 MHz
	I2C1_CCR = 80;											// ustawienie szybkości transmisji I2C na 100 kHz
	I2C1_TRISE = 17;										// ustawienie maksymalnego czasu narastania sygnału I2C na 1000 ns przy zegarze 16 MHz
	I2C1_CR1 |= (1U << 0);									// włączenie interfejsu I2C1


	USART2_BRR = 0x8B;
	USART2_CR1 |= (1U << 3) | (1U << 2) | (1U << 13);

	if (!i2c_write_reg(0x76, 0xF4, 0x27)) {
	    uart_send_string("Config write failed\r\n");
	}

	uint32_t time = 0;

	uint8_t calib[6];

	if (i2c_read_burst(0x76, 0x88, calib, 6)) {
					uart_send_string("Calibration data: ");
					uart_send_hex(calib, 6);
					uart_send_string("\r\n");

					uint16_t dig_T1 = (uint16_t)((calib[1] << 8) | calib[0]);
					int16_t  dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
					int16_t  dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);
					uart_send_string("dig_T1: ");
					uart_send_number(dig_T1);
					uart_send_string("\r\n");
					uart_send_string("dig_T2: ");
					uart_send_number(dig_T2);
					uart_send_string("\r\n");
					uart_send_string("dig_T3: ");
					uart_send_number(dig_T3);

				}
				else {
				    uart_send_string("I2C error");
				}
				uart_send_string("\r\n");

	for (;;) {
	}
}

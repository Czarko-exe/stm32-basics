#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)
#define GPIOA_AFRL   (*(volatile uint32_t *)0x40020020)
#define USART2_SR    (*(volatile uint32_t *)0x40004400)
#define USART2_DR    (*(volatile uint32_t *)0x40004404)
#define USART2_BRR   (*(volatile uint32_t *)0x40004408)
#define USART2_CR1   (*(volatile uint32_t *)0x4000440C)

#define RCC_APB2ENR  (*(volatile uint32_t *)0x40023844)
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define ADC1_SR	  	 (*(volatile uint32_t *)0x40012000)
#define ADC1_CR2	 (*(volatile uint32_t *)0x40012008)
#define ADC1_SMPR2	 (*(volatile uint32_t *)0x40012010)
#define ADC1_SQR1	 (*(volatile uint32_t *)0x4001202C)
#define ADC1_SQR3	 (*(volatile uint32_t *)0x40012034)
#define ADC1_DR		 (*(volatile uint32_t *)0x4001204C)


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

int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

	RCC_AHB1ENR |= (1U << 0);						// włączenie zegara dla portu A
	RCC_APB1ENR |= (1U << 17);						// włączenie zegara dla USART2

	GPIOA_MODER &= ~(3U << (2 * 2));        // wyzerowanie trybu PA2
	GPIOA_MODER |=  (2U << (2 * 2));        // PA2 = alternate function
	GPIOA_AFRL  &= ~(0xFU << (4 * 2));      // wyzerowanie AFRL dla PA2
	GPIOA_AFRL  |=  (7U << (4 * 2));        // AF7 = USART2

	USART2_BRR = 0x8B;								// ustawienie prędkości transmisji na 115200 bps przy zegarze 16 MHz
	USART2_CR1 |= (1U << 3) | (1U << 2) | (1U << 13);	// włączenie odbiornika, nadajnika i USART

	RCC_APB2ENR |= (1U << 8);						// włączenie zegara dla ADC1
	GPIOA_MODER |= (3U << (2 * 0));					// ustawienie pinu PA0 jako wejście analogowe

	ADC1_SMPR2 |= 7U;                        // 480 cykli próbkowania dla kanału 0
	ADC1_SQR1  &= ~(0xFU << 20);             // jedna konwersja w sekwencji
	ADC1_SQR3  &= ~0x1FU;                    // pierwsza konwersja: kanał 0 (PA0)// włączenie ADC1
	ADC1_CR2 |= (1U << 0);

	uint32_t time = 0;
	for (;;) {
		if (ms_ticks - time >= 200) {
			time = ms_ticks;
			ADC1_CR2 |= (1U << 30);					// rozpoczecie konwersji ADC1
			while (!(ADC1_SR & (1U << 1)));			// czekanie na zakończenie konwersji
			uint16_t adc_value = ADC1_DR;

			uart_send_string("ADC Value: ");
			uart_send_number(adc_value);

			uint32_t mv = (uint32_t)((adc_value * 3300) / 4095); // przeliczanie wartości ADC na napięcie w mV
			uart_send_string(", Voltage: ");
			uart_send_number(mv / 1000);
			uart_send_string(".");
			uart_send_number((mv % 1000) / 100);

			uart_send_string("\r\n");
		}
	}

}

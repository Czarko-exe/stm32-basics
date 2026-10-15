#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_ODR    (*(volatile uint32_t *)0x40020014)
#define GPIOA_BSRR	 (*(volatile uint32_t *)0x40020018)

#define GPIOC_MODER  (*(volatile uint32_t *)0x40020800)
#define GPIOC_IDR	 (*(volatile uint32_t *)0x40020810)

#define LED_PIN 5
#define BTN_PIN 13

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)
#define GPIOA_AFRL   (*(volatile uint32_t *)0x40020020)
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

int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

	RCC_AHB1ENR |= (1U << 0);						// włączenie zegara dla portu A
	RCC_APB1ENR |= (1U << 17);						// włączenie zegara dla USART2

	GPIOA_MODER &= ~(3U << (2 * 2));				// wyzerowanie trybu pinu PA2
	GPIOA_MODER |= (2U << (2 * 2));					// ustawienie trybu alternatywnego dla pinu PA2
	GPIOA_AFRL &= ~(0xFU << (4 * 2));				// wyzerowanie rejestru AFRL dla pinu PA2
	GPIOA_AFRL |= (7U << (4 * 2));					// ustawienie funkcji alternatywnej AF7 (USART2) dla pinu PA2

	USART2_BRR = 0x8B;								// ustawienie prędkości transmisji na 115200 bps przy zegarze 16 MHz
	USART2_CR1 |= (1U << 3) | (1U << 2) | (1U << 13);	// włączenie odbiornika, nadajnika i USART

	uint32_t time = 0;
	for (;;) {
		if (ms_ticks - time >= 1000) {
			time = ms_ticks;
			uart_send_string("Hello, world!\r\n");
		}
	}

}

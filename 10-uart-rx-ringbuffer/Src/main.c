#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_BSRR	 (*(volatile uint32_t *)0x40020018)

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)
#define GPIOA_AFRL   (*(volatile uint32_t *)0x40020020)
#define USART2_SR    (*(volatile uint32_t *)0x40004400)
#define USART2_DR    (*(volatile uint32_t *)0x40004404)
#define USART2_BRR   (*(volatile uint32_t *)0x40004408)
#define USART2_CR1   (*(volatile uint32_t *)0x4000440C)
#define NVIC_ISER1 (*(volatile uint32_t *)0xE000E104)

#define LED_PIN 5
#define RX_PIN 3
#define TX_PIN 2

#define RX_BUF_SIZE 64

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

volatile uint8_t  rx_buf[RX_BUF_SIZE];
volatile uint32_t rx_head = 0;
volatile uint32_t rx_tail = 0;

void USART2_IRQHandler(void)
{
    if (USART2_SR & (1U << 5)) {
        uint8_t c = USART2_DR;
        uint32_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);
        if (next != rx_tail) {
			rx_buf[rx_head] = c;
			rx_head = next;
        }
    }
}

void uart_send_char(char c){
	while (!(USART2_SR & (1U << 7)));
	USART2_DR = c;
}

void uart_send_string(const char *str){
	while (*str){
		uart_send_char(*str++);
	}
}

int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

	RCC_AHB1ENR |= (1U << 0);
	RCC_APB1ENR |= (1U << 17);

	GPIOA_MODER &= ~(3U << (2 * TX_PIN));
	GPIOA_MODER |=  (2U << (2 * TX_PIN));
	GPIOA_AFRL  &= ~(0xFU << (4 * TX_PIN));
	GPIOA_AFRL  |=  (7U << (4 * TX_PIN));

	GPIOA_MODER &= ~(3U << (2 * LED_PIN));
	GPIOA_MODER |=  (1U << (2 * LED_PIN));

	GPIOA_MODER &= ~(3U << (2 * RX_PIN));
	GPIOA_MODER |= (2U << (2 * RX_PIN));
	GPIOA_AFRL &= ~(0xFU << (4 * RX_PIN));
	GPIOA_AFRL |= (7U << (4 * RX_PIN));

	USART2_BRR = 0x8B;
	USART2_CR1 |= (1U << 3) | (1U << 2) | (1U << 13) | (1U << 5);
	NVIC_ISER1 |= (1U << 6);

	for (;;) {
		while (rx_head != rx_tail) {
			uint8_t rx_char = rx_buf[rx_tail];
			rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
			switch (rx_char) {
				case '1':
					GPIOA_BSRR = (1U << LED_PIN);
					uart_send_string("LED ON\r\n");
					break;
				case '0':
					GPIOA_BSRR = (1U << (LED_PIN + 16));
					uart_send_string("LED OFF\r\n");
					break;
				case '?':
					uart_send_string("Commands:\r\n");
					uart_send_string("1 - Turn LED ON\r\n");
					uart_send_string("0 - Turn LED OFF\r\n");
					uart_send_string("? - Show this help message\r\n");
					break;
				case '\r':
				case '\n':
					break;
				default:
					uart_send_string("Invalid input\r\n");
					break;
			}
		}
	}

}

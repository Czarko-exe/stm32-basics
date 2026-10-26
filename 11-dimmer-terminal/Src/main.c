#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_AFRL   (*(volatile uint32_t *)0x40020020)

#define GPIOC_MODER  (*(volatile uint32_t *)0x40020800)

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)

#define USART2_SR    (*(volatile uint32_t *)0x40004400)
#define USART2_DR    (*(volatile uint32_t *)0x40004404)
#define USART2_BRR   (*(volatile uint32_t *)0x40004408)
#define USART2_CR1   (*(volatile uint32_t *)0x4000440C)
#define NVIC_ISER1 	 (*(volatile uint32_t *)0xE000E104)

#define TIM2_PSC	(*(volatile uint32_t *)0x40000028)
#define TIM2_ARR	(*(volatile uint32_t *)0x4000002C)
#define TIM2_CCR1	(*(volatile uint32_t *)0x40000034)
#define TIM2_CCMR1	(*(volatile uint32_t *)0x40000018)
#define TIM2_CCER	(*(volatile uint32_t *)0x40000020)
#define TIM2_EGR	(*(volatile uint32_t *)0x40000014)
#define TIM2_CR1	(*(volatile uint32_t *)0x40000000)

#define RCC_APB2ENR  	(*(volatile uint32_t *)0x40023844)
#define SYSCFG_EXTICR4  (*(volatile uint32_t *)0x40013814)
#define EXTI_IMR   		(*(volatile uint32_t *)0x40013C00)
#define EXTI_FTSR 		(*(volatile uint32_t *)0x40013C0C)
#define EXTI_PR   		(*(volatile uint32_t *)0x40013C14)

#define RX_BUF_SIZE 64
#define LED_PIN 5
#define BTN_PIN 13
#define RX_PIN 3
#define TX_PIN 2

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

volatile uint32_t button_event = 0;
volatile uint32_t last_press = 0;
void EXTI15_10_IRQHandler(void)
{
	if (EXTI_PR & (1U << 13)) {
		EXTI_PR = (1U << 13);
		if (ms_ticks - last_press >= 50) {
			last_press = ms_ticks;
			button_event = 1;
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

void uart_send_number(uint32_t number) {
	char buffer[12]; // enough to hold the maximum value of a 32-bit unsigned integer
	uint32_t count = 0;
	uint32_t temp = number;
	do {
		count++;
		temp /= 10;
	} while (temp != 0);
	for (uint32_t i = 0; i < count; i++) {
		buffer[i] = number % 10 + '0';
		number /= 10;
	}
	for (int i = count - 1; i >= 0; i--) {
		uart_send_char(buffer[i]);
	}
}

void apply_duty(uint32_t duty) {
	if (duty > 1000) {
		duty = 1000;
	}
	TIM2_CCR1 = duty;
}

void print_duty_cycle(const char *prefix) {
	uart_send_string(prefix);
	uart_send_number(TIM2_CCR1);
	uart_send_string("\r\n");
}

int main(void) {
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

	RCC_AHB1ENR |= (1U << 0);
	RCC_AHB1ENR |= (1U << 2);
	GPIOA_MODER &= ~(3U << (2 * TX_PIN));
	GPIOA_MODER |= (2U << (2 * TX_PIN));
	GPIOA_AFRL &= ~(0xFU << (4 * TX_PIN));
	GPIOA_AFRL |= (7U << (4 * TX_PIN));

	GPIOA_MODER &= ~(3U << (2 * RX_PIN));
	GPIOA_MODER |= (2U << (2 * RX_PIN));
	GPIOA_AFRL &= ~(0xFU << (4 * RX_PIN));
	GPIOA_AFRL |= (7U << (4 * RX_PIN));

	GPIOA_MODER &= ~(3U << (2 * LED_PIN));
	GPIOA_MODER |= (2U << (2 * LED_PIN));
	GPIOA_AFRL &= ~(0xFU << (4 * LED_PIN));
	GPIOA_AFRL |= (1U << (4 * LED_PIN));

	RCC_APB1ENR |= (1U << 0);
	TIM2_PSC = 15;
	TIM2_ARR = 999;
	TIM2_CCR1 = 0;
	TIM2_CCMR1 |= (6U << 4);
	TIM2_CCMR1 |= (1U << 3);
	TIM2_CCER |= (1U << 0);
	TIM2_EGR |= (1U << 0);
	TIM2_CR1 |= (1U << 0);

	RCC_APB1ENR |= (1U << 17);
	USART2_BRR = 0x8B;
	USART2_CR1 |= (1U << 3) | (1U << 2) | (1U << 13) | (1U << 5);
	NVIC_ISER1 |= (1U << 6);

    GPIOC_MODER &= ~(3U << (BTN_PIN * 2));

    RCC_APB2ENR |= (1U << 14);
    SYSCFG_EXTICR4 &= ~(0xFU << 4);
    SYSCFG_EXTICR4 |= (2U << 4);
    EXTI_IMR |= (1U << BTN_PIN);
    EXTI_FTSR |= (1U << BTN_PIN);
    NVIC_ISER1 |= (1U << 8);

	uint32_t duty = TIM2_CCR1;
	uint32_t led_state = 0;
	for (;;) {
		if (button_event) {
			button_event = 0;
			if (TIM2_CCR1 > 0) {
				led_state = 0;
				TIM2_CCR1 = 0;
				uart_send_string("LED OFF\r\n");
			} else if (TIM2_CCR1 == 0 && duty == 0) {
				led_state = 1;
				duty = 500;
				apply_duty(duty);
				uart_send_string("LED ON with 500 duty cycle\r\n");
			} else if (TIM2_CCR1 == 0 && duty > 0) {
				led_state = 1;
				apply_duty(duty);
				print_duty_cycle("LED ON with duty cycle: ");
			}
		}

		while (rx_head != rx_tail) {
			uint8_t c = rx_buf[rx_tail];
			rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
			if (led_state == 0) {
				uart_send_string("LED is OFF. Press the button to turn it ON.\r\n");
				continue;
			}
			switch(c) {
			case '+':
				if (TIM2_CCR1 < 1000) {
					duty += 100;
					apply_duty(duty);
				}
				if (TIM2_CCR1 >= 1000) {
					print_duty_cycle("Duty cycle is at maximum: ");
				}
				else {
					print_duty_cycle("Duty cycle increased to: ");
				}
				break;
			case '-':
				if (TIM2_CCR1 > 0) {
					duty -= 100;
					apply_duty(duty);
				}
				if (TIM2_CCR1 <= 0) {
					print_duty_cycle("Duty cycle is at minimum: ");
				}
				else {
					print_duty_cycle("Duty cycle decreased to: ");
				}
				break;
			case '?':
				print_duty_cycle("Current duty cycle: ");
				break;
			case '\r':
			case '\n':
				break;
			default:
				uart_send_string("Unknown command\r\n");
				break;
			}
		}
	}
}

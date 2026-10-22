#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_ODR    (*(volatile uint32_t *)0x40020014)
#define GPIOA_BSRR	 (*(volatile uint32_t *)0x40020018)

#define GPIOC_MODER  (*(volatile uint32_t *)0x40020800)

#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)

#define RCC_APB2ENR  	(*(volatile uint32_t *)0x40023844)
#define SYSCFG_EXTICR4  (*(volatile uint32_t *)0x40013814)
#define EXTI_IMR   		(*(volatile uint32_t *)0x40013C00)
#define EXTI_FTSR 		(*(volatile uint32_t *)0x40013C0C)
#define EXTI_PR   		(*(volatile uint32_t *)0x40013C14)
#define NVIC_ISER1 		(*(volatile uint32_t *)0xE000E104)


#define LED_PIN 5
#define BTN_PIN 13

volatile uint32_t ms_ticks = 0;
void SysTick_Handler(void)
{
    ms_ticks++;
}

volatile uint32_t button_event = 0;
volatile uint32_t last_press = 0;
void EXTI15_10_IRQHandler(void)
{
	if (EXTI_PR & (1U << 13)) {
		EXTI_PR = (1U << 13);					//zerowanie flagi przerwania dla pinu 13
		if (ms_ticks - last_press >= 50) {
			last_press = ms_ticks;
			button_event = 1;					//ustawienie flagi zdarzenia przycisku
		}
	}
}

int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);

    RCC_AHB1ENR |= (1U << 0);
    RCC_AHB1ENR |= (1U << 2);

    GPIOA_MODER &= ~(3U << (LED_PIN * 2));
    GPIOA_MODER |=  (1U << (LED_PIN * 2));

    GPIOC_MODER &= ~(3U << (BTN_PIN * 2));

    RCC_APB2ENR |= (1U << 14);					// włączenie zegara dla SYSCFG
    SYSCFG_EXTICR4 &= ~(0xFU << 4);
    SYSCFG_EXTICR4 |= (2U << 4);				// wybór portu C dla pinu 13 w rejestrze SYSCFG_EXTICR4
    EXTI_IMR |= (1U << BTN_PIN);				// włączenie przerwania dla pinu 13
    EXTI_FTSR |= (1U << BTN_PIN);				// ustawienie wyzwalania przerwania na zbocze opadające dla pinu 13
    NVIC_ISER1 |= (1U << 8);					// włączenie przerwania EXTI15_10 w NVIC (IRQ 40, czyli bit 8 w ISER1)

    for (;;) {
    	if (button_event) {
			button_event = 0;
			if (GPIOA_ODR & (1U << LED_PIN)) {
				GPIOA_BSRR = (1U << (LED_PIN + 16));	// wyłączenie diody LED
			} else {
				GPIOA_BSRR = (1U << LED_PIN);			// włączenie diody LED
			}
		}
    }
}

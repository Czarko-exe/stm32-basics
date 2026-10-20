#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)		// rejestr włączania zegara dla peryferiów na magistrali AHB1
#define RCC_APB1ENR  (*(volatile uint32_t *)0x40023840)		// rejestr włączania zegara dla peryferiów na magistrali APB1
#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)		// rejestr trybu pracy portu A
#define GPIOA_AFRL   (*(volatile uint32_t *)0x40020020)		// rejestr wyboru funkcji alternatywnej dla pinów 0-

#define TIM2_PSC	(*(volatile uint32_t *)0x40000028)		// rejestr preskalera timera TIM2
#define TIM2_ARR	(*(volatile uint32_t *)0x4000002C)		// rejestr wartości przepełnienia timera TIM2
#define TIM2_CCR1	(*(volatile uint32_t *)0x40000034)		// rejestr porównania kanału 1 timera TIM2
#define TIM2_CCMR1	(*(volatile uint32_t *)0x40000018)		// rejestr konfiguracji kanału 1 timera TIM2
#define TIM2_CCER	(*(volatile uint32_t *)0x40000020)		// rejestr włączania wyjścia kanału 1 timera TIM2
#define TIM2_EGR	(*(volatile uint32_t *)0x40000014)		// rejestr generowania zdarzeń timera TIM2
#define TIM2_CR1	(*(volatile uint32_t *)0x40000000)		// rejestr sterowania timera TIM2

#define SYST_CSR 	 (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR 	 (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR 	 (*(volatile uint32_t *)0xE000E018)

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

int main(void)
{
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);	// timer

	RCC_AHB1ENR |= (1U << 0);
	RCC_APB1ENR |= (1U << 0);      					// włączenie zegara dla portu A i timera TIM2

	GPIOA_MODER &= ~(3U << (2 * 5));
	GPIOA_MODER |= (2U << (2 * 5));
	GPIOA_AFRL &= ~(0xFU << (4 * 5));
	GPIOA_AFRL |= (1U << (4 * 5));					// ustawienie funkcji alternatywnej AF1 (TIM2_CH1) dla pinu PA5

	TIM2_PSC = 15;									// preskaler timera TIM2 (16 MHz / (15 + 1) = 1 MHz)
	TIM2_ARR = 999;									// ilość kroków w cyklu PWM (1000 kroków, co daje rozdzielczość 0,1%)
	TIM2_CCR1 = 0;									// początkowa wartość wypełnienia (0% jasności)
	TIM2_CCMR1 |= (6U << 4);						// ustawienie trybu PWM1 dla kanału 1
	TIM2_CCMR1 |= (1U << 3);						// włączenie wyjścia kanału 1 w trybie PWM
	TIM2_CCER |= (1U << 0);							// włączenie wyjścia kanału 1
	TIM2_EGR |= (1U << 0);							// wygenerowanie zdarzenia aktualizacji, aby zaktualizować rejestry timera
	TIM2_CR1 |= (1U << 0);							// włączenie timera TIM2

	RCC_APB2ENR |= (1U << 8);						// włączenie zegara dla ADC1
	GPIOA_MODER |= (3U << (2 * 0));					// ustawienie pinu PA0 jako wejście analogowe

	ADC1_SMPR2 |= 7U;                        // 480 cykli próbkowania dla kanału 0
	ADC1_SQR1  &= ~(0xFU << 20);             // jedna konwersja w sekwencji
	ADC1_SQR3  &= ~0x1FU;                    // pierwsza konwersja: kanał 0 (PA0)
	ADC1_CR2 |= (1U << 0);


	uint32_t time = 0;
	int32_t filt = 0;

	for (;;) {
		if (ms_ticks - time >= 5) {
			time = ms_ticks;
			ADC1_CR2 |= (1U << 30);					// rozpoczecie konwersji ADC1
			while (!(ADC1_SR & (1U << 1)));			// czekanie na zakończenie konwersji
			uint16_t adc_value = ADC1_DR;

			filt += ((int32_t)adc_value * 16 - filt) / 8;
			TIM2_CCR1 = (uint32_t)(filt * 999 / (4095 * 16));
		}
	}

}

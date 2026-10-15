#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
#define GPIOA_ODR    (*(volatile uint32_t *)0x40020014)
#define GPIOA_BSRR	 (*(volatile uint32_t *)0x40020018)

#define GPIOC_MODER  (*(volatile uint32_t *)0x40020800)
#define GPIOC_IDR	 (*(volatile uint32_t *)0x40020810)

#define LED_PIN 5
#define BTN_PIN 13

#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;											// inkrementacja licznika milisekund
}

int main(void)
{
	SYST_RVR = 16000 - 1;                       		// ustawienie rejestru przeładowania na 16000 (dla 1 ms przy zegarze 16 MHz)
	SYST_CVR = 0;                               		// wyzerowanie rejestru bieżącej wartości
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);   	// włączenie SysTick, włączenie przerwań i ustawienie źródła zegara na AHB

    RCC_AHB1ENR |= (1U << 0);          					//zegar dla portu A
    RCC_AHB1ENR |= (1U << 2);          					//zegar dla portu C

    GPIOA_MODER &= ~(3U << (LED_PIN * 2));   			//zeruje pin PA5
    GPIOA_MODER |=  (1U << (LED_PIN * 2));   			//ustawia pin PA5 jako wyjscie (01)

    GPIOC_MODER &= ~(3U << (BTN_PIN * 2));  			//zeruje i ustawia pin PC 13 jako wejście (00)

    uint32_t last_raw = (GPIOC_IDR >> BTN_PIN) & 1U;	// odczyt stanu przycisku
    uint32_t stable   = last_raw;						// stan stabilny przycisku
    uint32_t t_change = 0;								// czas ostatniej zmiany stanu przycisku
    for (;;) {
    	uint32_t now = (GPIOC_IDR >> BTN_PIN) & 1U;		// odczyt stanu przycisku

    	if (now != last_raw) {							// jeśli stan przycisku się zmienił
    	        last_raw = now;							// aktualizacja ostatniego odczytanego stanu przycisku
    	        t_change = ms_ticks;					// aktualizacja czasu ostatniej zmiany stanu przycisku
    	    }
    	if (now != stable && (ms_ticks - t_change) >= 20) {		// jeśli stan przycisku jest różny od stanu stabilnego i minęło 20 ms od ostatniej zmiany
    		stable = now;
    		if (stable == 0) {
    	    	if (GPIOA_ODR & (1U << LED_PIN)) {
    	        	GPIOA_BSRR = (1U << (LED_PIN + 16));
    	    	} else {
    	        	GPIOA_BSRR = (1U << LED_PIN);
    	        }
    	    }
    	}
    }
}

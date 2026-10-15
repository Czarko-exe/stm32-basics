#include <stdint.h>

#define RCC_AHB1ENR  (*(volatile uint32_t *)0x40023830)

#define GPIOA_MODER  (*(volatile uint32_t *)0x40020000)
//#define GPIOA_ODR    (*(volatile uint32_t *)0x40020014)
#define GPIOA_BSRR	 (*(volatile uint32_t *)0x40020018)

#define GPIOC_MODER  (*(volatile uint32_t *)0x40020800)
#define GPIOC_IDR	 (*(volatile uint32_t *)0x40020810)

#define LED_PIN 5
#define BTN_PIN 13

int main(void)
{
    RCC_AHB1ENR |= (1U << 0);          //zegar dla portu A
    RCC_AHB1ENR |= (1U << 2);          //zegar dla portu C

    GPIOA_MODER &= ~(3U << (LED_PIN * 2));   //zeruje pin PA5
    GPIOA_MODER |=  (1U << (LED_PIN * 2));   //ustawia pin PA5 jako wyjscie (01)

    GPIOC_MODER &= ~(3U << (BTN_PIN * 2));  //zeruje i ustawia pin PC 13 jako wejście (00)

    for (;;) {
    	if (GPIOC_IDR & (1U << BTN_PIN)) {
			//GPIOA_ODR &= ~(1U << 5);
    		GPIOA_BSRR = (1U << (LED_PIN + 16));
		} else {
			//GPIOA_ODR |= (1U << 5);
			GPIOA_BSRR = (1U << LED_PIN);
		}
    }
}

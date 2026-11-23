#include "timebase.h"
#include "regs.h"

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
	ms_ticks++;
}

void systick_init(void) {
	SYST_RVR = 16000 - 1;
	SYST_CVR = 0;
	SYST_CSR = (1U << 0) | (1U << 1) | (1U << 2);
}

void delay_ms(uint32_t ms)
{
	uint32_t start_ticks = ms_ticks;
	while ((ms_ticks - start_ticks) < ms);
}

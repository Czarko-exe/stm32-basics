# stm32-basics

Register-level C exercises on the STM32F446RE (NUCLEO-F446RE), written
without HAL to understand what the peripherals actually do.
Each folder is a separate STM32CubeIDE project.

## Hardware and tools
- NUCLEO-F446RE, STM32CubeIDE, PuTTY (serial terminal)
- 24 MHz 8-channel logic analyzer + PulseView
- Potentiometer, LEDs, breadboard, multimeter

## Exercises
| Folder | Topic |
|---|---|
| 01-gpio-bsrr | GPIO output/input, atomic set/reset with BSRR |
| 02-gpio-toggle | Button edge detection, LED toggle |
| 03-gpio-debounce | SysTick 1 ms timer, 20 ms debounce |
| 04-uart-tx | USART2 TX at 115200 baud |
| 05-tim2-pwm | TIM2 PWM, LED fade |
| 06-adc-pot | ADC1 polled conversion, readings over UART |
| 07-adc-pwm-dimmer | Potentiometer controls LED brightness |
| 08-exti-button | EXTI interrupt on the button |
| 09-uart-rx-irq | USART2 RX interrupt, terminal commands |
| 10-uart-rx-ringbuffer | RX ring buffer so no characters are lost |
| 11-dimmer-terminal | Add LED dimmer controlled by terminal and button |

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
| 11-dimmer-terminal | LED dimmer controlled by terminal and button (TIM2 + USART2 + EXTI) |

## Pins used

| Pin | Function | Peripheral | Notes |
|---|---|---|---|
| PA5 | LED LD2 | GPIO output, or TIM2_CH1 (AF1) in the PWM exercises | On-board LED. Arduino header D13 |
| PC13 | Button B1 | GPIO input, EXTI13 | On-board user button, active low |
| PA2 | USART2_TX (AF7) | USART2 | Connected to the ST-LINK virtual COM port |
| PA3 | USART2_RX (AF7) | USART2 | Connected to the ST-LINK virtual COM port |
| PA0 | ADC1_IN0 | ADC1 | Potentiometer wiper. Outer pins go to 3V3 and GND. Arduino header A0 |

## Getting started

1. Connect the NUCLEO-F446RE to the PC with a USB cable (ST-LINK port).
2. In STM32CubeIDE: **File → Import → General → Existing Projects into Workspace**, select an exercise folder (for example `01-gpio-bsrr`), and **untick** "Copy projects into workspace".
3. Build with the hammer icon, then **Run → Run As → STM32 C/C++ Application**.
4. For the UART exercises, open a serial terminal (PuTTY or similar) on the **ST-LINK Virtual COM Port** with **115200 baud, 8 data bits, no parity, 1 stop bit**. In PuTTY, set "Local line editing" to "Force off" so typed characters are sent immediately.

Every exercise is a separate project and does not depend on the others.



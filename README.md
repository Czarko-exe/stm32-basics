# stm32-basics

Register-level C exercises on the **STM32F446RE** (NUCLEO-F446RE), written **without HAL** to understand what the peripherals actually do: which clock to enable, which bits to set and in what order, based on the reference manual.

Each folder is a separate STM32CubeIDE project. Exercises are numbered and build on each other, from lighting an LED to a modular SPI OLED driver fed by DMA.

## What's covered

| Area | Topics |
|---|---|
| Digital I/O | GPIO, atomic set/reset with BSRR, edge detection, debounce |
| Timing | SysTick 1 ms timebase, TIM2 PWM |
| Analog | ADC1 polled conversion, software filtering |
| Interrupts | EXTI, NVIC, USART RX interrupt, ring buffer |
| Communication | USART2, I2C1 master (incl. N-byte burst read), SPI1 master |
| Sensors / displays | BME280 (chip ID, raw data, calibration, temperature compensation), SSD1306 OLED |
| Data transfer | DMA2 → SPI1_TX |
| Graphics | 1 KB framebuffer, lines and rectangles, 5x7 ASCII font |
| Code structure | Splitting a single-file project into `.c/.h` modules |

## Exercises

### Stage 1: GPIO, timers, UART, ADC, interrupts

| Folder | Topic |
|---|---|
| 01-gpio-bsrr | GPIO output/input, atomic set/reset with BSRR |
| 02-gpio-toggle | Button edge detection, LED toggle |
| 03-gpio-debounce | SysTick 1 ms timer, 20 ms debounce |
| 04-uart-tx | USART2 TX at 115200 baud |
| 05-tim2-pwm | TIM2 PWM, LED fade |
| 06-adc-pot | ADC1 polled conversion, readings over UART |
| 07-adc-pwm-dimmer | Potentiometer controls LED brightness |
| 08-exti-button | EXTI interrupt on the button, time-based lockout |
| 09-uart-rx-irq | USART2 RX interrupt, terminal commands |
| 10-uart-rx-ringbuffer | RX ring buffer so no characters are lost |
| 11-dimmer-terminal | LED dimmer controlled by terminal and button (TIM2 + USART2 + EXTI) |

### Stage 2: I2C, SPI, OLED, DMA, modules

| Folder | Topic |
|---|---|
| 12-i2c-bme280-id | I2C1 master, read BME280 chip ID (0x60) |
| 13-i2c-bme280-raw | Raw temperature read from BME280 |
| 14-i2c-bme280-burst | Burst read of N > 2 bytes, following the RM0390 sequence |
| 15-bme280-calibration | Temperature calibration coefficients dig_T1..dig_T3 |
| 16-bme280-celsius | Temperature in °C with the integer (int32) compensation formula |
| 17-spi-oled | SPI1 master driver, SSD1306 initialisation |
| 18-oled-framebuffer | 1024 B framebuffer sent in one SPI stream, hline / vline / rect |
| 19-oled-text | 5x7 font for ASCII 32-126, character and string drawing |
| 20-oled-temperature | BME280 temperature on the OLED every second, own number formatting (no printf), I2C error handling |
| 21-oled-dma | Framebuffer sent by DMA2 (stream 3, channel 3) to SPI1_TX, ~4 ms of CPU time freed per frame |
| 22-modules | Exercise 21 split into modules (see below) |

### Module layout (22-modules)

| Module | Responsibility |
|---|---|
| `regs.h` | Shared register addresses and bit definitions |
| `timebase` | SysTick millisecond timer and delays |
| `i2c1` | I2C1 master: register read/write, burst read |
| `bme280` | BME280 init (calibration data) and temperature compensation |
| `spi1` | SPI1 master |
| `dma` | DMA2 stream 3 transfers to SPI1_TX: start and wait |
| `oled` | SSD1306 driver, framebuffer, text, update by SPI or DMA |
| `format` | Temperature to text without printf |
| `main.c` | Initialisation and the main loop only |

Internal helpers are `static`; each header exposes only the module's public API.

## Lessons learned

- **Enable peripherals in layers:** port clock → peripheral clock → pin mode and alternate function → configuration → enable. Most "nothing happens" bugs were a missed step here.
- **Interrupt RX needs a buffer:** pasting a string into the terminal lost characters with a single-variable receive (09). A ring buffer fixed it (10).
- **Ending a DMA SPI transfer:** after the DMA transfer-complete flag, still wait for TXE and BSY before raising CS, otherwise the last byte gets cut off.
- **Check every offset:** in DMA, `LIFCR` is at offset `0x08`, not `0x04`. Found by watching registers in the SFR view.
- **Hardware matters too:** a silent SPI display turned out to be a bad breadboard contact, not a code bug.

## Hardware and tools

- NUCLEO-F446RE (STM32F446RE, Cortex-M4), STM32CubeIDE, PuTTY (serial terminal)
- BME280 sensor module (I2C, address 0x76)
- Waveshare 0.96" OLED, SSD1306 controller (4-wire SPI)
- 24 MHz 8-channel logic analyzer + PulseView, multimeter
- Potentiometer, LEDs, resistors, breadboard, jumper wires

## Pins used

| Pin | Function | Peripheral | Notes |
|---|---|---|---|
| PA5 | LED LD2 | GPIO output, or TIM2_CH1 (AF1) in the PWM exercises | On-board LED. Arduino header D13 |
| PC13 | Button B1 | GPIO input, EXTI13 | On-board user button, active low |
| PA2 | USART2_TX (AF7) | USART2 | Connected to the ST-LINK virtual COM port |
| PA3 | USART2_RX (AF7) | USART2 | Connected to the ST-LINK virtual COM port |
| PA0 | ADC1_IN0 | ADC1 | Potentiometer wiper. Outer pins go to 3V3 and GND. Arduino header A0 |
| PB8 | I2C1_SCL (AF4) | I2C1 | BME280 SCL, open-drain |
| PB9 | I2C1_SDA (AF4) | I2C1 | BME280 SDA, open-drain |
| PB3 | SPI1_SCK (AF5) | SPI1 | OLED CLK |
| PB5 | SPI1_MOSI (AF5) | SPI1 | OLED DIN |
| PA10 | OLED CS | GPIO output | Chip select, active low |
| PB4 | OLED D/C | GPIO output | Low = command, high = data |
| PB10 | OLED RES | GPIO output | Reset, active low |

## Getting started

1. Connect the NUCLEO-F446RE to the PC with a USB cable (ST-LINK port).
2. In STM32CubeIDE: **File → Import → General → Existing Projects into Workspace**, select an exercise folder (for example `01-gpio-bsrr`), and **untick** "Copy projects into workspace".
3. Build with the hammer icon, then **Run → Run As → STM32 C/C++ Application**.
4. For the UART exercises, open a serial terminal (PuTTY or similar) on the **ST-LINK Virtual COM Port** with **115200 baud, 8 data bits, no parity, 1 stop bit**. In PuTTY, set "Local line editing" to "Force off" so typed characters are sent immediately.

Every exercise is a separate project and does not depend on the others.

Debugging: global `dbg_*` variables in Live Expressions, breakpoints and the SFR view.

## References

- RM0390: STM32F446xx reference manual
- STM32F446xC/E datasheet (alternate function table)
- PM0214: Cortex-M4 programming manual (NVIC, SysTick)
- Bosch BME280 datasheet
- Solomon Systech SSD1306 datasheet

## Roadmap

- [x] Stage 1: GPIO, SysTick, UART, PWM, ADC, interrupts
- [ ] Stage 2: I2C, SPI, OLED, DMA, modules (done); next: CMSIS device headers, then one exercise rewritten with HAL for comparison
- [ ] Next (separate repo): FreeRTOS weather station with BME280, OLED and UART tasks communicating through queues, unit tests in C, CMake and CI

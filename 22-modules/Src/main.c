#include "timebase.h"
#include "spi1.h"
#include "i2c1.h"
#include "bme280.h"
#include "dma.h"
#include "oled.h"
#include "format.h"

int main(void)
{
    systick_init();
    spi1_init();
    i2c1_init();
    bme280_init();
    dma2_init();
    oled_init();

	uint32_t last_tick = 0;
	for (;;) {
		if (ms_ticks - last_tick >= 1000) {
			last_tick = ms_ticks;


			uint8_t temp_data[3];
			oled_clear();
			if (i2c_read_burst(0x76, 0xFA, temp_data, 3)) {
				int32_t adc_T = (int32_t)((temp_data[0] << 12) | (temp_data[1] << 4) | (temp_data[2] >> 4));
				int32_t temperature = bme280_compensate_T_int32(adc_T);

				char temperature_str[8];
				temperature_to_string(temperature, temperature_str);

				oled_draw_string(0, 0, "Temp: ");
				oled_draw_string(36, 0, temperature_str);
				oled_draw_string(72, 0, "C");
			}
			else {
				oled_draw_string(0, 0, "Temp: Error");
			}

			oled_update_dma();
			oled_update_dma_wait();
		}
	}
}

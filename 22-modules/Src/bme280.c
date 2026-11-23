#include "bme280.h"
#include "i2c1.h"

#define BME280_I2C_ADDRESS 0x76

static uint16_t dig_T1;
static int16_t  dig_T2;
static int16_t  dig_T3;
static int32_t t_fine;

int bme280_init(void) {
	if(!i2c_write_reg(BME280_I2C_ADDRESS, 0xF4, 0x27)) return 0; 	// ustawienie trybu normalnego, oversampling x1 dla temperatury i ciśnienia, oversampling x1 dla wilgotności

	uint8_t calib_data[6];
	i2c_read_burst(BME280_I2C_ADDRESS, 0x88, calib_data, 6); 	// odczytanie danych kalibracyjnych dla temperatury i ciśnienia
	dig_T1 = (uint16_t)((calib_data[1] << 8) | calib_data[0]);
	dig_T2 = (int16_t)((calib_data[3] << 8) | calib_data[2]);
	dig_T3 = (int16_t)((calib_data[5] << 8) | calib_data[4]);

	return 1;
}

int32_t bme280_compensate_T_int32(int32_t adc_T) {
	int32_t var1, var2, T;
	var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	return T;
}

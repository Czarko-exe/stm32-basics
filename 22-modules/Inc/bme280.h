#ifndef BME280_H
#define BME280_H

#include <stdint.h>

int bme280_init(void);
int32_t bme280_compensate_T_int32(int32_t adc_T);

#endif

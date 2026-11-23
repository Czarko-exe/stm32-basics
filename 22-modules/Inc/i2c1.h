#ifndef I2C1_H
#define I2C1_H

#include <stdint.h>

void i2c1_init(void);
int i2c_write_reg(uint8_t dev, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t dev, uint8_t reg, uint8_t *out);
int i2c_read_burst(uint8_t dev, uint8_t reg, uint8_t *buf, uint8_t n);

#endif

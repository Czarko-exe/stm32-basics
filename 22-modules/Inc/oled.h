#ifndef OLED_H
#define OLED_H

#include <stdint.h>

void oled_init(void);
void oled_update(void);
void oled_clear(void);
void oled_draw_string(uint8_t x, uint8_t y, const char *str);
void oled_update_dma_wait(void);
void oled_update_dma(void);

#endif

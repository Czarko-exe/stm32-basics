#include "format.h"

void temperature_to_string(int32_t temp, char *str) {
	int i = 0;

	if (temp < 0) {
		str[i++] = '-';
		temp = -temp;
	}

	int32_t integer_part = temp / 100;
	int32_t fractional_part = temp % 100;
	if (integer_part >= 10) {
		str[i++] = '0' + (integer_part / 10);
	}
	str[i++] = '0' + (integer_part % 10);
	str[i++] = '.';
	str[i++] = '0' + (fractional_part / 10);
	str[i++] = '0' + (fractional_part % 10);
	str[i] = '\0';
}

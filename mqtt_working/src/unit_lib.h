#ifndef __UNIT_LIB__
#define	__UNIT_LIB__

#include <sys/_types.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/types.h>

uint16_t crc16_ccitt_jch(uint8_t *data, uint8_t length);

void saturate_channel_values(int32_t *ch0, int32_t *ch1, int32_t *ch2, int32_t *ch3);

void clear_buffer(int16_t *buffer, size_t length);

#endif

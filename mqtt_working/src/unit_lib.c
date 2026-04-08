#include "unit_lib.h"



uint16_t crc16_ccitt_jch(uint8_t *data, uint8_t length)
{
	uint16_t crc = 0xFFFF;

	for (uint8_t i = 0; i < length; i++)
	{
		crc ^= ((uint16_t)data[i] << 8);

		for (uint8_t bit = 0; bit < 8; bit++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}


void saturate_channel_values(int32_t *ch0, int32_t *ch1, int32_t *ch2, int32_t *ch3)
{
    if (*ch0 > 32750)  *ch0 = 32750;
    if (*ch0 < -32750) *ch0 = -32750;

    if (*ch1 > 32750)  *ch1 = 32750;
    if (*ch1 < -32750) *ch1 = -32750;

    if (*ch2 > 32750)  *ch2 = 32750;
    if (*ch2 < -32750) *ch2 = -32750;

    if (*ch3 > 32750)  *ch3 = 32750;
    if (*ch3 < -32750) *ch3 = -32750;
}



void clear_buffer(int16_t *buffer, size_t length)
{
	memset(buffer, 0, length * sizeof(int16_t));
}
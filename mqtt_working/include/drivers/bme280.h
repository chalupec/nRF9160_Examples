/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_CUSTOM_BME280_H_
#define ZEPHYR_DRIVERS_SENSOR_CUSTOM_BME280_H_

#include <nrfx_spim.h>
#include "../../src/main.h"
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#define MEM_CS_PERIPHERAL_CONTROL_DISABLED

/* BME280 register definitions */
#define CTRLHUM 0xF2
#define CTRLMEAS 0xF4
#define CALIB00 0x88
#define CALIB24 0xA1
#define CALIB26 0xE1
#define ID 0xD0
#define PRESSMSB 0xF7
#define TEMPMSB 0xFA
#define HUMMSB 0xFD
#define REG_STATUS 0xF3

#define BME_RESET 0xE0
#define RST_SEQ   0xB6

#define BME280_CHIP_ID 0x60

#define STATUS_MEASURING 0x08
#define STATUS_IM_UPDATE 0x01

#define SENSOR_CHAN_AMBIENT_TEMP  1
#define SENSOR_CHAN_PRESS  2
#define SENSOR_CHAN_HUMIDITY  3


/* Driver runtime data */
struct custom_bme280_data
{
    /* Compensation parameters */
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;

    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;

    /* Compensated values */
    int32_t comp_temp;
    uint32_t comp_press;
    uint32_t comp_humidity;

    /* Fine temperature */
    int32_t t_fine;

    uint8_t chip_id;
};

extern nrfx_spim_t spim_inst;
extern struct custom_bme280_data sens_data_str;

/* Public helper functions (used inside driver, testable) */
int bme280_reg_read(uint8_t reg, uint8_t *data, int size);

int bme280_reg_write(uint8_t reg, uint8_t value);

int custom_bme280_init(struct custom_bme280_data *data);

int custom_bme280_sample_fetch(struct custom_bme280_data *data);

int bme280_sensor_channel_get(struct custom_bme280_data *data, uint8_t sensor, float *val);

int bme280_wait_until_ready(void);

int bme280_reset(void);

int bme280_read_compensation(struct custom_bme280_data *data);

#endif /* ZEPHYR_DRIVERS_SENSOR_CUSTOM_BME280_H_ */
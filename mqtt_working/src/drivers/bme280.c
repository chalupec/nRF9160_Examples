/*
 * Copyright (c) 2019 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// #include "../../src/main.h"

#include "../../include/drivers/bme280.h"

/* -------------------------------------------------------------------------- */
/* Low level SPI access                                                        */
/* -------------------------------------------------------------------------- */

int bme280_reg_read(uint8_t reg, uint8_t *data, int size)
{
    /* TX = [REG|0x80][dummy ...] */
    /* RX = [dummy][data ...]   */

    uint8_t tx_buf[size + 1];
    uint8_t rx_buf[size + 1];

    /* First byte: register address with READ bit */
    tx_buf[0] = reg | 0x80;

    /* Remaining bytes are dummy */
    memset(&tx_buf[1], 0xFF, size);

    /* Clear RX buffer */
    memset(rx_buf, 0x00, sizeof(rx_buf));

    nrfx_spim_xfer_desc_t xfer =
        NRFX_SPIM_SINGLE_XFER(tx_buf, size + 1,
                              rx_buf, size + 1);

    nrf_gpio_pin_clear(BME_CS_PIN);
    nrfx_err_t err = nrfx_spim_xfer(&spim_inst, &xfer, 0);
    nrf_gpio_pin_set(BME_CS_PIN);

    if (err != NRFX_SUCCESS)
    {
        return -EIO;
    }

    /* Copy received data (skip first dummy byte) */
    memcpy(data, &rx_buf[1], size);

    return 0;
}

int bme280_reg_write(uint8_t reg, uint8_t value)
{
    /* TX = [REG & 0x7F][VALUE] */
    /* RX není potřeba, ale nrfx vyžaduje buffer */

    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    tx_buf[0] = reg & 0x7F; /* WRITE = bit7 = 0 */
    tx_buf[1] = value;

    memset(rx_buf, 0x00, sizeof(rx_buf));

    nrfx_spim_xfer_desc_t xfer =
        NRFX_SPIM_SINGLE_XFER(tx_buf, sizeof(tx_buf),
                              rx_buf, sizeof(rx_buf));

    nrf_gpio_pin_clear(BME_CS_PIN);
    nrfx_err_t err = nrfx_spim_xfer(&spim_inst, &xfer, 0);
    nrf_gpio_pin_set(BME_CS_PIN);

    if (err != NRFX_SUCCESS)
    {
        return -EIO;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Compensation functions (datasheet based)                                    */
/* -------------------------------------------------------------------------- */

static void bme280_compensate_temp(struct custom_bme280_data *data,
                                   int32_t adc_temp)
{
    int32_t var1, var2;

    var1 = (((adc_temp >> 3) - ((int32_t)data->dig_t1 << 1)) * ((int32_t)data->dig_t2)) >> 11;

    var2 = (((((adc_temp >> 4) - ((int32_t)data->dig_t1)) * ((adc_temp >> 4) - ((int32_t)data->dig_t1))) >> 12) * ((int32_t)data->dig_t3)) >> 14;

    data->t_fine = var1 + var2;
    data->comp_temp = (data->t_fine * 5 + 128) >> 8;
}



// problem values
static void bme280_compensate_press_old(struct custom_bme280_data *data, int32_t adc_press)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)data->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)data->dig_p6;
    var2 = var2 + ((var1 * (int64_t)data->dig_p5) << 17);
    var2 += ((int64_t)data->dig_p4) << 35;
    var1 = ((var1 * var1 * (int64_t)data->dig_p3) >> 8) + ((var1 * (int64_t)data->dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)data->dig_p1) >> 33;

    if (var1 == 0)
    {
        data->comp_press = 0;
        return;
    }

    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;

    var1 = ((int64_t)data->dig_p9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)data->dig_p8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)data->dig_p7) << 4);

    data->comp_press = (uint32_t)p;
}

static void bme280_compensate_press(struct custom_bme280_data *data,
                                    int32_t adc_press)
{
    int64_t var1, var2, p;

    var1 = (((int64_t)data->t_fine >> 1) - (int64_t)64000);                                                                    // ok
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int64_t)data->dig_p6);                                                      // ok
    var2 = (var2 + ((var1 * ((int64_t)data->dig_p5)) << 1));                                                                   // ok
    var2 = ((var2 >> 2) + (((int64_t)data->dig_p4) << 16));                                                                    // ok
    var1 = (((((int64_t)data->dig_p3) * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int64_t)data->dig_p2) * var1) >> 1)); // ok
    var1 = var1 >> 18;                                                                                                         // ok
    var1 = ((((32768 + var1)) * ((int64_t)data->dig_p1)) >> 15);                                                               // ok

    if (var1 == 0)
    {
        data->comp_press = 0;
        return;
    }

    p = ((((unsigned long)(((long)1048576) - adc_press)) - (var2 >> 12))) * 3125; // ok

    if (p < 0x80000000)
    {
        p = ((p << 1) / ((unsigned long)var1));
    }
    else
    {
        p = ((p / (unsigned long)var1) * 2);
    } // ok

    var1 = (((int64_t)data->dig_p9) * ((int64_t)(((p >> 3) * (p >> 3)) >> 13))); // ok
    var1 = var1 >> 12;                                                           // ok
    var2 = (((int64_t)(p >> 2)) * ((int64_t)data->dig_p8));                      // ok
    var2 = var2 >> 13;                                                           // ok
    p = (int64_t)((int64_t)p + ((var1 + var2 + (int64_t)data->dig_p7) >> 4));
    data->comp_press = (uint32_t)p;
}

static void bme280_compensate_humidity(struct custom_bme280_data *data,
                                       int32_t adc_hum)
{
    int32_t h;

    h = data->t_fine - 76800;
    h = (((((adc_hum << 14) - ((int32_t)data->dig_h4 << 20) - ((int32_t)data->dig_h5 * h)) +  16384) >> 15) *
         (((((((h * data->dig_h6) >> 10) * (((h * data->dig_h3) >> 11) + 32768)) >> 10) +
            2097152) *  data->dig_h2 +  8192) >> 14));

    h -= (((((h >> 15) * (h >> 15)) >> 7) *  data->dig_h1) >>   4);

    h = CLAMP(h, 0, 419430400);
    data->comp_humidity = (uint32_t)(h >> 12);
}

/* -------------------------------------------------------------------------- */

int bme280_wait_until_ready(void)
{
    uint8_t status;
    int err;

    do
    {
        k_sleep(K_MSEC(3));
        err = bme280_reg_read(REG_STATUS, &status, 1);
        if (err)
        {
            return err;
        }
    } while (status & (STATUS_MEASURING | STATUS_IM_UPDATE));

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Zephyr sensor API                                                           */
/* -------------------------------------------------------------------------- */

int custom_bme280_sample_fetch(struct custom_bme280_data *data)
{

    uint8_t buf[8];
    int32_t adc_press, adc_temp, adc_hum;
    int32_t ltemp = 0;
    int err;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    err = bme280_wait_until_ready();
    if (!err)
    {
        err = bme280_reg_read(PRESSMSB, buf, sizeof(buf));
    }

    adc_press = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);

    // ltemp = (int32_t) buf[0] << 16;
    // ltemp |= (int32_t) buf[1] << 8;
    // ltemp |= (int32_t) buf[2];
    // ltemp = ltemp >> 4;
    // printk("puvoddni %d  stare %d",adc_press,ltemp );

    adc_temp = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);

    // ltemp = (int32_t) buf[3] << 16;
    // ltemp |= (int32_t) buf[4] << 8;
    // ltemp |= (int32_t) buf[5];
    // ltemp = ltemp >> 4;
    // printk("puvoddni %d  stare %d",adc_temp,ltemp );

    adc_hum = (buf[6] << 8) | buf[7];

    ltemp = (int32_t)buf[6] << 8;
    ltemp |= (int32_t)buf[7];
    // printk("puvoddni %d  stare %d",adc_hum,ltemp );

    bme280_compensate_temp(data, adc_temp);
    //bme280_compensate_press_old(data, adc_press);
    bme280_compensate_press(data, adc_press);
    bme280_compensate_humidity(data, adc_hum);

    return 0;
}

int bme280_sensor_channel_get(struct custom_bme280_data *data, uint8_t sensor, float *val)
{
    // struct custom_bme280_data *data = dev->data;

    switch (sensor)
    {
    case SENSOR_CHAN_AMBIENT_TEMP:
        *val = (float)data->comp_temp / 100.0f;
        //*val = (float)data->comp_temp / 1.0f;
        break;

    case SENSOR_CHAN_PRESS:
        *val = (float)(data->comp_press) / 100.0f; // returns value in hPa
                                                   // *val = (float)(data->comp_press) / 1.0f;
        break;

    case SENSOR_CHAN_HUMIDITY:
        *val = (float)(data->comp_humidity) / 1024.0f; // returns value in %RH
                                       
        //*val = (float)(data->comp_humidity);
        break;

    default:
        return -ENOTSUP;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Init + Power management                                                     */
/* -------------------------------------------------------------------------- */

int bme280_read_compensation(struct custom_bme280_data *data)
{

    uint16_t buf[12];
    uint8_t hbuf[7];
    int err;

    err = bme280_reg_read(CALIB00, (uint8_t *)buf, sizeof(buf));
    if (err)
    {
        return err;
    }

    data->dig_t1 = sys_le16_to_cpu(buf[0]);
    data->dig_t2 = sys_le16_to_cpu(buf[1]);
    data->dig_t3 = sys_le16_to_cpu(buf[2]);

    data->dig_p1 = sys_le16_to_cpu(buf[3]);
    data->dig_p2 = sys_le16_to_cpu(buf[4]);
    data->dig_p3 = sys_le16_to_cpu(buf[5]);
    data->dig_p4 = sys_le16_to_cpu(buf[6]);
    data->dig_p5 = sys_le16_to_cpu(buf[7]);
    data->dig_p6 = sys_le16_to_cpu(buf[8]);
    data->dig_p7 = sys_le16_to_cpu(buf[9]);
    data->dig_p8 = sys_le16_to_cpu(buf[10]);
    data->dig_p9 = sys_le16_to_cpu(buf[11]);

    err = bme280_reg_read(CALIB24, &data->dig_h1, 1);
    if (err)
    {
        return err;
    }

    err = bme280_reg_read(CALIB26, hbuf, 7);
    if (err)
    {
        return err;
    }

    data->dig_h2 = (hbuf[1] << 8) | hbuf[0];
    data->dig_h3 = hbuf[2];
    data->dig_h4 = (hbuf[3] << 4) | (hbuf[4] & 0x0F);
    data->dig_h5 = ((hbuf[4] >> 4) & 0x0F) | (hbuf[5] << 4);
    data->dig_h6 = hbuf[6];

    return 0;
}

int bme280_reset(void)
{
    // struct custom_bme280_data *data = dev->data;
    int err;
    k_sleep(K_MSEC(3));
    err = bme280_reg_write(BME_RESET, RST_SEQ);
    k_sleep(K_MSEC(3));
    return err;
}

int custom_bme280_init(struct custom_bme280_data *data)
{
    // struct custom_bme280_data *data = dev->data;
    int err;

    err = bme280_reg_read(ID, &data->chip_id, 1);
    if (err || data->chip_id != BME280_CHIP_ID)
    {
        return -ENOTSUP;
    }
    err = bme280_wait_until_ready();
    if (!err)
    {
        err = bme280_read_compensation(data);
    }
    if (!err)
    {
        err = bme280_reg_write(CTRLHUM, 0x04);
    }
    if (!err)
    {
        err = bme280_reg_write(CTRLMEAS, 0x93);
    }

    return 0;
}

// static int custom_bme280_pm_action(const struct device *dev,
//                                    enum pm_device_action action)
//{
//     switch (action)
//     {
//     case PM_DEVICE_ACTION_RESUME:
//         return custom_bme280_init(dev);
//
//     case PM_DEVICE_ACTION_SUSPEND:
//         return bme280_reg_write(dev, CTRLMEAS, 0x93);
//
//     default:
//         return -ENOTSUP;
//     }
// }
//
///* -------------------------------------------------------------------------- */
///* Device definition                                                           */
///* -------------------------------------------------------------------------- */
//
// #define CUSTOM_BME280_DEFINE(inst)                              \
//    static struct custom_bme280_data custom_bme280_data_##inst; \
//    static const struct custom_bme280_config                    \
//        custom_bme280_config_##inst = {                         \
//            .spi = SPI_DT_SPEC_INST_GET(inst, SPIOP, 0),        \
//    };                                                          \
//    PM_DEVICE_DT_INST_DEFINE(inst, custom_bme280_pm_action);    \
//    DEVICE_DT_INST_DEFINE(inst,                                 \
//                          custom_bme280_init,                   \
//                          PM_DEVICE_DT_INST_GET(inst),          \
//                          &custom_bme280_data_##inst,           \
//                          &custom_bme280_config_##inst,         \
//                          POST_KERNEL,                          \
//                          CONFIG_SENSOR_INIT_PRIORITY,          \
//                          &custom_bme280_api);
//
// DT_INST_FOREACH_STATUS_OKAY(CUSTOM_BME280_DEFINE)
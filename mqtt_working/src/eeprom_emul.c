#include "eeprom_emul.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_REGISTER(eeprom_emul, LOG_LEVEL_INF);

/* NVS filesystem instance */
static struct nvs_fs fs;

/* ---------------------------------------------------- */
/* INIT                                                 */
/* ---------------------------------------------------- */

int eeprom_emul_init(void)
{
    int rc;
    struct flash_pages_info info;

    fs.flash_device = FIXED_PARTITION_DEVICE(storage_partition);
    if (!device_is_ready(fs.flash_device))
    {
        LOG_ERR("Flash device not ready");
        return -ENODEV;
    }

    fs.offset = FIXED_PARTITION_OFFSET(storage_partition);

    rc = flash_get_page_info_by_offs(fs.flash_device,
                                    fs.offset,
                                    &info);
    if (rc)
    {
        LOG_ERR("flash_get_page_info failed (%d)", rc);
        return rc;
    }

    fs.sector_size  = info.size;
    fs.sector_count = 4;   /* wear leveling */

    rc = nvs_mount(&fs);
    if (rc)
    {
        LOG_ERR("NVS mount failed (%d)", rc);
        return rc;
    }

    LOG_INF("EEPROM emulation initialized");
    return 0;
}

/* ---------------------------------------------------- */
/* BASIC TYPES                                          */
/* ---------------------------------------------------- */

int eeprom_emul_write_u16(eeprom_key_t key, uint16_t value)
{
    return nvs_write(&fs, key, &value, sizeof(value));
}

int eeprom_emul_read_u16(eeprom_key_t key, uint16_t *value)
{
    int rc = nvs_read(&fs, key, value, sizeof(*value));
    return (rc > 0) ? 0 : rc;
}

int eeprom_emul_write_u32(eeprom_key_t key, uint32_t value)
{
    return nvs_write(&fs, key, &value, sizeof(value));
}

int eeprom_emul_read_u32(eeprom_key_t key, uint32_t *value)
{
    int rc = nvs_read(&fs, key, value, sizeof(*value));
    return (rc > 0) ? 0 : rc;
}

/* ---------------------------------------------------- */
/* BLOB                                                 */
/* ---------------------------------------------------- */

int eeprom_emul_write_blob(eeprom_key_t key,
                           const void *data,
                           uint16_t len)
{
    return nvs_write(&fs, key, data, len);
}

int eeprom_emul_read_blob(eeprom_key_t key,
                          void *data,
                          uint16_t len)
{
    int rc = nvs_read(&fs, key, data, len);
    return (rc > 0) ? 0 : rc;
}
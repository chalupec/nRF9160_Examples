#ifndef EEPROM_EMUL_H_
#define EEPROM_EMUL_H_

#include <stdint.h>

/*
 * EEPROM emulation based on Zephyr NVS
 * Designed for nRF9161
 *
 * Persistent storage for:
 *  - trigger thresholds
 *  - unit configuration
 *  - counters
 */

/* EEPROM key IDs */
typedef enum
{
    EEPROM_KEY_TRIG_DURATION  = 0x00,
    EEPROM_KEY_TRIG_START     = 0x01,
    EEPROM_KEY_TRIG_END       = 0x02,
    EEPROM_KEY_NR_OF_SMPLS    = 0x03,
    EEPROM_KEY_TOPIC_ID       = 0x04,
    EEPROM_KEY_PWR_CYCLES     = 0x05,
    EEPROM_KEY_TRAIN_COUNTER  = 0x06

} eeprom_key_t;

/* API */
int eeprom_emul_init(void);

/* uint16 */
int eeprom_emul_write_u16(eeprom_key_t key, uint16_t value);
int eeprom_emul_read_u16(eeprom_key_t key, uint16_t *value);

/* uint32 */
int eeprom_emul_write_u32(eeprom_key_t key, uint32_t value);
int eeprom_emul_read_u32(eeprom_key_t key, uint32_t *value);

/* raw blob */
int eeprom_emul_write_blob(eeprom_key_t key, const void *data, uint16_t len);
int eeprom_emul_read_blob(eeprom_key_t key, void *data, uint16_t len);

#endif /* EEPROM_EMUL_H_ */
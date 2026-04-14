#ifndef LOG_RAM_BACKEND_H
#define LOG_RAM_BACKEND_H

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_RAM_BUFFER_SIZE 4096
#define LOG_RAM_MAGIC       0x4C4F4752  /* 'LOGR' */

/* Retention struktura */
struct log_ram_retention {
    uint32_t magic;
    struct ring_buf rb;
    uint8_t buffer[LOG_RAM_BUFFER_SIZE];
};

extern struct log_ram_retention log_ram_ret;

/* API */
uint32_t log_ram_read(uint8_t *data, uint32_t len);
void log_ram_clear(void);
bool log_ram_is_retained(void);

#ifdef __cplusplus
}
#endif

#endif /* LOG_RAM_BACKEND_H */
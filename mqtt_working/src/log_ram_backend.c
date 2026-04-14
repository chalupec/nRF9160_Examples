#include "log_ram_backend.h"

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>

/*
 * Retention RAM (noinit):
 * - NEinicializuje se při resetu
 * - zůstane zachována
 */
__attribute__((section(".noinit")))
struct log_ram_retention log_ram_ret;

/* Log output helper */
static uint8_t log_output_buf[128];

static int ram_output_func(uint8_t *data, size_t length, void *ctx)
{
    ARG_UNUSED(ctx);
    ring_buf_put(&log_ram_ret.rb, data, length);
    return length;
}

LOG_OUTPUT_DEFINE(log_output,
                  ram_output_func,
                  log_output_buf,
                  sizeof(log_output_buf));

/* === BACKEND PROCESS === */
static void log_ram_process(const struct log_backend *backend,
                            union log_msg_generic *msg)
{
    char ts_buf[32];
    uint64_t ts_ms = k_uptime_get();

    int ts_len = snprintk(ts_buf, sizeof(ts_buf),
                          "[%llu]\t",
                          ts_ms);

    if (ts_len > 0) {
        ring_buf_put(&log_ram_ret.rb,
                     (uint8_t *)ts_buf,
                     ts_len);
    }

    log_output_msg_process(&log_output, msg, backend);
}

static const struct log_backend_api log_ram_backend_api = {
    .process = log_ram_process,
};

LOG_BACKEND_DEFINE(log_ram_backend,
                   log_ram_backend_api,
                   true);

/* === INIT RETENTION === */
static int log_ram_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    if (log_ram_ret.magic != LOG_RAM_MAGIC) {
        /* První start nebo power‑on */
        memset(&log_ram_ret, 0, sizeof(log_ram_ret));
        log_ram_ret.magic = LOG_RAM_MAGIC;

        ring_buf_init(&log_ram_ret.rb,
                      LOG_RAM_BUFFER_SIZE,
                      log_ram_ret.buffer);
    }

    return 0;
}

/* Spustí se velmi brzy při bootu */
SYS_INIT(log_ram_init, PRE_KERNEL_1, 0);

/* === PUBLIC API === */

uint32_t log_ram_read(uint8_t *data, uint32_t len)
{
    return ring_buf_get(&log_ram_ret.rb, data, len);
}

void log_ram_clear(void)
{
    ring_buf_reset(&log_ram_ret.rb);
}

bool log_ram_is_retained(void)
{
    return (log_ram_ret.magic == LOG_RAM_MAGIC);
}
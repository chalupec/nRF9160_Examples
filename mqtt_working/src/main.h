#ifndef _MAIN_H_
#define _MAIN_H_




#define SPIM_INST_IDX 1
#define MOSI_PIN 4
#define MISO_PIN 1
#define SCK_PIN 5
#define MEM_CS_PIN 0

// #define SAMPLE_PRINTING_ENABLED
// #define CIRC_BUFF_STAMP_VALUE_ADD

#define ALPHA_NUM 1	 // Numerator of alpha (e.g., 1)
#define ALPHA_DEN 25 // Denominator of alpha (e.g., 10) → alpha = 0.1

#define DEFAULT_START_RMS_TRIG_TRESHOLD 55
#define DEFAULT_END_RMS_TRIG_TRESHOLD 35
#define DEFAULTRMS_LOW_SAMPLES_TO_TRIGGER_END 6000 // cca 3 sec   2000smp=1sec

#define RMS_BUFFER_SIZE 100

#define NR_OF_SAMPLES_TO_MEASURE 27000 // 13500

#define RES_VAR_LEN 5000

#define WAVE_SAMPLE_LEN 1024 // MUST BE SAME AS CRCLR_BUFF_SIZE

#define CRCLR_BUFF_SIZE 1024 // MUST BE SAME AS WAVE_SAMPLE_LEN

#define FLASH_BYTE_READ_OUT_LEN 8 // POZOR OMEZENI VE FCI FLASH READ v driver knihovne, na max 32

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_US 500UL

#define DAQ_TIME_US 40

#define ADC_NODE DT_IO_CHANNELS_CTLR(DT_PATH(zephyr_user))

#define CHANNEL_0 0
#define CHANNEL_1 1
#define CHANNEL_2 2
#define CHANNEL_3 3
#define CHANNEL_4 4

#define BUFFER_WIDTH 4
#define BUFFER_LENGTH 4

// nt buffer[BUFFER_SIZE] = {0};
// int index = 0;

/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

#define LOG_MANIPULATION_BUFF_LEN 64


#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <ncs_version.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <poll.h>
#include <zephyr/types.h>

#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
/* STEP 2.3 - Include the header file for the MQTT Library*/
#include <zephyr/net/mqtt.h>

#include "mqtt_connection.h"

#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <nrfx_timer.h>

#include <date_time.h>

#include <nrfx_spim.h>
#include "../include/drivers/APS6404L.h"
#include "unit_lib.h"

#include <zephyr/sys/reboot.h>
#include <math.h>
// #include <clock.h>

#include <sys/_types.h>

#include "log_ram_backend.h"

#include "eeprom_emul.h"

/* ============================
 *  Konfigurační konstanty
 * ============================ */



/* ============================
 *  Datové struktury
 * ============================ */

struct __attribute__((__packed__)) data_packet_t
{
    uint16_t packet_header;
    uint16_t packet_version;
    uint16_t actual_packet_nr;
    uint16_t total_packet_nr;
    uint32_t timestamp;
    uint32_t total_sample_count;
    uint16_t train_counter;

    int16_t chan_0_vlt[WAVE_SAMPLE_LEN];
    int16_t chan_0_int[WAVE_SAMPLE_LEN];
    int16_t chan_1_vlt[WAVE_SAMPLE_LEN];
    int16_t chan_1_int[WAVE_SAMPLE_LEN];

    uint16_t CRC;
};

struct __attribute__((__packed__)) servis_packet_t
{
    uint16_t packet_header;
    uint16_t packet_version;
    uint16_t reserve_word;
    uint16_t packet_counter;
    uint16_t batt_voltage;
    int16_t unit_temperature;
    uint32_t IMEI;
    uint32_t DEV_ID;
    uint16_t train_counter;
    uint16_t pwr_cycle_counter;
    uint32_t uptime_minutes;
    uint32_t last_powercycle_timestamp;
    uint16_t unit_status_bits;
    uint8_t  signal_stength;
    uint16_t modem_status_word;
    float    GPS_lat;
    float    GPS_lon;
    float    GPS_alt;
    uint16_t CRC;
};

/* ============================
 *  Veřejné proměnné (OMEZENĚ)
 * ============================ */

/* Konfigurovatelné přes MQTT / EEPROM */
extern uint16_t rms_trig_start;
extern uint16_t rms_trig_end;
extern uint16_t rms_trig_end_duration;

/* CFG buffer z MQTT */
extern char cfg_buff[256];
extern uint8_t config_request_flag;

/* ============================
 *  Veřejné funkce
 * ============================ */

/* MQTT / LTE */
uint8_t init_modem_and_mqtt(void);
int8_t mqtt_pooling_procedure(void);

/* Přenos dat */

void send_measured_train_data_with_multiple_packets(void);
void send_measured_train_data_with_multiple_packets_from_flash(void);

/* DAQ / DSP pomocné funkce */
void add_samples_to_buffer(int16_t samples[BUFFER_WIDTH],
                           int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH]);

void average_of_vectors(int16_t array[BUFFER_LENGTH][BUFFER_WIDTH],
                        int16_t averages[BUFFER_WIDTH]);

int32_t exponential_filter(int32_t previous_filtered, int32_t new_sample);

int32_t update_rms(int32_t *buffer,
                   size_t *index,
                   int64_t *sum_squares,
                   int32_t new_sample);

/* Circular buffer */
void circular_buffer_init(void);
uint16_t circular_buffer_add_value(int16_t v0,
                                   int16_t v1,
                                   int16_t v2,
                                   int16_t v3);

/* Utility */
void get_time_procedure(void);
void test_flash(void);

#endif /* APP_MAIN_H_ */
/*
 * TODO
 * - ziskavani casu
 * - rozliseni pomoci define na ruzne jednotky and topics NRF/01/UP_STREAM
 * - mereni a ukladani hodnot
 */

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

#include <math.h>

//#define SAMPLE_PRINTING_ENABLED

#define WAVE_SAMPLE_LEN 1024

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_US 4000UL

#define DAQ_TIME_US 40

#define ADC_NODE DT_IO_CHANNELS_CTLR(DT_PATH(zephyr_user))

#define CHANNEL_0 0
#define CHANNEL_1 1
#define CHANNEL_2 2
#define CHANNEL_3 3
#define CHANNEL_4 4

#define BUFFER_WIDTH 4
#define BUFFER_LENGTH 5

#define CIRCULAR_BUFFER_SIZE 1024

// nt buffer[BUFFER_SIZE] = {0};
// int index = 0;

/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH] = {0};
volatile uint8_t index = 0;

/* The mqtt client struct */
static struct mqtt_client client;
/* File descriptor */
static struct pollfd fds;

static K_SEM_DEFINE(lte_connected, 0, 1);

struct __attribute__((__packed__)) data_packet_t
{
	uint16_t packet_header;
	uint16_t packet_version;
	uint16_t actual_packet_nr;
	uint16_t total_packet_nr;
	uint32_t timestamp;
	uint32_t total_sample_count;
	uint16_t train_counter;

	uint16_t chan_0_vlt[WAVE_SAMPLE_LEN];
	uint16_t chan_0_int[WAVE_SAMPLE_LEN];
	uint16_t chan_1_vlt[WAVE_SAMPLE_LEN];
	uint16_t chan_1_int[WAVE_SAMPLE_LEN];

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
	uint8_t signal_stength;
	uint16_t modem_status_word;
	float GPS_lat;
	float GPS_lon;
	float GPS_alt;
	uint16_t CRC;
};

static struct data_packet_t seed_packet;
uint16_t train_counter = 0;

uint16_t chan_dat[64] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint16_t chan_dat_128[128] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(0);

volatile uint8_t ADC_SAMPLE_FLAG = 0;

int16_t sample_buffer[4];

#define RES_VAR_LEN 16000

uint16_t rec_counter = 0;

int16_t ch0_volt[RES_VAR_LEN];
int16_t ch1_volt[RES_VAR_LEN];
int16_t ch0_int[RES_VAR_LEN];
int16_t ch1_int[RES_VAR_LEN];

static const struct adc_sequence sequence = {
	.channels = BIT(CHANNEL_1) | BIT(CHANNEL_2) | BIT(CHANNEL_3) | BIT(CHANNEL_4),
	.buffer = sample_buffer,
	.buffer_size = sizeof(sample_buffer),
	.resolution = 14
	//.oversampling = NRF_SAADC_OVERSAMPLE_16X
};

static const struct adc_channel_cfg channel_cfg_0 = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, DAQ_TIME_US),
	.channel_id = CHANNEL_0,
	.input_positive = NRF_SAADC_INPUT_AIN0};

static const struct adc_channel_cfg channel_cfg_1 = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, DAQ_TIME_US),
	.channel_id = CHANNEL_1,
	.input_positive = NRF_SAADC_INPUT_AIN1};

static const struct adc_channel_cfg channel_cfg_2 = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, DAQ_TIME_US),
	.channel_id = CHANNEL_2,
	.input_positive = NRF_SAADC_INPUT_AIN2};

static const struct adc_channel_cfg channel_cfg_3 = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, DAQ_TIME_US),
	.channel_id = CHANNEL_3,
	.input_positive = NRF_SAADC_INPUT_AIN3};

static const struct adc_channel_cfg channel_cfg_4 = {
	.gain = ADC_GAIN_1_6,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, DAQ_TIME_US),
	.channel_id = CHANNEL_4,
	.input_positive = NRF_SAADC_INPUT_AIN4};

static void timer_handler(nrf_timer_event_t event_type, void *p_context)
{
	if (event_type == NRF_TIMER_EVENT_COMPARE0)
	{
		ADC_SAMPLE_FLAG = 1;
	}
}

LOG_MODULE_REGISTER(Lesson4_Exercise1, LOG_LEVEL_INF);

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type)
	{
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
			(evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING))
		{
			break;
		}
		LOG_INF("Network registration status: %s",
				evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		LOG_INF("RRC mode: %s", evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ? "Connected" : "Idle");
		break;
	default:
		break;
	}
}

static int modem_configure(void)
{
	int err;

	LOG_INF("Initializing modem library");
	err = nrf_modem_lib_init();
	if (err)
	{
		LOG_ERR("Failed to initialize the modem library, error: %d", err);
		return err;
	}

	LOG_INF("Connecting to LTE network");
	err = lte_lc_connect_async(lte_handler);
	if (err)
	{
		LOG_ERR("Error in lte_lc_connect_async, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE network");
	dk_set_led_on(DK_LED2);

	return 0;
}

void send_multiple_packets(uint16_t total_packet_to_send)
{

	uint16_t pckt_cnt = 1;

	while (pckt_cnt <= total_packet_to_send)
	{
		seed_packet.packet_header = 0xBEEF;
		seed_packet.packet_version = 0x0101;
		seed_packet.actual_packet_nr = pckt_cnt++;
		seed_packet.total_packet_nr = total_packet_to_send;

		seed_packet.timestamp = 1748277406;
		seed_packet.train_counter = train_counter;

		seed_packet.CRC = 0xABCD;

		memcpy(seed_packet.chan_0_vlt, chan_dat, 64);
		memcpy(seed_packet.chan_1_vlt, chan_dat, 16);
		memcpy(seed_packet.chan_0_int, chan_dat_128, 128);
		memcpy(seed_packet.chan_1_int, &chan_dat_128[10], 50);

		uint16_t sizestruct = sizeof(seed_packet);
		LOG_INF("size of struct is: %d", sizestruct);
		uint8_t *byte_ptr = (uint8_t *)&seed_packet;

		//	int err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
		//						   byte_ptr, sizestruct);

		int err = data_publish(&client, MQTT_QOS_0_AT_MOST_ONCE,
							   byte_ptr, sizestruct);

		if (err)
		{
			LOG_INF("Failed to send message, %d", err);
			return;
		}
	}
	train_counter++;
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	switch (has_changed)
	{
	case DK_BTN1_MSK:

		if (button_state & DK_BTN1_MSK)
		{
			seed_packet.packet_header = 0xBEEF;
			seed_packet.packet_version = 0x0101;
			seed_packet.actual_packet_nr = 0x0001;
			seed_packet.total_packet_nr = 0x0001;

			seed_packet.timestamp = 1748277406;
			seed_packet.train_counter = train_counter;
			train_counter++;
			seed_packet.CRC = 0xABCD;

			memcpy(seed_packet.chan_0_vlt, chan_dat, 64);
			memcpy(seed_packet.chan_1_vlt, chan_dat, 16);
			memcpy(&seed_packet.chan_0_int[5], chan_dat_128, 128);
			memcpy(seed_packet.chan_1_int, &chan_dat_128[10], 50);

			uint16_t sizestruct = sizeof(seed_packet);
			LOG_INF("size of struct is: %d", sizestruct);
			uint8_t *byte_ptr = (uint8_t *)&seed_packet;

			k_sleep(K_MSEC(100));
			LOG_INF("step: %d", 4);
			k_sleep(K_MSEC(100));
			int err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
								   byte_ptr, sizestruct);
			if (err)
			{
				LOG_INF("Failed to send message, %d", err);
				return;
			}
		}
		break;

	case DK_BTN2_MSK:
		if (button_state & DK_BTN2_MSK)
		{
			send_multiple_packets(50);
		}
		break;

	default:
		break;
	}
}

void add_samples_to_buffer(int16_t samples[BUFFER_WIDTH], int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH])
{
	for (int i = 0; i < BUFFER_WIDTH; i++)
	{
		buffer[index][i] = samples[i];
	}
	index++;
	if (index == BUFFER_LENGTH)
	{
		index = 0;
	}
}

void print_4buffer(int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH])
{
	for (int i = 0; i < BUFFER_LENGTH; i++)
	{
		for (int j = 0; j < BUFFER_WIDTH; j++)
		{
			printf("%d,", buffer[i][j]);
		}
		printf("\n");
	}
}

void average_of_vectors(int16_t array[BUFFER_LENGTH][BUFFER_WIDTH], int16_t averages[BUFFER_WIDTH])
{
	for (int i = 0; i < BUFFER_WIDTH; i++)
	{
		int32_t total_sum = 0;
		for (int j = 0; j < BUFFER_LENGTH; j++)
		{
			total_sum += array[j][i];
		}
		averages[i] = (int16_t)(total_sum / BUFFER_LENGTH);
	}
}

#define ALPHA_NUM 1	 // Numerator of alpha (e.g., 1)
#define ALPHA_DEN 25 // Denominator of alpha (e.g., 10) → alpha = 0.1

int32_t exponential_filter(int32_t previous_filtered, int32_t new_sample)
{
	return (ALPHA_NUM * new_sample + (ALPHA_DEN - ALPHA_NUM) * previous_filtered) / ALPHA_DEN;
}

#define BUFFER_SIZE 100

int32_t update_rms(int32_t *buffer, size_t *index, int64_t *sum_squares, int32_t new_sample)
{
	// Remove the oldest sample's square from the sum
	int32_t old_sample = buffer[*index];
	*sum_squares -= (int64_t)old_sample * old_sample;

	// Add the new sample's square to the sum
	buffer[*index] = new_sample;
	*sum_squares += (int64_t)new_sample * new_sample;

	// Update index
	*index = (*index + 1) % BUFFER_SIZE;

	// Calculate RMS
	int32_t mean_square = *sum_squares / BUFFER_SIZE;
	return (int32_t)sqrt((double)mean_square);
}

#define CRCLR_BUFF_SIZE 1024
int16_t circ_buff_ch0[CRCLR_BUFF_SIZE] = {0};
int16_t circ_buff_ch1[CRCLR_BUFF_SIZE] = {0};
int16_t circ_buff_ch2[CRCLR_BUFF_SIZE] = {0};
int16_t circ_buff_ch3[CRCLR_BUFF_SIZE] = {0};
uint16_t buff_head = 0;

uint16_t add_value_to_buffer(int16_t value0, int16_t value1, int16_t value2, int16_t value3)
{
	if (buff_head < CRCLR_BUFF_SIZE)
	{
		circ_buff_ch0[buff_head] = value0;
		circ_buff_ch1[buff_head] = value1;
		circ_buff_ch2[buff_head] = value2;
		circ_buff_ch3[buff_head] = value3;
		buff_head++;
	}
	else
	{
		buff_head = 0;
		circ_buff_ch0[buff_head] = value0;
		circ_buff_ch1[buff_head] = value1;
		circ_buff_ch2[buff_head] = value2;
		circ_buff_ch3[buff_head] = value3;
		buff_head++;
	}
	return (buff_head);
}

// Clears a buffer of int16_t values
void clear_buffer(int16_t *buffer, size_t length)
{
	memset(buffer, 0, length * sizeof(int16_t));
}

int main(void)
{
	int err;

	uint32_t connect_attempt = 0;

	uint32_t record_cnt = 1780;

	nrfx_err_t status;
	(void)status;

#if defined(__ZEPHYR__)
	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST, NRFX_TIMER_INST_HANDLER_GET(TIMER_INST_IDX), 0, 0);
#endif

	int handle = 0;
	int ret = 0;

	nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
	uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
	nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
	config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	config.p_context = "Some context";
	// config.interrupt_priority=20;

	status = nrfx_timer_init(&timer_inst, &config, timer_handler);
	NRFX_ASSERT(status == NRFX_SUCCESS);

	nrfx_timer_clear(&timer_inst);
	uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer_inst, TIME_TO_WAIT_US);
	nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
								NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);

	k_sleep(K_MSEC(500));

	printk("\nSAMPLE APP STARTS\n");

	if (dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LED library");
	}

	if (!device_is_ready(adc_dev))
	{
		printk("ADC not ready\n");
		return;
	}

	adc_channel_setup(adc_dev, &channel_cfg_0);
	adc_channel_setup(adc_dev, &channel_cfg_1);
	adc_channel_setup(adc_dev, &channel_cfg_2);
	adc_channel_setup(adc_dev, &channel_cfg_3);
	adc_channel_setup(adc_dev, &channel_cfg_4);

	printk("\nstarting timer\n");
	k_sleep(K_MSEC(100));
	nrfx_timer_enable(&timer_inst);

	rec_counter = 0;
	int16_t sample_print = 0;
	int16_t sample_buffer2[4] = {0};

	int16_t sample_solve_rms = 0;

	int32_t filtered_value_array[4] = {5200, 5200, 5200, 5200};
	int32_t filtered_value = 5200;
	int32_t temp_value = 5200;
	int32_t new_sample;

	int16_t offsets[4];
	uint8_t chsel = 0;

	uint16_t offset_gather_cnt = 200;
	while (offset_gather_cnt--)
	{
		while (ADC_SAMPLE_FLAG == 0)
		{
			// wait
		}
		if (ADC_SAMPLE_FLAG == 1)
		{
			// gpio_pin_set_dt(&led0, 1);
			ADC_SAMPLE_FLAG = 0;
			adc_read(adc_dev, &sequence);				   // takes 248 us
			add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
			average_of_vectors(buffer, &sample_buffer2);   // 3.5 us

			chsel = 0;
			while (chsel < 4)
			{
				filtered_value = filtered_value_array[chsel];
				temp_value = sample_buffer2[chsel];
				filtered_value = exponential_filter(filtered_value, temp_value);
				filtered_value_array[chsel] = filtered_value;
				chsel++;
			}
		}
	}
	offsets[0] = filtered_value_array[0];
	offsets[1] = filtered_value_array[1];
	offsets[2] = filtered_value_array[2];
	offsets[3] = filtered_value_array[3];

	int32_t rms_value[4] = {0};

	int32_t buffer_rms_ch0[BUFFER_SIZE] = {0};
	int32_t buffer_rms_ch1[BUFFER_SIZE] = {0};
	int32_t buffer_rms_ch2[BUFFER_SIZE] = {0};
	int32_t buffer_rms_ch3[BUFFER_SIZE] = {0};

	int32_t ch0_off_value;
	int32_t ch1_off_value;
	int32_t ch2_off_value;
	int32_t ch3_off_value;

	size_t index = 0;
	int64_t sum_squares_ch0 = 0;
	int64_t sum_squares_ch1 = 0;
	int64_t sum_squares_ch2 = 0;
	int64_t sum_squares_ch3 = 0;
	uint16_t last_circ_buff_record = 0;



	while (1) // waiting for trigger
	{

		if (ADC_SAMPLE_FLAG == 1)
		{
			// gpio_pin_set_dt(&led0, 1);
			ADC_SAMPLE_FLAG = 0;
			adc_read(adc_dev, &sequence);				   // takes 248 us
			add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
			average_of_vectors(buffer, &sample_buffer2);   // 3.5 us

			new_sample = sample_buffer2[0] - offsets[0]; // Replace with actual sensor/ADC reading
			ch0_off_value = new_sample;
			rms_value[0] = update_rms(buffer_rms_ch0, &index, &sum_squares_ch0, new_sample);

			new_sample = sample_buffer2[1] - offsets[1]; // Replace with actual sensor/ADC reading
			ch1_off_value = new_sample;
			rms_value[1] = update_rms(buffer_rms_ch1, &index, &sum_squares_ch1, new_sample);

			new_sample = sample_buffer2[2] - offsets[2]; // Replace with actual sensor/ADC reading
			ch2_off_value = new_sample;
			rms_value[2] = update_rms(buffer_rms_ch2, &index, &sum_squares_ch2, new_sample);

			new_sample = sample_buffer2[3] - offsets[3]; // Replace with actual sensor/ADC reading
			ch3_off_value = new_sample;
			rms_value[3] = update_rms(buffer_rms_ch3, &index, &sum_squares_ch3, new_sample);

			if (ch0_off_value > 32750)
			{
				ch0_off_value = 32750;
			}
			if (ch0_off_value < -32750)
			{
				ch0_off_value = -32750;
			}

			if (ch1_off_value > 32750)
			{
				ch1_off_value = 32750;
			}
			if (ch1_off_value < -32750)
			{
				ch1_off_value = -32750;
			}

			if (ch2_off_value > 32750)
			{
				ch2_off_value = 32750;
			}
			if (ch2_off_value < -32750)
			{
				ch2_off_value = -32750;
			}

			if (ch3_off_value > 32750)
			{
				ch3_off_value = 32750;
			}
			if (ch3_off_value < -32750)
			{
				ch3_off_value = -32750;
			}

			last_circ_buff_record = add_value_to_buffer(ch0_off_value, ch1_off_value, ch2_off_value, ch3_off_value);

#ifdef SAMPLE_PRINTING_ENABLED
			if (sample_print >= 3)
			{
				sample_print = 0;

				printk("%d, %d, %d, %d\n",
					   ch0_off_value,
					   ch1_off_value,
					   ch2_off_value,
					   ch3_off_value);
			}
			sample_print++;
#endif
		}

		if ((rms_value[0] > 500) || (rms_value[2] > 500))
		{
		//	last_circ_buff_record = add_value_to_buffer(10000, 10000, 10000, 10000); //FIXME JEN PRO FLAG


			uint16_t sample_cntr=0;
			sample_cntr=last_circ_buff_record;
			while (1)
			{

				if (ADC_SAMPLE_FLAG == 1)
				{


					// gpio_pin_set_dt(&led0, 1);
					ADC_SAMPLE_FLAG = 0;
					adc_read(adc_dev, &sequence);				   // takes 248 us
					add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
					average_of_vectors(buffer, &sample_buffer2);   // 3.5 us

					new_sample = sample_buffer2[0] - offsets[0]; // Replace with actual sensor/ADC reading
					ch0_off_value = new_sample;
					rms_value[0] = update_rms(buffer_rms_ch0, &index, &sum_squares_ch0, new_sample);

					new_sample = sample_buffer2[1] - offsets[1]; // Replace with actual sensor/ADC reading
					ch1_off_value = new_sample;
					rms_value[1] = update_rms(buffer_rms_ch1, &index, &sum_squares_ch1, new_sample);

					new_sample = sample_buffer2[2] - offsets[2]; // Replace with actual sensor/ADC reading
					ch2_off_value = new_sample;
					rms_value[2] = update_rms(buffer_rms_ch2, &index, &sum_squares_ch2, new_sample);

					new_sample = sample_buffer2[3] - offsets[3]; // Replace with actual sensor/ADC reading
					ch3_off_value = new_sample;
					rms_value[3] = update_rms(buffer_rms_ch3, &index, &sum_squares_ch3, new_sample);

					if (ch0_off_value > 32750)
					{
						ch0_off_value = 32750;
					}
					if (ch0_off_value < -32750)
					{
						ch0_off_value = -32750;
					}

					if (ch1_off_value > 32750)
					{
						ch1_off_value = 32750;
					}
					if (ch1_off_value < -32750)
					{
						ch1_off_value = -32750;
					}

					if (ch2_off_value > 32750)
					{
						ch2_off_value = 32750;
					}
					if (ch2_off_value < -32750)
					{
						ch2_off_value = -32750;
					}

					if (ch3_off_value > 32750)
					{
						ch3_off_value = 32750;
					}
					if (ch3_off_value < -32750)
					{
						ch3_off_value = -32750;
					}


					ch0_volt[sample_cntr]=ch0_off_value;
					ch0_int[sample_cntr]=ch1_off_value;
					ch1_volt[sample_cntr]=ch2_off_value;
					ch1_int[sample_cntr]=ch3_off_value;
				
				
					sample_cntr++;


					if (sample_cntr>3000) {
						break;
					}



				}
			}

			// FIXME CHECKNOUT OKRAJOVE PODMINKY atd
			if ((last_circ_buff_record == 0) || (last_circ_buff_record == (CRCLR_BUFF_SIZE - 1)))
			{
				memcpy(ch0_volt, circ_buff_ch0, CRCLR_BUFF_SIZE * 2);
				memcpy(ch0_int, circ_buff_ch2, CRCLR_BUFF_SIZE * 2);
				memcpy(ch1_volt, circ_buff_ch1, CRCLR_BUFF_SIZE * 2);
				memcpy(ch1_int, circ_buff_ch3, CRCLR_BUFF_SIZE * 2);
			}
			else
			{

				memcpy(ch0_volt, &circ_buff_ch0[last_circ_buff_record], (CRCLR_BUFF_SIZE - last_circ_buff_record) * 2);
				memcpy(ch0_int, &circ_buff_ch1[last_circ_buff_record], (CRCLR_BUFF_SIZE - last_circ_buff_record) * 2);
				memcpy(ch1_volt, &circ_buff_ch2[last_circ_buff_record], (CRCLR_BUFF_SIZE - last_circ_buff_record) * 2);
				memcpy(ch1_int, &circ_buff_ch3[last_circ_buff_record], (CRCLR_BUFF_SIZE - last_circ_buff_record) * 2);

				memcpy(&ch0_volt[CRCLR_BUFF_SIZE - last_circ_buff_record], circ_buff_ch0, last_circ_buff_record * 2);
				memcpy(&ch0_int[CRCLR_BUFF_SIZE - last_circ_buff_record], circ_buff_ch1, last_circ_buff_record * 2);
				memcpy(&ch1_volt[CRCLR_BUFF_SIZE - last_circ_buff_record], circ_buff_ch2, last_circ_buff_record * 2);
				memcpy(&ch1_int[CRCLR_BUFF_SIZE - last_circ_buff_record], circ_buff_ch3, last_circ_buff_record * 2);
			}

			clear_buffer(circ_buff_ch0, CRCLR_BUFF_SIZE);
			clear_buffer(circ_buff_ch1, CRCLR_BUFF_SIZE);
			clear_buffer(circ_buff_ch2, CRCLR_BUFF_SIZE);
			clear_buffer(circ_buff_ch3, CRCLR_BUFF_SIZE);

			uint16_t prntcnt = 0;
			while (prntcnt < 4000)
			{
				printk("%d, %d, %d, %d\n",
					   ch0_volt[prntcnt],
					   ch0_int[prntcnt],
					   ch1_volt[prntcnt],
					   ch1_int[prntcnt]);
				k_sleep(K_USEC(1000));

				prntcnt++;
			}
		}
	}

	err = modem_configure();
	if (err)
	{
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

	if (dk_buttons_init(button_handler) != 0)
	{
		LOG_ERR("Failed to initialize the buttons library");
	}

	err = client_init(&client);
	if (err)
	{
		LOG_ERR("Failed to initialize MQTT client: %d", err);
		return 0;
	}

do_connect:
	if (connect_attempt++ > 0)
	{
		LOG_INF("Reconnecting in %d seconds...",
				CONFIG_MQTT_RECONNECT_DELAY_S);
		k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
	}

	err = mqtt_connect(&client);
	if (err)
	{
		LOG_ERR("Error in mqtt_connect: %d", err);
		goto do_connect;
	}

	err = fds_init(&client, &fds);
	if (err)
	{
		LOG_ERR("Error in fds_init: %d", err);
		return 0;
	}

	rec_counter = 0;
	while (1)
	{

		// gpio_pin_set_dt(&led0, 1);
		// gpio_pin_toggle_dt(&led0);
		// adc_read(adc_dev, &sequence);
		if (ADC_SAMPLE_FLAG == 1)
		{
			// gpio_pin_set_dt(&led0, 1);
			ADC_SAMPLE_FLAG = 0;
			adc_read(adc_dev, &sequence);				   // takes 248 us
			add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
			average_of_vectors(buffer, &sample_buffer2);   // 3.5 us

			//    printk("\nfinito\n");

			/*
			ch0_volt[rec_counter]=sample_buffer2[0];
			ch1_volt[rec_counter]=sample_buffer2[1];
			ch0_int[rec_counter]=sample_buffer2[2];
			ch1_int[rec_counter]=sample_buffer2[3];
			rec_counter++;                              // 1us
				*/

			if (sample_print >= 4)
			{
				sample_print = 0;

				printk("%d, %d, %d, %d\n",
					   sample_buffer2[0],
					   sample_buffer2[1],
					   sample_buffer2[2],
					   sample_buffer2[3]);
			}
			sample_print++;
		}

		err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
		if (err < 0)
		{
			LOG_ERR("Error in poll(): %d", errno);
			break;
		}

		err = mqtt_live(&client);
		if ((err != 0) && (err != -EAGAIN))
		{
			LOG_ERR("Error in mqtt_live: %d", err);
			break;
		}

		if ((fds.revents & POLLIN) == POLLIN)
		{
			err = mqtt_input(&client);
			if (err != 0)
			{
				LOG_ERR("Error in mqtt_input: %d", err);
				break;
			}
		}

		if ((fds.revents & POLLERR) == POLLERR)
		{
			LOG_ERR("POLLERR");
			break;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL)
		{
			LOG_ERR("POLLNVAL");
			break;
		}
	}

	LOG_INF("Disconnecting MQTT client");

	err = mqtt_disconnect(&client);
	if (err)
	{
		LOG_ERR("Could not disconnect MQTT client: %d", err);
	}
	goto do_connect;

	return 0;
}

/*



	if (!device_is_ready(adc_dev))
	{
		printk("ADC not ready\n");
		return;
	}

	// adc_channel_setup(adc_dev, &channel_cfg_0);
	adc_channel_setup(adc_dev, &channel_cfg_1);
	adc_channel_setup(adc_dev, &channel_cfg_2);
	adc_channel_setup(adc_dev, &channel_cfg_3);
	adc_channel_setup(adc_dev, &channel_cfg_4);

	if (dk_leds_init() != 0)
	{
		LOG_ERR("Failed to initialize the LED library");
	}

	err = modem_configure();
	if (err)
	{
		LOG_ERR("Failed to configure the modem");
		return 0;
	}

	if (dk_buttons_init(button_handler) != 0)
	{
		LOG_ERR("Failed to initialize the buttons library");
	}

	err = client_init(&client);
	if (err)
	{
		LOG_ERR("Failed to initialize MQTT client: %d", err);
		return 0;
	}

do_connect:
	if (connect_attempt++ > 0)
	{
		LOG_INF("Reconnecting in %d seconds...",
				CONFIG_MQTT_RECONNECT_DELAY_S);
		k_sleep(K_SECONDS(CONFIG_MQTT_RECONNECT_DELAY_S));
	}

	err = mqtt_connect(&client);
	if (err)
	{
		LOG_ERR("Error in mqtt_connect: %d", err);
		goto do_connect;
	}

	err = fds_init(&client, &fds);
	if (err)
	{
		LOG_ERR("Error in fds_init: %d", err);
		return 0;
	}

	if (record_cnt > 500)
	{
		send_multiple_packets(5);
		record_cnt = 0;
	}
	else
	{
		record_cnt++;
		k_sleep(K_MSEC(100));
	}

	while (1)
	{

		rec_counter = 0;
		int16_t sample_buffer2[4] = {0};

		// gpio_pin_set_dt(&led0, 1);
		// gpio_pin_toggle_dt(&led0);
		// adc_read(adc_dev, &sequence);
		if (ADC_SAMPLE_FLAG == 1)
		{
			// gpio_pin_set_dt(&led0, 1);
			ADC_SAMPLE_FLAG = 0;
			adc_read(adc_dev, &sequence);				   // takes 248 us
			add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
			average_of_vectors(buffer, &sample_buffer2);   // 3.5 us

			ch0_volt[rec_counter] = sample_buffer2[0];
			ch1_volt[rec_counter] = sample_buffer2[1];
			ch0_int[rec_counter] = sample_buffer2[2];
			ch1_int[rec_counter] = sample_buffer2[3];
			rec_counter++; // 1us

			if (rec_counter > RES_VAR_LEN - 1)
			{
				while (1)
				{
					printk("\nfinito\n");
					k_sleep(K_MSEC(1000));
				};
			}

		} //	gpio_pin_set_dt(&led0, 0);
	}







	err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
	if (err < 0)
	{
		LOG_ERR("Error in poll(): %d", errno);
		break;
	}

	err = mqtt_live(&client);
	if ((err != 0) && (err != -EAGAIN))
	{
		LOG_ERR("Error in mqtt_live: %d", err);
		break;
	}

	if ((fds.revents & POLLIN) == POLLIN)
	{
		err = mqtt_input(&client);
		if (err != 0)
		{
			LOG_ERR("Error in mqtt_input: %d", err);
			break;
		}
	}

	if ((fds.revents & POLLERR) == POLLERR)
	{
		LOG_ERR("POLLERR");
		break;
	}

	if ((fds.revents & POLLNVAL) == POLLNVAL)
	{
		LOG_ERR("POLLNVAL");
		break;
	}

	LOG_INF("Disconnecting MQTT client");

	err = mqtt_disconnect(&client);
	if (err)
	{
		LOG_ERR("Could not disconnect MQTT client: %d", err);
	}
	goto do_connect;
*/

/* This is never reached */

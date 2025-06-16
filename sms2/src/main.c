/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <modem/sms.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <hal/nrf_saadc.h>
#include <nrfx_timer.h>



#define BTN0_NODE DT_ALIAS(sw0)
#define LED0_NODE DT_ALIAS(led0)
#define DAQ_TIME_US 40

#define ADC_NODE DT_IO_CHANNELS_CTLR(DT_PATH(zephyr_user))

#define CHANNEL_0 0
#define CHANNEL_1 1
#define CHANNEL_2 2
#define CHANNEL_3 3
#define CHANNEL_4 4

#define BUFFER_WIDTH 4
#define BUFFER_LENGTH 5

// nt buffer[BUFFER_SIZE] = {0};
// int index = 0;



/** @brief Symbol specifying timer instance to be used. */
#define TIMER_INST_IDX 0

/** @brief Symbol specifying time in milliseconds to wait for handler execution. */
#define TIME_TO_WAIT_US 1000UL



int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH] = {0};
volatile uint8_t index = 0;

/*
int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH] = {0};
int16_t index = 0;
*/
static const struct gpio_dt_spec btn0 = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

static const nrfx_timer_t timer = NRFX_TIMER_INSTANCE(0);

volatile uint8_t ADC_SAMPLE_FLAG=0;

int16_t sample_buffer[4];

#define RES_VAR_LEN  16000

uint16_t rec_counter=0;

uint16_t ch0_volt[RES_VAR_LEN];
uint16_t ch1_volt[RES_VAR_LEN];
uint16_t ch0_int[RES_VAR_LEN];
uint16_t ch1_int[RES_VAR_LEN];

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




static void timer_handler(nrf_timer_event_t event_type, void * p_context)
{
    if(event_type == NRF_TIMER_EVENT_COMPARE0)
    {
		ADC_SAMPLE_FLAG=1;
    }
}


static void sms_callback(struct sms_data *const data, void *context)
{
	if (data == NULL)
	{
		printk("%s with NULL data\n", __func__);
		return;
	}

	if (data->type == SMS_TYPE_DELIVER)
	{
		/* When SMS message is received, print information */
		struct sms_deliver_header *header = &data->header.deliver;

		printk("\nSMS received:\n");
		printk("\tTime:   %02d-%02d-%02d %02d:%02d:%02d\n", header->time.year,
			   header->time.month, header->time.day, header->time.hour, header->time.minute,
			   header->time.second);

		printk("\tText:   '%s'\n", data->payload);
		printk("\tLength: %d\n", data->payload_len);

		if (header->app_port.present)
		{
			printk("\tApplication port addressing scheme: dest_port=%d, src_port=%d\n",
				   header->app_port.dest_port, header->app_port.src_port);
		}
		if (header->concatenated.present)
		{
			printk("\tConcatenated short message: ref_number=%d, msg %d/%d\n",
				   header->concatenated.ref_number, header->concatenated.seq_number,
				   header->concatenated.total_msgs);
		}
	}
	else if (data->type == SMS_TYPE_STATUS_REPORT)
	{
		printk("SMS status report received\n");
	}
	else
	{
		printk("SMS protocol message with unknown type received\n");
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

int main(void)
{
	
    nrfx_err_t status;
    (void)status;

#if defined(__ZEPHYR__)
    IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER_INST_GET(TIMER_INST_IDX)), IRQ_PRIO_LOWEST,NRFX_TIMER_INST_HANDLER_GET(TIMER_INST_IDX), 0, 0);
#endif
	
	int handle = 0;
	int ret = 0;




 	nrfx_timer_t timer_inst = NRFX_TIMER_INSTANCE(TIMER_INST_IDX);
    uint32_t base_frequency = NRF_TIMER_BASE_FREQUENCY_GET(timer_inst.p_reg);
    nrfx_timer_config_t config = NRFX_TIMER_DEFAULT_CONFIG(base_frequency);
    config.bit_width = NRF_TIMER_BIT_WIDTH_32;
    config.p_context = "Some context";
	//config.interrupt_priority=20;

    status = nrfx_timer_init(&timer_inst, &config, timer_handler);
    NRFX_ASSERT(status == NRFX_SUCCESS);

    nrfx_timer_clear(&timer_inst);
	uint32_t desired_ticks = nrfx_timer_us_to_ticks(&timer_inst, TIME_TO_WAIT_US);
    nrfx_timer_extended_compare(&timer_inst, NRF_TIMER_CC_CHANNEL0, desired_ticks,
                                NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);





	k_sleep(K_MSEC(500));




	printk("\nSAMPLE APP\n");

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

	

	ret = gpio_pin_configure_dt(&btn0, GPIO_INPUT);
	if (ret < 0)
	{
		printk("BTN0 initialization failed, error: %d\n", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		printk("LED0 initialization failed, error: %d\n", ret);
		return 0;
	}
	else
	{
		gpio_pin_set_dt(&led0, 0);
	}

	printk("led and button done\n");

/*
	ret = nrf_modem_lib_init();
	if (ret)
	{
		printk("Modem library initialization failed, error: %d\n", ret);
		return 0;
	}
	else
	{
		printk("modem init ok\n");
	}

	ret = lte_lc_connect();
	if (ret)
	{
		printk("Lte_lc failed to initialize and connect, err %d", ret);
		return 0;
	}
	else
	{
		printk("lte connected\n");
	}

	handle = sms_register_listener(sms_callback, NULL);
	if (handle)
	{
		printk("sms_register_listener returned err: %d\n", handle);
		return 0;
	}
	else
	{
		printk("sms listener init init ok\n");
	}

	printk("SMS sample is ready for receiving messages\n");

	*/
 





	printk("\nstarting timer\n");
	k_sleep(K_MSEC(1000));
    nrfx_timer_enable(&timer_inst);
	//k_sleep(K_MSEC(5000));
	//nrfx_timer_disable(&timer_inst);

	while (1)
	{



rec_counter=0;
		int16_t sample_buffer2[4] = {0};
		while (1){
			
		//gpio_pin_set_dt(&led0, 1);
		//gpio_pin_toggle_dt(&led0);
		//adc_read(adc_dev, &sequence);
		if (ADC_SAMPLE_FLAG==1) {
			gpio_pin_set_dt(&led0, 1);
			ADC_SAMPLE_FLAG=0;
			adc_read(adc_dev, &sequence);  // takes 248 us
			add_samples_to_buffer(sample_buffer, &buffer); // takes 2 us
			average_of_vectors(buffer, &sample_buffer2); // 3.5 us


			ch0_volt[rec_counter]=sample_buffer2[0];
			ch1_volt[rec_counter]=sample_buffer2[1];
			ch0_int[rec_counter]=sample_buffer2[2];
			ch1_int[rec_counter]=sample_buffer2[3];
			rec_counter++;                              // 1us

			if (rec_counter>RES_VAR_LEN-1) {
				while (1) {
					printk("\nfinito\n");
					k_sleep(K_MSEC(1000));
				};
			}


			gpio_pin_set_dt(&led0, 0);

		}
		
		}


		uint8_t smpcnt = 0;
		while (1)
		{
			int ret = adc_read(adc_dev, &sequence);
			smpcnt++;
			if (ret == 0)
			{
				add_samples_to_buffer(sample_buffer, &buffer);
			}
			else
			{
				printk("ADC read error: %d\n", ret);
			}
			k_sleep(K_USEC(100));

			if (smpcnt == BUFFER_LENGTH)
			{
				smpcnt = 0;
				int16_t sample_buffer2[4] = {0};
				average_of_vectors(buffer, &sample_buffer2);
				printk("%d,%d,%d,%d\n\r", sample_buffer2[0], sample_buffer2[1], sample_buffer2[2], sample_buffer2[3]);

				// print_4buffer(buffer);
			}
		}

		if (gpio_pin_get_dt(&btn0))
		{
			gpio_pin_set_dt(&led0, 1);
			if (strcmp(CONFIG_SMS_SEND_PHONE_NUMBER, ""))
			{
				printk("Sending SMS: number=%s, text=\"SMS sample: testing\"\n",
					   CONFIG_SMS_SEND_PHONE_NUMBER);
				ret = sms_send_text(CONFIG_SMS_SEND_PHONE_NUMBER,
									"Co na to hrabes curaku?");
				if (ret)
				{
					printk("Sending SMS failed with error: %d\n", ret);
				}
			}
			else
			{
				printk("\nSMS sending is skipped but receiving will still work.\n"
					   "If you wish to send SMS, please configure "
					   "CONFIG_SMS_SEND_PHONE_NUMBER\n");
			}
			gpio_pin_set_dt(&led0, 0);
			k_msleep(2000);
		}
	}

	/* In our application, we should unregister SMS in some conditions with:
	 *   sms_unregister_listener(handle);
	 * However, this sample will continue to be registered for
	 * received SMS messages and they can be seen in serial port log.
	 */
	return 0;
}

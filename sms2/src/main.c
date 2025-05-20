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

//nt buffer[BUFFER_SIZE] = {0};
//int index = 0;





int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH] = {0};
int index = 0;



/*
int16_t buffer[BUFFER_LENGTH][BUFFER_WIDTH] = {0};
int16_t index = 0;
*/
static const struct gpio_dt_spec btn0 = GPIO_DT_SPEC_GET(BTN0_NODE, gpios);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int16_t sample_buffer[4];

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
	if (index==BUFFER_LENGTH) {index=0;}
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




void average_of_vectors(int16_t array[BUFFER_LENGTH][BUFFER_WIDTH], int16_t averages[BUFFER_WIDTH]) {
for (int i = 0; i < BUFFER_WIDTH; i++) {
int32_t total_sum = 0;
for (int j = 0; j < BUFFER_LENGTH; j++) {
total_sum += array[j][i];
}
averages[i] =  (int16_t)(total_sum / BUFFER_LENGTH);
}
}





/*
void add_sample_to_buffer(int sample, int *buffer, int *index)
{
	buffer[*index] = sample;
	*index = (*index + 1) % BUFFER_SIZE;
}

void print_buffer(int *buffer)
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		printf("%d ", buffer[i]);
	}
	printf("\n");
}
*/
/*

void add_sample(int sample, int *array, int *index) {
    array[*index] = sample;
    *index = (*index + 1) % ARRAY_SIZE;
}

float calculate_average(int *array) {
    int sum = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sum += array[i];
    }
    return (float)sum / ARRAY_SIZE;
}

*/

int main(void)
{
	int handle = 0;
	int ret = 0;

	const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

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

	/*
		//.channels    = BIT(CHANNEL_1) | BIT(CHANNEL_2),
		struct adc_sequence sequence = {
			.channels    = BIT(CHANNEL_3),
			.buffer      = sample_buffer,
			.buffer_size = sizeof(sample_buffer),
			.resolution  = 14,
			//.oversampling = NRF_SAADC_OVERSAMPLE_16X
		};
	*/
	struct adc_sequence sequence = {
		.channels = BIT(CHANNEL_1) | BIT(CHANNEL_2) | BIT(CHANNEL_3) | BIT(CHANNEL_4),
		.buffer = sample_buffer,
		.buffer_size = sizeof(sample_buffer),
		.resolution = 14
		//.oversampling = NRF_SAADC_OVERSAMPLE_16X
	};

	uint8_t smpcnt = 0;
	while (1)
	{
		int ret = adc_read(adc_dev, &sequence);
		smpcnt++;
		if (ret == 0)
		{
			//printk("%d,%d,%d,%d\n\r", sample_buffer[0], sample_buffer[1], sample_buffer[2], sample_buffer[3]);
			//	add_samples_to_buffer(&sample_buffer, &buffer, &index);
			 add_samples_to_buffer(sample_buffer, &buffer);
			//add_sample_to_buffer(sample_buffer[2], buffer, &index);
		}
		else
		{
			printk("ADC read error: %d\n", ret);
		}
		k_sleep(K_USEC(100));

		if (smpcnt == BUFFER_LENGTH)
		{
			smpcnt = 0;
			int16_t sample_buffer2[4]={0};
			average_of_vectors(buffer,&sample_buffer2);
			printk("%d,%d,%d,%d\n\r", sample_buffer2[0], sample_buffer2[1], sample_buffer2[2], sample_buffer2[3]);

			//print_4buffer(buffer);
		}
	}

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

	/* Sending is done to the phone number specified in the configuration,
	 * or if it's left empty, an information text is printed.
	 */

	while (1)
	{

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

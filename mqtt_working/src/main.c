/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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

#define WAVE_SAMPLE_LEN 256
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

static struct data_packet_t seed_packet;
uint16_t train_counter = 0;

uint16_t chan_dat[64]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
uint16_t chan_dat_128[128]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

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
			seed_packet.total_packet_nr = 0x0008;
			
			seed_packet.timestamp = 1748277406;
			seed_packet.train_counter = train_counter;
			train_counter++;
			seed_packet.CRC=0xABCD;

			memcpy(seed_packet.chan_0_vlt,chan_dat,64);
			memcpy(seed_packet.chan_1_vlt,chan_dat,16);
			memcpy(seed_packet.chan_1_int,chan_dat_128,128);


			uint16_t sizestruct=sizeof(seed_packet); 
			LOG_INF("size of struct is: %d", sizestruct);
			uint8_t *byte_ptr = (uint8_t *)&seed_packet;

			

		
/*
			for(uint16_t c=0; c<sizestruct; c=c+4) {
				LOG_INF(" x%0x\t x%0x\t x%0x\t x%0x", byte_ptr[c],byte_ptr[c+1],byte_ptr[c+2],byte_ptr[c+3]);
			    k_sleep(K_USEC(8000));
			}
*/
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

		/*
		if (button_state & DK_BTN1_MSK){
			int err = data_publish(&client, MQTT_QOS_1_AT_LEAST_ONCE,
				   CONFIG_BUTTON_EVENT_PUBLISH_MSG, sizeof(CONFIG_BUTTON_EVENT_PUBLISH_MSG)-1);
			if (err) {
				LOG_INF("Failed to send message, %d", err);
				return;
			}
		}
*/

		break;
	}
}

int main(void)
{
	int err;
	uint32_t connect_attempt = 0;

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

	while (1)
	{
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

	/* This is never reached */
	return 0;
}

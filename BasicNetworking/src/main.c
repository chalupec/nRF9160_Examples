#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <modem/sms.h>
#include <modem/nrf_modem_lib.h>
#include <date_time.h>
#include <zephyr/posix/time.h>
#include <zephyr/posix/sys/time.h>
#include <zephyr/net/mqtt.h>
#include "http_get.h"

K_SEM_DEFINE(lte_connected, 0, 1);


	/* Buffers for MQTT client. */
	static uint8_t rx_buffer[256];
	static uint8_t tx_buffer[256];

	/* MQTT client context */
	static struct mqtt_client client_ctx;

	static struct sockaddr_storage broker;

void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
switch (evt->type) {
/* Handle events here. */
}
}



static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
		case LTE_LC_EVT_NW_REG_STATUS:
				if (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME &&
					evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING) {
						break;
				}

				printk("\nConnected to: %s network\n",
						evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ? "home" : "roaming");

				k_sem_give(&lte_connected);
				break;

		default:
				break;
	}
}

void print_modem_info(enum modem_info info)
{
	int len;
	char buf[80];

	switch (info) {
		case MODEM_INFO_RSRP:
			printk("Signal Strength: ");
			break;
		case MODEM_INFO_IP_ADDRESS:
			printk("IP Addr: ");
			break;
		case MODEM_INFO_FW_VERSION:
			printk("Modem FW Ver: ");
			break;
		case MODEM_INFO_ICCID:
			printk("SIM ICCID: ");
			break;
		case MODEM_INFO_IMSI:
			printk("IMSI: ");
			break;
		case MODEM_INFO_IMEI:
			printk("IMEI: ");
			break;
		case MODEM_INFO_DATE_TIME:
			printk("Network Date/Time: ");
			break;
		case MODEM_INFO_APN:
			printk("APN: ");
			break;
		default:
			printk("Unsupported: ");
			break;
	}

	len = modem_info_string_get(info, buf, 80);
	if (len > 0) {
		printk("%s\n",buf);
	} else {
		printk("Error\n");
	}
}

int main(void)
{
	int err;
	int sock;
	struct zsock_addrinfo *res;






	printk("\nnRF9160 Basic Networking Example (%s)\n", CONFIG_BOARD);

	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem initialization failed, err %d\n", err);
		return -1;
	}

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_UICC);
	if (err) printk("MODEM: Failed enabling UICC power, error: %d\n", err);
	k_msleep(100);
	
	err = modem_info_init();
	if (err) printk("MODEM: Failed initializing modem info module, error: %d\n", err);

	print_modem_info(MODEM_INFO_FW_VERSION);
	print_modem_info(MODEM_INFO_IMEI);
	print_modem_info(MODEM_INFO_ICCID);

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		printk("Failed to connect to the LTE network, err %d\n", err);
		return -1;
	}

	printk("Waiting for network... ");
	k_sem_take(&lte_connected, K_FOREVER);

	print_modem_info(MODEM_INFO_APN);
	print_modem_info(MODEM_INFO_IP_ADDRESS);
	print_modem_info(MODEM_INFO_RSRP);

	k_msleep(5000);

	printk("\r\n");
	printk("Looking up IP addresses\n");
	nslookup("gpstracking.cz", &res);
	print_addrinfo_results(&res);
		
	printk("\r\n");
	printk("Connecting to HTTP Server:\n");
	sock = connect_socket(&res, 80);
	http_get(sock, "http2demo.io","/");
	zsock_close(sock);

	mqtt_client_init(&client_ctx);

	/* MQTT client configuration */
	client_ctx.broker = &broker;
	client_ctx.evt_cb = mqtt_evt_handler;
	client_ctx.client_id.utf8 = (uint8_t *)"zephyr_mqtt_client";
	client_ctx.client_id.size = sizeof("zephyr_mqtt_client") - 1;
	client_ctx.password = NULL;
	client_ctx.user_name = NULL;
	client_ctx.protocol_version = MQTT_VERSION_3_1_1;
	client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;
	
	/* MQTT buffers configuration */
	client_ctx.rx_buf = rx_buffer;
	client_ctx.rx_buf_size = sizeof(rx_buffer);
	client_ctx.tx_buf = tx_buffer;
	client_ctx.tx_buf_size = sizeof(tx_buffer);





	while (1) {
		/*
		 * Do something here
		 */

		k_msleep(1000);
	}
}
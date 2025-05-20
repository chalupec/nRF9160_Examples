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

#include <zephyr/net/socket.h>

#include <zephyr/net/net_ip.h>


#include "http_get.h"

#define CONFIG_MQTT_PUB_TOPIC "your/publish/topic"
#define CONFIG_MQTT_SUB_TOPIC "your/subscribe/topic"
#define CONFIG_MQTT_BROKER_HOSTNAME "iot-course-but.cloud.shiftr.io"
#define CONFIG_MQTT_IP_ADR		"34.77.13.55"
#define CONFIG_MQTT_MOSQ_IP_ADR "5.196.78.28"


#define CONFIG_MQTT_USERNAME "iot-course-but"
#define CONFIG_MQTT_PASSWORD "thisisthemostsecretsecretever"
// #define CONFIG_LTE_AUTO_INIT_AND_CONNECT=y
#define CONFIG_MQTT_BROKER_PORT 1883
// #define CONFIG_MQTT_BROKER_PORT 1883
#define CONFIG_MQTT_CLIENT_ID "i_am_client_jch"

/*
#define CONFIG_MQTT_PUB_TOPIC "your/publish/JCH"
#define CONFIG_MQTT_SUB_TOPIC "your/subscribe/JCH"
#define CONFIG_MQTT_BROKER_HOSTNAME "test.mosquitto.org"

#define CONFIG_MQTT_USERNAME NULL
#define CONFIG_MQTT_PASSWORD NULL
// #define CONFIG_LTE_AUTO_INIT_AND_CONNECT=y
#define CONFIG_MQTT_BROKER_PORT 1883
// #define CONFIG_MQTT_BROKER_PORT 1883
#define CONFIG_MQTT_CLIENT_ID "i_am_client_jch"
*/

K_SEM_DEFINE(lte_connected, 0, 1);

/* Buffers for MQTT client. */
static uint8_t rx_buffer[256];
static uint8_t tx_buffer[256];

/* MQTT client context */
static struct mqtt_client client_ctx;

static struct sockaddr_storage broker;


struct sockaddr_in broker2;


void mqtt_evt_handler(struct mqtt_client *client, const struct mqtt_evt *evt)
{
	printk("mqtt_handler_enter\n");

	switch (evt->type)
	{
	case MQTT_EVT_CONNACK:
		//LOG_INF("MQTT connected");
		break;
	case MQTT_EVT_DISCONNECT:
		//LOG_INF("MQTT disconnected");
		break;
	default:
		break;
	}
}

static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type)
	{
	case LTE_LC_EVT_NW_REG_STATUS:
		if (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME &&
			evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)
		{
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

	switch (info)
	{
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
	if (len > 0)
	{
		printk("%s\n", buf);
	}
	else
	{
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
	if (err)
	{
		printk("Modem initialization failed, err %d\n", err);
		return -1;
	}

	err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_UICC);
	if (err)
		printk("MODEM: Failed enabling UICC power, error: %d\n", err);
	k_msleep(100);

	err = modem_info_init();
	if (err)
		printk("MODEM: Failed initializing modem info module, error: %d\n", err);

	print_modem_info(MODEM_INFO_FW_VERSION);
	print_modem_info(MODEM_INFO_IMEI);
	print_modem_info(MODEM_INFO_ICCID);

	err = lte_lc_connect_async(lte_handler);
	//err = lte_lc_init_and_connect();
	if (err)
	{
		printk("Failed to connect to the LTE network, err %d\n", err);
		return -1;
	}

	printk("Waiting for network... ");
	k_sem_take(&lte_connected, K_FOREVER);
	//k_msleep(8000);
	print_modem_info(MODEM_INFO_APN);
	print_modem_info(MODEM_INFO_IP_ADDRESS);
	print_modem_info(MODEM_INFO_RSRP);

	k_msleep(5000);
	/*
		printk("\r\n");
		printk("Looking up IP addresses\n");
		nslookup("webhook.site", &res);
		print_addrinfo_results(&res);

		printk("\r\n");
		printk("Connecting to HTTP Server:\n");
		sock = connect_socket(&res, 80);
		http_get(sock, "webhook.site", "/f57eaf86-1c5d-481f-9d29-2e8a9e04c1ea");
		zsock_close(sock);
	*/

	/*
		err = nrf_modem_lib_init();
		if (err)
		{
			printk("Modem initialization failed, err %d\n", err);
			return -1;
		}
	k_msleep(2000);
		err = lte_lc_func_mode_set(LTE_LC_FUNC_MODE_ACTIVATE_UICC);
		if (err)
			printk("MODEM: Failed enabling UICC power, error: %d\n", err);
		k_msleep(2000);

		err = modem_info_init();
		if (err)
			printk("MODEM: Failed initializing modem info module, error: %d\n", err);

		print_modem_info(MODEM_INFO_FW_VERSION);
		print_modem_info(MODEM_INFO_IMEI);
		print_modem_info(MODEM_INFO_ICCID);

		k_msleep(2000);

		err = lte_lc_connect_async(lte_handler);
		if (err)
		{
			printk("Failed to connect to the LTE network, err %d\n", err);
			return -1;
		}

		printk("Waiting for network... ");
		k_sem_take(&lte_connected, K_FOREVER);

		print_modem_info(MODEM_INFO_APN);
		print_modem_info(MODEM_INFO_IP_ADDRESS);
		print_modem_info(MODEM_INFO_RSRP);

		k_msleep(5000);

	*/



/*
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);
	inet_pton(AF_INET, CONFIG_MQTT_BROKER_HOSTNAME, &broker4->sin_addr);
*/


	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;
	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(CONFIG_MQTT_BROKER_PORT);
	//zsock_inet_pton(AF_INET, CONFIG_MQTT_IP_ADR, &broker4->sin_addr);
	zsock_inet_pton(AF_INET, CONFIG_MQTT_MOSQ_IP_ADR, &broker4->sin_addr);



/*
broker2.sin_family = AF_INET;
broker2.sin_port = htons(CONFIG_MQTT_BROKER_PORT);
inet_pton(AF_INET, CONFIG_MQTT_BROKER_HOSTNAME, &broker2.sin_addr);
*/


	mqtt_client_init(&client_ctx);

	/* MQTT client configuration */

//client_ctx.broker = (struct sockaddr *)broker;
//client_ctx.broker.sa_family = AF_INET;


	client_ctx.broker = &broker;
	client_ctx.evt_cb = mqtt_evt_handler;
	//client_ctx.client_id.utf8 = CONFIG_MQTT_CLIENT_ID;
	client_ctx.client_id.utf8 = (uint8_t *)CONFIG_MQTT_CLIENT_ID;
	client_ctx.client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);

	//client_ctx.user_name= (uint8_t *)CONFIG_MQTT_USERNAME;
	client_ctx.user_name= (uint8_t *)"";
	//client_ctx.user_name->size = strlen(CONFIG_MQTT_USERNAME);
	//	client_ctx.user_name_len =

	//client_ctx.password = (uint8_t *)CONFIG_MQTT_PASSWORD;
	client_ctx.password = (uint8_t *)"";
    //client_ctx.password->size = strlen(CONFIG_MQTT_PASSWORD);
	//	client_ctx.password_len =

	client_ctx.protocol_version = MQTT_VERSION_3_1_1;
	client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

	client_ctx.rx_buf = rx_buffer;
	client_ctx.rx_buf_size = sizeof(rx_buffer);
	client_ctx.tx_buf = tx_buffer;
	client_ctx.tx_buf_size = sizeof(tx_buffer);

	k_msleep(200);
	printk("MQTT connection start\n");
	printk("client id: %s\n", client_ctx.client_id);
	printk("username: %s\n", client_ctx.user_name);
	printk("pass: %s\n", client_ctx.password);

	k_msleep(200);

	// Connect to MQTT broker
	err = mqtt_connect(&client_ctx);
	if (err)
	{
		printk("MQTT connect failed, error: %d\n", err);
		return;
	}
	else
	{
		printk("MQTT connected %d\n", err);
	}

	k_msleep(2000);

	while (1)
	{
		/*
		 * Do something here
		 */

		mqtt_input(&client_ctx);
		mqtt_live(&client_ctx);
		k_sleep(K_SECONDS(1));
	}
}

/*


	client_ctx.broker = &broker;
	client_ctx.evt_cb = mqtt_evt_handler;
	client_ctx.client_id.utf8 = (uint8_t *)CONFIG_MQTT_CLIENT_ID;
	client_ctx.client_id.size = sizeof(CONFIG_MQTT_CLIENT_ID) - 1;

	client_ctx.user_name = (uint8_t *)CONFIG_MQTT_USERNAME;
	//	client_ctx.user_name_len = strlen(CONFIG_MQTT_USERNAME);

	client_ctx.password = (uint8_t *)CONFIG_MQTT_PASSWORD;
	//	client_ctx.password_len = strlen(CONFIG_MQTT_PASSWORD);

	client_ctx.protocol_version = MQTT_VERSION_3_1_1;
	client_ctx.transport.type = MQTT_TRANSPORT_NON_SECURE;

	client_ctx.rx_buf = rx_buffer;
	client_ctx.rx_buf_size = sizeof(rx_buffer);
	client_ctx.tx_buf = tx_buffer;
	client_ctx.tx_buf_size = sizeof(tx_buffer);

*/
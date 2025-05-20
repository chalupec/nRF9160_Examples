#include <zephyr/net/socket.h>
#include <poll.h>
#include <netdb.h>
#include <zephyr/posix/arpa/inet.h>

#ifndef _MQTTCONNECTION_H_
#define _MQTTCONNECTION_H_
#define LED_CONTROL_OVER_MQTT          DK_LED1 /*The LED to control over MQTT*/
#define IMEI_LEN 15
#define CGSN_RESPONSE_LENGTH (IMEI_LEN + 6 + 1) /* Add 6 for \r\nOK\r\n and 1 for \0 */
#define CLIENT_ID_LEN sizeof("nrf-") + IMEI_LEN
#define MQTT_BUFFER_SIZE 2048

//	client->password->utf8 = "thisisthemostsecretsecretever";
	//client->user_name->utf8 = "iot-course-but";


static struct mqtt_utf8 username = {
	.utf8 = (uint8_t *)"iot-course-but",
	.size = sizeof("iot-course-but") - 1
};

static struct mqtt_utf8 password = {
	.utf8 = (uint8_t *)"thisisthemostsecretsecretever",
	.size = sizeof("thisisthemostsecretsecretever") - 1
};




/**@brief Initialize the MQTT client structure
 */
int client_init(struct mqtt_client *client);

/**@brief Initialize the file descriptor structure used by poll.
 */
int fds_init(struct mqtt_client *c, struct pollfd *fds);

/**@brief Function to publish data on the configured topic
 */
int data_publish(struct mqtt_client *c, enum mqtt_qos qos,
	uint8_t *data, size_t len);

#endif /* _CONNECTION_H_ */

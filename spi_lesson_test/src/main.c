/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* STEP 1.2 - Include the header files for SPI, GPIO and devicetree */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
//#include <zephyr/drivers/spi.h>
#include <nrfx_spim.h>

#define AUTO_TRIG_CS  //exchanging 6bytes cs lo to cs hi auto 14,9us   manual 21us 

LOG_MODULE_REGISTER(Lesson5_Exercise1, LOG_LEVEL_INF);


#define SPIM_INST_IDX 1

/** @brief Symbol specifying pin number for MOSI. */
#define MOSI_PIN 17

/** @brief Symbol specifying pin number for MISO. */
#define MISO_PIN 19

/** @brief Symbol specifying pin number for SCK. */
#define SCK_PIN 16

/** @brief Symbol specifying message to be sent via SPIM data transfer. */
#define MSG_TO_SEND "Nordic Semiconductor"


/** @brief Transmit buffer initialized with the specified message ( @ref MSG_TO_SEND ). */
static uint8_t m_tx_buffer[] = MSG_TO_SEND;

/** @brief Receive buffer defined with the size to store specified message ( @ref MSG_TO_SEND ). */
static uint8_t m_rx_buffer[sizeof(MSG_TO_SEND)];


const struct gpio_dt_spec ledspec = GPIO_DT_SPEC_GET(DT_NODELABEL(led0), gpios);



#ifdef AUTO_TRIG_CS


#else
static const struct gpio_dt_spec MEM_CS = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin  = 18,
    .dt_flags = GPIO_ACTIVE_LOW,
};

#endif







int main(void)
{
	int err;
	  nrfx_err_t status;
    (void)status;








   // NRFX_EXAMPLE_LOG_INIT();

    LOG_INF("Starting nrfx_spim basic blocking example.");
 //   NRFX_EXAMPLE_LOG_PROCESS();


nrfx_spim_t spim_inst = NRFX_SPIM_INSTANCE(SPIM_INST_IDX);
#ifdef AUTO_TRIG_CS
nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(SCK_PIN,
                                                              MOSI_PIN,
                                                              MISO_PIN,
                                                              18);

#else

gpio_pin_configure_dt(&MEM_CS, GPIO_OUTPUT_HIGH); 

nrfx_spim_config_t spim_config = NRFX_SPIM_DEFAULT_CONFIG(SCK_PIN,
                                                              MOSI_PIN,
                                                              MISO_PIN,
                                                              NRF_SPIM_PIN_NOT_CONNECTED);
#endif




    

    
    spim_config.frequency=8000000;

    status = nrfx_spim_init(&spim_inst, &spim_config, NULL, NULL);
    NRFX_ASSERT(status == NRFX_SUCCESS);


    m_tx_buffer[0]=0x9f;


    nrfx_spim_xfer_desc_t spim_xfer_desc = NRFX_SPIM_XFER_TRX(m_tx_buffer, 1,
                                                              m_rx_buffer, 6);

    //status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);

    while (1) {




#ifdef AUTO_TRIG_CS
        status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
#else
		gpio_pin_set_dt(&MEM_CS, 1);  /* CS low */
        status = nrfx_spim_xfer(&spim_inst, &spim_xfer_desc, 0);
		gpio_pin_set_dt(&MEM_CS, 0);
#endif









		LOG_INF("mybuff %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx",
			m_rx_buffer[0],m_rx_buffer[1],m_rx_buffer[2],m_rx_buffer[3],m_rx_buffer[4],m_rx_buffer[5]);

		gpio_pin_toggle_dt(&ledspec);
		k_msleep(1000);

    }

    NRFX_ASSERT(status == NRFX_SUCCESS);

	return 0;
}

#include "../../include/drivers/APS6404L.h"

// extern unsigned char SPI_RW(const unsigned char value);

void FLASH_READ_ID(unsigned char *buff)
{
    uint8_t tx[6] = {0x9F, 0, 0, 0, 0, 0};
    uint8_t rx[6];

    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 6, rx, 6);
    nrf_gpio_pin_clear(MEM_CS_PIN);
    nrfx_spim_xfer(&spim_inst, &xfer, 0);
    nrf_gpio_pin_set(MEM_CS_PIN);

    buff[0] = rx[4];
    buff[1] = rx[5];
}

void FLASH_GLOBAL_UNLOCK(void)
{
    __asm__ volatile("nop");
}

void FLASH_MEMORY_ERASE(void)
{
    __asm__ volatile("nop");
}

void FLASH_MEMORY_WRITE_BYTE_ARRAY(unsigned long address, unsigned char *pBuffer, unsigned int length)
{
    uint8_t tx[36] = {0};
    uint8_t rx[36] = {0};
    tx[0] = SPI_FLASH_WRITE;
    tx[1] = ((address >> 16) & 0xff);
    tx[2] = ((address >> 8) & 0xff);
    tx[3] = (address & 0xff);



    		


    /* Copy src into the last 32 bytes of dest */
    // memcpy(tx + (36 - 32), pBuffer, 32);
    memcpy(tx + 4, pBuffer, length);

    // nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 36,rx, 36);
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, length + 4, rx, length + 4);

    // nrfx_spim_xfer(&spim_inst, &xfer, NRFX_SPIM_FLAG_HOLD_XFER);


   // printk("tx buffer: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X", tx[0], tx[1], tx[2], tx[3], tx[4], tx[5], tx[6], tx[7], tx[8], tx[9]);


    nrf_gpio_pin_clear(MEM_CS_PIN);
    nrfx_spim_xfer(&spim_inst, &xfer, 0);
    nrf_gpio_pin_set(MEM_CS_PIN);
}

void FLASH_MEMORY_READ_DATA(unsigned long address, unsigned char *pBuffer, unsigned int length)
{
    uint8_t tx[36] = {0};
    uint8_t rx[36] = {0};
    tx[0] = SPI_FLASH_READ;
    tx[1] = ((address >> 16) & 0xff);
    tx[2] = ((address >> 8) & 0xff);
    tx[3] = (address & 0xff);

    // nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 36,rx, 36);
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, length + 4, rx, length + 4);
   // printk("tx buffer: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X \n", tx[0], tx[1], tx[2], tx[3], tx[4], tx[5], tx[6], tx[7], tx[8], tx[9]);
  //  k_sleep(K_MSEC(1000));
    
    nrf_gpio_pin_clear(MEM_CS_PIN);
    nrfx_spim_xfer(&spim_inst, &xfer, 0);
    nrf_gpio_pin_set(MEM_CS_PIN);
    //  nrfx_spim_xfer(&spim_inst, &xfer, NRFX_SPIM_FLAG_HOLD_XFER);

 //   printk("rx buffer: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X , 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", rx[0], rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7], rx[8], rx[9], rx[10], rx[11]);
 //   k_sleep(K_MSEC(1000));
    // memcpy(pBuffer, &rx[36 - 32], 32);
    memcpy(pBuffer, &rx[4], length);
}

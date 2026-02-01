#include "../../include/drivers/APS6404L.h"


//extern unsigned char SPI_RW(const unsigned char value);

void FLASH_READ_ID(unsigned char *buff) {
    uint8_t tx[6] = { 0x9F, 0, 0, 0, 0, 0 };
    uint8_t rx[6];

    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 6,rx, 6);
    nrfx_spim_xfer(&spim_inst, &xfer, 0);  
    buff[0]=rx[4];
    buff[1]=rx[5]; 
}

void FLASH_GLOBAL_UNLOCK(void) {
    asm("NOP");
}

void FLASH_MEMORY_ERASE(void) {
    asm("NOP");
}

void FLASH_MEMORY_WRITE_BYTE_ARRAY(unsigned long address, unsigned char *pBuffer, unsigned int length) {
    uint8_t tx[36] = { 0};
    uint8_t rx[36] = { 0};
    tx[0]=SPI_FLASH_WRITE;
    tx[1]=((address >> 16)&0xff);
    tx[2]=((address >> 8)&0xff);
    tx[3]=(address & 0xff);
    /* Copy src into the last 32 bytes of dest */
    memcpy(tx + (36 - 32), pBuffer, 32);

    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 36,rx, 36);
    nrfx_spim_xfer(&spim_inst, &xfer, 0);  
}

void FLASH_MEMORY_READ_DATA(unsigned long address, unsigned char *pBuffer, unsigned int length) {
    uint8_t tx[36] = { 0};
    uint8_t rx[36] = { 0};
    tx[0]=SPI_FLASH_READ;
    tx[1]=((address >> 16)&0xff);
    tx[2]=((address >> 8)&0xff);
    tx[3]=(address & 0xff);
    
    nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_SINGLE_XFER(tx, 36,rx, 36);

    nrfx_spim_xfer(&spim_inst, &xfer, 0);
    memcpy(pBuffer, &rx[36 - 32], 32);

}


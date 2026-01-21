#include "APS6404L.h"

extern unsigned char SPI_RW(const unsigned char value);

void FLASH_READ_ID(unsigned char *buff) {
    unsigned char value = 0;
    SPI_FLASH_CS_SET_LOW // CS low, initialize SPI communication...
    SPI_RW(SPI_FLASH_JEDEC_ID);
    SPI_RW(0);
    SPI_RW(0);
    SPI_RW(0);
    buff[0] = SPI_RW(0);
    buff[1] = SPI_RW(0);
    SPI_FLASH_CS_SET_HIGH // CS high, terminate SPI communication
}

void FLASH_GLOBAL_UNLOCK(void) {
    asm("NOP");
}

void FLASH_MEMORY_ERASE(void) {
    asm("NOP");
}

void FLASH_MEMORY_WRITE_BYTE_ARRAY(unsigned long address, unsigned char *pBuffer, unsigned int length) {
    unsigned int pointer = 0;
    SPI_FLASH_CS_SET_LOW // CS low, initialize SPI communication...
    SPI_RW(SPI_FLASH_WRITE);
    SPI_RW((address >> 16)&0xff);
    SPI_RW((address >> 8)&0xff);
    SPI_RW(address & 0xff);

    // BUFFER write
    while (pointer < length) {
        SPI_RW(pBuffer[pointer]); // Send buffered data
        pointer++;
    }
    SPI_FLASH_CS_SET_HIGH // CS high, terminate SPI communication
}

void FLASH_MEMORY_READ_DATA(unsigned long address, unsigned char *pBuffer, unsigned int length) {
    unsigned int pointer = 0;

    SPI_FLASH_CS_SET_LOW // CS low, initialize SPI communication...
    SPI_RW(SPI_FLASH_READ);
    SPI_RW((address >> 16)&0xff);
    SPI_RW((address >> 8)&0xff);
    SPI_RW(address & 0xff);

    // BUFFER write
    while (pointer < length) {

        pBuffer[pointer] = SPI_RW(0); // read input from send CMD moment, neplatne data       
        pointer++;

    }
    SPI_FLASH_CS_SET_HIGH // CS high, terminate SPI communication
}


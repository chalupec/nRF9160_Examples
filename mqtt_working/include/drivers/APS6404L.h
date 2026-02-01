#ifndef _H_APS6404L__
#define _H_APS6404L__

#ifndef EXTERNAL_FLASH_MEMORY_USED
#define EXTERNAL_FLASH_MEMORY_USED
#endif

#include <nrfx_spim.h>

//******************* Instruction Set ************// 
#define SPI_FLASH_JEDEC_ID 	0x9f
#define SPI_FLASH_WRITE    	0x02  
#define SPI_FLASH_READ  	0x03


#define SPI_FLASH_RSTEN  	0x66
#define SPI_FLASH_RST    	0x99
#define SPI_FLASH_RDSR   	0x05
#define SPI_FLASH_WRSR   	0x01
#define SPI_FLASH_RDCR   	0x35

#define SPI_FLASH_READHS 	0x0b

#define SPI_FLASH_WREN   	0x06
#define SPI_FLASH_WRDI   	0x04
#define SPI_FLASH_CE     	0xc7

#define SPI_FLASH_ULBPR  	0x98

//#define SPI_FLASH_CS_SET_LOW  digitalWrite(BCM27, 0);
//#define SPI_FLASH_CS_SET_HIGH  digitalWrite(BCM27, 1);



extern nrfx_spim_t spim_inst;


//unsigned char SPI_BYTE_RW(unsigned char value);

void FLASH_READ_ID(unsigned char *buff);

void FLASH_GLOBAL_UNLOCK(void);

void FLASH_MEMORY_ERASE(void);


void FLASH_MEMORY_WRITE_BYTE_ARRAY(unsigned long address, unsigned char *pBuffer, unsigned int length);

void FLASH_MEMORY_READ_DATA(unsigned long address, unsigned char *pBuffer, unsigned int length);

#endif
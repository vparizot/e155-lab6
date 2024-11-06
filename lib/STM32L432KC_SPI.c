// STM32L432KC_SPI.c
// Victoria Parizot
// vparizot@hmc.edu
// 11/5/2024
// 
// Source code for SPI communication for Lab 6

#include "STM32L432KC_SPI.h"
#include "STM32L432KC_RCC.h"
/* Enables the SPI peripheral and intializes its clock speed (baud rate), polarity, and phase.
 *    -- br: (0b000 - 0b111). The SPI clk will be the master clock / 2^(BR+1).
 *    -- cpol: clock polarity (0: inactive state is logical 0, 1: inactive state is logical 1).
 *    -- cpha: clock phase (0: data captured on leading edge of clk and changed on next edge, 
 *          1: data changed on leading edge of clk and captured on next edge)
 * Refer to the datasheet for more low-level details. */ 
void initSPI(int br, int cpol, int cpha){

RCC->APB2ENR |= (RCC_APB2ENR_SPI1EN); // enable SPI

// 2. write to SPI_CR1 register
// 2a. config serial clk baud rate w/ BR[2:0] bits
SPI1->CR1 |= _VAL2FLD(SPI_CR1_BR, 0b110); // Used to be 0b111

// 2b. config CPOL & CPHA bits to define one of the four relationships between data transfer and serial clk
SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPOL, cpol);
SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPHA, cpha);

SPI1->CR1 |= _VAL2FLD(SPI_CR1_LSBFIRST, 0b0); // data transmitted & recieved with MSB
SPI1->CR1 |= _VAL2FLD(SPI_CR1_CRCEN, 0b0);
SPI1->CR1 |= _VAL2FLD(SPI_CR1_SSM, 0b1);
SPI1->CR1 |= _VAL2FLD(SPI_CR1_SSI, 0b1);


// 2g. Configure the MSTR bit (in multimaster NSS configuration, avoid conflict state on NSS if master is configured to prevent MODF error).
SPI1->CR1 |= _VAL2FLD(SPI_CR1_MSTR, 0b1);  // other syntax:  |= SPI_CR1_MSTR

SPI1->CR2 |= (0b0111 << SPI_CR2_DS_Pos);

SPI1->CR2 |= _VAL2FLD(SPI_CR2_SSOE, 0b1);
SPI1->CR2 |= _VAL2FLD(SPI_CR2_FRF, 0b0);

// 3e. Configure the FRXTH bit. The RXFIFO threshold must be aligned to the read access size for the SPIx_DR register.
SPI1->CR2 |= _VAL2FLD(SPI_CR2_FRXTH, 0b1); //equal to 8 bit

SPI1->CR1 |= SPI_CR1_SPE; // enable spi
}

/* Transmits a character (1 byte) over SPI and returns the received character.
 *    -- send: the character to send over SPI
 *    -- return: the character received over SPI */
char spiSendReceive(char send){
  // DMA requeste when TXE or RNXE enable bits in the SPIx_CR2 register is set (pg 1317)
  while (!(SPI_SR_TXE & SPI1->SR)){ // TXE event for write access, DMA writes to SPIx_DR register
    // create 8 bit pointer for data register (orig. 16bits)
  }

  volatile uint8_t* drptr =  (volatile uint8_t*)&SPI1->DR; 
  *drptr = send;

  while (!(SPI_SR_RXNE & SPI1->SR)){ //RXNE event triggered when data stored in RXFIFO, DMA reads PIx_DR register
  }

  return *drptr; 
  
}

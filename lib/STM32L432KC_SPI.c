// STM32L432KC_SPI.c
// TODO: <YOUR NAME>
// TODO: <YOUR EMAIL>
// TODO: <DATE>
// TODO: <SHORT DESCRIPTION OF WHAT THIS FILE DOES>

#include "STM32L432KC_SPI.h"

/* Enables the SPI peripheral and intializes its clock speed (baud rate), polarity, and phase.
 *    -- br: (0b000 - 0b111). The SPI clk will be the master clock / 2^(BR+1).
 *    -- cpol: clock polarity (0: inactive state is logical 0, 1: inactive state is logical 1).
 *    -- cpha: clock phase (0: data captured on leading edge of clk and changed on next edge, 
 *          1: data changed on leading edge of clk and captured on next edge)
 * Refer to the datasheet for more low-level details. */ 
void initSPI(int br, int cpol, int cpha){
// baud rate = 111 (256)
// set SSM = 1, ignore SSI
// set CR2 
// set packets to 8 bits
// frf to motorola
// not using crc



// 2. write to SPI_CR1 register
// 2a. config serial clk baud rate w/ BR[2:0] bits
SPI1->CR1 |= _VAL2FLD(SPI_CR1_BR, 0b111);

// 2b. config CPOL & CPHA bits to define one of the four relationships between data transfer and serial clk
SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPOL, 0b0);
SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPHA, 0b1);

// 2g. Configure the MSTR bit (in multimaster NSS configuration, avoid conflict state on NSS if master is configured to prevent MODF error).
SPI1->CR1 |= _VAL2FLD(SPI_CR1_MSTR, 0b1);  // other syntax:  |= SPI_CR1_MSTR

// 2f. Configure SSM and SSI (Notes: 2 & 3).
SPI1->CR1 |= _VAL2FLD(SPI_CR1_SSM, 0b1); //enabled (we r the software)
//SPI1->CR1 |= _VAL2FLD(SPI_CR1_SSI, 0b1);


// 3. Write to SPI_CR2 register:
// 3a. Configure the DS[3:0] bits to select the data length for the transfer.
SPI1->CR2 |= _VAL2FLD(SPI_CR2_DS, 0b0111); //0111: 8-bit
//SPI1->CR2 |= (0b0111 << SPI_CR2_DS_Pos);



// 3e. Configure the FRXTH bit. The RXFIFO threshold must be aligned to the read access size for the SPIx_DR register.
SPI1->CR2 |= _VAL2FLD(SPI_CR2_FRXTH, 0b1); //equal to 8 bit


// 3b. Configure SSOE (Notes: 1 & 2 & 3).
SPI1->CR2 |= _VAL2FLD(SPI_CR2_SSOE, 0b1);



SPI1->CR1 |= SPI_CR1_SPE; // enable spi


// set word size to 8


// 2c. Select simplex or half-duplex mode by configuring RXONLY or BIDIMODE and BIDIOE (RXONLY and BIDIMODE can't be set at the same time).
//SPI1->CR1 |= _VAL2FLD(RXONLY, 0); // 0 is transmit & recieve

// 2d. Configure the LSBFIRST bit to define the frame format (Note: 2)
//SPI1->CR1 |= _VAL2FLD(SPI_CR1_LSBFIRST, 0b0); // data transmitted & recieved with MSB

// 2e. Configure the CRCL and CRCEN bits if CRC is needed (while SCK clock signal is at idle state).
// NO NEED FOR CRC
//SPI1->CR1 |= _VAL2FLD(SPI_CR1_CRCEN, 0b0);



// 3c. Set the FRF bit if the TI protocol is required (keep NSSP bit cleared in TI mode).
//SPI1->CR2 |= _VAL2FLD(SPI_CR2_FRF, 0b0); //motorola mode

// 3d. Set the NSSP bit if the NSS pulse mode between two data units is required (keep CHPA and TI bits cleared in NSSP mode).
// NO NEED FOR PULE MODE


// 3f. Initialize LDMA_TX and LDMA_RX bits if DMA is used in packed mode.
// 0 is even, 1 is odd
//SPI1->CR2 |= _VAL2FLD(SPI_CR2_LDMA_RX, 0b0); // Enable DMA Rx buffer in RXDMAEN bit SPI_CR2 register
//SPI1->CR2 |= _VAL2FLD(SPI_CR2_LDMA_TX, 0b0);

//Questions: de we need to set SPE? (spi enable)
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

  return drptr; 
  
}


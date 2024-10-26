/*
File: Lab_6_JHB.c
Author: Josh Brake
Email: jbrake@hmc.edu
Date: 9/14/19
*/


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"

/////////////////////////////////////////////////////////////////
// Provided Constants and Functions
/////////////////////////////////////////////////////////////////

//Defining the web page in two chunks: everything before the current time, and everything after the current time
char* webpageStart = "<!DOCTYPE html><html><head><title>E155 Web Server Demo Webpage</title>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
	</head>\
	<body><h1>E155 Web Server Demo Webpage</h1>";
char* ledStr = "<p>LED Control:</p><form action=\"ledon\"><input type=\"submit\" value=\"Turn the LED on!\"></form>\
	<form action=\"ledoff\"><input type=\"submit\" value=\"Turn the LED off!\"></form>";
char* webpageEnd   = "</body></html>";

// TODO: whats a good baud rate?
int br = 0b010;  // 200000
int cpol = 0;
int cpha = 1;
char templsb;
char tempmsb;


//determines whether a given character sequence is in a char array request, returning 1 if present, -1 if not present
int inString(char request[], char des[]) {
	if (strstr(request, des) != NULL) {return 1;}
	return -1;
}

int updateLEDStatus(char request[])
{
	int led_status = 0;
	// The request has been received. now process to determine whether to turn the LED on or off
	if (inString(request, "ledoff")==1) {
		digitalWrite(LED_PIN, PIO_LOW);
		led_status = 0;
	}
	else if (inString(request, "ledon")==1) {
		digitalWrite(LED_PIN, PIO_HIGH);
		led_status = 1;
	}

	return led_status;
}

// Function used by printf to send characters to the laptop
int _write(int file, char *ptr, int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    ITM_SendChar((*ptr++));
  }
  return len;
}


/////////////////////////////////////////////////////////////////
// Solution Functions
/////////////////////////////////////////////////////////////////

int main(void) {
  configureFlash();
  configureClock();

  gpioEnable(GPIO_PORT_A);
  gpioEnable(GPIO_PORT_B);
  gpioEnable(GPIO_PORT_C);

  pinMode(PB3, GPIO_OUTPUT); // LED pin
  
  RCC->APB2ENR |= (RCC_APB2ENR_TIM15EN);
  initTIM(TIM15);
  
  USART_TypeDef * USART = initUSART(USART1_ID, 125000);

  // TODO: Add SPI initialization code
  // 1. write proper GPIO registers: configure GPIO for MOSI, MISO, CLK
  //MISO = PA6, MOSI = PA12, CLK = PA5 (see datasheet pg. 263)
  pinMode(PA8, GPIO_ALT); // chip select as PA12 (PA11 sucks)
  pinMode(PA6, GPIO_ALT);  // MISO
  pinMode(PA12, GPIO_ALT); // MOSI
  pinMode(PA5, GPIO_ALT); // CLK

  // set to proper alternate function
  GPIOA->AFR[0] |= (0b0101 << GPIO_AFRL_AFSEL5_Pos); // PA5 set to AF5 to be SPI1_SCK
  GPIOA->AFR[1] |= (0b0101 << GPIO_AFRL_AFSEL12_Pos); // PA12 set to AF5 to be SPI1_MOSI
  GPIOA->AFR[0] |= (0b0101 << GPIO_AFRL_AFSEL6_Pos); // PA6 set to to AF5 be SPI1_MISO 


  initSPI(br, cpol, cpha); // call SPI initialization

  
  while(1) {
    /* Wait for ESP8266 to send a request.
    Requests take the form of '/REQ:<tag>\n', with TAG begin <= 10 characters.
    Therefore the request[] array must be able to contain 18 characters.
    */

    // Receive web request from the ESP
    char request[BUFF_LEN] = "                  "; // initialize to known value
    int charIndex = 0;
  
    // Keep going until you get end of line character
    while(inString(request, "\n") == -1) {
      // Wait for a complete request to be transmitted before processing
      while(!(USART->ISR & USART_ISR_RXNE));
      request[charIndex++] = readChar(USART);
    }

    ///////////////////////////////////////////
    //// SPI CODE
    ///////////////////////////////////////////
   

    digitalWrite(PA8, PIO_HIGH); //turn on Chip Enable

    // send config bits [1,1,1,1shot, r1, r2, r3, SD] to 80h
    spiSendReceive(0x80); 
    // TODO: will have to change to make adjustable
    spiSendReceive(0b11100000); // 1shot = 0 for cont. temp readings, r1,2,3 = 000 sets 8-bit resolution, SD = 0

    // toggle chip enable
    //digitalWrite(PA8, PIO_LOW);
    //digitalWrite(PA8, PIO_HIGH);

    // interface with the temp sensor for lsb
    spiSendReceive(0x01); // Send addr to read temp LSB
    templsb = spiSendReceive(0x00); // recieve temp LSB

    // toggle chip enable
    //digitalWrite(PA8, PIO_LOW);
    //digitalWrite(PA8, PIO_HIGH);

    // interface with the temp sensor for msb
    spiSendReceive(0x02); //sen addr to read temp MSB
    tempmsb = spiSendReceive(0x00); // recieve temp MSB

    // toggle chip enable
    //digitalWrite(PA8, PIO_LOW);
    //digitalWrite(PA8, PIO_HIGH);


    digitalWrite(PA8, PIO_LOW); // turn off chip enable
    delay_millis(TIM15, 100); // delay before next read


    printf("msb: %d [rev/s]\n", tempmsb);
    printf("lsb: %d [rev/s]\n", templsb);


    //decode temperature w/ lsb and msb
    uint16_t temp = (tempmsb << 8) | templsb; //combine temperatures together
    float temperature = temp * 0.0625; // convert to float, based on data sheet

    printf("lsb: %f [rev/s]\n", temperature);





    ///////////////////////////////////////////////////////////////
    // Update string with current LED state
 
    int led_status = updateLEDStatus(request);

    char ledStatusStr[20];
    if (led_status == 1)
      sprintf(ledStatusStr,"LED is on!");
    else if (led_status == 0)
      sprintf(ledStatusStr,"LED is off!");

    // finally, transmit the webpage over UART
    sendString(USART, webpageStart); // webpage header code
    sendString(USART, ledStr); // button for controlling LED

    sendString(USART, "<h2>LED Status</h2>");

    sendString(USART, "<p>");
    sendString(USART, ledStatusStr);
    sendString(USART, "</p>");
  
    sendString(USART, webpageEnd);
  }
}

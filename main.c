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
#include "stm32l432xx.h"


//Defining the web page in two chunks: everything before the current time, and everything after the current time
char* webpageStart = "<!DOCTYPE html><html><head><title>E155 Web Server Demo Webpage</title>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
	</head>\
	<body><h1>E155 Web Server Demo Webpage</h1>";
char* ledStr = "<p>LED Control:</p><form action=\"ledon\"><input type=\"submit\" value=\"Turn the LED on!\"></form>\
	<form action=\"ledoff\"><input type=\"submit\" value=\"Turn the LED off!\"></form>";
char* webpageEnd   = "</body></html>";

char* tempfunc = "<p>Select Temperature Resolution:</p>\
  <form action=\"8bit\"><input type=\"submit\" value=\"8-bit Resolution\"></form>\
  <form action=\"9bit\"><input type=\"submit\" value=\"9-bit Resolution\"></form>\
  <form action=\"10bit\"><input type=\"submit\" value=\"10-bit Resolution\"></form>\
  <form action=\"11bit\"><input type=\"submit\" value=\"11-bit Resolution\"></form>\
  <form action=\"12bit\"><input type=\"submit\" value=\"12-bit Resolution\"></form>";


// Set global variables
int br = 200000;  // 010 - 200000
int cpol = 0;
int cpha = 1;

int temp_resolution = 0b11100000;

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

//determines temperatue resolution needed 
int updateTempResolution(char request[]){
  //int temp_resolution = 0b11100000; // default in 8 bit

  if (inString(request, "8bit")== 1) {
		temp_resolution = 0b11100000;
	} else if (inString(request, "9bit")== 1) {
		temp_resolution = 0b11100010;
	} else if (inString(request, "10bit")== 1) {
		temp_resolution = 0b11100100;
	} else if (inString(request, "11bit")== 1) {
		temp_resolution = 0b11100110;
	} else if (inString(request, "12bit")== 1) {
		temp_resolution = 0b11101000;
	}

	return temp_resolution;
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
  pinMode(PA8, GPIO_OUTPUT); // chip select as PA8` (PA11 sucks) TODO: MAKE TO OUTPUT
  
  RCC->APB2ENR |= (RCC_APB2ENR_TIM15EN);
  initTIM(TIM15);
  
  USART_TypeDef * USART = initUSART(USART1_ID, 125000);

  // TODO: Add SPI initialization code
  // 1. write proper GPIO registers: configure GPIO for MOSI, MISO, CLK
  //MISO = PA6, MOSI = PA12, CLK = PA5 (see datasheet pg. 263)
  
  pinMode(PA6, GPIO_ALT);  // MISO
  pinMode(PA12, GPIO_ALT); // MOSI
  pinMode(PA5, GPIO_ALT); // CLK

  // set to proper alternate function
  GPIOA->AFR[0] |= (0b0101 << GPIO_AFRL_AFSEL5_Pos); // PA5 set to AF5 to be SPI1_SCK
  GPIOA->AFR[1] |= (0b0101 << GPIO_AFRH_AFSEL12_Pos); // PA12 set to AF5 to be SPI1_MOSI
  GPIOA->AFR[0] |= (0b0101 << GPIO_AFRL_AFSEL6_Pos); // PA6 set to to AF5 be SPI1_MISO 

  
  initSPI(br, cpol, cpha); // call SPI initialization

  while(1) {
     printf("starting \n");
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

    int res = updateTempResolution(request);

    digitalWrite(PA8, PIO_HIGH); //turn on Chip Enable
    
    // send config bits [1,1,1,1shot, r1, r2, r3, SD] to 80h
    spiSendReceive(0x80); 

    // TODO: will have to change to make adjustable
    spiSendReceive(res); // 1shot = 0 for cont. temp readings, r1,2,3 = 000 sets 8-bit resolution, SD = 0
   
    // toggle chip enable
    digitalWrite(PA8, PIO_LOW);
    digitalWrite(PA8, PIO_HIGH);
    
    // interface with the temp sensor for msb
    spiSendReceive(0x02); //sen addr to read temp MSB
    char tempmsb = spiSendReceive(0x00); // recieve temp MSB
   
    // toggle chip enable
    digitalWrite(PA8, PIO_LOW);
    digitalWrite(PA8, PIO_HIGH);

    // interface with the temp sensor for lsb
    spiSendReceive(0x01); // Send addr to read temp LSB
    int templsb = spiSendReceive(0x00); // recieve temp LSB
   
    // toggle chip enable
    digitalWrite(PA8, PIO_LOW);// turn off chip enable

    delay_millis(TIM15, 120); // delay before next read

    //printf("resol: %d \n", res);
    //printf("msb: %d \n", tempmsb); // sign bit
    //printf("lsb: %d \n", templsb);


    //float temperature = tempmsb & 0b01111111;
   // printf("temperaturemsb: %f \n", temperature);

    // create mask for sign bit
    float temperature;// 
    int sign = (tempmsb & 0b10000000); //~(0b1<<7)); //0b10000000);

    if (!sign){ // positive
      temperature = tempmsb & 0b01111111;
      // add precisions:
      if(1 << 7 & templsb) temperature += 0.5;
      if(1 << 6 & templsb) temperature += 0.25;
      if(1 << 5 & templsb) temperature += 0.125;
      if(1 << 4 & templsb) temperature += 0.0625; 
    } else { // negative
      temperature = -128 + (tempmsb & 0b01111111); // -(~tempmsb+1);
      if(1 << 7 & templsb) temperature -= 0.5;
      if(1 << 6 & templsb) temperature -= 0.25;
      if(1 << 5 & templsb) temperature -= 0.125;
      if(1 << 4 & templsb) temperature -= 0.0625; 

    }

    //printf("temp: %d \n", temp);
    //printf("temperature: %f \n", temperature);




    ///////////////////////////////////////////////////////////////
    // Update string with current LED state
 
    int led_status = updateLEDStatus(request);

    char ledStatusStr[20];
    if (led_status == 1)
      sprintf(ledStatusStr,"LED is on!");
    else if (led_status == 0)
      sprintf(ledStatusStr,"LED is off!");


    char temperatureStr[20];
    sprintf(temperatureStr, "%f ", temperature);


    // finally, transmit the webpage over UART
    sendString(USART, webpageStart); // webpage header code
    sendString(USART, ledStr); // button for controlling LED

    sendString(USART, "<h2>LED Status</h2>");

    sendString(USART, "<p>");
    sendString(USART, ledStatusStr);
    sendString(USART, "</p>");
  
    sendString(USART, "<h2>Temperature Resoultion control </h2>");

    sendString(USART, tempfunc);

    sendString(USART, "<h2>Temperature:</h2>");
    sendString(USART, "</p>");
    sendString(USART, temperatureStr);
    sendString(USART, "<p>Degrees Celcius</p>");
    sendString(USART, "</p>");

    sendString(USART, webpageEnd);

  }
}

/*
 * esp8266_tools.c
 *
 *  Created on: Mar 1, 2025
 *      Author: daoch
 */

#include "esp8266_tools.h"
#include <string.h>
#include <cmsis_os2.h>
#include "uart.h"

uint8_t* esp_RxBuffer = NULL;

void ESP8266Init(void) {
	PRINT_UART("sent command AT\r\n");
    COMMAND("AT+RST");
    osDelay(10000);
    processResponse();
    COMMAND("AT+CWJAP=\"QK Apartment 5_301\",\"0919455888\"");
    osDelay(10000);
    processResponse();
    COMMAND("ATE0"); //Disable Echo
    osDelay(5000);
    processResponse();
    PRINT_UART("Done init esp8266\r\n");
}


void processResponse(void) {
	esp_RxBuffer = get_dataBuffer("usart2");
    PRINT_UART("data buffer: %s\r\n",esp_RxBuffer);
}


void ping() {
	COMMAND("AT");
	osDelay(3000);


}

/*
 * http_services.c
 *
 *  Created on: Mar 1, 2025
 *      Author: daoch
 */


#include <stdio.h>
#include <stdbool.h>
#include <cmsis_os2.h>
#include <string.h>
#include "uart.h"
#include "esp8266_tools.h"
#include "http_services.h"

bool esp_command = false;
uint8_t* esp_dataBuffer = NULL;

void CreateHttpThread(void) {
	// httpSerivce Thread
	osThreadId_t httpSerivce;
	const osThreadAttr_t httpSerivce_attributes ={
	.name = "httpSerivce",
	.stack_size = 512 * 4,
	.priority = (osPriority_t) osPriorityNormal4,
	};

	httpSerivce = osThreadNew(startHttpService, NULL, &httpSerivce_attributes);
	if (NULL == httpSerivce) PRINT_UART("httpSerivce Thread is failed to be created\r\n");
	else PRINT_UART("httpSerivce Thread created successfully\r\n");
}


void startHttpService(void *argument) {
	PRINT_UART("httpSerivce Thread run\r\n");
	ESP8266Init();
	osDelay(1000);

	for (;;) {
		ping();
		esp_command = get_uartCommandReceived("usart2");
		if (esp_command) {
			PRINT_UART("response receive!\r\n");
			esp_command = false; // Reset flag
			set_uartCommandReceived("usart2", esp_command);
			esp_dataBuffer = get_dataBuffer("usart2");
			PRINT_UART("%s\r\n",esp_dataBuffer); //, strlen((char *)esp_dataBuffer)
		}
		osDelay(20);
	}
}

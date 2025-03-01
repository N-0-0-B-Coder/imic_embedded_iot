/*
 * uart_control.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */



#include <stdio.h>
#include <stdbool.h>
#include <cmsis_os2.h>
#include <string.h>
#include "uart_control.h"
#include "button.h"
#include "uart.h"

bool command = false;
uint8_t* dataBuffer = NULL;

void CreateUARTLedControlThread(void) {
  // uartLedControl Thread
  osThreadId_t uartLedControl;
  const osThreadAttr_t uartLedControl_attributes ={
	.name = "uartLedControl",
	.stack_size = 512 * 4,
	.priority = (osPriority_t) osPriorityNormal,
  };

  uartLedControl = osThreadNew(startUARTLedControl, NULL, &uartLedControl_attributes);
  if (NULL == uartLedControl) PRINT_UART("uartLedControl Thread is failed to be created\r\n");
  else PRINT_UART("uartLedControl Thread created successfully\r\n");
}
void startUARTLedControl(void *argument) {
	PRINT_UART("uartLedControl Thread run \r\n");
	osDelay(1);
	for (;;) {
		command = get_uartCommandReceived("uart4");
		if (command) {
			command = false;  // Clear flag
			set_uartCommandReceived("uart4", command);
			dataBuffer = get_dataBuffer("uart4");

			PRINT_UART("dataBuffer: %s, string length: %d\r\n", dataBuffer, strlen((char *)dataBuffer));

			if (strcmp((char*)dataBuffer, "led on") == 0) {
				//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);  // Turn LED ON
				PRINT_UART("LED is ON\r\n");
				set_extiToggle(true);
			} else if (strcmp((char*)dataBuffer, "led off") == 0) {
				//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // Turn LED OFF
				PRINT_UART("LED is OFF\r\n");
				set_extiToggle(false);
			} else {
				PRINT_UART("Invalid Command\r\n");
			}
		}
		osDelay(20);
	}
}

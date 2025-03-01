/*
 * uart.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <cmsis_os2.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#include "uart.h"

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart2;

uint8_t uart4_RxBuffer[UART_BUFFER_SIZE];  // Buffer for received string
uint8_t uart4_RxIndex = 0;  // Current position in buffer
uint8_t uart4_RxData;  // Temporary byte storage

uint8_t uart2_RxBuffer[UART_BUFFER_SIZE];  // Buffer for received string
uint8_t uart2_RxIndex = 0;  // Current position in buffer
uint8_t uart2_RxData;  // Temporary byte storage


bool uart4_CommandReceived = false;  // Flag when a command is ready
bool uart2_CommandReceived = false;

void print_uart(const char *format, ...) {
    char _buffer[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(_buffer, sizeof(_buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart4, (uint8_t *)_buffer, strlen(_buffer), 1000);
    osDelay(10);
}

void send_command(const char *format, ...) {
    char _buffer[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(_buffer, sizeof(_buffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart2, (uint8_t *)_buffer, strlen(_buffer), 1000);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 1000);
    osDelay(10);
}

void MX_UART4_Init(void)
{
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    while(1){}
  }
}

void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
	while(1){}
  }

}

void StartUARTReceive_IT() {
    HAL_UART_Receive_IT(&huart4, &uart4_RxData, 1);  // Receive one byte at a time
    HAL_UART_Receive_IT(&huart2, &uart2_RxData, 1);  // Receive one byte at a time

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == UART4) {  // Ensure it's UART4
		if (uart4_RxData == '\n' || uart4_RxData == '\r') {  // End of command
			uart4_RxBuffer[uart4_RxIndex] = '\0';  // Null-terminate the string
			uart4_RxIndex = 0;  // Reset buffer index
			uart4_CommandReceived = true;  // Set flag to process command
		} else {
			if (uart4_RxIndex < UART_BUFFER_SIZE - 1) {
				uart4_RxBuffer[uart4_RxIndex++] = uart4_RxData;	// Store received char
			} else uart4_RxIndex = 0;
		}
		HAL_UART_Receive_IT(&huart4, &uart4_RxData, 1);  // Restart reception
	} else if (huart->Instance == USART2) {
        if (uart2_RxData == '\n' || uart2_RxData == '\r') { // End of response
            uart2_RxBuffer[uart2_RxIndex] = '\0'; // Null-terminate string
            uart2_RxIndex = 0; // Reset buffer index
            uart2_CommandReceived = true;
        } else {
            if (uart2_RxIndex < UART_BUFFER_SIZE - 1) {
            	uart2_RxBuffer[uart2_RxIndex++] = uart2_RxData;
            } else uart2_RxIndex = 0;
        }
        HAL_UART_Receive_IT(&huart2, &uart2_RxData, 1); // Restart UART Interrupt
    }
}

bool get_uartCommandReceived(char *uart) {
  if (strcmp(uart, "uart4") == 0)	return uart4_CommandReceived;
  if (strcmp(uart, "usart2") == 0)	return uart2_CommandReceived;
  return false;
}

void set_uartCommandReceived(char *uart, bool command) {
	if (strcmp(uart, "uart4") == 0) uart4_CommandReceived = command;
	if (strcmp(uart, "usart2") == 0) uart2_CommandReceived = command;

}

uint8_t* get_dataBuffer(char *uart) {
  if (strcmp(uart, "uart4") == 0) return uart4_RxBuffer;
  if (strcmp(uart, "usart2") == 0) return uart2_RxBuffer;
  return NULL;  // Return NULL if no match is found
}

void uartInit(void) {
	MX_UART4_Init();
	MX_USART2_UART_Init();
	StartUARTReceive_IT();
}

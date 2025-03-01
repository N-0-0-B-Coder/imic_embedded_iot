/*
 * uart.h
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */

#ifndef INC_INIT_UART_H_
#define INC_INIT_UART_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define PRINT_UART(f_, ...) print_uart(f_, ##__VA_ARGS__)
#define COMMAND(f_, ...) send_command(f_, ##__VA_ARGS__)
#define UART_BUFFER_SIZE 64

void uartInit(void);
void MX_UART4_Init(void);
void MX_USART2_UART_Init(void);
void print_uart(const char *format, ...);
void send_command(const char *format, ...);
void StartUARTReceive_IT();
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void set_uartCommandReceived(char *uart, bool command);
bool get_uartCommandReceived(char *uart);
uint8_t* get_dataBuffer(char *uart);

#endif /* INC_INIT_UART_H_ */

/*
 * button.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <stdio.h>
#include <stdbool.h>
#include <cmsis_os2.h>
#include "button.h"
#include "led_tools.h"
#include "registers_defs.h"
#include "uart.h"

static bool timer6Alarm = false;
static bool timer6LedToggle = false;
static bool timer7Alarm = false;
static bool timer7LedToggle = false;
static bool extiAlarmPA0 = false;
static bool extiButtonToggle = false;

void set_timer6Alarm(bool timer6Alarm_in) {
	timer6Alarm = timer6Alarm_in;
}
void set_timer7Alarm(bool timer7Alarm_in) {
	timer7Alarm = timer7Alarm_in;
}
void set_extiAlarmPA0(bool extiAlarmPA0_in) {
	extiAlarmPA0 = extiAlarmPA0_in;
}

void set_extiToggle(bool extiButtonToggle_in) {
	extiButtonToggle = extiButtonToggle_in;
}

void interferenceCheck(void) {
	if (registerBitCheck(REG_GPIO_A_IDR, BIT_0)) {
		osDelay(100);
		if (registerBitCheck(REG_GPIO_A_IDR, BIT_0)) {
			uint32_t count = 8000000; //count value for 1s based on 8Mhz frequency
			while (registerBitCheck(REG_GPIO_A_IDR, BIT_0) && count >= 0) {
				count--;
			}
			if (count >= 0) {
				PRINT_UART("Button is pressed\r\n");
				ledBlink(LED_ORANGE, 500);
				extiAlarmPA0 = false;
				extiButtonToggle ^= 1;
			}
		}
	}
	else extiAlarmPA0 = false;
}



void ButtonHandleTask(void *argument) {
	PRINT_UART("Button Handler Thread run\r\n");
	osDelay(1);
	for(;;) {
		if(extiAlarmPA0) {
			interferenceCheck();
		}


		osDelay(1);
	}
}

void ButtonExecuteTask(void *argument) {
	PRINT_UART("Button Execute Thread run\r\n");
	osDelay(1);
	for(;;) {
		if (extiButtonToggle) {

			if (timer6Alarm) {
				timer6Alarm = false;
				timer6LedToggle ^= 1;
				ledSet(LED_GREEN, timer6LedToggle);
			}

			if (timer7Alarm) {
				timer7Alarm = false;
				timer7LedToggle ^= 1;
				ledSet(LED_BLUE, timer7LedToggle);
			}
		}
		else {
			ledSet(LED_GREEN, 0);
			ledSet(LED_BLUE, 0);
		}
		osDelay(1);
	}
}



void CreateButtonThread(void) {
    // buttonHandle Thread
    osThreadId_t buttonHandle;
    const osThreadAttr_t buttonHandle_attributes = {
        .name = "buttonInterference",
        .stack_size = 768 * 4,
        .priority = (osPriority_t) osPriorityNormal3,
    };

    buttonHandle = osThreadNew(ButtonHandleTask, NULL, &buttonHandle_attributes);
    if (NULL == buttonHandle) PRINT_UART("buttonHandle Thread is failed to be created\r\n");
    else PRINT_UART("buttonHandle Thread created successfully\r\n");


    // buttonExecute Thread
    osThreadId_t buttonExecute;
    const osThreadAttr_t buttonExecute_attributes = {
        .name = "buttonExecution",
        .stack_size = 512 * 4,
        .priority = (osPriority_t) osPriorityNormal2,
    };

    buttonExecute = osThreadNew(ButtonExecuteTask, NULL, &buttonExecute_attributes);
    if (NULL == buttonExecute) PRINT_UART("buttonExecute Thread is failed to be created\r\n");
    else PRINT_UART("buttonExecute Thread created successfully\r\n");
}

/*
 * user_main_init.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <bus.h>
#include <gpios.h>
#include <i2c.h>
#include <interrupts.h>
#include <uart.h>
#include <timers_init.h>
#include "user_main_init.h"

void userInit(void) {
	busInit();
	gpiosInit();
	interruptsInit();
	i2cInit();
	timersInit();
	uartInit();
}

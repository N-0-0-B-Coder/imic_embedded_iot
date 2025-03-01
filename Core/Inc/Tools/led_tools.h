/*
 * led_tools.h
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */

#ifndef INC_TOOLS_LED_TOOLS_H_
#define INC_TOOLS_LED_TOOLS_H_

#include "stdint.h"
#include <stdbool.h>
#include "registers_tools.h"

#define LED_GREEN 	BIT_12
#define LED_ORANGE	BIT_13
#define LED_RED 	BIT_14
#define LED_BLUE 	BIT_15
#define LOCK_KEY	BIT_16

void ledSet(uint32_t led, bool state);
void ledBlink(uint32_t led, uint32_t delayms);

#endif /* INC_TOOLS_LED_TOOLS_H_ */

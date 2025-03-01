/*
 * gpios.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


//#include <stm32f407xx.h>
#include <gpios.h>
#include "registers_tools.h"
#include "registers_defs.h"
#include "led_tools.h"

void gpiosInit(void) {
	gpioRegisterSetup();
}

void gpioRegisterSetup(void) {

	/* START registers set up USER BUTTON PIN PA0 in PORT A */

	// For MODER, set mode 00 (General purpose input mode)
	registerBitClear(REG_GPIO_A_MODER, (BIT_0 | BIT_1));

	// For PUPDR, using mode 00: No Pull-up/ Pull-down
	registerBitClear(REG_GPIO_A_PUPDR, (BIT_0 | BIT_1));

	/* END registers set up USER BUTTON PIN 0  in PORT A */



	/* START registers set up PIN 1 in PORT A */

	// For MODER, set mode 01 (General purpose output mode)
	registerBitSet(REG_GPIO_A_MODER, BIT_2);
	registerBitClear(REG_GPIO_A_MODER, BIT_3);

	// For OTYPER, using mode 0: Output push-pull
	registerBitClear(REG_GPIO_A_OTYPER, BIT_1);

	// For OSPEEDR, using mode 00: Low speed
	registerBitClear(REG_GPIO_A_OSPEEDR, (BIT_2 | BIT_3));

	// For PUPDR, using mode 00: No Pull-up/ Pull-down
	registerBitClear(REG_GPIO_A_PUPDR, (BIT_2 | BIT_3));

	/* END registers set up PIN 1  in PORT A */



	/* START registers set up PIN 12, 13, 14, 15 in PORT D */

	// For MODER, set mode 01 (General purpose output mode)
	registerBitSet(REG_GPIO_D_MODER, (BIT_24 | BIT_26 | BIT_28 | BIT_30));
	registerBitClear(REG_GPIO_D_MODER, (BIT_25 | BIT_27 | BIT_29 | BIT_31));

	// For OTYPER, using mode 0: Output push-pull
	registerBitClear(REG_GPIO_D_OTYPER, LED_BLUE | LED_RED | LED_GREEN | LED_ORANGE);

	// For OSPEEDR, using mode 00: Low speed
	registerBitClear(REG_GPIO_D_OSPEEDR, (BIT_24 | BIT_25 | BIT_26 | BIT_27 | BIT_28 | BIT_29 | BIT_30 | BIT_31));

	// For PUPDR, using mode 00: No Pull-up/ Pull-down
	registerBitClear(REG_GPIO_D_PUPDR, (BIT_24 | BIT_25 | BIT_26 | BIT_27 | BIT_28 | BIT_29 | BIT_30 | BIT_31));

	/*
	 * For IDR, no input data
	 * Using BSRR instead of ODR for more safer and easier
	 * For AFRL and AFRH, no alternate function used, keep at AF0 (0000) at all pins
	 * */




	 /* END registers set up PIN 12, 13, 14, 15 in PORT D */



	 /* START registers set up PIN 12, 13, 14, 15 in PORT H */
	 /* END registers set up PIN 12, 13, 14, 15 in PORT H */
}

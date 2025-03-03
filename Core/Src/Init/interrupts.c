/*
 * interrupts.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <interrupts.h>
#include "registers_tools.h"
#include "registers_defs.h"

void interruptsInit(void) {
	interruptRegisterSetup();
}

void interruptRegisterSetup(void) {

	/* registers set up for Extend Interrupt in PORT A*/

	// Enable EXTI0 group for PA0 button PIN (Mode 0000) in SYSCFG external interrupt configuration register 1
	registerBitClear(REG_SYSCFG_EXTICR1, (BIT_0 | BIT_1 | BIT_2 | BIT_3));
	// Un-mask EXTI0 group in Interrupt mask register (EXTI_IMR) for enabling detecting interrupt
	registerBitSet(REG_EXTI_IMR, BIT_0);
	// Enable Rising trigger selection register (EXTI_RTSR) for rising edge at EXTI0 group
	registerBitSet(REG_EXTI_RTSR, BIT_0);
	// Disable Falling trigger selection register (EXTI_FTSR) for falling edge at EXTI0 group
	registerBitClear(REG_EXTI_FTSR, BIT_0);
	// Set Interrupt set-enable register 0 (NVIC_ISERx) enable EXTI0 (position 6 in NVIC table)
	//registerBitSet(REG_NVIC_ISER0, BIT_6);
}

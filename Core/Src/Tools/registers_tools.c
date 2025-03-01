/*
 * registers_tools.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "registers_tools.h"

uint32_t registerRead(uint32_t *registerAddress) {
	if (registerAddress == NULL) return 0;
	return *registerAddress;
}

uint8_t registerRead_8(uint8_t *registerAddress) {
	if (registerAddress == NULL) return 0;
	return *registerAddress;
}

uint16_t registerRead_16(uint16_t *registerAddress) {
	if (registerAddress == NULL) return 0;
	return *registerAddress;
}

void registerWrite(uint32_t *registerAddress, uint32_t value) {
	if (registerAddress == NULL) return;
	*registerAddress = value;
}

void registerWrite_8(uint8_t *registerAddress, uint8_t value) {
	if (registerAddress == NULL) return;
	*registerAddress = value;
}

void registerWrite_16(uint16_t *registerAddress, uint16_t value) {
	if (registerAddress == NULL) return;
	*registerAddress = value;
}

void registerBitSet(uint32_t *registerAddress, uint32_t mask) {
	if (registerAddress == NULL) return;
	uint32_t reg = registerRead(registerAddress);
	reg |= mask;
	registerWrite(registerAddress, reg);
}

void registerBitSet_8(uint8_t *registerAddress, uint8_t mask) {
	if (registerAddress == NULL) return;
	uint8_t reg = registerRead_8(registerAddress);
	reg |= mask;
	registerWrite_8(registerAddress, reg);
}

void registerBitSet_16(uint16_t *registerAddress, uint16_t mask) {
	if (registerAddress == NULL) return;
	uint16_t reg = registerRead_16(registerAddress);
	reg |= mask;
	registerWrite_16(registerAddress, reg);
}

void registerBitClear(uint32_t *registerAddress, uint32_t mask) {
	if (registerAddress == NULL) return;
	uint32_t reg = registerRead(registerAddress);
	reg &= ~mask;
	registerWrite(registerAddress, reg);
}

void registerBitClear_8(uint8_t *registerAddress, uint8_t mask) {
	if (registerAddress == NULL) return;
	uint8_t reg = registerRead_8(registerAddress);
	reg &= ~mask;
	registerWrite_8(registerAddress, reg);
}

void registerBitClear_16(uint16_t *registerAddress, uint16_t mask) {
	if (registerAddress == NULL) return;
	uint16_t reg = registerRead_16(registerAddress);
	reg &= ~mask;
	registerWrite_16(registerAddress, reg);
}


bool registerBitCheck(uint32_t *registerAddress, uint32_t mask) {
	if (registerAddress == NULL) return false;
	if (*registerAddress & mask) return true;
	else return false;
}

bool registerBitCheck_8(uint8_t *registerAddress, uint8_t mask) {
	if (registerAddress == NULL) return false;
	if (*registerAddress & mask) return true;
	else return false;
}

bool registerBitCheck_16(uint16_t *registerAddress, uint16_t mask) {
	if (registerAddress == NULL) return false;
	if (*registerAddress & mask) return true;
	else return false;
}

/*
 * i2c.h
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */

#ifndef INC_INIT_I2C_H_
#define INC_INIT_I2C_H_

#include <stdbool.h>
#include "stdint.h"



#define TIMEOUT_LIMIT 800000

void i2cInit(void);
void i2cPinSetup(void);
void i2cRegisterSetup(void);


bool i2cWrite(uint8_t slaveAddr, uint8_t regAddr, uint8_t data);
uint8_t i2cRead(uint8_t slaveAddr, uint8_t regAddr);
uint8_t BCDToDec(uint8_t BCD);
uint8_t DecToBCD(uint8_t Dec);

#endif /* INC_INIT_I2C_H_ */

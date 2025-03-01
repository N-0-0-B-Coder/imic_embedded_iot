/*
 * ds3231_tools.h
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */

#ifndef INC_TOOLS_DS3231_TOOLS_H_
#define INC_TOOLS_DS3231_TOOLS_H_

#include "stdint.h"

//DS3231 Slave address
#define DS3231_ADDRESS				0x68
//DS3231 Registers
#define DS3231_SECONDS  			0x00
#define DS3231_MINUTES  			0x01
#define DS3231_HOURS  				0x02
#define DS3231_DAY 					0x03
#define DS3231_DATE 				0x04
#define DS3231_CEN_MONTH 			0x05
#define DS3231_2LDIGI_YEAR 			0x06
#define DS3231_ALARM1_SECONDS 		0x07
#define DS3231_ALARM1_MINUTES 		0x08
#define DS3231_ALARM1_HOURS 		0x09
#define DS3231_ALARM1_DAY_DATE 		0x0A
#define DS3231_ALARM2_MINUTES 		0x0B
#define DS3231_ALARM2_HOURS 		0x0C
#define DS3231_ALARM2_DAY_DATE 		0x0D
#define DS3231_CONTROL 				0x0E
#define DS3231_CTL_STATUS 			0x0F
#define DS3231_AGING_OFFSET 		0x10
#define DS3231_TEMP_MSB 			0x11
#define DS3231_TEMP_LSB 			0x12


void DS3231Init(void);

void readDS3231Time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year, uint8_t *isPM);


#endif /* INC_TOOLS_DS3231_TOOLS_H_ */

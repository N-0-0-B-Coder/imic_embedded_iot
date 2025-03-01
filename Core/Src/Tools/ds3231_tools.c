/*
 * ds3231_tools.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <stdio.h>
#include <stdint.h>
#include "ds3231_tools.h"
#include "i2c.h"
#include "registers_tools.h"
#include "registers_defs.h"

#define DS3231_EOSC_BIT BIT_7

void DS3231Init(void) {
	uint8_t controlReg;

	// Disable the oscillator by setting the EOSC bit
	controlReg = i2cRead(DS3231_ADDRESS, DS3231_CONTROL);					// Check the Control register in DS3231
	controlReg |= DS3231_EOSC_BIT;													// Stop oscillator
	i2cWrite(DS3231_ADDRESS, DS3231_CONTROL, controlReg);					// Send the new setting to DS3231

	// Set up the RTC DS3231 for the first time
	i2cWrite(DS3231_ADDRESS, DS3231_SECONDS, DecToBCD(12)); 				// Seconds as 12 seconds
	i2cWrite(DS3231_ADDRESS, DS3231_MINUTES, DecToBCD(12)); 				// Minutes as 12 minutes
	i2cWrite(DS3231_ADDRESS, DS3231_HOURS, 0x72); 							// Hours as 12 hours, 12h format, PM
	i2cWrite(DS3231_ADDRESS, DS3231_DAY, 0x01);								// Week day as 1 (Sunday)
	i2cWrite(DS3231_ADDRESS, DS3231_DATE, DecToBCD(5));						// Date of month (5)
	i2cWrite(DS3231_ADDRESS, DS3231_CEN_MONTH, 0x01);						// Month (1 as January), Century (Bit 7 as 0 for 2000 - 2099)
	i2cWrite(DS3231_ADDRESS, DS3231_2LDIGI_YEAR, DecToBCD(25));				// Year (25 as 2 last digit)

    // Enable the oscillator by clearing the EOSC bit
    controlReg &= ~DS3231_EOSC_BIT;
    i2cWrite(DS3231_ADDRESS, DS3231_CONTROL, controlReg);
}


void readDS3231Time(uint8_t *seconds, uint8_t *minutes, uint8_t *hours, uint8_t *day, uint8_t *date, uint8_t *month, uint8_t *year, uint8_t *isPM) {

    // Read Seconds
    *seconds = BCDToDec(i2cRead(DS3231_ADDRESS, DS3231_SECONDS));	// Read n Transfer the BIN value, which is read from DS3231, to DEC

    // Read Minutes
    *minutes = BCDToDec(i2cRead(DS3231_ADDRESS, DS3231_MINUTES));

    // Read Hours (and interpret 12-hour/24-hour format)
    *hours = i2cRead(DS3231_ADDRESS, DS3231_HOURS);

    if ((*hours & 0x40) >> 6) { 								// Check 12-hour mode (Bit 6 is 1/0 for 12/24h mode))
        *isPM = (*hours & 0x20) >> 5; 						// Get AM/PM (Bit 5 is 0/1 for AM/PM)
        *hours = BCDToDec(*hours & 0x1F); 					// Mask bits to take only the hour value
    } else {
        *hours = BCDToDec(*hours & 0x3F); 					// 24-hour mode. Mask bits to take only the hour value
    }

    // Read the Week day
    *day = i2cRead(DS3231_ADDRESS, DS3231_DAY);
    *day = BCDToDec(*day);

    // Read the day of Month
    *date = i2cRead(DS3231_ADDRESS, DS3231_DATE);
    *date = BCDToDec(*date);

    // Read the month
    *month = i2cRead(DS3231_ADDRESS, DS3231_CEN_MONTH);
    *month = BCDToDec(*month & 0x1F); 						// Mask century bit, take only month value

    // Read the year
    *year = i2cRead(DS3231_ADDRESS, DS3231_2LDIGI_YEAR);
    *year = BCDToDec(*year);

}

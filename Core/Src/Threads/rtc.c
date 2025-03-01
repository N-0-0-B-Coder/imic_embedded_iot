/*
 * rtc.c
 *
 *  Created on: Feb 6, 2025
 *      Author: daoch
 */


#include <stdio.h>
#include <stdbool.h>
#include <cmsis_os2.h>
#include "rtc.h"
#include "uart.h"
#include "ds3231_tools.h"

void CreateRTCTimeThread(void) {
  // rtcTimeExecute Thread
  osThreadId_t rtcTimeExecute;
  const osThreadAttr_t rtcTimeExecute_attributes ={
	.name = "rtcTimeExecute",
	.stack_size = 512 * 4,
	.priority = (osPriority_t) osPriorityNormal1,
  };

  rtcTimeExecute = osThreadNew(startRTCTime, NULL, &rtcTimeExecute_attributes);
  if (NULL == rtcTimeExecute) PRINT_UART("rtcTimeSet Thread is failed to be created\r\n");
  else PRINT_UART("rtcTimeSet Thread created successfully\r\n");
}

void startRTCTime(void *argument) {
	PRINT_UART("RTC Time Thread run\r\n");
	osDelay(1);
	//char weekDay[8][9] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
	//char pm[2][2] = {"AM", "PM"};
	DS3231Init();

	// Infinite loop to read the data every 10s
	for (;;) {
		PRINT_UART("print DTS value\r\n");
		uint8_t seconds, minutes, hours, day, date, month, year, isPM;
		readDS3231Time(&seconds, &minutes, &hours, &day, &date, &month, &year, &isPM);
		osDelay(1);
		PRINT_UART("Time: %02d:%02d:%02d\r\n", hours, minutes, seconds);//pm[isPM]
		PRINT_UART("Date: %02d/%02d/20%02d, Day: %d\r\n\r\n", date, month, year, day); //weekDay[day]
		//PRINT_UART("Time: %02d:%02d:%02d %s\r\n", 12, 12, 12, "PM");
		//PRINT_UART("Date: %02d/%02d/20%02d, Day: %s\r\n", 12, 12, 2012, "Tue");
		osDelay(10000);
	}
}

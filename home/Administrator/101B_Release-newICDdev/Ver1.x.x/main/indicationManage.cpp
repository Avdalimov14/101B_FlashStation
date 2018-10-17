/*
 * indicationManage.cpp
 *
 *  Created on: Jul 11, 2018
 *      Author: albert
 */
#include "mainHeader.h"
#include "ioManage.h"
#include "indicationManage.h"

void ledHandleTask(void *pvParam)
{
	indicationStatus_t rxIndicationStatus = IND_RESET;
	indicationStatus_t lastIndicationStatus = IND_RESET;
	bool loopStatus = false;
	int itrCounter = 1;
	while(1) {
		if (xQueueReceive(indicationLedQueue, &rxIndicationStatus, 100 / portTICK_RATE_MS) || loopStatus) {
			switch (rxIndicationStatus)
			{
				case IND_RESET:
					turnRedLed(false);
					turnGreenLed(false);
					turnBlueLed(false);

					loopStatus = false;
					lastIndicationStatus = IND_RESET;
					break;
				case IND_BIT_SUCCESS:
					turnBlueLedForXmSec(2000);

					loopStatus = false;
					lastIndicationStatus = IND_BIT_SUCCESS;
					break;
				case IND_NTAG_SCAN_SUCCESS:
					turnBlueLedForXmSec(2000);
					vTaskDelay(100 / portTICK_RATE_MS);
//					loopStatus = true; // not changed because it should go back to last state so... loopStatus = loopStatus;
//					if (lastIndicationStatus != IND_NTAG_SCAN_SUCCESS)
					rxIndicationStatus = lastIndicationStatus;
//					lastIndicationStatus = IND_NTAG_SCAN_SUCCESS;
					break;
				case IND_SCAN_SUCESS:
					turnBlueLedForXmSec(2000);
					rxIndicationStatus = IND_MONITOR_CONNECTED;
					itrCounter = 9;

					loopStatus = true;
					lastIndicationStatus = IND_SCAN_SUCESS;
					break;
				case IND_SCAN_START:
					turnBlueLedForXmSec(500);
					vTaskDelay(500 / portTICK_RATE_MS);

					loopStatus = true;
					lastIndicationStatus = IND_SCAN_START;
					break;
				case IND_MONITOR_CONNECTED:
					if (itrCounter % 7 == 0){
						turnGreenLedForXmSec(250);
						itrCounter = 0;
					}
					itrCounter++;
					vTaskDelay(250 / portTICK_RATE_MS);

					loopStatus = true;
					lastIndicationStatus = IND_MONITOR_CONNECTED;
					break;
				case IND_SCAN_FAILED: // IND_MONITOR_DISCONNECTED
				case IND_MONITOR_DISCONNECTED:
					if (lastIndicationStatus == IND_MONITOR_CONNECTED)
						itrCounter = 4;
					if (itrCounter % 4 == 0){
						turnYellowColorForXmSec(250);
						turnBlueLedForXmSec(250);
						turnYellowColorForXmSec(250);
						turnBlueLedForXmSec(250);
						itrCounter = 0;
					}
					itrCounter++;
					vTaskDelay(250 / portTICK_RATE_MS);

					loopStatus = true;
					lastIndicationStatus = IND_MONITOR_DISCONNECTED;
					break;
				default:
					break;

			}
//			ESP_LOGI("DEBUGGG", " rxIndicationStatus is  %d", rxIndicationStatus);
//			vTaskDelay(100 / portTICK_RATE_MS);
		}
	}
}

void turnBlueLedForXmSec (int miliSeconds)
{
	turnBlueLed(1);
	vTaskDelay(miliSeconds / portTICK_RATE_MS);
	turnBlueLed(0);
}

void turnGreenLedForXmSec (int miliSeconds)
{
	turnGreenLed(1);
	vTaskDelay(miliSeconds / portTICK_RATE_MS);
	turnGreenLed(0);
}

void turnYellowColorForXmSec (int miliSeconds)
{
	turnYellowColor(1);
	vTaskDelay(miliSeconds / portTICK_RATE_MS);
	turnYellowColor(0);
}

void turnYellowColor(bool on)
{
	if (on) {
		ESP_LOGD("Yellow Color", "on");
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_R, 0);
		gpio_set_level(GPIO_OUTPUT_G, 0);
#else
		gpio_set_level(GPIO_OUTPUT_R, 1);
		gpio_set_level(GPIO_OUTPUT_G, 1);
#endif
	}
	else {
		ESP_LOGD("Yellow Color", "off");
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_R, 1);
		gpio_set_level(GPIO_OUTPUT_G, 1);
#else
		gpio_set_level(GPIO_OUTPUT_R, 0);
		gpio_set_level(GPIO_OUTPUT_G, 0);
#endif
	}
}

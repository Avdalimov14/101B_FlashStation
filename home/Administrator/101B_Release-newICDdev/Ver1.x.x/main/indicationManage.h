/*
 * indicationManage.h
 *
 *  Created on: Jul 11, 2018
 *      Author: albert
 */

#ifndef MAIN_INDICATIONMANAGE_H_
#define MAIN_INDICATIONMANAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	IND_RESET                = 0,       /*!< When register application id, the event comes */
	IND_BIT_SUCCESS,      			 /* Build-In self-Test!< When gatt client request read operation, the event comes */
	IND_SCAN_START,       /*!< When gatt client request write operation, the event comes */
	IND_SCAN_SUCESS,       /*!< When gatt client request execute write, the event comes */
	IND_SCAN_FAILED,
	IND_NTAG_SCAN_SUCCESS,
	IND_MONITOR_CONNECTED,
	IND_MONITOR_DISCONNECTED,
} indicationStatus_t;

void ledHandleTask(void *pvParam);
void turnBlueLedForXmSec (int miliSeconds);
void turnGreenLedForXmSec (int miliSeconds);
void turnYellowColorForXmSec (int miliSeconds);
void turnYellowColor(bool on);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_INDICATIONMANAGE_H_ */

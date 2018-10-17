/*
 * mainHeader.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_MAINHEADER_H_
#define MAIN_MAINHEADER_H_


//#include "SPI.h"
//#include "PN532_SPI.h"
//#include "PN532.h"
//#include "Arduino.h"
//#include "WString.h"
//#include "NfcAdapter.h"

#ifdef __cplusplus
extern "C" {
#endif


#include "sdkconfig.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include <string.h>
#include <stdbool.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "controller.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_system.h"

#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "esp_system.h"

// related to all files
typedef struct numericRecord_inst {      // members of structures need to be sorted in ascending order
	uint32_t data;
	uint16_t timePast;
	uint8_t type;
	//uint8_t len; // or size may be optional, consider to delete if not necessary
} numericRecord_inst;

typedef struct charRecord_inst {
	uint8_t data[20];
	uint16_t timePast;
	uint8_t type;
	//uint8_t len;
} charRecord_inst;

typedef enum {
	NUMERIC_RECORD_TYPE	= 0,
	CHAR_RECORD_TYPE	= 1,
}recordType_t;

typedef struct record_inst {
	union {

		charRecord_inst* charRecord;
		numericRecord_inst* numericRecord;
	} recordInfo;
	recordType_t recordType;
} record_inst;

const int NUMERIC_RECORD_SIZE = sizeof(numericRecord_inst);
const int CHAR_RECORD_SIZE = sizeof(charRecord_inst);
const int RECORD_SIZE = sizeof(record_inst);
#define NUMERIC_BLE_BUFF_SIZE 7
#define CHAR_BLE_BUFF_SIZE 23
#define MAC_SIZE 6

//#define ACTIVE_LOW_LEDS

// extern vars and global vars
extern uint32_t timePastMinutes;
extern RTC_DATA_ATTR uint32_t timePastSeconds;
extern uint32_t maxCurrentRecordListCounter;
extern record_inst* recordList;
extern uint32_t recordListCounter;
extern uint8_t bleNumericSendBuffer[NUMERIC_BLE_BUFF_SIZE];
extern uint8_t bleCharSendBuffer[CHAR_BLE_BUFF_SIZE];
extern TaskHandle_t nfcTaskHandle;
extern bool runNfcFunction;

// Global Defines
#define NVSLOG "NVS LOG"
#define TIMERLOG "Timer Log"
#define NFCLOG "NFC LOG"

#ifdef __cplusplus
}
#endif




#endif /* MAIN_MAINHEADER_H_ */

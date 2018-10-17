/*
 * mainHeader.h
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#ifndef MAIN_MAINHEADER_H_
#define MAIN_MAINHEADER_H_

//#define BoardV3
#define ACTIVE_LOW_LEDS
#define REAL_TIME_TEST_PROG
//#define OLD_INDICATIONS
#define REAL_BIOBEAT_DEVICE  // define is ignoring passTheNotify var
#define ZERO_FROM_BIOBEAT_TEST
//#define SECURE_BLE
#define MEDIC_SIM_OFF

#ifdef __cplusplus
extern "C" {
#endif

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
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "controller.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_system.h"

#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "esp_system.h"
#include <sys/time.h>
#include "sdkconfig.h"

#include "dbManage.h"
#include "timerManage.h"

#include "indicationManage.h"

// Global Defines
#define NVSLOG "NVS LOG"
#define TIMERLOG "Timer Log"
#define NFCLOG "NFC LOG"
#define INDLOG "Indication Log"
#define DBLOG "DB LOG"
#define MAC_SIZE 6
#define START_RSSI_VAL -54
#define GATTSC_TAG "GATTSC"

void start_scan(uint32_t duration);
void deinit_ble();
bool sendIndicationStatusToQueue(indicationStatus_t status);
char *esp_key_type_to_str(esp_ble_key_type_t key_type);
void show_bonded_devices(void);

extern uint8_t myDeviceBtMacAddrs[6];
extern bool gattsInitDone;// = false;
extern bool scanIsOn; // = false;
extern bool passTheNotify;// = true;
extern bool passMonitorMeasurements; // = true
extern bool openBleConnection;// = false;
extern int16_t rssiPassRange;// = START_RSSI_VAL; // got change on nfcRfFieldTask

extern xQueueHandle indicationLedQueue;
#ifdef __cplusplus
}
#endif

#endif /* MAIN_MAINHEADER_H_ */

/*
 * mainHeader.h
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#ifndef MAIN_MAINHEADER_H_
#define MAIN_MAINHEADER_H_

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


// Global Defines
#define NVSLOG "NVS LOG"
#define TIMERLOG "Timer Log"
#define NFCLOG "NFC LOG"
#define MAC_SIZE 6

#ifdef __cplusplus
}
#endif

#endif /* MAIN_MAINHEADER_H_ */

/*
 * bleManage.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_BLEMANAGE_H_
#define MAIN_BLEMANAGE_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_system.h"


//extern uint32_t timePastMinutes;
// BLE Defines

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_A_SERVICE_UUID        0x00FF//0x1805//0x00FF
#define REMOTE_A_NOTIFY_CHAR_UUID    0xFF01//0x2A39//0x2A37//0x2A0F//0x2A2B//0xFF01
#define REMOTE_B_SERVICE_UUID		 0x181C // User Data service --> NFC
#define REMOTE_B_NOTIFY_CHAR_UUID	 0x2A8A // First Name --> NFC decode/real name for demo
#define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1
#define INVALID_HANDLE   0

static char remote_device_name[2][32] = {"ESP_GATTS_BB_DEMO", "Galaxy Note5"};//"ESP_GATTS_DEMO";//"Galaxy Note5"; it was const

void connectToSmartphoneProcedure(char* name, uint8_t length);
bool connectToMeasurementDevice(uint8_t* measureDeviceBtMacAddrs);
void readFromMeasurementDevice();
//esp_err_t mac_ret = ESP_OK;

// BIOBEAT details
const uint8_t bb_serviceUUID[16] = {0x90, 0x02, 0x35, 0x88, 0xfc, 0x9b, 0x26, 0x9c, 0x49, 0x4c, 0x1f, 0x6b, 0xaa, 0xb9, 0x05, 0x29,};
const uint8_t bb_measurements_characteristicUUID[16] = {0xe7, 0x7e, 0x06, 0x9b, 0x9d, 0xf9, 0x0f, 0xb3, 0xf0, 0x4a, 0x75, 0x09, 0x84, 0x57, 0x0d, 0x22};
const uint8_t bb_profile_characteristicUUID[16] = {0xee, 0xde, 0x0c, 0x19, 0x39, 0x83, 0xed, 0xac, 0x57, 0x41, 0x25, 0x20, 0x06, 0x88, 0x13, 0x66};
const uint8_t bb_writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};

static bool conn_device_a   = false;
static bool conn_device_b   = false;

static bool get_service_a   = false;
static bool get_service_b   = false;

static bool Isconnecting    = false;
static bool stop_scan_done  = false;

static esp_gattc_char_elem_t  *char_elem_result_a   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result_a  = NULL;
static esp_gattc_char_elem_t  *char_elem_result_b   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result_b  = NULL;

void start_scan(void);

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_a_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_b_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
void init_ble();

bool getConnDeviceA();
bool getConnDeviceB();
void setConnDeviceA(bool status);
void setConnDeviceB(bool status);

static esp_bt_uuid_t remote_a_filter_service_uuid = {
//    .len = ESP_UUID_LEN_16,
	.len = ESP_UUID_LEN_128,
//    .uuid = {.uuid16 = REMOTE_A_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_a_filter_profile_char_uuid = {
    .len = ESP_UUID_LEN_128,
};

static esp_bt_uuid_t remote_a_filter_measurements_char_uuid = {
//    .len = ESP_UUID_LEN_16,
	.len = ESP_UUID_LEN_128,
//    .uuid = {.uuid16 = REMOTE_A_NOTIFY_CHAR_UUID,},
};

static esp_bt_uuid_t remote_b_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_B_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_b_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = REMOTE_B_NOTIFY_CHAR_UUID,},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
};



#ifdef __cplusplus
}
#endif

#endif /* MAIN_BLEMANAGE_H_ */

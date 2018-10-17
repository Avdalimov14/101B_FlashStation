/*
 * gattc_related.h
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */

#ifndef MAIN_GATTC_RELATED_H_
#define MAIN_GATTC_RELATED_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tag
#define GATTC_TAG "GATTC_DEMO"

// Declare functions
void init_gattc_ble();

// BioBeat's server related vars ---> related to my Gatt Client
static const uint8_t bb_serviceUUID[16] = {0x90, 0x02, 0x35, 0x88, 0xfc, 0x9b, 0x26, 0x9c, 0x49, 0x4c, 0x1f, 0x6b, 0xaa, 0xb9, 0x05, 0x29,};
static const uint8_t bb_characteristicUUID[16] = {0xe7, 0x7e, 0x06, 0x9b, 0x9d, 0xf9, 0x0f, 0xb3, 0xf0, 0x4a, 0x75, 0x09, 0x84, 0x57, 0x0d, 0x22};
static const uint8_t bb_profile_characteristicUUID[16] = {0xee, 0xde, 0x0c, 0x19, 0x39, 0x83, 0xed, 0xac, 0x57, 0x41, 0x25, 0x20, 0x06, 0x88, 0x13, 0x66};
static const uint8_t bb_writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};

// Gatt Client related defines
#define REMOTE_SERVICE_UUID        0x00FF // bb_serviceUUID
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01 // bb_characteristicUUID
#define GATTC_PROFILE_NUM      1
#define GATTC_PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0



// Gatt Client related defines
static const char remote_device_name[] = "ESP_GATTS_BB_DEMO"; // remote is BioBeat's device
extern uint8_t myDeviceBtMacAddrs[6];
static bool gattc_conn = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;

/* Declare static functions */
//static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_128,
};

static esp_bt_uuid_t remote_filter_profile_char_uuid = {
    .len = ESP_UUID_LEN_128,
};

static esp_bt_uuid_t remote_filter_measurements_uuid = {
    .len = ESP_UUID_LEN_128,
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
    uint16_t profile_char_handle;
    uint16_t measurements_char_handle;
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
extern struct gattc_profile_inst gattc_gl_profile_tab[GATTC_PROFILE_NUM];

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GATTC_RELATED_H_ */

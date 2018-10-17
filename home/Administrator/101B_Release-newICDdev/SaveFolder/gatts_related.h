/*
 * gatts_related.h
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */

#ifndef MAIN_GATTS_RELATED_H_
#define MAIN_GATTS_RELATED_H_


#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tag
#define GATTS_TAG "GATTS_DEMO"

// Declare functions
void init_gatts_ble();
void start_advertise();

///Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
//static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
//static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

// Gatt Server related defines
#define GATTS_SERVICE_UUID_A   0x181C // User Data servic
#define GATTS_NOTIFY_CHAR_UUID_A      0x2A8A // First Name
#define GATTS_WRITE_CHAR_UUID_A      0x2A90 // First Name
//#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     6

#define TEST_DEVICE_NAME            "ESP_GATTS_DEMO"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

#define GATTS_PROFILE_NUM 1
#define GATTS_PROFILE_A_APP_ID 0
//#define GATTS_PROFILE_B_APP_ID 1
#define MAX_CHAR_LENGTH 17

// Gatt Server related vars and declarations
extern bool gatts_conn;// = false;
extern bool indicate_enabled;// = false;

//static uint8_t notifyCharValue[23] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 91, 95, 100, 80, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 11};
extern uint8_t indicateCharValue[MAX_CHAR_LENGTH];
extern uint8_t writeCharValue[MAX_CHAR_LENGTH];
static esp_gatt_char_prop_t indicateCharProperty = 0;
static esp_gatt_char_prop_t writeCharProperty = 0;


static esp_attr_value_t gatts_indicateCharValue =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(indicateCharValue),
    .attr_value   = indicateCharValue,
};
static esp_attr_value_t gatts_writeCharValue =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(writeCharValue),
    .attr_value   = writeCharValue,
};

static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd
};
static uint8_t raw_scan_rsp_data[] = {
        0x0f, 0x09, 0x45, 0x53, 0x50, 0x5f, 0x47, 0x41, 0x54, 0x54, 0x53, 0x5f, 0x44,
        0x45, 0x4d, 0x4f
};
#else

static uint8_t adv_service_uuid128[16] = { //instead of 32
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
		0x29, 0x05, 0xb9, 0xaa, 0x6b, 0x1f, 0x4c, 0x49, 0x9c, 0x26, 0x9b, 0xfc, 0x88, 0x35, 0x02, 0x90,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
//    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL, //bb_serviceUUID,
    .service_uuid_len = 16, //was 32
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = NULL, // was 32
    .p_service_uuid = NULL, //adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params;// = {
//    .adv_int_min        = 0x20,
//    .adv_int_max        = 0x40,
//    .adv_type           = ADV_TYPE_IND,
//    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
////    .peer_addr            = NULL,
////    .peer_addr_type       = BLE_ADDR_TYPE_PUBLIC,
//    .channel_map        = ADV_CHNL_ALL,
//    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
//};

static char TEST_STRING[20];

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t indicate_char_handle;
    esp_bt_uuid_t indicate_char_uuid;
    esp_gatt_perm_t indicate_char_perm;
    esp_gatt_char_prop_t indicate_char_property;
    uint16_t indicate_char_descr_handle;
    esp_bt_uuid_t descr_uuid;
    uint16_t write_char_handle;
	esp_bt_uuid_t write_char_uuid;
	esp_gatt_perm_t write_char_perm;
	esp_gatt_char_prop_t write_char_property;
    uint8_t connected; // in order to count how much connected device to this profile
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
extern struct gatts_profile_inst gatts_gl_profile_tab[GATTS_PROFILE_NUM];
//static struct gatts_profile_inst gatts_gl_profile_tab[GATTS_PROFILE_NUM] = {
//    [GATTS_PROFILE_A_APP_ID] = {
//        .gatts_cb = gatts_profile_a_event_handler,
//        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
//		.connected = 0,
//    },
//    [PROFILE_B_APP_ID] = {
//        .gatts_cb = gatts_profile_b_event_handler,                   /* This demo does not implement, similar as profile A */
//        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
//    },
//};

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;
//static prepare_type_env_t b_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

void changeIndicateCharValue(record_inst* record);
void sendRecordListByIndicate();
void sendMeasurementsByIndicate();

#ifdef __cplusplus
}
#endif

#endif /* MAIN_GATTS_RELATED_H_ */

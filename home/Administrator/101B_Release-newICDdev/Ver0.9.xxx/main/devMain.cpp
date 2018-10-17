#include <string.h>
#include "SPI.h"
#include "PN532_SPI.h"
#include "PN532.h"
#include "NfcAdapter.h"
#include "Arduino.h"

extern "C" {
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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

//#define ACTIVE_LOW_LEDS
}


// Timer configurations section
extern "C" {

void haltProgram()
{
	vTaskSuspendAll();
	while(1);
}

typedef enum {
    MESUREMENT_TYPE_HEARTRATE		=	81,
	MESUREMENT_TYPE_BLOODPRESURE	=	82,
	MESUREMENT_TYPE_SPO2			=	83,
	MESUREMENT_TYPE_RESPRATE		=	84,
	MESUREMENT_TYPE_VAS				=	85,
} record_type_t;

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (60.6) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (1)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

#define TIMERLOG "Timer Log"

/*
 * A sample structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct {
    int type;  // the type of timer's event
    timer_group_t timer_group;
    timer_idx_t timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

//DRAM_ATTR timer_event_t evt; // was needed because of the assertaion failed bug ----> FIXED: Use ESP_LOG* function to plot on serial monitor

xQueueHandle timer_queue;
uint32_t timePastMinutes; //was static = 0;
bool passTheNotify = true;
RTC_DATA_ATTR uint32_t timePastSeconds; //was static = 0; // consider to typdef a struct timePast
#define NUM_OF_SEC_DEF 60

static void inline print_timer_counter(uint64_t counter_value);
void IRAM_ATTR timer_group0_isr(void *para);
static void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec);

void init_timers();

}

// Data Base management section
//uint64_t debugTimer = 0;

extern "C" {

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
	NUMERIC_RECORD_TYPE2 = 2, // experimental
}recordType_t;

typedef struct record_inst {
	union {

		charRecord_inst* charRecord;
		numericRecord_inst* numericRecord;
	} recordInfo;
	recordType_t recordType;
} record_inst;

#define NUMERIC_RECORD_SIZE sizeof(numericRecord_inst)
#define CHAR_RECORD_SIZE sizeof(charRecord_inst)
#define NUMERIC_BLE_BUFF_SIZE 7
#define CHAR_BLE_BUFF_SIZE 23

int MAX_RECORD_IN_NVS = 500;
numericRecord_inst* numericRecordList = (numericRecord_inst*)malloc(100 * NUMERIC_RECORD_SIZE);
uint32_t numericRecordListCounter;// = 0; was static
uint8_t bleNumericSendBuffer[NUMERIC_BLE_BUFF_SIZE];
uint8_t bleCharSendBuffer[CHAR_BLE_BUFF_SIZE];

numericRecord_inst makeNumericRecord (uint16_t timePast, uint8_t type, uint32_t data);
bool addToNumericRecordList (numericRecord_inst record);
void printNumericRecordList();
void blePreSendNumericRecord();
void bleSendNumericRecord();
void bleSendNumericRecordList();

bool firstConnectToPhone = true;

// Array List using record_inst

static int RECORD_SIZE = sizeof(record_inst);
record_inst* recordList = (record_inst*)malloc(MAX_RECORD_IN_NVS * RECORD_SIZE);
record_inst bleMeasurementsStack[4];
static uint32_t recordListCounter = 0;
static uint32_t recordSendCoutner = 0;
static uint32_t mesurementSendCounter = 0;
static uint32_t stackSendCounter = 0;
static bool sendingRecordsToSmartphone = false;
static bool sendingMeasurementsToSmartphone = false;
static bool sendingStackToSmartphone = false;
static bool scanIsOn = false;

record_inst makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t data, uint8_t charData[]); // recordType -> 0-Numeric 1-Char
bool addToRecordList (record_inst record, bool writeToNvs, uint32_t recordNum);
void printRecordList();
bool checkRecordType(record_inst record);
recordType_t blePreSendRecord(record_inst record, int i);
void bleSendRecord(recordType_t recordType);
void bleSendRecordList();
void bleSendMeasurements();
void bleSendStack();
}

// BLE Defines

extern "C" {
#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_A_SERVICE_UUID        0x00FF//0x1805//0x00FF
#define REMOTE_A_NOTIFY_CHAR_UUID    0xFF01//0x2A39//0x2A37//0x2A0F//0x2A2B//0xFF01
#define REMOTE_B_SERVICE_UUID		 0x181C // User Data service --> NFC
#define REMOTE_B_NOTIFY_CHAR_UUID	 0x2A8A // First Name --> NFC decode/real name for demo
#define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1
#define INVALID_HANDLE   0

static char remote_device_name[][32] = {"ESP_GATTS_BB_DEMO", "Galaxy Note5"};//"ESP_GATTS_DEMO";//"Galaxy Note5"; it was const
static uint8_t measureDeviceBtMacAddrs[6] = {0,0,0,0,0,0};//{0x78, 0xB6, 0x90, 0x35, 0xB6, 0x98};
uint8_t myDeviceBtMacAddrs[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t smartphoneMacAddress[6] = {0};
const uint8_t emptyMacAddress[6] = {0,0,0,0,0,0};
const uint8_t espressifFirstMacBytes[3] = {0x30, 0xAE, 0xA4};
const uint8_t siliconLabsFirstMacBytes[3] = {0x00, 0x0B, 0x57};
//esp_err_t mac_ret = ESP_OK;

static bool open_conn_device_a = false;
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

/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_a_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void gattc_profile_b_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
void init_ble();
void deinit_ble();

// BIOBEAT details
const uint8_t bb_serviceUUID[16] = {0x90, 0x02, 0x35, 0x88, 0xfc, 0x9b, 0x26, 0x9c, 0x49, 0x4c, 0x1f, 0x6b, 0xaa, 0xb9, 0x05, 0x29,};
const uint8_t bb_measurements_characteristicUUID[16] = {0xe7, 0x7e, 0x06, 0x9b, 0x9d, 0xf9, 0x0f, 0xb3, 0xf0, 0x4a, 0x75, 0x09, 0x84, 0x57, 0x0d, 0x22};
const uint8_t bb_profile_characteristicUUID[16] = {0xee, 0xde, 0x0c, 0x19, 0x39, 0x83, 0xed, 0xac, 0x57, 0x41, 0x25, 0x20, 0x06, 0x88, 0x13, 0x66};
uint8_t bb_writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};

// new UUID of Smartphone (sp) GATT Server
//const uint8_t sp_serviceUUID[16] = {0x0c, 0xf2, 0xd4, 0x87, 0xaa, 0xf4, 0xa0, 0x8a, 0xb4, 0x4d, 0x9e, 0x37, 0xce, 0xc1, 0x2f, 0xb8};
//const uint8_t sp_writeCharUUID[16] = {0x65, 0x43, 0xd6, 0x1b, 0xd8, 0xea ,0xf3, 0x8c, 0x5d, 0x47, 0x53, 0x7c, 0x47, 0x13, 0x7c, 0x58};

static esp_bt_uuid_t remote_a_filter_service_uuid = {
    .len = ESP_UUID_LEN_128,
//    .uuid = {.uuid128 = REMOTE_A_SERVICE_UUID,},
};

static esp_bt_uuid_t remote_a_filter_profile_char_uuid = {
    .len = ESP_UUID_LEN_128,
//    .uuid = {.uuid16 = REMOTE_A_NOTIFY_CHAR_UUID,},
};

static esp_bt_uuid_t remote_a_filter_measurements_char_uuid = {
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

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_a_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
	[PROFILE_B_APP_ID] = {
		.gattc_cb = gattc_profile_b_event_handler,
		.gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
	},
};

static void start_scan(uint32_t duration)
{
    stop_scan_done = false;
    scanIsOn = true;
//    uint32_t duration = 2;
    esp_ble_gap_start_scanning(duration);
}
}

// NFC Defines

#define MAC_SIZE 6
PN532_SPI pn532spi(SPI, 5);
NfcAdapter nfc = NfcAdapter(pn532spi);

static int i_nfcScanResult = -1;

extern "C" {
	int app_main(void);

//}
#define NFC_DECODE_SIZE 2

typedef enum {
	    AIRWAY_MDC_EVT	=	22,       /*!< Airway event */
		INTBA_MDC_EVT	=	23,       /*!< Intubation event */
		KTMN_MDC_EVT	=	25,       /*!< Ketamin event */
		DRMCM_MDC_EVT	=	26,       /*!< Doromicum event */
		CNOTM_MDC_EVT	=	27,       /*!< Coniotomy event */
} medical_event_type_list_t;

uint8_t nfcScanResultString[20]; // temporary for the Demo to Kimat Champion Asaf Navot
void nfcEncodeToString(medical_event_type_list_t event);

#define NFCLOG "NFC LOG"

	// defines regards NFC HCE
bool checkIfHce();

// PWM Defines

#define LEDC_HS_TIMER         	LEDC_TIMER_0
#define LEDC_HS_MODE          	LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_RRGB2       (GPIO_NUM_33)
#define LEDC_HS_CH0_CHANNEL   	LEDC_CHANNEL_0
#define LEDC_HS_CH1_GRGB2       (GPIO_NUM_26)
#define LEDC_HS_CH1_CHANNEL		LEDC_CHANNEL_1
#define LEDC_HS_CH2_BRGB2 		(GPIO_NUM_32)
#define LEDC_HS_CH2_CHANNEL		LEDC_CHANNEL_2
#define LEDC_HS_CH3_BZZR		(GPIO_NUM_13)
#define LEDC_HS_CH3_CHANNEL		LEDC_CHANNEL_3

#define LEDC_TEST_CH_NUM       (4)
#define LEDC_FULL_DUTY			(8191)
#define LEDC_ZERO_DUTY         (0)
#define LEDC_TEST_FADE_TIME    (2000)

// PWM global variables
ledc_timer_config_t ledc_timer;
ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM];
char LedColor[3][6] = {"Red", "Green", "Blue"};

// GPIO Defines

//#define GPIO_OUTPUT_BZZR		GPIO_NUM_13
#define GPIO_OUTPUT_GRGB1   	GPIO_NUM_14//14
#define GPIO_OUTPUT_BRGB1   	GPIO_NUM_12//12
#define GPIO_OUTPUT_RRGB1   	GPIO_NUM_27//27
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_RRGB1) | (1ULL<<GPIO_OUTPUT_GRGB1) | (1ULL<<GPIO_OUTPUT_BRGB1))
#define GPIO_INPUT_IO_0     GPIO_NUM_0//0
#define GPIO_INPUT_IO_1     GPIO_NUM_4//4
#define GPIO_INPUT_IRQ		GPIO_NUM_35
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1) | (1ULL<<GPIO_INPUT_IRQ))
//#define ESP_INTR_FLAG_DEFAULT 0

// Declare of init functions
void init_gpio();
void init_pwm();
void beep();
#define BEEP beep()
void longBeep();

// Declare general functions
void waitUntilPress(gpio_num_t gpioNum);
void nfcHandle(void *pvParameters);
}

// NVS declare section
extern "C" {

#define NVSLOG "NVS LOG"
#define MAX_KEY_LEN 15
//#define MAX_RECORD_IN_NVS 400

nvs_handle storageNvsHandle;

void init_nvs() {
	// Initialize NVS
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
		// NVS partition was truncated and needs to be erased
		// Retry nvs_flash_init
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK( err );
	// Open
	ESP_LOGD(NVSLOG, "Opening Non-Volatile Storage (NVS) handle... ");

	err = nvs_open("storage", NVS_READWRITE, &storageNvsHandle);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Error (%d) opening NVS handle!\n", err);
	} else {
		ESP_LOGD(NVSLOG, "Done\n");
	}
}

void nvsTimeRead()
{
	esp_err_t err;

	// Read
	ESP_LOGI(NVSLOG, "Reading minutes counter from Flash memory (NVS) ... ");
//		int32_t minutes_counter = 0; // value will default to 0, if not set yet in NVS
	err = nvs_get_u32(storageNvsHandle, "minutes_counter", &timePastMinutes);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "Minutes counter = %d", timePastMinutes);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "The value is not initialized yet!	Value will default to 0 (minutes)");
			timePastMinutes = 0;
			timePastSeconds = 0;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}
}

void nvsReadRecordListCounter()
{
	esp_err_t err;

	// Read
	ESP_LOGI(NVSLOG, "Reading record list counter(recListCount) from NVS ... ");
//		int32_t minutes_counter = 0; // value will default to 0, if not set yet in NVS
	err = nvs_get_u32(storageNvsHandle, "recListCount", &recordListCounter);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "record list counter = %d", recordListCounter);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "record list counter is not initialized yet! Value will default to 0 (records)");
			recordListCounter = 0;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}

}

void nvsWriteRecordListCounter()
{
// Write
	esp_err_t err;
	if (recordListCounter < MAX_RECORD_IN_NVS) {
		ESP_LOGD(NVSLOG, "Updating record list counter(recListCount) in NVS ... ");
		err = nvs_set_u32(storageNvsHandle, "recListCount", recordListCounter);
		if (err != ESP_OK) {
			ESP_LOGE(NVSLOG, "Failed!");
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			ESP_ERROR_CHECK (err);
		}
		else
			ESP_LOGD(NVSLOG, "Done");

		// Commit written value.
		// After setting any values, nvs_commit() must be called to ensure changes are written
		// to flash storage. Implementations may write to storage at other times,
		// but this is not guaranteed.
		ESP_LOGD(NVSLOG, "Committing updates in NVS ... ");
		err = nvs_commit(storageNvsHandle);
		if (err != ESP_OK) {
			ESP_LOGE(NVSLOG, "Failed!");
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
		}
		else
			ESP_LOGD(NVSLOG, "Done");
	}

}

void nvsReadRecordList(uint32_t recordNum)
{
	esp_err_t err;

	if (recordNum < MAX_RECORD_IN_NVS) {

			// Read
		ESP_LOGD(NVSLOG, "Reading record %d from NVS ... ", recordNum);

		// generate keyName = record# (char array)
		char keyName[MAX_KEY_LEN] = {0};
		String keyNameString = "record" + recordNum;
		keyNameString.toCharArray(keyName, MAX_KEY_LEN);

		uint8_t tempStr[CHAR_BLE_BUFF_SIZE];
		size_t tempStrLen = CHAR_BLE_BUFF_SIZE;
		err = nvs_get_blob(storageNvsHandle, keyName, tempStr, &tempStrLen);
		switch (err) {
			case ESP_OK: {
				ESP_LOGD(NVSLOG, "Done");
				ESP_LOGD(NVSLOG, "record[%d] fetched from nvs memory:", recordNum);
				ESP_LOG_BUFFER_HEX_LEVEL(NVSLOG, tempStr, tempStrLen, ESP_LOG_DEBUG);

				record_inst record;
				uint16_t timePast_tmp = (tempStr[0] << 8) & 0xFF00;
				timePast_tmp |= tempStr[1];
				uint8_t type_tmp = tempStr[2];
				if (type_tmp == 0) {
					timePast_tmp = 2;
					type_tmp = 84;
					uint32_t data_tmp = 0;
					data_tmp = 29;
					record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
				}
				if (tempStr[2] == 0) { // need to add all characteristic types
					record = makeRecord(1, timePast_tmp, type_tmp, NULL, tempStr + 3);
				}
				else {
					uint32_t data_tmp = 0;
					for (int i = 0; i < NUMERIC_BLE_BUFF_SIZE - 3; i++)
						data_tmp |= tempStr[3 + i] << (24 - 8*i);
					record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
				}
				addToRecordList(record, false, recordNum);
	//			recordList[recordNum].recordInfo.charRecord->timePast = (tempStr[0] << 8) & tempStr[1];
	//			recordList[recordNum].recordInfo.charRecord->type = tempStr[2];
	//			if (tempStr[2] == 0) { // need to add all characteristic types
	//				recordList[recordNum].recordType = CHAR_RECORD_TYPE;
	//				for (int i = 0; i < CHAR_BLE_BUFF_SIZE - 3; i++)
	//					recordList[recordNum].recordInfo.charRecord->data[i] = tempStr[i + 3];
	//			}
	//			else {
	//				recordList[recordNum].recordType = NUMERIC_RECORD_TYPE;
	//				for (int i = 0; i < NUMERIC_BLE_BUFF_SIZE - 3; i++)
	//					recordList[recordNum].recordInfo.
	//			}
			}
				break;
			case ESP_ERR_NVS_NOT_FOUND: {
				ESP_LOGI(NVSLOG, "There is no record #%d", recordNum);
				ESP_LOGI(NVSLOG, "making false record in order to avoid crashing");
				record_inst record;
				uint16_t timePast_tmp;
				uint8_t type_tmp;
				timePast_tmp = 2;
				type_tmp = 84;
				uint32_t data_tmp = 0;
				data_tmp = 29;
				record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
				addToRecordList(record, false, recordNum);
				break;
			}
			default :
				ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
				break;
		}
	}
}

void nvsWriteRecordList(uint32_t recordNum)
{
// Write
	esp_err_t err;
	ESP_LOGD(NVSLOG, "Updating record %d in NVS ... ", recordNum);

	// generate keyName = record# (char array)
	char keyName[MAX_KEY_LEN] = {0};
	String keyNameString = "record" + recordNum;
	keyNameString.toCharArray(keyName, MAX_KEY_LEN);

	// generate tempStr = recordList[recordNum] to save blob in nvs
	if (recordList[recordNum].recordType == NUMERIC_RECORD_TYPE/* && recordNum < MAX_RECORD_IN_NVS*/) {
		uint8_t tempStr[NUMERIC_BLE_BUFF_SIZE];
		size_t tempStrLen = NUMERIC_BLE_BUFF_SIZE;

		tempStr[0] = (recordList[recordNum].recordInfo.numericRecord->timePast >> 8) & 0xFF;
		tempStr[1] = (recordList[recordNum].recordInfo.numericRecord->timePast) & 0xFF;
		tempStr[2] = (recordList[recordNum].recordInfo.numericRecord->type);
		for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
			tempStr[i] = ((recordList[recordNum].recordInfo.numericRecord->data) >> (24 - 8*(i-3))) & 0xFF;
			ESP_LOGD(NVSLOG, "temp[%d] = %X", i, tempStr[i]);
		}

		err = nvs_set_blob(storageNvsHandle, keyName, tempStr, tempStrLen);
		if (err != ESP_OK) {
			ESP_LOGE(NVSLOG, "Failed!");
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			ESP_ERROR_CHECK (err);
			if (err == 4357)
				haltProgram();
		}
		else
			ESP_LOGD(NVSLOG, "Done");
	}
	else /*if (recordNum < MAX_RECORD_IN_NVS)*/{
		uint8_t tempStr[CHAR_BLE_BUFF_SIZE];
		size_t tempStrLen = CHAR_BLE_BUFF_SIZE;

		tempStr[0] = (recordList[recordNum].recordInfo.charRecord->timePast >> 8) & 0xFF;
		tempStr[1] = (recordList[recordNum].recordInfo.charRecord->timePast) & 0xFF;
		tempStr[2] = (recordList[recordNum].recordInfo.charRecord->type);
		for (int i = 3; i < CHAR_BLE_BUFF_SIZE; i++)
			tempStr[i] = (recordList[recordNum].recordInfo.charRecord->data[i-3]);

		err = nvs_set_blob(storageNvsHandle, keyName, tempStr, tempStrLen);
		if (err != ESP_OK) {
			ESP_LOGE(NVSLOG, "Failed!");
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			ESP_ERROR_CHECK (err);
			if (err == 4357)
				haltProgram();
		}
		else
			ESP_LOGD(NVSLOG, "Done");
	}
//	else {
//		ESP_LOGI(NVSLOG, "THERE IS NO SPACE ANY MORE IN NVS!!!!!!");
//	}


	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	ESP_LOGD(NVSLOG, "Committing %s updates in NVS ... ", keyName);
	err = nvs_commit(storageNvsHandle);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//		ESP_ERROR_CHECK (err);
	}
	else
		ESP_LOGD(NVSLOG, "Done");

}

}

RTC_DATA_ATTR static time_t rtcLastFromResetInSeconds = 0;

void nfcFromSleep()
{
	ESP_LOGI(NFCLOG, "\nScan a NFC tag\n");
	char desString[50];

	if (nfc.tagPresent())
	{
		if (checkIfHce()) {    // peer to peer connection -> APDU
			uint8_t selectApdu[] = { 0x00, /* CLA */
									  0xA4, /* INS */
									  0x04, /* P1  */
									  0x00, /* P2  */
									  0x05, /* Length of AID  */
									  0xF2, 0x22, 0x22, 0x22, 0x22/* AID defined on Android App */
									  /*0x00   Le  */ };
			uint8_t responseLength = 32;
			uint8_t response[32];

			nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

			ESP_LOGD(NFCLOG, "I'm in HCE code.");
			ESP_LOGD(NFCLOG, "responseLength: %d", responseLength);
			ESP_LOGI(NFCLOG, "Phone detected from NFC reader.");

			esp_log_buffer_hex(NFCLOG, response, responseLength);
			if (response[2] == ':') {
				uint8_t tmpLow = 0, tmpHigh = 0;
				for (int i = 0, j = 0; i < responseLength && j < 6; i += 3) {
					if (response[i] <= 'f' && response[i] >= 'a') {
						tmpHigh =  0xA + response[i] - 'a';
					}
					else if (response[i] <= 'F' && response[i] >= 'A')
						tmpHigh = 0xA + response[i] - 'A';
					else if (response[i] <= '9' && response[i] >= '0')
						tmpHigh = response[i] - '0';

					if (response[i+1] <= 'f' && response[i+1] >= 'a') {
						tmpLow =  0xA + response[i+1] - 'a';
					}
					else if (response[i+1] <= 'F' && response[i+1] >= 'A')
						tmpLow = 0xA + response[i+1] - 'A';
					else if (response[i+1] <= '9' && response[i+1] >= '0')
						tmpLow = response[i+1] - '0';

					measureDeviceBtMacAddrs[j++] = (tmpHigh << 4) | (tmpLow & 0x0f);
				}

				ESP_LOGI(GATTC_TAG, "Trying to establish BLE connection. MAC address:");
				esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, sizeof(measureDeviceBtMacAddrs));
//					esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
			}

			strcpy((char*)remote_device_name[1], (char*)response);
			esp_log_buffer_hex(GATTC_TAG, remote_device_name[1], responseLength);
			remote_device_name[1][(int)responseLength-2] = 0;
			remote_device_name[1][(int)responseLength-1] = 0;
//				for (int i = 0; i < responseLength; i++)
//					remote_device_name[1][i] = (char)response[i];
			ESP_LOGD(GATTC_TAG, "Trying to establish BLE connection. Response Length: %d, Device name: %s", responseLength, remote_device_name[1]);
			ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");

//			start_scan(5);

		}

		else {
			  // Extract MAC Address or Medical Equipment/Process code to use
			NfcTag tag = nfc.read();
//			tag.getTagType().toCharArray(desString, 50); //      --> Do not use printf!
//			puts(desString);
//			puts("UID: ");
//			tag.getUidString().toCharArray(desString, 50);
//			puts(desString);

			if (tag.hasNdefMessage()) // every tag won't have a message
			{

			  NdefMessage message = tag.getNdefMessage();
			  ESP_LOGD(NFCLOG, "\nThis NFC Tag contains an NDEF Message with %d NDEF Records/s.", message.getRecordCount());
//				  printf("%d", message.getRecordCount());
//				  printf(" NDEF Record");
//				  if (message.getRecordCount() != 1) {
//					printf("s");
//				  }
//				  puts(".");

			  // cycle through the records, printing some info from each
			  int recordCount = message.getRecordCount();
			  for (int i = 0; i < recordCount; i++)
			  {
				  ESP_LOGD(NFCLOG, "NDEF Record %d", i+1);
				NdefRecord record = message.getRecord(i);
				// NdefRecord record = message[i]; // alternate syntax

				ESP_LOGD(NFCLOG, "  TNF: %d", record.getTnf());  // Tnf = Type Name Field. T - text, 2 - Bluetooth

				record.getType().toCharArray(desString, 50);
				ESP_LOGD(NFCLOG, "  Type: %s", desString);
//					puts(desString); // will be "" for TNF_EMPTY

				// The TNF and Type should be used to determine how your application processes the payload
				// There's no generic processing for the payload, it's returned as a byte[]
				int payloadLength = record.getPayloadLength();
				byte payload[payloadLength];
				record.getPayload(payload);

				// check for TNF: if TNF = 2 so save to mac address field
				if (record.getTnf() == 2) {
					for (int i = 0; i < MAC_SIZE; i++) {
						measureDeviceBtMacAddrs[i] = payload[payloadLength - i - 1];
//							printf("%2X:", measureDeviceBtMacAddrs[i]);
					}
//						printf("\n");
					esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE);
//					if (conn_device_a == false) {
//						conn_device_a = true;
//						ESP_LOGI(GATTC_TAG, "connect to the ESP BLE via NFC device.");
//						esp_ble_gap_stop_scanning();
//						esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
//					}
				}
				else if (record.getTnf() == 1 && record.getType() == "T") {
					char nfcScanResult[NFC_DECODE_SIZE];
					for (int szPos=0, i = 0; szPos < payloadLength; szPos++)
					{
						if (payload[szPos] >= 0x30 && payload[szPos] <= 0x38) {
							nfcScanResult[i] = payload[szPos];
							i++;
						}
					}
					i_nfcScanResult = (nfcScanResult[0] - '0') * 10 + (nfcScanResult[1] - '0'); // turned to be global -- may be there is better solution

					ESP_LOGI(NFCLOG, "the medical event decode is: %d", i_nfcScanResult);
					// no Encode to string it was for debugging
//						nfcEncodeToString((medical_event_type_list_t)i_nfcScanResult);
//						esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//														gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//														gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//														sizeof(nfcScanResultString),
//														i_nfcScanResult,
//														ESP_GATT_WRITE_TYPE_RSP,
//														ESP_GATT_AUTH_REQ_NONE);
					if (i_nfcScanResult == 2) {
						init_nvs();
						if (nvs_erase_all(storageNvsHandle) == ESP_OK) {
							nvs_commit(storageNvsHandle);
							nvs_flash_erase();
							rtcLastFromResetInSeconds = 0;
							ESP_LOGI(NFCLOG, "Flash Memory (NVS) is erased"); // need to handle timing bug after erasing and resatrt
							BEEP;
							BEEP;
							BEEP;
							esp_restart();
						}
						else
							ESP_LOGI(NFCLOG, "Flash Memory (NVS) erase error!");
					}
//					record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
//					addToRecordList(numRec, true, NULL); // addToNumericRecordList(numRec);
//					printRecordList(); // printNumericRecordList();
//
//					if (conn_device_b) {
//						bleSendRecord(blePreSendRecord(numRec));
//					}

				}
				// Print the Hex and Printable Characters
				ESP_LOGD(NFCLOG, "  Payload (HEX): ");
				esp_log_buffer_hex(NFCLOG, payload, payloadLength);

				// Force the data into a String (might work depending on the content)
				// Real code should use smarter processing
				String payloadAsString = "";
				for (int c = 0; c < payloadLength; c++) {
				  payloadAsString += (char)payload[c];
				}
				payloadAsString.toCharArray(desString, 50);
				ESP_LOGD(NFCLOG, "  Payload (as String): %s", desString);
//					payloadAsString.toCharArray(desString, 50);
//					puts(desString);
				// id is probably blank and will return ""
				String uid = record.getId();
				if (uid != "") {
					uid.toCharArray(desString, 50);
					ESP_LOGD(NFCLOG, "  ID: %s", desString);  //puts(desString);
				}
			  }
			}

			  }
		  BEEP;
	  }
}

timer_event_t evt;
void nfcHandle(void *pvParameters) {

	esp_err_t err;
        // Close
        //nvs_close(storageNvsHandle); No need to close
	uint8_t nfcReadAttempt = 0;
	// Read from nvs the value from key minutes_counter
	// Print the value and reset it

//	timer_event_t evt; //moved to be global
//	static int intCounter = 0;
    while (1) {
    	// Check if ISR sent Queue
    	ESP_LOGD(NFCLOG, "I'm checking if there ISR request..");
		if (xQueueReceive(timer_queue, &evt, 5)) {
//			intCounter = 0;
//		if (evt.timer_idx == 1)	{
			/* Print information that the timer reported an event */
			if (evt.type == TEST_WITHOUT_RELOAD) {
				ESP_LOGD(TIMERLOG, "\n    Example timer without reload");
			} else if (evt.type == TEST_WITH_RELOAD) {
				ESP_LOGD(TIMERLOG, "\n    Example timer with auto reload");

				ESP_LOGI(TIMERLOG, "it past %d minutes from last restart", ++timePastMinutes);
//				passTheNotify = true;      / -----------> every minute
				if (timePastMinutes % 2 == 0)
					passTheNotify = true;

				// check if need to fetch measures
//				if (timePastMinutes % 2 == 0 && (measureDeviceBtMacAddrs[0] != NULL || measureDeviceBtMacAddrs[1] != NULL)) { // was && conn_device_a
//					// past 2 minutes so need to sample BioBeat
////					esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
////												gl_profile_tab[PROFILE_A_APP_ID].conn_id,
////												gl_profile_tab[PROFILE_A_APP_ID].char_handle,
////												ESP_GATT_AUTH_REQ_NONE);
//					esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE); // from main
//					if (conn_device_a == false) {
//						conn_device_a = true;
//						ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");
//		//						esp_ble_gap_stop_scanning();
//						esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/,BLE_ADDR_TYPE_PUBLIC , true);
//					}
//					else {
//						esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
//												gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//												gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//												ESP_GATT_AUTH_REQ_NONE);
//					}
//				}

				if (recordListCounter < MAX_RECORD_IN_NVS) {
					// Write
					ESP_LOGD(NVSLOG, "Updating minutes counter in NVS ... ");
	//				timePastMinutes++;
					err = nvs_set_u32(storageNvsHandle, "minutes_counter", timePastMinutes);
//					err = nvs_commit(storageNvsHandle);
					if (err != ESP_OK) {
						ESP_LOGE(NVSLOG, "Failed!");
						ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
					}
					else
						ESP_LOGD(NVSLOG, "Done");

					// Commit written value.
					// After setting any values, nvs_commit() must be called to ensure changes are written
					// to flash storage. Implementations may write to storage at other times,
					// but this is not guaranteed.
					ESP_LOGD(NVSLOG, "Committing updates in NVS ... ");
					err = nvs_commit(storageNvsHandle);
					if (err != ESP_OK) {
						ESP_LOGE(NVSLOG, "Failed!");
						ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
					}
					else
						ESP_LOGD(NVSLOG, "Done");
				}
			} else {
				ESP_LOGD(TIMERLOG, "\n    UNKNOWN EVENT TYPE\n");
			}
			ESP_LOGD(TIMERLOG, "Group[%d], timer[%d] alarm event\n", evt.timer_group, evt.timer_idx);

			/* Print the timer values passed by event */
			ESP_LOGD(TIMERLOG, "------- EVENT TIME --------\n");
			print_timer_counter(evt.timer_counter_value);

			/* Print the timer values as visible by this task */
			ESP_LOGD(TIMERLOG, "-------- TASK TIME --------\n");
			uint64_t task_counter_value;
			timer_get_counter_value(evt.timer_group, evt.timer_idx, &task_counter_value);
			print_timer_counter(task_counter_value);

		}
		else {
			ESP_LOGD(NFCLOG, "there is no ISR request..\n");
//			intCounter++;
		}

    	// ReadTag
		ESP_LOGI(NFCLOG, "\nScan a NFC tag\n");
    	char desString[50];

    	if (nfc.tagPresent())
    	{
    		if (checkIfHce()) {    // peer to peer connection -> APDU
    			uint8_t selectApdu[] = { 0x00, /* CLA */
										  0xA4, /* INS */
										  0x04, /* P1  */
										  0x00, /* P2  */
										  0x06, /* Length of AID  */
										  0x00, 0x01, 0x02, 0x03, 0x04, 0x05/* AID defined on Android App */
										  /*0x00   Le  */ };
    			for (int apduIndex = 5; apduIndex < 11; apduIndex++)
    				selectApdu[apduIndex] = myDeviceBtMacAddrs[apduIndex - 5];

    			uint8_t responseLength = 32;
				uint8_t response[32];

				nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
				haltProgram();
				ESP_LOGD(NFCLOG, "I'm in HCE code.");
				ESP_LOGD(NFCLOG, "responseLength: %d", responseLength);

				ESP_LOG_BUFFER_HEX_LEVEL(NFCLOG, response, responseLength, ESP_LOG_DEBUG);
				if (response[2] == ':') {
					uint8_t tmpLow = 0, tmpHigh = 0;
					for (int i = 0, j = 0; i < responseLength && j < 6; i += 3) {
						if (response[i] <= 'f' && response[i] >= 'a') {
							tmpHigh =  0xA + response[i] - 'a';
						}
						else if (response[i] <= 'F' && response[i] >= 'A')
							tmpHigh = 0xA + response[i] - 'A';
						else if (response[i] <= '9' && response[i] >= '0')
							tmpHigh = response[i] - '0';

						if (response[i+1] <= 'f' && response[i+1] >= 'a') {
							tmpLow =  0xA + response[i+1] - 'a';
						}
						else if (response[i+1] <= 'F' && response[i+1] >= 'A')
							tmpLow = 0xA + response[i+1] - 'A';
						else if (response[i+1] <= '9' && response[i+1] >= '0')
							tmpLow = response[i+1] - '0';

						measureDeviceBtMacAddrs[j++] = (tmpHigh << 4) | (tmpLow & 0x0f);
					}

					ESP_LOGI(GATTC_TAG, "Trying to establish BLE connection. MAC address:");
					esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, sizeof(measureDeviceBtMacAddrs));
//					esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
				}

				strcpy((char*)remote_device_name[1], (char*)response);
				ESP_LOG_BUFFER_HEX_LEVEL(GATTC_TAG, remote_device_name[1], responseLength, ESP_LOG_DEBUG);
				remote_device_name[1][(int)responseLength-2] = 0;
				remote_device_name[1][(int)responseLength-1] = 0;
//				for (int i = 0; i < responseLength; i++)
//					remote_device_name[1][i] = (char)response[i];
				ESP_LOGD(GATTC_TAG, "Trying to establish BLE connection. Response Length: %d, Device name: %s", responseLength, remote_device_name[1]);
				ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");

				BEEP;

				// Pluster
				if (conn_device_a) {
					ESP_LOGI(GATTC_TAG, "Disconnecting from measurement device");
					esp_ble_gap_disconnect(measureDeviceBtMacAddrs);
					vTaskDelay(50 / portTICK_RATE_MS);
				// next pluster after sending record 99
				}
				if (conn_device_b) { // if another smartphone captured, I should disconnect from current
					ESP_LOGI(GATTC_TAG, "Disconnecting from smartphone device");
					esp_ble_gap_disconnect(smartphoneMacAddress);
					vTaskDelay(50 / portTICK_RATE_MS);
				}
				// start scan to connect smartphone
		        if (!scanIsOn) {
		        	start_scan(5);
		        }
		        else{
		        	esp_ble_gap_stop_scanning();
		        	vTaskDelay(50 / portTICK_RATE_MS);
		        	start_scan(5);
		        }


    		}

    		else {
				  // Extract MAC Address or Medical Equipment/Process code to use
				NfcTag tag = nfc.read();
//				tag.getTagType().toCharArray(desString, 50); //      --> Do not use printf!
//				puts(desString);
//				puts("UID: ");
//				tag.getUidString().toCharArray(desString, 50);
//				puts(desString);

				if (tag.hasNdefMessage()) // every tag won't have a message
				{

				  NdefMessage message = tag.getNdefMessage();
				  ESP_LOGD(NFCLOG, "\nThis NFC Tag contains an NDEF Message with %d NDEF Records/s.", message.getRecordCount());
//				  printf("%d", message.getRecordCount());
//				  printf(" NDEF Record");
//				  if (message.getRecordCount() != 1) {
//					printf("s");
//				  }
//				  puts(".");

				  // cycle through the records, printing some info from each
				  int recordCount = message.getRecordCount();
				  for (int i = 0; i < recordCount; i++)
				  {
					  ESP_LOGD(NFCLOG, "NDEF Record %d", i+1);
					NdefRecord record = message.getRecord(i);
					// NdefRecord record = message[i]; // alternate syntax

					ESP_LOGD(NFCLOG, "  TNF: %d", record.getTnf());  // Tnf = Type Name Field. T - text, 2 - Bluetooth

					record.getType().toCharArray(desString, 50);
					ESP_LOGD(NFCLOG, "  Type: %s", desString);
//					puts(desString); // will be "" for TNF_EMPTY

					// The TNF and Type should be used to determine how your application processes the payload
					// There's no generic processing for the payload, it's returned as a byte[]
					int payloadLength = record.getPayloadLength();
					byte payload[payloadLength];
					record.getPayload(payload);

					// check for TNF: if TNF = 2 so save to mac address field
					if (record.getTnf() == 2) {
						for (int i = 0; i < MAC_SIZE; i++) {
							measureDeviceBtMacAddrs[i] = payload[payloadLength - i - 1];
//							printf("%2X:", measureDeviceBtMacAddrs[i]);
						}
//						printf("\n");
						esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE);
						if (memcmp(measureDeviceBtMacAddrs, siliconLabsFirstMacBytes, MAC_SIZE / 2) == 0 ||
								memcmp(measureDeviceBtMacAddrs, espressifFirstMacBytes, MAC_SIZE / 2) == 0) {
							BEEP;
							if (conn_device_a == false) {
								conn_device_a = true;
								ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");  // --------> here I can make a record of connection if needed
		//						esp_ble_gap_stop_scanning();
								esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, BLE_ADDR_TYPE_PUBLIC, true);
							}
						}
					}
					else if (record.getTnf() == 1 && record.getType() == "T") {
						char nfcScanResult[NFC_DECODE_SIZE];
						for (int szPos=0, i = 0; szPos < payloadLength; szPos++)
						{
							if (payload[szPos] >= 0x30 && payload[szPos] <= 0x39) {
								nfcScanResult[i] = payload[szPos];
								ESP_LOGI(NFCLOG, "the medical event decode is: %d %d", payload[szPos], szPos);
								i++;
							}
							if (payload[szPos] >= 0x42 && payload[szPos] <= 0x44) {
								nfcScanResult[i] = payload[szPos];
								ESP_LOGI(NFCLOG, "the medical event decode is: %d", payload[szPos]);
								i++;
							}
						}

						int i_nfcScanResult = (nfcScanResult[0] - '0') * 10 + (nfcScanResult[1] - '0');
						ESP_LOGI(NFCLOG, "the medical event decode is: %d", i_nfcScanResult);
						// no Encode to string it was for debugging
//						nfcEncodeToString((medical_event_type_list_t)i_nfcScanResult);
//						esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//														gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//														gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//														sizeof(nfcScanResultString),
//														i_nfcScanResult,
//														ESP_GATT_WRITE_TYPE_RSP,
//														ESP_GATT_AUTH_REQ_NONE);
//						if (i_nfcScanResult == 2) { // ----------------------------->  old code of DDB
//							if (nvs_erase_all(storageNvsHandle) == ESP_OK) {
//								nvs_commit(storageNvsHandle);
//								nvs_flash_erase();
//								rtcLastFromResetInSeconds = 0;
//								ESP_LOGI(NVSLOG, "Flash memort (NVS) is erased");  // need to handle timing bug after erasing and resatrt
//								BEEP;
//								BEEP;
//								BEEP;
//								esp_restart();
//							}
//							else
//								ESP_LOGE(NVSLOG, "Flash memort (NVS) erase error!");
//						}
						if (strcmp(nfcScanResult, "DDB") == 0) {
							if (nvs_erase_all(storageNvsHandle) == ESP_OK) {
								nvs_commit(storageNvsHandle);
								rtcLastFromResetInSeconds = 0;
								ESP_LOGI(NVSLOG, "Flash memort (NVS) is erased");  // need to handle timing bug after erasing and resatrt
								BEEP;
								BEEP;
								BEEP;
								esp_restart();
							}
							else
								ESP_LOGE(NVSLOG, "Flash memort (NVS) erase error!");
						}
						if (i_nfcScanResult >= 20 && i_nfcScanResult < 60) {
							record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
							addToRecordList(numRec, true, NULL); // addToNumericRecordList(numRec);
							BEEP; // beep here because printing takes time
							printRecordList(); // printNumericRecordList();

							if (conn_device_b) {
								bleSendRecord(blePreSendRecord(numRec, recordListCounter - 1));
							}


						}
					}
					// Print the Hex and Printable Characters
					ESP_LOGD(NFCLOG, "  Payload (HEX): ");
					ESP_LOG_BUFFER_HEX_LEVEL(NFCLOG, payload, payloadLength, ESP_LOG_DEBUG);

					// Force the data into a String (might work depending on the content)
					// Real code should use smarter processing
					String payloadAsString = "";
					for (int c = 0; c < payloadLength; c++) {
					  payloadAsString += (char)payload[c];
					}
					payloadAsString.toCharArray(desString, 50);
					ESP_LOGD(NFCLOG, "  Payload (as String): %s", desString);
//					payloadAsString.toCharArray(desString, 50);
//					puts(desString);
					// id is probably blank and will return ""
					String uid = record.getId();
					if (uid != "") {
						uid.toCharArray(desString, 50);
						ESP_LOGD(NFCLOG, "  ID: %s", desString);  //puts(desString);
					}
				  }
				}
    	    }
//    	    // Sending Test example Debugging
//			  printf("For sending please keep the tag close");
//			  vTaskDelay(500);
//			  puts("...");
//
//			  if (nfc.tagPresent()) {
//
//				  uint64_t sendValue = 0;
//				  //char* numnum;
//				  //numnum = (char*)malloc(10*sizeof(char));
////				  strcpy(numnum, "Success!");
////				  test1.pData = numnum;
//				  sendValue += (test1.timePast << (8*(3))) | (test1.type << (8*2)) | (test1.data & (65535));
//				  printf ("0x%8X 0x%8X is sent!\n", (int)(sendValue >> 32), (int)sendValue);
//			  }
//			  BEEP; // BEEP Only when sucess to read something.
			  // need BAD_BEEP  and POS_BEEP

			  // nfc was read, reset nfcReadAttempt
			  nfcReadAttempt = 0;
    	  }
    	  //vTaskDelay(1000 / portTICK_RATE_MS);

//    	if (nfcReadAttempt++ >= 10 && !conn_device_b) { // after 3 attempts, go to sleep again
//
//    		if (nfc.initRfField())
//				ESP_LOGD(NFCLOG, "NFC RF Field is On");
//
//    		uint64_t sleepDuration = TIMER_INTERVAL1_SEC * 1000000; // sleepDuration is in us
//    		struct timeval now;
//    		gettimeofday(&now, 0);
//    		time_t fromLastSleep = now.tv_sec-rtcLastFromResetInSeconds;
//
//    		ESP_LOGD("RTC LOG", "(%lds since last reset, %lds since last sleep)\n",now.tv_sec,fromLastSleep);
//    		rtcLastFromResetInSeconds = now.tv_sec;
//
//    		timePastSeconds += fromLastSleep;
//    		if (timePastSeconds < TIMER_INTERVAL1_SEC) {
//    			sleepDuration -= (timePastSeconds * 1000000);
//    		}
//    		else
//    			sleepDuration -= ((timePastSeconds % TIMER_INTERVAL1_SEC) * 1000000);
//
//    		if (timePastSeconds >= TIMER_INTERVAL1_SEC) {
//    			timePastMinutes += timePastSeconds / TIMER_INTERVAL1_SEC;
//    			timePastSeconds %= TIMER_INTERVAL1_SEC;
//    		}
////    		else if (timePastSeconds >= 60) {
////    			timePastMinutes++;
////    			timePastSeconds %= 60;
////    		}
//
//    		ESP_LOGD(NVSLOG, "Updating minutes counter in NVS ... ");
//			err = nvs_set_u32(storageNvsHandle, "minutes_counter", timePastMinutes);
//			if (err != ESP_OK) {
//				ESP_LOGE(NVSLOG, "Failed!");
//				ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			}
//			else
//				ESP_LOGD(NVSLOG, "Done");
//
//			// Commit written value.
//			// After setting any values, nvs_commit() must be called to ensure changes are written
//			// to flash storage. Implementations may write to storage at other times,
//			// but this is not guaranteed.
//			ESP_LOGD(NVSLOG, "Committing updates in NVS ... ");
//			err = nvs_commit(storageNvsHandle);
//			if (err != ESP_OK) {
//				ESP_LOGE(NVSLOG, "Failed!");
//				ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			}
//			else
//				ESP_LOGD(NVSLOG, "Done");
//
//    		ESP_LOGI(NFCLOG, "Entering deep sleep\nsleepDuration = %lld (secs)\n", sleepDuration / 1000000);
//			ESP_ERROR_CHECK( esp_sleep_enable_ext1_wakeup((1ULL<<GPIO_NUM_35), ESP_EXT1_WAKEUP_ALL_LOW) );
//			ESP_ERROR_CHECK( esp_sleep_enable_timer_wakeup(sleepDuration) );
//
//			nvs_close(storageNvsHandle);
//			deinit_ble();
//			esp_deep_sleep_start();
//    	}
    	vTaskDelay(200 / portTICK_RATE_MS);
    }
}

int app_main(void) {

	int itCnt = 0;


	init_gpio();
	vTaskDelay(100 / portTICK_RATE_MS);
	init_pwm();
	vTaskDelay(100 / portTICK_RATE_MS);
	Serial.begin(115200);
	vTaskDelay(100 / portTICK_RATE_MS);
	while(nfc.begin()) {
		ESP_LOGE(NFCLOG, "Connect NFC Module to continue!!!");
		vTaskDelay(300 / portTICK_RATE_MS);
	}
	// check wake up cause
	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_EXT1) {
		nfcFromSleep();
		ESP_LOGD("SLEEP", "boom!!!");
		ESP_LOGD("SLEEP", "remote 0 = %s, remote 1 = %s", remote_device_name[0], remote_device_name[1]);
	}
	vTaskDelay(100 / portTICK_RATE_MS);
	init_ble();
	vTaskDelay(100 / portTICK_RATE_MS);
	init_timers();

	ESP_LOGI("System", "Welcome to 101DevBoard testing code.");

//	BEEP;
//	while (1) {
//		printf("Please press on GPIO 0 to start test#%2d... (Buzzer beeps twice)\n", itCnt);
//		waitUntilPress(GPIO_INPUT_IO0);
//		 buzzer tests
//		BEEP;
//		BEEP;
//		 end of buzzer tests
//
//		printf("Please press on GPIO 0 to continue test#%2d... (Lighting order--> red,green,blue,all-together)\n", itCnt);
//		waitUntilPress(GPIO_INPUT_IO0);
//
//		start_scan(30);
		init_nvs();
		nvsTimeRead();
		nvsReadRecordListCounter();

		if (recordListCounter == 0) {
//			esp_base_mac_addr_set(myDeviceBtMacAddrs);
			//BEEP;   // -------> beep on every restart after reading records from nvs... in order to indicate reboot
			esp_read_mac(myDeviceBtMacAddrs, ESP_MAC_BT);
			record_inst macAddressRecord = makeRecord(1, timePastMinutes, 0, NULL, myDeviceBtMacAddrs);
			addToRecordList(macAddressRecord, true, NULL);
			printRecordList();
		}
		else {
			for (int i = 0; i < recordListCounter; i++)
				nvsReadRecordList(i);

			printRecordList();
		}

		BEEP;

//		if (cause == ESP_SLEEP_WAKEUP_EXT1)
//		{
//			if (strcmp(remote_device_name[1], "Galaxy Note5")) {
//				ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");
//
//				start_scan(5);
//			}
//			else if  (i_nfcScanResult != -1){
//				record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
//				addToRecordList(numRec, true, NULL); // addToNumericRecordList(numRec);
//				printRecordList(); // printNumericRecordList();
//
//				if (conn_device_b) {
//					bleSendRecord(blePreSendRecord(numRec, recordListCounter - 1));
//				}
//			}
//			else if ((measureDeviceBtMacAddrs[0] != NULL || measureDeviceBtMacAddrs[1] != NULL)) {
//				if (conn_device_a == false) {
//					conn_device_a = true;
//					ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");  // --------> here I can make a record of connection if needed
//					esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, 6);
////						esp_ble_gap_stop_scanning();
//					esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, BLE_ADDR_TYPE_PUBLIC, true);
//				}
//			}
//
//		}

//		if (cause == ESP_SLEEP_WAKEUP_TIMER && (measureDeviceBtMacAddrs[0] != NULL || measureDeviceBtMacAddrs[1] != NULL))
//		{
//			esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE);
//			if (conn_device_a == false) {
//				conn_device_a = true;
//				ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");
////						esp_ble_gap_stop_scanning();
//				timePastMinutes++;
//				esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, BLE_ADDR_TYPE_PUBLIC, true);
//			}
//		}

		// check wake up cause
//		esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
//		if (cause == ESP_SLEEP_WAKEUP_TIMER) {
//			ESP_LOGI(NFCLOG, "Not EXT1 wakeup, initializing ULP\n");
//			// need to read from BioBeat
//		} else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
//			xTaskCreate(nfcHandle, "NFCis", 4096, NULL, 2, NULL);
//		} else {
//
//			ESP_LOGI(NFCLOG, "Regular wake up");
//			xTaskCreate(nfcHandle, "NFCis", 4096, NULL, 2, NULL);
//		}
		//vTaskDelay(500);

		xTaskCreate(nfcHandle, "NFCis", 8192, NULL, 2, NULL);
//		xTaskCreatePinnedToCore(activeNfcFiels, "NfcField", 2048, NULL, 10, NULL, 1);
//		nfcHandle();
//		 end of NFC module tests
//
//		itCnt++;
//	}

		// need to xTaskCreate
	return 0;
	}

void nfcEncodeToString(medical_event_type_list_t event)
{
	switch (event)
	{
		case  AIRWAY_MDC_EVT:
			memcpy(nfcScanResultString, "Airway", strlen("Airway") + 1);
			break;
		case INTBA_MDC_EVT:
			memcpy(nfcScanResultString, "Intubation", strlen("Intubation") + 1);
			break;
		case KTMN_MDC_EVT:
			memcpy(nfcScanResultString, "Ketamine", strlen("Ketamine") + 1);
			break;
		case DRMCM_MDC_EVT:
			memcpy(nfcScanResultString, "Doromicum", strlen("Doromicum") + 1);
			break;
		case CNOTM_MDC_EVT:
			memcpy(nfcScanResultString, "Coniotomy", strlen("Coniotomy") + 1);
			break;
		default:
			memcpy(nfcScanResultString, "NULL", strlen("NULL") + 1);
			break;
	}
}

void waitUntilPress(gpio_num_t gpioNum)
{
	while (gpio_get_level(gpioNum));
}

void init_gpio()
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
#ifdef ACTIVE_LOW_LEDS
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	gpio_set_level(GPIO_OUTPUT_GRGB1, 1ULL);
	gpio_set_level(GPIO_OUTPUT_BRGB1, 1ULL);
#else
	gpio_set_level(GPIO_OUTPUT_RRGB1, 0);
	gpio_set_level(GPIO_OUTPUT_GRGB1, 0);
	gpio_set_level(GPIO_OUTPUT_BRGB1, 0);
#endif
	//bit mask of the pins
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&io_conf);
}

void init_pwm()
{
	/*
	 * Prepare and set configuration of timers
	 * that will be used by LED Controller
	 */
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
	ledc_timer.freq_hz = 5000;                      // frequency of PWM signal
	ledc_timer.speed_mode = LEDC_HS_MODE;           // timer mode
	ledc_timer.timer_num = LEDC_HS_TIMER;            // timer index

	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);

	ledc_channel[0].channel    = LEDC_HS_CH0_CHANNEL,
	ledc_channel[0].duty       = LEDC_FULL_DUTY,
	ledc_channel[0].gpio_num   = LEDC_HS_CH0_RRGB2,
	ledc_channel[0].speed_mode = LEDC_HS_MODE,
	ledc_channel[0].timer_sel  = LEDC_HS_TIMER;

	ledc_channel[1].channel    = LEDC_HS_CH1_CHANNEL;
	ledc_channel[1].duty       = LEDC_FULL_DUTY;
	ledc_channel[1].gpio_num   = LEDC_HS_CH1_GRGB2;
	ledc_channel[1].speed_mode = LEDC_HS_MODE;
	ledc_channel[1].timer_sel  = LEDC_HS_TIMER;

	ledc_channel[2].channel	= LEDC_HS_CH2_CHANNEL;
	ledc_channel[2].duty		= LEDC_FULL_DUTY;
	ledc_channel[2].gpio_num	= LEDC_HS_CH2_BRGB2;
	ledc_channel[2].speed_mode	= LEDC_HS_MODE;
	ledc_channel[2].timer_sel	= LEDC_HS_TIMER;

	ledc_channel[3].channel = LEDC_HS_CH3_CHANNEL;
	ledc_channel[3].duty		= LEDC_ZERO_DUTY;
	ledc_channel[3].gpio_num	= LEDC_HS_CH3_BZZR;
	ledc_channel[3].speed_mode	= LEDC_HS_MODE;
	ledc_channel[3].timer_sel	= LEDC_HS_TIMER;

	// Set LED Controller with previously prepared configuration
	for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
		ledc_channel_config(&ledc_channel[ch]);
	}

	// Initialize fade service.
	ledc_fade_func_install(0);
}

void beep()
{
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_FULL_DUTY / 2);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
	vTaskDelay(100 / portTICK_RATE_MS);
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_ZERO_DUTY);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
	vTaskDelay(100 / portTICK_RATE_MS);
}

void longBeep()
{
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_FULL_DUTY / 2);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
	vTaskDelay(4000 / portTICK_RATE_MS);
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_ZERO_DUTY);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
#ifdef ACTIVE_LOW_LEDS
	gpio_set_level(GPIO_OUTPUT_GRGB1, 1ULL);
	gpio_set_level(GPIO_OUTPUT_BRGB1, 1ULL);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1);
	gpio_set_level(GPIO_OUTPUT_GRGB1, !conn_device_a);
	gpio_set_level(GPIO_OUTPUT_BRGB1, !conn_device_b);
#else
	gpio_set_level(GPIO_OUTPUT_GRGB1, NULL);
	gpio_set_level(GPIO_OUTPUT_BRGB1, NULL);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
	vTaskDelay(500 / portTICK_RATE_MS);
	gpio_set_level(GPIO_OUTPUT_RRGB1, 0);
	gpio_set_level(GPIO_OUTPUT_GRGB1, conn_device_a);
	gpio_set_level(GPIO_OUTPUT_BRGB1, conn_device_b);
#endif

}

static void gattc_profile_a_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
	esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

	switch (event) {
	case ESP_GATTC_REG_EVT: {
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
		}
		break;

	/* one device connect successfully, all profiles callback function will get the ESP_GATTC_CONNECT_EVT,
 	 so must compare the mac address to check which device is connected, so it is a good choice to use ESP_GATTC_OPEN_EVT. */
	case ESP_GATTC_CONNECT_EVT:
		ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
		break;

	case ESP_GATTC_OPEN_EVT: {
		if (p_data->open.status != ESP_GATT_OK){
			//open failed, ignore the first device, connect the second device
			ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			if (p_data->open.status != 0x91) // status 0x91 stay for already open so conn_device_a should stay true
				conn_device_a = false;
			//start_scan();
			break;
		}

		memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		ESP_LOGD(GATTC_TAG, "REMOTE BDA:");
		esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
		esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret){
			ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_GRGB1, 0);
#else
		gpio_set_level(GPIO_OUTPUT_GRGB1, 1);
#endif
		}
		break;

	case ESP_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
		}
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_a_filter_service_uuid);
		break;
	case ESP_GATTC_SEARCH_RES_EVT: {
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_SEARCH_RES_EVT");
		esp_gatt_srvc_id_t *srvc_id =(esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
		ESP_LOGD(GATTC_TAG, "SEARCH RES: conn_id = %x", p_data->search_res.conn_id);
        ESP_LOGI(GATTC_TAG, "UUID128: ");
        esp_log_buffer_hex(GATTC_TAG, srvc_id->id.uuid.uuid.uuid128, ESP_UUID_LEN_128);
		if (srvc_id->id.uuid.len == ESP_UUID_LEN_128 && !memcmp(srvc_id->id.uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128)) {
			ESP_LOGD(GATTC_TAG, "service found");
			get_service_a = true;
			gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID128: ");
            esp_log_buffer_hex(GATTC_TAG, srvc_id->id.uuid.uuid.uuid128, ESP_UUID_LEN_128);
		}
		break;
	}
	case ESP_GATTC_SEARCH_CMPL_EVT:
		if (p_data->search_cmpl.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
			break;
		}
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_A_SEARCH_CMPL_EVT");

        // update conn params
//		esp_ble_conn_update_params_t testingParam;
//		memcpy(testingParam.bda, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, 6);
//		testingParam.min_int = 0;
//		testingParam.max_int = 6.5;
//		testingParam.latency = 10;
//		testingParam.timeout = 200;
//		esp_ble_gap_update_conn_params(&testingParam);

		if (get_service_a){
			uint16_t count = 0;
			esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
																	 p_data->search_cmpl.conn_id,
																	 ESP_GATT_DB_CHARACTERISTIC,
																	 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
																	 INVALID_HANDLE,
																	 &count);
			if (status != ESP_GATT_OK){
				ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
			}

			if (count > 0){
				ESP_LOGD("GATTC- Debug", "Testing char of first service count: %d\n", count); //Debugging
				char_elem_result_a = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
				if (!char_elem_result_a){
					ESP_LOGE(GATTC_TAG, "gattc no mem");
				}else{
					status = esp_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
															 remote_a_filter_profile_char_uuid,
															 char_elem_result_a,
															 &count);
					if (count > 0 && (char_elem_result_a[0].properties & ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
							gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result_a[1].char_handle;
							esp_ble_gattc_write_char(gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id, char_elem_result_a[0].char_handle, 13, bb_writeToBiobeatProfile, ESP_GATT_WRITE_TYPE_NO_RSP,
									ESP_GATT_AUTH_REQ_NONE);
					}

					status = esp_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
															 remote_a_filter_measurements_char_uuid,
															 char_elem_result_a,
															 &count);
					if (status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result_a' */
					if (count > 0 && (char_elem_result_a[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
						gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result_a[0].char_handle;
						esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result_a[0].char_handle);
					}
					else {
						ESP_LOGE(GATTC_TAG, "char_elem_result_a[index].properties not have notify permission");

					}
				}
				/* free char_elem_result_a */
				free(char_elem_result_a);
			}else{
				ESP_LOGE(GATTC_TAG, "no char found");
			}
		}
		 break;
	case ESP_GATTC_REG_FOR_NOTIFY_EVT: { // Need to handle
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
		if (p_data->reg_for_notify.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
		}else{
			uint16_t count = 0;
			uint16_t notify_en = 1;
			ESP_LOGD("GATTC- Debug", "Testing: char_handle is: %d\n", gl_profile_tab[PROFILE_A_APP_ID].char_handle);
			esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
																		 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																		 ESP_GATT_DB_DESCRIPTOR,
																		 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
																		 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
																		 gl_profile_tab[PROFILE_A_APP_ID].char_handle,
																		 &count);
			if (ret_status != ESP_GATT_OK){
				ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
			}
			if (count > 0){
				descr_elem_result_a = (esp_gattc_descr_elem_t*)malloc(sizeof(esp_gattc_descr_elem_t) * count); // --------- NEED TO CAST MALLOC
				if (!descr_elem_result_a){
					ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
				}else{
					ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
																		 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																		 p_data->reg_for_notify.handle,
																		 notify_descr_uuid,
																		 descr_elem_result_a,
																		 &count);
					if (ret_status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
					}

					/* Erery char have only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result_a' */
					ESP_LOGD("GATTC- Debug", "Testing descr count: %d\n", count); //Debugging
					if (count > 0 && descr_elem_result_a[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_a[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
						ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
																	 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																	 descr_elem_result_a[0].handle,
																	 sizeof(notify_en),
																	 (uint8_t *)&notify_en,
																	 ESP_GATT_WRITE_TYPE_NO_RSP,
																	 ESP_GATT_AUTH_REQ_NONE);
					}

					if (ret_status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");

					}

					/* free descr_elem_result_a */
					free(descr_elem_result_a);
				}
			}
			else{
				ESP_LOGE(GATTC_TAG, "decsr not found");
			}

		}
		break;
	}
	case ESP_GATTC_NOTIFY_EVT: {
		if (p_data->notify.is_notify){
			ESP_LOGD(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
		}else{
			ESP_LOGD(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
		}
		// Blink GPIO 14 corresponding to Gatt_server's pulses
		esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);

        static int notifyCounter = 0;
//        if (notifyCounter == 11) {
			record_inst record;
			int i = 6;
		if (passTheNotify) {
			ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i]);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_SPO2, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i]);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_HEARTRATE, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i]);
			uint32_t tmpHighVal = p_data->notify.value[i++];


			ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i]);
			uint32_t tmpLowVal = p_data->notify.value[i++];
			uint32_t  tmpVal = 0;
			tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_BLOODPRESURE, (uint32_t)tmpVal, NULL); //p_data->read.value
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "TEMP: %f", (float)(p_data->notify.value[16] + 200) / 10);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_RESPRATE, (uint32_t)p_data->notify.value[16], NULL); //p_data->read.value
			addToRecordList(record, true, NULL);

			printRecordList();

			if (conn_device_b) {
				if (sendingRecordsToSmartphone) {
					for (int k = 0; k < 4; k++){
						bleMeasurementsStack[k].recordType = recordList[recordListCounter - 4 + k].recordType;//, recordListCounter - 4 + k;
						if (bleMeasurementsStack[k].recordType == NUMERIC_RECORD_TYPE) {// not in use but --> || bleMeasurementsStack[k].recordType == NUMERIC_RECORD_TYPE2)
							bleMeasurementsStack[k].recordInfo.charRecord = recordList[recordListCounter - 4 + k].recordInfo.charRecord;
						}
						else {
							bleMeasurementsStack[k].recordInfo.numericRecord = recordList[recordListCounter - 4 + k].recordInfo.numericRecord;
						}
					}
				}
				else {
					bleSendMeasurements();
				}
			}
			notifyCounter = 0;
			passTheNotify = false;
		}
//        }
//        else
//        	notifyCounter++;


//		if (p_data->notify.value[0])  --> Not relevant by now
//			gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
//		else
//			gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
//		record_inst record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 222, p_data->notify.value[0], NULL); // numericRecord_inst record = makeNumericRecord(timePastMinutes, 222, toggleCounter);
//		addToRecordList(record, true, NULL); // addToNumericRecordList(record);
		}
		break;
	case ESP_GATTC_WRITE_DESCR_EVT: {
		if (p_data->write.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		ESP_LOGD(GATTC_TAG, "write descr success ");

//		uint8_t write_char_data[20];
//		for (int i = 0; i < sizeof(write_char_data); ++i)
//		{
//			write_char_data[i] = i % 256;
//		}
//		esp_ble_gattc_write_char( gattc_if,
//								  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//								  gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//								  sizeof(write_char_data),
//								  write_char_data,
//								  ESP_GATT_WRITE_TYPE_NO_RSP,
//								  ESP_GATT_AUTH_REQ_NONE);

		break;
	}
	case ESP_GATTC_SRVC_CHG_EVT: {
		esp_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
		esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
		break;
	}
	case ESP_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
			// Read Attempt
//			ESP_LOGE("GATTC", "need ot reed now");
//			esp_ble_gattc_read_char(gattc_if,
//									gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//									gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//									ESP_GATT_AUTH_REQ_NONE);
			break;
		}

		ESP_LOGD(GATTC_TAG, "write char success ");

		// Read Attempt
//		ESP_LOGE("GATTC", "need ot reed now");
//		esp_ble_gattc_read_char(gattc_if,
//								gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//								gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//								ESP_GATT_AUTH_REQ_NONE);
		break;
	case ESP_GATTC_READ_CHAR_EVT: {
		if (p_data->read.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "read char failed, error status = %x", p_data->read.status);
			break;
		}
		ESP_LOGD(GATTC_TAG, "read char success. value len = %d, value data:", p_data->read.value_len);
		esp_log_buffer_hex(GATTC_TAG, p_data->read.value, sizeof(esp_bd_addr_t));

//		record_inst record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 81, (uint32_t)(random(50, 130)), NULL); // random dofek
//		addToRecordList(record, true, NULL);
//		uint32_t tmpLowVal = random(50, 90); // random lahatz dam
//		uint32_t tmpHighVal = random(80, 120);
//		uint32_t  tmpVal = 0;
//		tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
//		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 82, (uint32_t)tmpVal, NULL); //p_data->read.value
//		addToRecordList(record, true, NULL);
//		 record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 83, (uint32_t)(random(90, 100)), NULL); // random dofek
//		addToRecordList(record, true, NULL);
//		 record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 84, (uint32_t)(random(10, 20)), NULL); // random dofek
//		addToRecordList(record, true, NULL);
//
//
//		printRecordList();
//
//		if (conn_device_b) {
//			bleSendRecord(blePreSendRecord(recordList[recordListCounter - 2], recordListCounter - 2));
//			bleSendRecord(blePreSendRecord(recordList[recordListCounter - 1], recordListCounter - 1));
//		}
//		if (timePastMinutes % 2 == 0 && conn_device_a) {
//			// past 2 minutes so need to sample BioBeat
//			esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
//										gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//										gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//										ESP_GATT_AUTH_REQ_NONE);
//		}
//		int i;
//		for(i=0; i<(p_data->read.value_len); i++)
//			printf("0x%X ", p_data->read.value[i]);
//		printf("and now?");

		break;
	}
	case ESP_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, 6) == 0){
			ESP_LOGI(GATTC_TAG, "device 'A' disconnect");
			conn_device_a = false;
			get_service_a = false;
            if (!conn_device_b)
                Isconnecting = false;
			// so turn off green led
#ifdef ACTIVE_LOW_LEDS
    		gpio_set_level(GPIO_OUTPUT_GRGB1, 1);
#else
    		gpio_set_level(GPIO_OUTPUT_GRGB1, 0);
#endif
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_b_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:{
		ESP_LOGD(GATTC_TAG, "REG_EVT");
		esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
		if (scan_ret){
			ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
		}
//		esp_ble_gap_config_local_privacy(true);
		}
        break;

    case ESP_GATTC_CONNECT_EVT:
        break;

    case ESP_GATTC_OPEN_EVT: {
        if (p_data->open.status != ESP_GATT_OK){
            //open failed, ignore the second device, connect the third device
            ESP_LOGE(GATTC_TAG, "connect device 2(smartphone) failed, status %d", p_data->open.status);
            conn_device_b = false;
            //start_scan();
            break;
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_OPEN_EVT");
        memcpy(gl_profile_tab[PROFILE_B_APP_ID].remote_bda, p_data->open.remote_bda, 6);
        gl_profile_tab[PROFILE_B_APP_ID].conn_id = p_data->open.conn_id;
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGD(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));

		esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_BRGB1, 0);
#else
		gpio_set_level(GPIO_OUTPUT_BRGB1, 1);
#endif
    	}
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGD(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_b_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        esp_gatt_srvc_id_t *srvc_id = (esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
        ESP_LOGD(GATTC_TAG, "SEARCH RES: conn_id = %x", p_data->search_res.conn_id);
        if (srvc_id->id.uuid.len == ESP_UUID_LEN_16 && srvc_id->id.uuid.uuid.uuid16 == REMOTE_B_SERVICE_UUID) {
        	ESP_LOGD(GATTC_TAG, "service found");
        	ESP_LOGD(GATTC_TAG, "UUID16: %x", srvc_id->id.uuid.uuid.uuid16);
            get_service_b = true;
            gl_profile_tab[PROFILE_B_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_B_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }

        // update conn params
//		esp_ble_conn_update_params_t testingParam;
//		memcpy(testingParam.bda, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, 6);
//		testingParam.min_int = 0;
//		testingParam.max_int = 6.5;
//		testingParam.latency = 10;
//		testingParam.timeout = 2000;
//		esp_ble_gap_update_conn_params(&testingParam);

        ESP_LOGD(GATTC_TAG, "ESP_GATTC_B_SEARCH_CMPL_EVT");
        if (get_service_b){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
            	ESP_LOGD("GATTC- Debug", "Testing char of service count: %d\n", count); //Debugging
                char_elem_result_b = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_b){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                             remote_b_filter_char_uuid,
                                                             char_elem_result_b,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
                    ESP_LOGD("GATTC- Debug", "Testing char found count: %d\n", count); //Debugging
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    // from here it should register for notify and write to descriptor, meanwhile we don't register for notify ------- NEED TO DO!!!
                    if (count > 0 && (char_elem_result_b[0].properties & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR))){
                        gl_profile_tab[PROFILE_B_APP_ID].char_handle = char_elem_result_b[0].char_handle;
//                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, char_elem_result_b[0].char_handle); //$$$$$$
                        // send test array...
//                        uint8_t write_char_data[26];
//						for (int i = 0; i < sizeof(write_char_data); ++i)
//						{
//							write_char_data[i] = i + 65;
//						}
//                        esp_ble_gattc_write_char( gattc_if,
//                                                          gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//                                                          gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//                                                          sizeof(write_char_data),
//                                                          write_char_data,
//														  ESP_GATT_WRITE_TYPE_NO_RSP,
//                                                          ESP_GATT_AUTH_REQ_NONE);
                        // end of send test array|
                        bleSendRecordList();
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_b);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {

        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_b = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_b){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_b,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }

                /* Erery char have only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                ESP_LOGD("GATTC- Debug", "Testing descr count: %d\n", count); //Debugging
                if (count > 0 && descr_elem_result_b[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_b[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                 descr_elem_result_b[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_NO_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }

                /* free descr_elem_result */
                free(descr_elem_result_b);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, Receive notify value:");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGD(GATTC_TAG, "write descr success");

		if (conn_device_b)
			bleSendRecordList(); //bleSendNumericRecordList()

        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
        }else{
            ESP_LOGD(GATTC_TAG, "Write char success");
            if (recordSendCoutner < recordListCounter && sendingRecordsToSmartphone) {
            	ESP_LOGI(GATTC_TAG, "sending next record %d", recordSendCoutner);

				bleSendRecord(blePreSendRecord(recordList[recordSendCoutner], recordSendCoutner));
				recordSendCoutner++;

            }
            else if (recordSendCoutner == recordListCounter && sendingRecordsToSmartphone){
            	// send type 99 - record list counter
				record_inst recordListCounterRecord;
				uint32_t dataOfType99 = 0;
				dataOfType99 = (recordListCounter & 0xFFFF) | ((uint32_t)evt.timer_counter_value << 16);
//				recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, dataOfType99, NULL); // when Gal will be ready get off from comment
				recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
				blePreSendRecord(recordListCounterRecord, recordListCounter);
				// Client need to approve / authenticate that the phone got all records
				// bleSendNumericRecord(numericRecordListCounterRecord);
				char tmpRecordBuffer_TAG[25] = "Record #";
				char tmpItoa[5];
				itoa(recordListCounter, tmpItoa, 10);
				strcat(tmpRecordBuffer_TAG, tmpItoa);
				strcat(tmpRecordBuffer_TAG, " - ");
				ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data: %d", recordListCounterRecord.recordInfo.numericRecord->timePast, recordListCounterRecord.recordInfo.numericRecord->type, recordListCounterRecord.recordInfo.numericRecord->data);
				esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
											  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
											  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
											  NUMERIC_BLE_BUFF_SIZE,
											  bleNumericSendBuffer,
											  ESP_GATT_WRITE_TYPE_NO_RSP,
											  ESP_GATT_AUTH_REQ_NONE);
				ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

				recordSendCoutner++;
				sendingRecordsToSmartphone = false;

				if (stackSendCounter > 2) { // in case it not full
					bleSendStack();
				}
				else if (memcmp(measureDeviceBtMacAddrs, emptyMacAddress, MAC_SIZE)) {
					// Pluster
					ESP_LOGI(GATTC_TAG, "memcmp(measureDeviceBtMacAddrs");
					conn_device_a = true;
					esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs, BLE_ADDR_TYPE_PUBLIC, true);
				}
            }

            if (mesurementSendCounter < 4 && sendingMeasurementsToSmartphone) {
            	ESP_LOGI(GATTC_TAG, "sending next measure %d", mesurementSendCounter);

            	bleSendRecord(blePreSendRecord(recordList[recordListCounter - 4 + mesurementSendCounter], recordListCounter - 4 + mesurementSendCounter));
            	mesurementSendCounter++;
            }
            else if (sendingMeasurementsToSmartphone) {
            	sendingMeasurementsToSmartphone = false;
            	mesurementSendCounter = 0;
            }

            if (stackSendCounter < 4 && sendingStackToSmartphone) {
            	ESP_LOGI(GATTC_TAG, "sending next measure %d (from stack)", stackSendCounter);

            	bleSendRecord(blePreSendRecord(bleMeasurementsStack[stackSendCounter], recordListCounter - 4 + stackSendCounter));
            	stackSendCounter++;
            }
            else if (sendingStackToSmartphone) {
            	sendingStackToSmartphone = false;
            	stackSendCounter = 0;
            	if (memcmp(measureDeviceBtMacAddrs, emptyMacAddress, MAC_SIZE)) {
            		ESP_LOGI(GATTC_TAG, "try to connect back to the measurement device.");
					conn_device_a = true;
					esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs, BLE_ADDR_TYPE_PUBLIC, true);
            	}
            }

        }
//        start_scan();
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, 6) == 0){
            ESP_LOGI(GATTC_TAG, "device 'B' disconnect");
            conn_device_b = false;
            get_service_b = false;
            if (!conn_device_a)
                Isconnecting = false;
    		// so turn off blue led

            // try to connect up to 3 times   ---- as olamot asked, I removed it.... was for debuging
//            static int smartphoneConnectAttempt = 0;
//            if (smartphoneConnectAttempt < 3) {
//            	esp_ble_gap_disconnect(measureDeviceBtMacAddrs);  // Pluster
//            	start_scan(2);
//            	smartphoneConnectAttempt++;
//            }
//            else
//            	smartphoneConnectAttempt = 0;


#ifdef ACTIVE_LOW_LEDS
    		gpio_set_level(GPIO_OUTPUT_BRGB1, 1);
#else
    		gpio_set_level(GPIO_OUTPUT_BRGB1, 0);
#endif
        }
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    uint8_t *adv_uuid_cmpl = NULL;
    uint8_t adv_uuid_cmpl_len = 0;

    uint8_t *adv_uuid_part = NULL;
    uint8_t adv_uuid_part_len = 0;

    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 2;
//        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

            adv_uuid_cmpl = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_CMPL, &adv_uuid_cmpl_len);

            adv_uuid_part = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_PART, &adv_uuid_part_len);
            // service uuid is bb_serviceUUID

            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
//            if (Isconnecting){
//                break;
//            }
            if (conn_device_a && conn_device_b && !stop_scan_done){
                stop_scan_done = true;
                esp_ble_gap_stop_scanning();
                ESP_LOGI(GATTC_TAG, "all devices are connected");
                break;
            }
            if (adv_name != NULL) {

            	// Connection to "BioBeat" in via NFC only.
//                if (strlen(remote_device_name[0]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[0], adv_name_len) == 0) {
//                    if (conn_device_a == false) {
//                        conn_device_a = true;
//                        ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[0]);
//                        esp_ble_gap_stop_scanning();
//                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, BLE_ADDR_TYPE_PUBLIC, true);
//                        memcpy(measureDeviceBtMacAddrs, scan_result->scan_rst.bda, MAC_SIZE);
//                        Isconnecting = true;
//                    }
//                    break;
//                }
                if (strlen(remote_device_name[1]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[1], adv_name_len) == 0) {
                    if (conn_device_b == false) {
                        conn_device_b = true;
                        ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[1]);
                        esp_ble_gap_stop_scanning();

//						vTaskDelay(500 / portTICK_RATE_MS);
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, scan_result->scan_rst.bda, BLE_ADDR_TYPE_RANDOM, true);
                        memcpy(smartphoneMacAddress, scan_result->scan_rst.bda, MAC_SIZE);
                        Isconnecting = true;
                        //vTaskDelay(10000/portTICK_RATE_MS);  // debug for assertion failing

                    }
                }
            }

            // connection to neerest BioBeat
//            if (scan_result->scan_rst.rssi > -60) {
//				if (adv_uuid_cmpl != NULL || adv_uuid_part != NULL) {
//					if ((16 == adv_uuid_cmpl_len && (memcmp(adv_uuid_cmpl, bb_serviceUUID, adv_uuid_cmpl_len) == 0))
//							|| (16 == adv_uuid_part_len && (memcmp(adv_uuid_part, bb_serviceUUID, adv_uuid_part_len)))) {
//						ESP_LOGI(GATTC_TAG, "searched device\n");
//						esp_log_buffer_hex(GATTC_TAG, bb_serviceUUID, 16);
//						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_cmpl)\n");
//						esp_log_buffer_hex(GATTC_TAG, adv_uuid_cmpl, 16);
//						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_part)\n");
//						if (conn_device_a == false) {
//							conn_device_a = true;
//							ESP_LOGI(GATTC_TAG, "connect to the remote device.");
//							esp_ble_gap_stop_scanning();
//							esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
//							memcpy(measureDeviceBtMacAddrs, scan_result->scan_rst.bda, MAC_SIZE);
//						}
//					}
//				}
//			}
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
        	if (memcmp(measureDeviceBtMacAddrs, emptyMacAddress, MAC_SIZE)) {
        		// Pluster
				ESP_LOGI(GATTC_TAG, "try to connect back to the measurement device.");
				conn_device_a = true;
				esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs, BLE_ADDR_TYPE_PUBLIC, true);
        	}
            scanIsOn = false;
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");
        scanIsOn = false;

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    //ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void init_ble()
{
	// init BioBeat filters
	memcpy(remote_a_filter_service_uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128);
	memcpy(remote_a_filter_measurements_char_uuid.uuid.uuid128, bb_measurements_characteristicUUID, ESP_UUID_LEN_128);
	memcpy(remote_a_filter_profile_char_uuid.uuid.uuid128, bb_profile_characteristicUUID, ESP_UUID_LEN_128);

	// init smartphone filters
//	memcpy(remote_b_filter_service_uuid.uuid.uuid128, sp_serviceUUID, ESP_UUID_LEN_128);
//	memcpy(remote_b_filter_char_uuid.uuid.uuid128, sp_writeCharUUID, ESP_UUID_LEN_128);

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s initialize controller failed\n", __func__);
		return;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s enable controller failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s init bluetooth failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed\n", __func__);
		return;
	}

	//register the  callback function to the gap module
	ret = esp_ble_gap_register_callback(esp_gap_cb);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gap register error, error code = %x", ret);
		return;
	}

	//register the callback function to the gattc module
	ret = esp_ble_gattc_register_callback(esp_gattc_cb);
	if(ret){
		ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gattc_app_register(PROFILE_B_APP_ID);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gatt_set_local_mtu(200);
	if (ret){
		ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
	}
}

void deinit_ble()
{
	esp_err_t ret = esp_bluedroid_disable();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s disable bluedroid failed, error code = %x\n", __func__, ret);
	ret = esp_bluedroid_deinit();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s deinit bluedroid failed, error code = %x\n", __func__, ret);
	ret = esp_bt_controller_disable();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s disable bt controller failed, error code = %x\n", __func__, ret);
}


numericRecord_inst makeNumericRecord (uint16_t timePast, uint8_t type, uint32_t data)
{
	numericRecord_inst tempRecord;
	tempRecord.timePast = timePast;
	tempRecord.type = type;
	tempRecord.data = data;
	return tempRecord;

}

bool addToNumericRecordList (numericRecord_inst record)
{
	if (numericRecordListCounter >= MAX_RECORD_IN_NVS)
		return false; // re-allocation need
	numericRecordList[numericRecordListCounter++] = record;
	return true;
}

void printNumericRecordList()
{
	ESP_LOGI("Numeric Records List", "Printing records list:");

	for (int i = 0; i < numericRecordListCounter; i++) {
		ESP_LOGI("Record List", "timePast: %d, type: %03d, data: %d", numericRecordList[i].timePast, numericRecordList[i].type, numericRecordList[i].data);
	}

}

bool checkIfHce()
{
	ESP_LOGI(NFCLOG, "Found something!");

	uint8_t selectApdu[] = { 0x00, /* CLA */
							  0xA4, /* INS */
							  0x04, /* P1  */
							  0x00, /* P2  */
							  0x06, /* Length of AID  */
							  0x00, 0x01, 0x02, 0x03, 0x04, 0x05/* AID defined on Android App */
							  /*0x00   Le  */ };
	for (int apduIndex = 5; apduIndex < 11; apduIndex++)
		selectApdu[apduIndex] = myDeviceBtMacAddrs[apduIndex - 5];

	uint8_t responseLength = 32;
	uint8_t response[32];

	return nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
}

/*
 * A simple helper function to print the raw timer counter value
 * and the counter value converted to seconds
 */
static void inline print_timer_counter(uint64_t counter_value)
{
    ESP_LOGD(TIMERLOG, "Counter: 0x%08x%08x\n", (uint32_t) (counter_value >> 32),
                                    (uint32_t) (counter_value));
    ESP_LOGD(TIMERLOG, "Time   : %.8f s\n", (double) counter_value / TIMER_SCALE);
}

/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evtISR;
    evtISR.timer_group = TIMER_GROUP_0;
    evtISR.timer_idx = (timer_idx_t)timer_idx;
    evtISR.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        evtISR.type = TEST_WITHOUT_RELOAD;
        TIMERG0.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
        TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        evtISR.type = TEST_WITH_RELOAD;
        TIMERG0.int_clr_timers.t1 = 1;
    } else {
        evtISR.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue, &evtISR, NULL);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void example_tg0_timer_init(timer_idx_t timer_idx, bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

void init_timers() {
	timer_queue = xQueueCreate(30, sizeof(timer_event_t));
	    example_tg0_timer_init(TIMER_0, TEST_WITHOUT_RELOAD, TIMER_INTERVAL0_SEC);
	    example_tg0_timer_init(TIMER_1, TEST_WITH_RELOAD,    TIMER_INTERVAL1_SEC);
}

void blePreSendNumericRecord(numericRecord_inst record)
{
	bleNumericSendBuffer[0] = (record.timePast >> 8) & 0xFF;
	bleNumericSendBuffer[1] = record.timePast & 0xFF;
	bleNumericSendBuffer[2] = record.type;
	for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
		bleNumericSendBuffer[i] = (record.data >> (24 - 8*(i-3))) & 0xFF;
	}
	esp_log_buffer_hex("BLE Send Buffer", bleNumericSendBuffer, NUMERIC_BLE_BUFF_SIZE);

}

void bleSendNumericRecord()
{
	if (gl_profile_tab[PROFILE_B_APP_ID].gattc_if != ESP_GATT_IF_NONE) {
		esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
									  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
									  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
									  NUMERIC_BLE_BUFF_SIZE,
									  bleNumericSendBuffer,
									  ESP_GATT_WRITE_TYPE_NO_RSP,
									  ESP_GATT_AUTH_REQ_NONE);
	}
}

void bleSendNumericRecordList()
{
	for (int i = 0; i < numericRecordListCounter; i++) {
		blePreSendNumericRecord(numericRecordList[i]);
		bleSendNumericRecord();
	}
	// send type 99 - record list counter
	numericRecord_inst numericRecordListCounterRecord;
	numericRecordListCounterRecord = makeNumericRecord(timePastMinutes, 99, numericRecordListCounter);
	blePreSendNumericRecord(numericRecordListCounterRecord);
	// Client need to approve / authenticate that the phone got all records
	// bleSendNumericRecord(numericRecordListCounterRecord);
	esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
										  NUMERIC_BLE_BUFF_SIZE,
										  bleNumericSendBuffer,
										  ESP_GATT_WRITE_TYPE_NO_RSP,
										  ESP_GATT_AUTH_REQ_NONE);


}

record_inst makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t numericData, uint8_t charData[]) // recordType -> 0-Numeric(charData=NULL), 1-Char
{
	record_inst tempRecord;
	if (recordType == 0 && charData == NULL) {
		tempRecord.recordInfo.charRecord = NULL;
		tempRecord.recordType = NUMERIC_RECORD_TYPE;
		tempRecord.recordInfo.numericRecord = (numericRecord_inst*)malloc(NUMERIC_RECORD_SIZE);
		tempRecord.recordInfo.numericRecord->timePast = timePast;
		tempRecord.recordInfo.numericRecord->type = type;
		tempRecord.recordInfo.numericRecord->data = numericData;
		return tempRecord;
	}
	else if (recordType == 1) {
		tempRecord.recordType = CHAR_RECORD_TYPE;
		tempRecord.recordInfo.charRecord = (charRecord_inst*)malloc(CHAR_RECORD_SIZE);
		tempRecord.recordInfo.charRecord->timePast = timePast;
		tempRecord.recordInfo.charRecord->type = type;
		for (int i = 0; i < 20; i++)
			tempRecord.recordInfo.charRecord->data[i] = charData[i];
		return tempRecord;
	}
	tempRecord.recordInfo.numericRecord = NULL;
	return tempRecord;
}

bool addToRecordList (record_inst record, bool writeToNvs, uint32_t recordNum)
{
	if (recordListCounter >= MAX_RECORD_IN_NVS) {

//		record_inst* tmp_recordList;
		recordList = (record_inst*)realloc(recordList, RECORD_SIZE * (recordListCounter + MAX_RECORD_IN_NVS));
		if (recordList == NULL){
			ESP_LOGE("MEMORY", "couldn't re-allocate record list for another MAX_RECORD_IN_NVS");
			ESP_LOGE("MEMORY", "		FATAL MEMORY ERRO!!		");
			haltProgram();
			return false; // re-allocation failed
		}
		MAX_RECORD_IN_NVS = MAX_RECORD_IN_NVS + MAX_RECORD_IN_NVS;

	}

	if (recordListCounter == MAX_RECORD_IN_NVS)
		longBeep();


	if (writeToNvs) {
		recordList[recordListCounter++] = record;
		nvsWriteRecordList(recordListCounter - 1);
		nvsWriteRecordListCounter();
	}
	else
		recordList[recordNum] = record;
	return true;

}

void printRecordList()
{
	ESP_LOGI("Records List", "Printing records list:");

//	for (int i = 0; i < recordListCounter; i++) {
	for (int i = recordListCounter - 100; i < recordListCounter; i++) {

		char tmpRecordBuffer_TAG[25] = "Record #";
		char tmpItoa[5];
		itoa(i, tmpItoa, 10);
		strcat(tmpRecordBuffer_TAG, tmpItoa);
		strcat(tmpRecordBuffer_TAG, " - ");

		if (recordList[i].recordType == NUMERIC_RECORD_TYPE) {

			strcat(tmpRecordBuffer_TAG, "Numeric");
			if (recordList[i].recordInfo.numericRecord->type == 82) { //may have more exceptions
				ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data1: %d, data2: %d",
						recordList[i].recordInfo.numericRecord->timePast, recordList[i].recordInfo.numericRecord->type,
						(recordList[i].recordInfo.numericRecord->data >> 16) & 0xFFFF, (recordList[i].recordInfo.numericRecord->data) & 0xFFFF);
			} else
				ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data: %d", recordList[i].recordInfo.numericRecord->timePast, recordList[i].recordInfo.numericRecord->type, recordList[i].recordInfo.numericRecord->data);
		}
		else {
			strcat(tmpRecordBuffer_TAG, "Char");
			ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data: (next line)", recordList[i].recordInfo.charRecord->timePast, recordList[i].recordInfo.charRecord->type);
			strcat(tmpRecordBuffer_TAG, " Data");
			esp_log_buffer_hex(tmpRecordBuffer_TAG, recordList[i].recordInfo.charRecord->data, 20);
		}
	}

}

recordType_t blePreSendRecord(record_inst record, int i)
{
	if (record.recordType == NUMERIC_RECORD_TYPE){
		bleNumericSendBuffer[0] = (record.recordInfo.numericRecord->timePast >> 8) & 0xFF;
		bleNumericSendBuffer[1] = record.recordInfo.numericRecord->timePast & 0xFF;
		bleNumericSendBuffer[2] = record.recordInfo.numericRecord->type;
		for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
			bleNumericSendBuffer[i] = (record.recordInfo.numericRecord->data >> (24 - 8*(i-3))) & 0xFF;
		}

		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
		strcat(tmpBleSendBuffer_TAG, "[");
		char tmpItoa[5];
		itoa(i, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
		strcat(tmpBleSendBuffer_TAG, "]");

		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleNumericSendBuffer, NUMERIC_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
		return record.recordType;
	}
	else {
		bleCharSendBuffer[0] = (record.recordInfo.charRecord->timePast >> 8) & 0xFF;
		bleCharSendBuffer[1] = record.recordInfo.charRecord->timePast & 0xFF;
		bleCharSendBuffer[2] = record.recordInfo.charRecord->type;
		for (int i = 3; i < CHAR_BLE_BUFF_SIZE; i++) {
			bleCharSendBuffer[i] = (record.recordInfo.charRecord->data[i-3]);
		}

		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
		strcat(tmpBleSendBuffer_TAG, "[");
		char tmpItoa[5];
		itoa(i, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
		strcat(tmpBleSendBuffer_TAG, "]");

		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleCharSendBuffer, CHAR_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
		return record.recordType;
	}
}

void bleSendRecord(recordType_t recordType)
{
	if (gl_profile_tab[PROFILE_B_APP_ID].gattc_if != ESP_GATT_IF_NONE) {
		if (recordType == NUMERIC_RECORD_TYPE)
			esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
										  NUMERIC_BLE_BUFF_SIZE,
										  bleNumericSendBuffer,
										  ESP_GATT_WRITE_TYPE_NO_RSP,
										  ESP_GATT_AUTH_REQ_NONE);
		else
			esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
										  CHAR_BLE_BUFF_SIZE,
										  bleCharSendBuffer,
										  ESP_GATT_WRITE_TYPE_NO_RSP,
										  ESP_GATT_AUTH_REQ_NONE);
	}
}

void bleSendRecordList()
{
//	for (int i = 0; i < recordListCounter; i++) {
//		bleSendRecord(blePreSendRecord(recordList[i], i));
//		vTaskDelay(15 / portTICK_RATE_MS);
//		if (i % 10 == 0) {
//			vTaskDelay(150 / portTICK_RATE_MS);
////			break;
//		}
//		else if (i == 50) {
//			vTaskDelay(1500 / portTICK_RATE_MS);
//			break;
//		}
//	}
	recordSendCoutner = 0;
	sendingRecordsToSmartphone = true;
	bleSendRecord(blePreSendRecord(recordList[recordSendCoutner], recordSendCoutner));
	recordSendCoutner++;
//	// send type 99 - record list counter
//	record_inst recordListCounterRecord;
//	recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
//	blePreSendRecord(recordListCounterRecord, recordListCounter);
//	// Client need to approve / authenticate that the phone got all records
//	// bleSendNumericRecord(numericRecordListCounterRecord);
//	char tmpRecordBuffer_TAG[25] = "Record #";
//	char tmpItoa[5];
//	itoa(recordListCounter, tmpItoa, 10);
//	strcat(tmpRecordBuffer_TAG, tmpItoa);
//	strcat(tmpRecordBuffer_TAG, " - ");
//	ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data: %d", recordListCounterRecord.recordInfo.numericRecord->timePast, recordListCounterRecord.recordInfo.numericRecord->type, recordListCounterRecord.recordInfo.numericRecord->data);
//	esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//								  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//								  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//								  NUMERIC_BLE_BUFF_SIZE,
//								  bleNumericSendBuffer,
//								  ESP_GATT_WRITE_TYPE_NO_RSP,
//								  ESP_GATT_AUTH_REQ_NONE);
//	ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

//	for (int i = 51; i < recordListCounter; i++) {
//		bleSendRecord(blePreSendRecord(recordList[i], i));
//		vTaskDelay(50 / portTICK_RATE_MS);
//		if (i % 10 == 0) {
////			vTaskDelay(150 / portTICK_RATE_MS);
////			break;
//		}
//		else if (i == 50) {
////			vTaskDelay(1500 / portTICK_RATE_MS);
//			break;
//		}
//	}

//	// Pluster
//	esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs, BLE_ADDR_TYPE_PUBLIC, true);
}
void bleSendMeasurements()
{
	mesurementSendCounter = 0;
	sendingMeasurementsToSmartphone = true;
//	for (int j = 1; j < 5; j++){
		bleSendRecord(blePreSendRecord(recordList[recordListCounter - 4], recordListCounter - 4));
//	}
	mesurementSendCounter++;
}

void bleSendStack()
{
	stackSendCounter = 0;
	sendingStackToSmartphone = true;
	bleSendRecord(blePreSendRecord(bleMeasurementsStack[0], recordListCounter - 4));
	stackSendCounter++;
}

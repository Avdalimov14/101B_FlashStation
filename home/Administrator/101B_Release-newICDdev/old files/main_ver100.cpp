

#include "mainHeader.h"
#include "bleManage.h"
#include "dbManage.h"
#include "nfcManage.h"
#include "ioManage.h"
#include "timerManage.h"
#include "nvsManage.h"


#ifdef __cplusplus
extern "C" {
#endif

int app_main(void);


// Records variables
// Array List using record_inst
uint32_t maxCurrentRecordListCounter = 10;
record_inst* recordList = (record_inst*)malloc(maxCurrentRecordListCounter * RECORD_SIZE);
uint32_t recordListCounter = 0; // need to be initialized in nvs

// BLE Records operations and vars

uint8_t bleNumericSendBuffer[NUMERIC_BLE_BUFF_SIZE];
uint8_t bleCharSendBuffer[CHAR_BLE_BUFF_SIZE];
bool firstConnectToPhone = true;

uint8_t myDeviceBtMacAddrs[20] = {0};
uint8_t measureDeviceBtMacAddrs[6] = {0};

// timers and counters and queue

RTC_DATA_ATTR static time_t rtcLastFromResetInSeconds = 0;

//extern uint32_t timePastMinutes;// = 0;
//extern RTC_DATA_ATTR uint32_t timePastSeconds;// = 0; // consider to typdef a struct timePast

// Task handle

TaskHandle_t nfcTaskHandle;
bool runNfcFunction = false;


#ifdef __cplusplus
}
#endif


int app_main(void) {

	int itCnt = 0;


	init_gpio();
	vTaskDelay(25 / portTICK_RATE_MS);
	init_pwm(true);
	vTaskDelay(25 / portTICK_RATE_MS);
	Serial.begin(115200);
	vTaskDelay(25 / portTICK_RATE_MS);
	while(nfc.begin()) {
		ESP_LOGE(NFCLOG, "Connect NFC Module to continue!!!");
		vTaskDelay(300 / portTICK_RATE_MS);
	}
	// check wake up cause
	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
//	if (cause == ESP_SLEEP_WAKEUP_EXT1) { there is no ESP_SLEEP_WAKEUP_EXT1 option
//		nfcFromSleep();
//		ESP_LOGD("SLEEP", "boom!!!");
//		ESP_LOGD("SLEEP", "remote 0 = %s, remote 1 = %s", remote_device_name[0], remote_device_name[1]);
//	}
	vTaskDelay(25 / portTICK_RATE_MS);
	init_ble();
	vTaskDelay(100 / portTICK_RATE_MS);
	init_timers();

	ESP_LOGI("System", "Welcome to 101DevBoard testing code.");
	BEEP; //for initial debugs only

	start_scan();
	init_nvs();
	nvsTimeCheck();
	nvsReadRecordListCounter();//&recordListCounter);

	if (recordListCounter == 0) {
		BEEP;
		esp_read_mac(myDeviceBtMacAddrs, ESP_MAC_BT);
		record_inst macAddressRecord = makeRecord(1, timePastMinutes, 0, NULL, myDeviceBtMacAddrs);
		addToRecordList(macAddressRecord, true, NULL);//, recordList, &recordListCounter);
		printRecordList();//recordList, &recordListCounter);
	}
	else {
		for (int i = 0; i < recordListCounter; i++)
			nvsReadRecordList(i);//, recordList, &recordListCounter);

		printRecordList();//recordList, &recordListCounter);
	}

//	if (cause == ESP_SLEEP_WAKEUP_EXT1)  -----> this version ESP soesn't go to sleep, he is strong he loves YABA
//	{
//		if (strcmp(remote_device_name[1], "Galaxy Note5")) {
//			ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");
//
//			start_scan();
//		}
//		else if  (i_nfcScanResult != -1){
//			record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
//			addToRecordList(numRec, true, NULL); // addToNumericRecordList(numRec);
//			printRecordList(); // printNumericRecordList();
//
//			if (conn_device_b) {
//				bleSendRecord(blePreSendRecord(numRec, recordListCounter - 1));
//			}
//		}
//		else if ((measureDeviceBtMacAddrs[0] != NULL || measureDeviceBtMacAddrs[1] != NULL)) {
//			if (conn_device_a == false) {
//				conn_device_a = true;
//				ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");  // --------> here I can make a record of connection if needed
//				esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, 6);
////						esp_ble_gap_stop_scanning();
//				esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
//			}
//		}
//
//	}

//	if (cause == ESP_SLEEP_WAKEUP_TIMER && (measureDeviceBtMacAddrs[0] != NULL || measureDeviceBtMacAddrs[1] != NULL))
//	{
//		esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE);
//		if (conn_device_a == false) {
//			conn_device_a = true;
//			ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");
////						esp_ble_gap_stop_scanning();
//			esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
//		}
//	}

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
	runNfcFunction = true;
//	xTaskCreate(nfcHandleTask, "NfcTask", 4096, NULL, 2, &nfcTaskHandle);
	xTaskCreatePinnedToCore(nfcHandleTask, "NfcTask", 4096, NULL, 2, &nfcTaskHandle, 1);
//		xTaskCreatePinnedToCore(activeNfcFiels, "NfcField", 2048, NULL, 10, NULL, 1);
//		nfcHandle();
//		 end of NFC module tests
//
//		itCnt++;
//	}

	// need to xTaskCreate
	return 0;
}


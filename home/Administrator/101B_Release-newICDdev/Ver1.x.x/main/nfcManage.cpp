/*
 * nfcManage.cpp
 *
 *  Created on: Jun 27, 2018
 *      Author: albert
 */

#include "nfcManage.h"
#include "esp_log.h"
#include "timerManage.h"
#include "gatts_related.h"
#include "gattc_related.h"
#include "nvsManage.h"
#include "ioManage.h"

//xQueueHandle rfFieldQueue;
bool simulatorDevice = true;

bool init_nfc()
{
	Serial.begin(115200);
	Serial.println("NDEF Reader");
	while(nfc.begin()) {
		ESP_LOGE(NFCLOG, "Connect NFC Module to continue!!!");
		vTaskDelay(300 / portTICK_RATE_MS);
	}
//	rfFieldQueue = xQueueCreate(10, sizeof(bool));
	return true;
}
void nfcRfFieldTask(void *pvParameters)
{
	nfc.setRfField(false);
	bool foundSomething = false;
	ESP_LOGI(NFCLOG, "NFC is On!!\nScan a NFC tag\n");
	while(1)
	{
//		nfc.setRfField(true);
		if(nfc.tagPresent(NFC_RF_PULSE_WIDTH)) {
			foundSomething = true;
			readTagRecords();
			while(nfc.tagPresent(NFC_RF_PULSE_WIDTH));
			vTaskDelay(20 / portTICK_RATE_MS);

		}
		nfc.setRfField(false);
		vTaskDelay(333 / portTICK_RATE_MS);
		if (scanIsOn && rssiPassRange > -83)
			rssiPassRange -= 2;
//		nfc.setRfField(true);
//		if(nfc.tagPresent(NFC_RF_PULSE_WIDTH)) {
//			foundSomething = true;
////			if (! xQueueSend(rfFieldQueue, &foundSomething, 100 / portTICK_RATE_MS)) {
////					ESP_LOGE(INDLOG, "Failed to send to indicationLedQueue...");
////			}
//			readTagRecords();
//			vTaskDelay(20 / portTICK_RATE_MS);
//		}
//		nfc.setRfField(false);
//		vTaskDelay(333 / portTICK_RATE_MS);
////		nfc.setRfField(true);
//		if(nfc.tagPresent(NFC_RF_PULSE_WIDTH)) {
//			foundSomething = true;
////			if (! xQueueSend(rfFieldQueue, &foundSomething, 100 / portTICK_RATE_MS)) {
////					ESP_LOGE(INDLOG, "Failed to send to indicationLedQueue...");
////			}
//			readTagRecords();
//			vTaskDelay(20 / portTICK_RATE_MS);
//		}
//		nfc.setRfField(false);
//		vTaskDelay(333 / portTICK_RATE_MS);
	}
}

void nfcHandleTask(void *pvParameters)
{

	esp_err_t err;
	timer_event_t evt;
//	xTaskCreatePinnedToCore(nfcRfFieldTask, "RFfunc", 2048, NULL, 2, NULL, 1);
	while (1) {
		ESP_LOGD(TIMERLOG, "I'm checking if there ISR request..");

		if (xQueueReceiveFromISR(timer_queue, &evt, NULL)) {
			if (gattc_conn)
				checkMeasurementDescriptor();
			/* Print information that the timer reported an event */
			if (evt.type == TEST_WITHOUT_RELOAD) {
				ESP_LOGD(TIMERLOG, "\n    Example timer without reload");
				// need to send to write sample period to BioBeat
#ifdef REAL_TIME_TEST_PROG
				if (gatts_conn) {
					passTheNotify = true;
					ESP_LOGI(TIMERLOG, "passTheNotify = true.");
				}
#endif
			} else if (evt.type == TEST_WITH_RELOAD) {
				ESP_LOGD(TIMERLOG, "\n    Example timer with auto reload");

				ESP_LOGI(TIMERLOG, "it past %d minutes from last restart", ++timePastMinutes);
//					passTheNotify = true;      // -----------> every minute
#ifdef REAL_TIME_TEST_PROG
				if (timePastMinutes % 2 == 0 && !gatts_conn)
#else
					if (timePastMinutes % 2 == 0)
#endif
					passTheNotify = true;

				updateTimeParams();
//				nvsTimeWrite(); // inside of updateTimeParams()

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
//		ESP_LOGI(NFCLOG, "\nScan a NFC tag\n");
//		char desString[50];
//
//		bool tmpBoolTest;
//		if (xQueueReceive(rfFieldQueue, &tmpBoolTest, 100 / portTICK_RATE_MS)) //xQueueReceive(rfFieldQueue, &rxIndicationStatus, 100 / portTICK_RATE_MS)nfc.tagPresent(250)
//		if(nfc.tagPresent(250))
//		{
//			readTagRecords();
//		  }
		vTaskDelay(500 / portTICK_RATE_MS);
	}
}


void readTagRecords()
{
	char desString[50];

	if (checkIfHce() && !sendingRecordsToSmartphone) { // if sendingRecordsToSmartphone so we shouldn't get another smartphone! in order to avoid disconnect conflicts
		// peer to peer connection -> APDU
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

		ESP_LOG_BUFFER_HEX_LEVEL(NFCLOG, response, responseLength, ESP_LOG_DEBUG);

		char deviceNameFromNfc[30] = {0};
		strcpy(deviceNameFromNfc, (char*)response);
		ESP_LOG_BUFFER_CHAR_LEVEL(NFCLOG, deviceNameFromNfc, responseLength, ESP_LOG_DEBUG);

		if (responseLength) {
			if (gatts_conn /*&& sendingRecordsToSmartphone*/) { // need to ensure it is another phone ---> FIXED above (if !sendingRecordsToSmartphone)
				ESP_LOGI(GATTS_TAG, "Disconnecting from smartphone device. MAC Address:");
				esp_log_buffer_hex(GATTS_TAG, smartphoneMacAddress, MAC_SIZE);
				esp_ble_gap_disconnect(smartphoneMacAddress);
				vTaskDelay(50 / portTICK_RATE_MS);
			}

	//		if (!gattc_conn && openBleConnection){ // stop open connection in order to be able to advertise
	//			ESP_LOGI(GATTC_TAG, "Stops open connection  device");
	//			esp_ble_gap_disconnect(bbBtMacAddrs);
	//			resetOpenBleConnVars();
	//		}

			if (deviceNameFromNfc[2] == ':' /*&& sendingRecordsToSmartphone*/) {
				deviceNameFromNfc[(int)responseLength-2] = 0;
				deviceNameFromNfc[(int)responseLength-1] = 0;
				for (int i = 0; i < responseLength - 2; i++)
					deviceNameFromNfc[i] = (char)response[i];
				if (scanIsOn)
					esp_ble_gap_stop_scanning();
				if (!gatts_conn) {
					simulatorDevice = false;
					setAdvertisingName(deviceNameFromNfc);
					BEEP;
				}
			}
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
				for (int i = 0; i < 6/*MAC_SIZE*/; i++) {
					measureDeviceBtMacAddrs[i] = payload[payloadLength - i - 1];
//							printf("%2X:", measureDeviceBtMacAddrs[i]);
				}
//						printf("\n");
//						esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE);
//						if (memcmp(measureDeviceBtMacAddrs, siliconLabsFirstMacBytes, MAC_SIZE / 2) == 0 ||
//								memcmp(measureDeviceBtMacAddrs, espressifFirstMacBytes, MAC_SIZE / 2) == 0) {
//							BEEP;
//							if (conn_device_a == false) {
//								conn_device_a = true;
//								ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");  // --------> here I can make a record of connection if needed
//		//						esp_ble_gap_stop_scanning();
//								esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, BLE_ADDR_TYPE_PUBLIC, true);
//							}
//						}
			}
			else if (record.getTnf() == 1 && record.getType() == "T") {
				char nfcScanResult[NFC_DECODE_SIZE];
				for (int szPos=0, i = 0; szPos < payloadLength; szPos++)
				{
					if (payload[szPos] >= 0x30 && payload[szPos] <= 0x39) {
						nfcScanResult[i] = payload[szPos];
						ESP_LOGI(NFCLOG, "the medical event decode is: %d (%c) %d", payload[szPos], payload[szPos], szPos);
						i++;
					}
					if (payload[szPos] >= 0x42 && payload[szPos] <= 0x44) {
						nfcScanResult[i] = payload[szPos];
						ESP_LOGI(NFCLOG, "the medical event decode is: %d (%c)", payload[szPos], payload[szPos]);
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
				ESP_LOGI(NVSLOG, "nfcScanResult = %s...", nfcScanResult);
				if (memcmp(nfcScanResult, "DDB", 3) == 0) {
					ESP_LOGI(NVSLOG, "Deinit Ble then erase Flash memory (NVS)...");
					if (gatts_conn || gattc_conn)
						deinit_ble();
					if (gatts_conn)
						timer_pause(TIMER_GROUP_0, TIMER_0);
					timer_pause(TIMER_GROUP_0, TIMER_1);
					FLICK; // here is 500 ms delay inside...
					vTaskDelay(250 /portTICK_RATE_MS);
//					vTaskSuspendAll();
					esp_err_t nvsErr = nvs_erase_all(storageNvsHandle);
					if (nvsErr == ESP_OK) {
						nvs_commit(storageNvsHandle);
//								rtcLastFromResetInSeconds = 0;
						ESP_LOGI(NVSLOG, "Flash memort (NVS) is erased");  // need to handle timing bug after erasing and resatrt
						BEEP;
						BEEP;
						BEEP;
						FLICK;
						esp_restart();
					}
					else
						ESP_LOGE(NVSLOG, "Flash memort (NVS) erase error! Code = 0x%x", nvsErr);
				}
				if (tagValidation(i_nfcScanResult) && !sendingMeasurementsToSmartphone) {

					uint8_t tmpData = 1;
					GenericRecord record(i_nfcScanResult, &tmpData);
					addToRecordList(record, true, NULL); // addToNumericRecordList(numRec);
#ifdef OLD_INDICATIONS
					BEEP; // beep here because printing takes time
//							FLICK_T;
#else
					sendIndicationStatusToQueue((indicationStatus_t)IND_NTAG_SCAN_SUCCESS);
#endif
					printRecordList(11); // printNumericRecordList();
					automaticRecordUpdate(i_nfcScanResult);
					if (gatts_conn) {
						sendRecordByIndicate(record);
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

}

bool checkIfHce()
{
	ESP_LOGI(NFCLOG, "Found something!");

	uint8_t selectApdu[] = { 0x00, /* CLA */
							  0xA4, /* INS */
							  0x04, /* P1  */
							  0x00, /* P2  */
							  0x06, /* Length of AID  */
							  0xF2, 0x22, 0x22, 0x22, 0x22/* AID defined on Android App */
							  /*0x00   Le  */ };

	uint8_t responseLength = 32;
	uint8_t response[32];

	return nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
}

bool tagValidation(int tagNum)
{
	if (tagNum < 22 || tagNum > 59)
		return false;
	if (tagNum >= 40 && tagNum <= 59)
		return true;
	if (tagNum >= 30 && tagNum <= 35)
		return true;
	switch (tagNum)
	{
		case  22:
		case  23:
		case  27:
			return true;
			break;
		default:
			break;
	}
	return false;
}


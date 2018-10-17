/*
 * nfcManage.cpp
 *
 *  Created on: May 16, 2018
 *      Author: albert
 */
#include "mainHeader.h"
#include "nfcManage.h"
#include "dbManage.h"
#include "timerManage.h"
#include "ioManage.h"
#include "nvsManage.h"


PN532_SPI pn532spi(SPI, 5);
NfcAdapter nfc = NfcAdapter(pn532spi);

void nfcHandleTask(void *pvParameters) {

	esp_err_t err;
	while(nfc.begin()) {
		ESP_LOGE(NFCLOG, "Connect NFC Module to continue!!!");
		vTaskDelay(300 / portTICK_RATE_MS);
	}
	uint8_t nfcReadAttempt = 0;
	// Read from nvs the value from key minutes_counter
	// Print the value and reset it
	Serial.begin(115200);
	timer_event_t evt; //moved to be global
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
//					passTheNotify = true;

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

				uint8_t responseLength = 32;
				uint8_t response[32];

				nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
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

//					ESP_LOGI(GATTC_TAG, "Trying to establish BLE connection. MAC address:");
//					esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, sizeof(measureDeviceBtMacAddrs));
//					esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
				}

//				strcpy((char*)remote_device_name[1], (char*)response);
//				ESP_LOG_BUFFER_HEX_LEVEL(GATTC_TAG, remote_device_name[1], responseLength, ESP_LOG_DEBUG);
//				remote_device_name[1][(int)responseLength-2] = 0;
//				remote_device_name[1][(int)responseLength-1] = 0;
////				for (int i = 0; i < responseLength; i++)
////					remote_device_name[1][i] = (char)response[i];
//				ESP_LOGD(GATTC_TAG, "Trying to establish BLE connection. Response Length: %d, Device name: %s", responseLength, remote_device_name[1]);
//				ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");

				BEEP;

				// Pluster
//				if (conn_device_a) {
//					ESP_LOGI(GATTC_TAG, "Disconnecting from measurement device");
//					esp_ble_gap_disconnect(measureDeviceBtMacAddrs);
//					vTaskDelay(50 / portTICK_RATE_MS);
//				// next pluster after sending record 99
//				}
//				if (conn_device_b) { // if another smartphone captured, I should disconnect from current
//					ESP_LOGI(GATTC_TAG, "Disconnecting from smartphone device");
//					esp_ble_gap_disconnect(smartphoneMacAddress);
//					vTaskDelay(50 / portTICK_RATE_MS);
//				}
//				// start scan to connect smartphone
//				if (!scanIsOn) {
//					start_scan(5);
//				}
//				else{
//					esp_ble_gap_stop_scanning();
//					vTaskDelay(50 / portTICK_RATE_MS);
//					start_scan(5);
//				}


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
//								rtcLastFromResetInSeconds = 0;
								ESP_LOGI(NVSLOG, "Flash memort (NVS) is erased");  // need to handle timing bug after erasing and resatrt
								BEEP;
								BEEP;
								BEEP;
								esp_restart();
							}
							else
								ESP_LOGE(NVSLOG, "Flash memort (NVS) erase error!");
						}
//						if (i_nfcScanResult >= 20 && i_nfcScanResult < 60) {
//							record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
//							addToRecordList(numRec, true, NULL); // addToNumericRecordList(numRec);
//							BEEP; // beep here because printing takes time
//							printRecordList(); // printNumericRecordList();
//
//							if (conn_device_b) {
//								bleSendRecord(blePreSendRecord(numRec, recordListCounter - 1));
//							}
//
//
//						}
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

		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

bool checkIfHce()
{
	ESP_LOGI(NFCLOG, "Found something!");

	uint8_t selectApdu[] = { 0x00, /* CLA */
							  0xA4, /* INS */
							  0x04, /* P1  */
							  0x00, /* P2  */
							  0x05, /* Length of AID  */
							  0xF2, 0x22, 0x22, 0x22, 0x22/* AID defined on Android App */
							  /*0x00   Le  */ };
	uint8_t responseLength = 32;
	uint8_t response[32];

	return nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
}

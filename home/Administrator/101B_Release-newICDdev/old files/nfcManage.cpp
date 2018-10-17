/*
 * nfcManage.cpp
 *
 *  Created on: May 16, 2018
 *      Author: albert
 */
#include "mainHeader.h"
#include "nfcManage.h"
#include "bleManage.h"
#include "dbManage.h"
#include "timerManage.h"
#include "ioManage.h"
#include "nvsManage.h"

void nfcHandleTask(void *pvParameters) {

	esp_err_t err;

    while (runNfcFunction) {

    	// check for ISR request
    	checkForTimerISR();
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
										  0x05, /* Length of AID  */
										  0xF2, 0x22, 0x22, 0x22, 0x22/* AID defined on Android App */
										  /*0x00   Le  */ };
    			uint8_t responseLength = 32;
				uint8_t response[32];

				nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

				ESP_LOGD(NFCLOG, "I'm in HCE code.");
				ESP_LOGD(NFCLOG, "responseLength: %d", responseLength);

				ESP_LOG_BUFFER_HEX_LEVEL(NFCLOG, response, responseLength, ESP_LOG_DEBUG);

//				if (response[2] == ':') { --------------------------------> not in use....
//					uint8_t tmpLow = 0, tmpHigh = 0;
//					for (int i = 0, j = 0; i < responseLength && j < 6; i += 3) {
//						if (response[i] <= 'f' && response[i] >= 'a') {
//							tmpHigh =  0xA + response[i] - 'a';
//						}
//						else if (response[i] <= 'F' && response[i] >= 'A')
//							tmpHigh = 0xA + response[i] - 'A';
//						else if (response[i] <= '9' && response[i] >= '0')
//							tmpHigh = response[i] - '0';
//
//						if (response[i+1] <= 'f' && response[i+1] >= 'a') {
//							tmpLow =  0xA + response[i+1] - 'a';
//						}
//						else if (response[i+1] <= 'F' && response[i+1] >= 'A')
//							tmpLow = 0xA + response[i+1] - 'A';
//						else if (response[i+1] <= '9' && response[i+1] >= '0')
//							tmpLow = response[i+1] - '0';
//
//						measureDeviceBtMacAddrs[j++] = (tmpHigh << 4) | (tmpLow & 0x0f);
//					}

//					ESP_LOGI(GATTC_TAG, "Trying to establish BLE connection. MAC address:");
//					esp_log_buffer_hex(GATTC_TAG, measureDeviceBtMacAddrs, sizeof(measureDeviceBtMacAddrs));
//					esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/, true);
//				}

				connectToSmartphoneProcedure((char*)response, responseLength);

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

						connectToMeasurementDevice(measureDeviceBtMacAddrs);
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
						if (i_nfcScanResult == 2) {
							if (eraseStorageNvsHandle() == ESP_OK) {
								ESP_LOGI(NVSLOG, "Flash memory (NVS) is erased");  // need to handle timing bug after erasing and resatrt
								BEEP;
								BEEP;
								BEEP;
								esp_restart();  // need to handle timing issues...
							}
							else
								ESP_LOGE(NVSLOG, "Flash memort (NVS) erase error!");
						}
						record_inst numRec = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, i_nfcScanResult, 1, NULL); // numericRecord_inst numRec = makeNumericRecord(timePastMinutes, i_nfcScanResult, 1);
						addToRecordList(numRec, true, NULL);//, recordList, &recordListCounter); // addToNumericRecordList(numRec);
						printRecordList();//recordList, &recordListCounter); // printNumericRecordList();

						if (getConnDeviceB()) {
							bleSendRecord(blePreSendRecord(numRec, recordListCounter - 1));
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
			  BEEP;

			  // nfc was read, reset nfcReadAttempt ----------------> not in use
//			  nfcReadAttempt = 0;
    	  }
    	  //vTaskDelay(1000 / portTICK_RATE_MS);

//    	if (nfcReadAttempt++ >= 2 && !conn_device_b) { // after 3 attempts, go to sleep again    ---> not in use
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
//    			timePastMinutes += TIMER_INTERVAL1_SEC / NUM_OF_SEC_DEF;
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

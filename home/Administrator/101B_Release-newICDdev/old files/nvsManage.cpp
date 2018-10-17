/*
 * nvsManage.cpp
 *
 *  Created on: May 14, 2018
 *      Author: albert
 */
#include "nvsManage.h"

esp_err_t eraseStorageNvsHandle() {
	nvs_erase_all(storageNvsHandle);
	return nvs_commit(storageNvsHandle);
}

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

void nvsTimeCheck()
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
			ESP_LOGE(NVSLOG, "Error (%d) reading TimeCheck!\n", err);
	}
}

void nvsReadRecordListCounter()//uint32_t* recordListCounter)
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
			ESP_LOGE(NVSLOG, "Error (%d) reading Record List Counter!\n", err);
	}

}

void nvsWriteRecordListCounter()//uint32_t* recordListCounter)
{
// Write
	esp_err_t err;
	ESP_LOGD(NVSLOG, "Updating record list counter(recListCount) in NVS ... ");
	err = nvs_set_u32(storageNvsHandle, "recListCount", recordListCounter);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) Writing(set) Record List Counter!\n", err);
		ESP_ERROR_CHECK (err);
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
		ESP_LOGE(NVSLOG, "Error (%d) Writing(commit) Record List Counter!\n", err);
	}
	else
		ESP_LOGD(NVSLOG, "Done");

}

void nvsReadRecordList(uint32_t recordNum)//, record_inst* recordList, uint32_t* recordListCounter)
{
	esp_err_t err;

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
			if (tempStr[2] == 0) { // need to add all characteristic types
				record = makeRecord(1, timePast_tmp, type_tmp, NULL, tempStr + 3);
			}
			else {
				uint32_t data_tmp = 0;
				for (int i = 0; i < NUMERIC_BLE_BUFF_SIZE - 3; i++)
					data_tmp |= tempStr[3 + i] << (24 - 8*i);
				record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
			}
			addToRecordList(record, false, recordNum);//, recordList, recordListCounter);
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
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "There is no record #%d", recordNum);
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading Record List!\n", err);
	}
}

void nvsWriteRecordList(uint32_t recordNum)//, record_inst* recordList)
{
// Write
	esp_err_t err;
	ESP_LOGD(NVSLOG, "Updating record %d in NVS ... ", recordNum);

	// generate keyName = record# (char array)
	char keyName[MAX_KEY_LEN] = {0};
	String keyNameString = "record" + recordNum;
	keyNameString.toCharArray(keyName, MAX_KEY_LEN);

	// generate tempStr = recordList[recordNum] to save blob in nvs
	if (recordList[recordNum].recordType == NUMERIC_RECORD_TYPE) {
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
			ESP_LOGE(NVSLOG, "Error (%d) Writing(set) Record List!\n", err);
			ESP_ERROR_CHECK (err);
		}
		else
			ESP_LOGD(NVSLOG, "Done");
	}
	else {
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
			ESP_LOGE(NVSLOG, "Error (%d) Writing(set) Record List!\n", err);
			ESP_ERROR_CHECK (err);
		}
		else
			ESP_LOGD(NVSLOG, "Done");
	}


	// Commit written value.
	// After setting any values, nvs_commit() must be called to ensure changes are written
	// to flash storage. Implementations may write to storage at other times,
	// but this is not guaranteed.
	ESP_LOGD(NVSLOG, "Committing %s updates in NVS ... ", keyName);
	err = nvs_commit(storageNvsHandle);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) Writing(commit) Record List!\n", err);
		ESP_ERROR_CHECK (err);
	}
	else
		ESP_LOGD(NVSLOG, "Done");

}

void nvsWriteMinutesCounter()//uint32_t timePastMinutes)
{
	esp_err_t err;
	// Write
	ESP_LOGD(NVSLOG, "Updating minutes counter in NVS ... ");
//				timePastMinutes++;
	err = nvs_set_u32(storageNvsHandle, "minutes_counter", timePastMinutes);
//	err = nvs_commit(storageNvsHandle);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) Writing(set) Minutes Counter!\n", err);
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
		ESP_LOGE(NVSLOG, "Error (%d) Writing(commit) Minutes Counter!\n", err);
	}
	else
		ESP_LOGD(NVSLOG, "Done");
}

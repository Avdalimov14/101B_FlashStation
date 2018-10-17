/*
 * nvsManage.cpp
 *
 *  Created on: Jun 27, 2018
 *      Author: albert
 */

#include "nvsManage.h"
#include "Arduino.h"
#include "dbManage.h"

#define NVSLOG "NVS LOG"
#define MAX_KEY_LEN 15
//#define MAX_RECORD_IN_NVS 400


nvs_handle storageNvsHandle;
uint32_t MAX_RECORD_IN_NVS;

void nvsWriteBbMacAddress(uint8_t* address) {
	esp_err_t err;

	size_t macSizet = 6;
	// Write
	ESP_LOGD(NVSLOG, "Updating BioBeat MAC Address in NVS ... ");
//				timePastMinutes++;
	err = nvs_set_blob(storageNvsHandle, "mac_add", address, macSizet);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			ESP_ERROR_CHECK (err);
//			if (err == 4357)
//				haltProgram();
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

bool nvsReadBbMacAddress(uint8_t* address) {
	esp_err_t err;

	// Write
	ESP_LOGD(NVSLOG, "Reading BioBeat MAC Address in NVS ... ");
//				timePastMinutes++;
//	uint8_t address[6];
	size_t macSizet = 6;

	err = nvs_get_blob(storageNvsHandle, "mac_add", address, &macSizet);
	switch (err) {
		case ESP_OK: {
			ESP_LOGD(NVSLOG, "Done");
			ESP_LOGD(NVSLOG, "MacAddress fetched from memory:");
			esp_log_buffer_hex(NVSLOG, address, 6);
			return true;
		}
			break;
		case ESP_ERR_NVS_NOT_FOUND: {
			ESP_LOGI(NVSLOG, "There is NO BioBeat Mac Address");
			break;
		}
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
			break;
	}
	return false;
}

void init_nvs()
{
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

//	MAX_RECORD_IN_NVS = 500;
	nvsReadMaxRecordParam();

}

void nvsTimeRead()
{
	esp_err_t err;

	// Read
	ESP_LOGI(NVSLOG, "Reading minutes and seconds counter from Flash memory (NVS)... ");
//		int32_t minutes_counter = 0; // value will default to 0, if not set yet in NVS
	err = nvs_get_u32(storageNvsHandle, "mins_counter", &timePastMinutes);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "Minutes counter = %d", timePastMinutes);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "Minutes counter is not initialized yet!	Value will default to 0 (minutes)");
			timePastMinutes = 0;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}

	err = nvs_get_u32(storageNvsHandle, "secs_counter", (uint32_t*)&timePastSeconds);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "Seconds counter = %d", timePastSeconds);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "Seconds counter is not initialized yet!	Value will default to 0 (minutes)");
			timePastSeconds = 0;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}
}

void nvsTimeWrite()
{
	esp_err_t err;

	if (recordListCounter < MAX_RECORD_IN_NVS) {
		// Write
		ESP_LOGD(NVSLOG, "Updating minutes counter in NVS ... ");
	//				timePastMinutes++;
		err = nvs_set_u32(storageNvsHandle, "mins_counter", timePastMinutes);
	//					err = nvs_commit(storageNvsHandle);
		if (err != ESP_OK) {
			ESP_LOGE(NVSLOG, "Failed!");
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
		}
		else
			ESP_LOGD(NVSLOG, "Done");

		ESP_LOGD(NVSLOG, "Updating seconds counter in NVS ... ");
	//				timePastMinutes++;
		err = nvs_set_u32(storageNvsHandle, "secs_counter", (uint32_t)timePastSeconds);
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

void nvsReadRamRecordListLimit()
{
	esp_err_t err;

	// Read
	ESP_LOGI(NVSLOG, "Reading ramRecordListLimit from NVS ... ");
	err = nvs_get_u8(storageNvsHandle, "ramLimit", &ramRecordListLimit);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "ramRecordListLimit = %d", ramRecordListLimit);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "ramRecordListLimit is not initialized yet! Value will default to 0 (records)");
			ramRecordListLimit = 0;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}
}

void nvsWriteRamRecordListLimit()
{
// Write
	esp_err_t err;

	ESP_LOGD(NVSLOG, "Updating ramRecordListLimit in NVS ... ");
	err = nvs_set_u8(storageNvsHandle, "recListCount", ramRecordListLimit);
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

void nvsReadMaxRecordParam()
{
	esp_err_t err;

	// Read
	ESP_LOGI(NVSLOG, "Reading MAX_RECORD_IN_NVS from NVS ... ");
	err = nvs_get_u32(storageNvsHandle, "MaxRecParam", &MAX_RECORD_IN_NVS);
	switch (err) {
		case ESP_OK:
			ESP_LOGI(NVSLOG, "Done");
			ESP_LOGI(NVSLOG, "MAX_RECORD_IN_NVS = %d", MAX_RECORD_IN_NVS);
			break;
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGI(NVSLOG, "MAX_RECORD_IN_NVS is not initialized yet! Value will default to 500 (records)");
			MAX_RECORD_IN_NVS = 500;
			break;
		default :
			ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
	}
}

void nvsWriteMaxRecordParam()
{
// Write
	esp_err_t err;

	ESP_LOGD(NVSLOG, "Updating MAX_RECORD_IN_NVS in NVS ... ");
	err = nvs_set_u32(storageNvsHandle, "MaxRecParam", MAX_RECORD_IN_NVS);
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) writing!\n", err);
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
		char keyName[MAX_KEY_LEN] = "record";
		char keyNameNumber[10];
		itoa(recordNum, keyNameNumber, 10);
	//	char *keyNameString = "record";// + recordNum;
		strcat(keyName, keyNameNumber);

		uint8_t tempStr[MAX_REC_SIZE] = {0};
		size_t tempStrLen = MAX_REC_SIZE;

		err = nvs_get_blob(storageNvsHandle, keyName, tempStr, &tempStrLen);
		switch (err) {
			case ESP_OK: {
				ESP_LOGD(NVSLOG, "Done");
				ESP_LOGD(NVSLOG, "record[%d] fetched from nvs memory:", recordNum);
				ESP_LOG_BUFFER_HEX_LEVEL(NVSLOG, tempStr, tempStrLen, ESP_LOG_DEBUG);
// oldICD
//				record_inst record;
//				uint16_t timePast_tmp = (tempStr[0] << 8) & 0xFF00;
//				timePast_tmp |= tempStr[1];
//				uint8_t type_tmp = tempStr[2];
//				if (type_tmp == 0) {
//					timePast_tmp = 2;
//					type_tmp = 84;
//					uint32_t data_tmp = 0;
//					data_tmp = 29;
//					record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
//				}
//				if (tempStr[2] == 0) { // need to add all characteristic types
//					record = makeRecord(1, timePast_tmp, type_tmp, NULL, tempStr + 3);
//				}
//				else {
//					uint32_t data_tmp = 0;
//					for (int i = 0; i < NUMERIC_BLE_BUFF_SIZE - 3; i++)
//						data_tmp |= tempStr[3 + i] << (24 - 8*i);
//					record = makeRecord(0, timePast_tmp, type_tmp, data_tmp, NULL);
//				}
//				addToRecordList(record, false, recordNum);
// newICD
				GenericRecord record;// = new GenericRecord();
				record.setBytesRecord(tempStr);
				addToRecordList(record, false, recordNum);

			}
				break;
			case ESP_ERR_NVS_NOT_FOUND: {
				ESP_LOGI(NVSLOG, "There is no record #%d", recordNum);
				ESP_LOGI(NVSLOG, "making false record in order to avoid crashing");
				uint8_t data_tmp = 29;
				GenericRecord record(2, &data_tmp);
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
	if (ramRecordListLimit)
		recordNum += MAX_RECORD_IN_NVS;
// Write
	esp_err_t err;
	ESP_LOGD(NVSLOG, "Updating record %d in NVS ... ", recordNum);

	// generate keyName = record# (char array)
	char keyName[MAX_KEY_LEN] = "record";
	char keyNameNumber[10];
	itoa(recordNum, keyNameNumber, 10);
//	char *keyNameString = "record";// + recordNum;
	strcat(keyName, keyNameNumber);

	GenericRecord& record = getRecord(recordNum);

	// generate tempRec = recordList[recordNum] to save blob in nvs
	uint8_t tempRec[MAX_REC_SIZE] = {0};
	record.getBytesRecord(tempRec);
	ESP_LOG_BUFFER_HEX_LEVEL(NVSLOG, tempRec, record.getRecordSize(), ESP_LOG_DEBUG);
	err = nvs_set_blob(storageNvsHandle, keyName, tempRec, record.getRecordSize());
	if (err != ESP_OK) {
		ESP_LOGE(NVSLOG, "Failed!");
		ESP_LOGE(NVSLOG, "Error (%d) reading!\n", err);
//			ESP_ERROR_CHECK (err);
//			if (err == 4357)
//				haltProgram();
	}
	else
		ESP_LOGD(NVSLOG, "Done");

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




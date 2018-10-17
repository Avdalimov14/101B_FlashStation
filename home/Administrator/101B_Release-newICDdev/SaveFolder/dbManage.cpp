/*
 * dbManage.c
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#include "dbManage.h"

//recordList = (record_inst*)malloc(300 * RECORD_SIZE);
//int RECORD_SIZE = sizeof(record_inst);
//recordList = (record_inst*)malloc(300 * RECORD_SIZE);
uint32_t recordListCounter;// = 0;
uint32_t recordSendCoutner;// = 0;
uint32_t mesurementSendCounter;// = 0;

bool sendingRecordsToSmartphone;// = false;
bool sendingMeasurementsToSmartphone;// = false;

void init_db()
{
	int RECORD_SIZE = sizeof(record_inst);
	recordList = (record_inst*)malloc(300 * RECORD_SIZE);
	recordListCounter = 0;
	recordSendCoutner = 0;
	mesurementSendCounter = 0;

	sendingRecordsToSmartphone = false;
	sendingMeasurementsToSmartphone = false;

}

record_inst* getRecord(int recordNum)
{
	return &recordList[recordNum];
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

bool addToRecordList (record_inst record)//, bool writeToNvs, uint32_t recordNum)
{
	if (recordListCounter >= 300) {

//		record_inst* tmp_recordList;
//		recordList = (record_inst*)realloc(recordList, RECORD_SIZE * (recordListCounter + 300));
//		if (recordList == NULL){
//			ESP_LOGE("MEMORY", "couldn't re-allocate record list for another 300");
//			ESP_LOGE("MEMORY", "		FATAL MEMORY ERRO!!		");
//			return false; // re-allocation failed
//		}

	}

//	if (recordListCounter == 300)
//		longBeep();


//	if (writeToNvs) {
		recordList[recordListCounter++] = record;
//		nvsWriteRecordList(recordListCounter - 1);
//		nvsWriteRecordListCounter();
//	}
//	else
//		recordList[recordNum] = record;
	return true;

}

void printRecordList()
{
	ESP_LOGI("Records List", "Printing records list:");

	for (int i = 0; i < recordListCounter; i++) {

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
//
//recordType_t blePreSendRecord(record_inst record, int i)
//{
//	if (record.recordType == NUMERIC_RECORD_TYPE){
//		bleNumericSendBuffer[0] = (record.recordInfo.numericRecord->timePast >> 8) & 0xFF;
//		bleNumericSendBuffer[1] = record.recordInfo.numericRecord->timePast & 0xFF;
//		bleNumericSendBuffer[2] = record.recordInfo.numericRecord->type;
//		for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
//			bleNumericSendBuffer[i] = (record.recordInfo.numericRecord->data >> (24 - 8*(i-3))) & 0xFF;
//		}
//
//		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
//		strcat(tmpBleSendBuffer_TAG, "[");
//		char tmpItoa[5];
//		itoa(i, tmpItoa, 10);
//		strcat(tmpBleSendBuffer_TAG, tmpItoa);
//		strcat(tmpBleSendBuffer_TAG, "]");
//
//		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleNumericSendBuffer, NUMERIC_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
//		return record.recordType;
//	}
//	else {
//		bleCharSendBuffer[0] = (record.recordInfo.charRecord->timePast >> 8) & 0xFF;
//		bleCharSendBuffer[1] = record.recordInfo.charRecord->timePast & 0xFF;
//		bleCharSendBuffer[2] = record.recordInfo.charRecord->type;
//		for (int i = 3; i < CHAR_BLE_BUFF_SIZE; i++) {
//			bleCharSendBuffer[i] = (record.recordInfo.charRecord->data[i-3]);
//		}
//
//		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
//		strcat(tmpBleSendBuffer_TAG, "[");
//		char tmpItoa[5];
//		itoa(i, tmpItoa, 10);
//		strcat(tmpBleSendBuffer_TAG, tmpItoa);
//		strcat(tmpBleSendBuffer_TAG, "]");
//
//		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleCharSendBuffer, CHAR_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
//		return record.recordType;
//	}
//}
//
//void bleSendRecord(recordType_t recordType)
//{
//	if (gl_profile_tab[PROFILE_B_APP_ID].gattc_if != ESP_GATT_IF_NONE) {
//		if (recordType == NUMERIC_RECORD_TYPE)
//			esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//										  NUMERIC_BLE_BUFF_SIZE,
//										  bleNumericSendBuffer,
//										  ESP_GATT_WRITE_TYPE_NO_RSP,
//										  ESP_GATT_AUTH_REQ_NONE);
//		else
//			esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//										  CHAR_BLE_BUFF_SIZE,
//										  bleCharSendBuffer,
//										  ESP_GATT_WRITE_TYPE_NO_RSP,
//										  ESP_GATT_AUTH_REQ_NONE);
//	}
//}
//
//void bleSendRecordList()
//{
////	for (int i = 0; i < recordListCounter; i++) {
////		bleSendRecord(blePreSendRecord(recordList[i], i));
////		vTaskDelay(15 / portTICK_RATE_MS);
////		if (i % 10 == 0) {
////			vTaskDelay(150 / portTICK_RATE_MS);
//////			break;
////		}
////		else if (i == 50) {
////			vTaskDelay(1500 / portTICK_RATE_MS);
////			break;
////		}
////	}
//	recordSendCoutner = 0;
//	sendingRecordsToSmartphone = true;
//	bleSendRecord(blePreSendRecord(recordList[recordSendCoutner], recordSendCoutner));
//	recordSendCoutner++;
////	// send type 99 - record list counter
////	record_inst recordListCounterRecord;
////	recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
////	blePreSendRecord(recordListCounterRecord, recordListCounter);
////	// Client need to approve / authenticate that the phone got all records
////	// bleSendNumericRecord(numericRecordListCounterRecord);
////	char tmpRecordBuffer_TAG[25] = "Record #";
////	char tmpItoa[5];
////	itoa(recordListCounter, tmpItoa, 10);
////	strcat(tmpRecordBuffer_TAG, tmpItoa);
////	strcat(tmpRecordBuffer_TAG, " - ");
////	ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d, type: %03d, data: %d", recordListCounterRecord.recordInfo.numericRecord->timePast, recordListCounterRecord.recordInfo.numericRecord->type, recordListCounterRecord.recordInfo.numericRecord->data);
////	esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
////								  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
////								  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
////								  NUMERIC_BLE_BUFF_SIZE,
////								  bleNumericSendBuffer,
////								  ESP_GATT_WRITE_TYPE_NO_RSP,
////								  ESP_GATT_AUTH_REQ_NONE);
////	ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");
//
////	for (int i = 51; i < recordListCounter; i++) {
////		bleSendRecord(blePreSendRecord(recordList[i], i));
////		vTaskDelay(50 / portTICK_RATE_MS);
////		if (i % 10 == 0) {
//////			vTaskDelay(150 / portTICK_RATE_MS);
//////			break;
////		}
////		else if (i == 50) {
//////			vTaskDelay(1500 / portTICK_RATE_MS);
////			break;
////		}
////	}
//
////	// Pluster
////	esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs, BLE_ADDR_TYPE_PUBLIC, true);
//}
//void bleSendMeasurements()
//{
//	mesurementSendCounter = 0;
//	sendingMeasurementsToSmartphone = true;
////	for (int j = 1; j < 5; j++){
//		bleSendRecord(blePreSendRecord(recordList[recordListCounter - 1], recordListCounter - 1));
////	}
//	mesurementSendCounter++;
//}

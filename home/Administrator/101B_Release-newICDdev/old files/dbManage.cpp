/*
 * dbManage.c
 *
 *  Created on: Apr 24, 2018
 *      Author: albert
 */

#include "dbManage.h"
#include "bleManage.h" // need for the struct
#include "mainHeader.h"
#include "nvsManage.h"

//extern static uint32_t recordListCounter; // can be transfer as argument but I prefer to have extern keyword in my code
//extern static uint32_t timePastInMinutes;
// possible to add more extern but as wrote in internet it is not rcommended
//extern static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM]  ------------------------------------------------------------> #include "bleManage.h"
extern struct gattc_profile_inst gl_profile_tab[PROFILE_NUM];

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

bool addToRecordList (record_inst record, bool writeToNvs, uint32_t recordNum)//, record_inst* recordList, uint32_t* recordListCounter)
{
	if (recordListCounter >= maxCurrentRecordListCounter) {
		maxCurrentRecordListCounter *= 2;
		recordList = (record_inst*)realloc(recordList, RECORD_SIZE * maxCurrentRecordListCounter);
		if (recordList == NULL){
			ESP_LOGE("MEMORY", "couldn't re-allocate record list for another 300");
			ESP_LOGE("MEMORY", "		FATAL MEMORY ERRO!!		")
			return false; // re-allocation failed
		}
	}

	if (writeToNvs) {
		recordList[(recordListCounter)++] = record;
		nvsWriteRecordList((recordListCounter) - 1);//, &record);
		nvsWriteRecordListCounter();//recordListCounter);
	}
	else
		recordList[recordNum] = record;
	return true;

}

void printRecordList()//record_inst* recordList, uint32_t* recordListCounter)
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

recordType_t blePreSendRecord(record_inst record, int i)//, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer)
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

void bleSendRecord(recordType_t recordType)//, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer)
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

void bleSendRecordList()//record_inst* recordList, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer, uint32_t* recordListCounter, uint32_t timePastInMinutes)
{
	for (int i = 0; i < recordListCounter; i++) {
		bleSendRecord(blePreSendRecord(recordList[i], i));//, bleNumericSendBuffer, bleCharSendBuffer), bleNumericSendBuffer, bleCharSendBuffer);
		vTaskDelay(50 / portTICK_RATE_MS);
		if (i == 50)
//			vTaskDelay(1000 / portTICK_RATE_MS);
			break;
	}
	// send type 99 - record list counter
	record_inst recordListCounterRecord;
	recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
	blePreSendRecord(recordListCounterRecord, recordListCounter);//, bleNumericSendBuffer, bleCharSendBuffer);
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
//	start_scan();
}


//numericRecord_inst makeNumericRecord (uint16_t timePast, uint8_t type, uint32_t data)
//{
//	numericRecord_inst tempRecord;
//	tempRecord.timePast = timePast;
//	tempRecord.type = type;
//	tempRecord.data = data;
//	return tempRecord;
//
//}
//
//bool addToNumericRecordList (numericRecord_inst record)
//{
//	if (numericRecordListCounter >= 300)
//		return false; // re-allocation need
//	numericRecordList[numericRecordListCounter++] = record;
//	return true;
//}
//
//void printNumericRecordList()
//{
//	ESP_LOGI("Numeric Records List", "Printing records list:");
//
//	for (int i = 0; i < numericRecordListCounter; i++) {
//		ESP_LOGI("Record List", "timePast: %d, type: %03d, data: %d", numericRecordList[i].timePast, numericRecordList[i].type, numericRecordList[i].data);
//	}
//
//}
//
//void blePreSendNumericRecord(numericRecord_inst record)
//{
//	bleNumericSendBuffer[0] = (record.timePast >> 8) & 0xF;
//	bleNumericSendBuffer[1] = record.timePast & 0xF;
//	bleNumericSendBuffer[2] = record.type;
//	for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
//		bleNumericSendBuffer[i] = (record.data >> (24 - 8*(i-3))) & 0xF;
//	}
//	esp_log_buffer_hex("BLE Send Buffer", bleNumericSendBuffer, NUMERIC_BLE_BUFF_SIZE);
//
//}
//
//void bleSendNumericRecord()
//{
//	if (gl_profile_tab[PROFILE_B_APP_ID].gattc_if != ESP_GATT_IF_NONE) {
//		esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//									  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//									  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//									  NUMERIC_BLE_BUFF_SIZE,
//									  bleNumericSendBuffer,
//									  ESP_GATT_WRITE_TYPE_RSP,
//									  ESP_GATT_AUTH_REQ_NONE);
//	}
//}
//
//void bleSendNumericRecordList()
//{
//	for (int i = 0; i < numericRecordListCounter; i++) {
//		blePreSendNumericRecord(numericRecordList[i]);
//		bleSendNumericRecord();
//	}
//	// send type 99 - record list counter
//	numericRecord_inst numericRecordListCounterRecord;
//	numericRecordListCounterRecord = makeNumericRecord(timePastInMinutes, 99, numericRecordListCounter);
//	blePreSendNumericRecord(numericRecordListCounterRecord);
//	// Client need to approve / authenticate that the phone got all records
//	// bleSendNumericRecord(numericRecordListCounterRecord);
//	esp_ble_gattc_write_char( gl_profile_tab[PROFILE_B_APP_ID].gattc_if,
//										  gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//										  gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//										  NUMERIC_BLE_BUFF_SIZE,
//										  bleNumericSendBuffer,
//										  ESP_GATT_WRITE_TYPE_RSP,
//										  ESP_GATT_AUTH_REQ_NONE);
//
//
//}

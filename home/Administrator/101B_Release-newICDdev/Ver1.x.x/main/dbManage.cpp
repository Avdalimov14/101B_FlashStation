/*
 * dbManage.c
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#include "dbManage.h"
#include "nvsManage.h"
#include "gatts_related.h"
#include "GenericRecord.h"

//recordList = (GenericRecord*)malloc(300 * RECORD_SIZE);
//int RECORD_SIZE = sizeof(GenericRecord);
//recordList = (GenericRecord*)malloc(300 * RECORD_SIZE);
uint32_t recordListCounter;// = 0;
uint32_t recordSendCoutner;// = 0;
uint32_t mesurementSendCounter;// = 0;

bool sendingRecordsToSmartphone;// = false;
bool sendingMeasurementsToSmartphone;// = false;
uint8_t ramRecordListLimit;

const int RECORD_CLASS_SIZE = sizeof(GenericRecord);

void init_db()
{
	MAX_RECORD_IN_NVS = 4500; //deubggg
	ESP_LOGI("NVS MAX", "RECORDS is %d", MAX_RECORD_IN_NVS);
	recordList = new GenericRecord[MAX_RECORD_IN_NVS];
	if (recordList == NULL)
		ESP_LOGE("FATAL", "ERROR");
	nvsTimeRead();
	nvsReadRecordListCounter();
//	recordListCounter = 0;
	recordSendCoutner = 0;
	mesurementSendCounter = 0;

	sendingRecordsToSmartphone = false;
	sendingMeasurementsToSmartphone = false;

//	nvsReadMaxRecordParam();
	nvsReadRamRecordListLimit();

}

GenericRecord& getRecord(int recordNum)
{
	return recordList[recordNum];
}

//GenericRecord makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t numericData, uint8_t charData[]) // recordType -> 0-Numeric(charData=NULL), 1-Char
//{
//	GenericRecord tempRecord;
//	if (recordType == 0 && charData == NULL) {
//		tempRecord.recordInfo.charRecord = NULL;
//		tempRecord.recordType = NUMERIC_RECORD_TYPE;
//		tempRecord.recordInfo.numericRecord = (numericGenericRecord*)malloc(NUMERIC_RECORD_SIZE);
//		tempRecord.recordInfo.numericRecord->timePast = timePast;
//		tempRecord.recordInfo.numericRecord->type = type;
//		tempRecord.recordInfo.numericRecord->data = numericData;
//		return tempRecord;
//	}
//	else if (recordType == 1) {
//		tempRecord.recordType = CHAR_RECORD_TYPE;
//		tempRecord.recordInfo.charRecord = (charGenericRecord*)malloc(CHAR_RECORD_SIZE);
//		tempRecord.recordInfo.charRecord->timePast = timePast;
//		tempRecord.recordInfo.charRecord->type = type;
//		for (int i = 0; i < 20; i++)
//			tempRecord.recordInfo.charRecord->data[i] = charData[i];
//		return tempRecord;
//	}
//	tempRecord.recordInfo.numericRecord = NULL;
//	return tempRecord;
//}

bool addToRecordList (GenericRecord record, bool writeToNvs, uint32_t recordNum)
{

	// this if is not relevant yet
	if (recordListCounter >= MAX_RECORD_IN_NVS && !ramRecordListLimit && NULL) {

		MAX_RECORD_IN_NVS = MAX_RECORD_IN_NVS + 500;
//		recordList = (GenericRecord*)realloc(recordList, RECORD_SIZE * (/*recordListCounter + */MAX_RECORD_IN_NVS));
//		free(recordList);
//		recordList = (GenericRecord*)malloc(RECORD_SIZE * (MAX_RECORD_IN_NVS));
//		while (recordList == NULL){
//			ESP_LOGE("MEMORY", "couldn't re-allocate record list for another %d (ramRecordListLimit = true)", MAX_RECORD_IN_NVS);
//			MAX_RECORD_IN_NVS = MAX_RECORD_IN_NVS - 500;
//			ESP_LOGE("MEMORY", "MAX_RECORD_IN_NVS = %d, recordListCounter = %d", MAX_RECORD_IN_NVS, recordListCounter);
//			recordList = (GenericRecord*)malloc(RECORD_SIZE * (MAX_RECORD_IN_NVS));
//			if (recordList == NULL) {
//				ESP_LOGE("MEMORY", "couldn't mallocate record list for another %d", MAX_RECORD_IN_NVS);
//				ESP_LOGE("MEMORY", "		FATAL MEMORY ERRO!!		(ramRecordListLimit = true)");
////				return false;
//			}
//			ramRecordListLimit = 1;
//			recordListCounter = 0;
//			memset(recordList, 0, MAX_RECORD_IN_NVS); // not sure if working

//		}
	nvsWriteMaxRecordParam();
	}

//	if (recordListCounter == 300)
//		longBeep();


	if (writeToNvs && recordListCounter <= MAX_RECORD_IN_NVS) {
		recordList[recordListCounter++] = record;
		nvsWriteRecordList(recordListCounter - 1);
		nvsWriteRecordListCounter();
	}
	else if (recordListCounter <= MAX_RECORD_IN_NVS)
		recordList[recordNum] = record;
	return true;

}

void printRecordList(int startFrom)
{
	ESP_LOGI(DBLOG, "Printing records list:");
	for (int i = recordListCounter - startFrom; i < recordListCounter /*&& !sendingRecordsToSmartphone*/; i++) {

		char tmpRecordBuffer_TAG[25] = "Record #";
		char tmpItoa[5];
		itoa(i, tmpItoa, 10);
		strcat(tmpRecordBuffer_TAG, tmpItoa);

		ESP_LOGI(tmpRecordBuffer_TAG, "timePast: %d(eventNum = %d), type: %03d, dataLen: %d, data: (next line)",
				 recordList[i].m_timePast, recordList[i].m_eventNum, recordList[i].m_type, recordList[i].m_dataLen);
		strcat(tmpRecordBuffer_TAG, " Data");
		esp_log_buffer_hex(tmpRecordBuffer_TAG, recordList[i].m_data, recordList[i].m_dataLen);
	}

}

void automaticRecordUpdate(uint8_t type)
{
	static bool rec20 = false;
	static bool rec30 = false;
	static bool rec40 = false;
	ESP_LOGI("DEBUGGG", "I;m in automaticRecordUpdate type = %d, %d %d %d", type, rec20, rec30, rec40);
	if (!rec20 || !rec30 || !rec40) {
		switch (type)
		{
		case 22:
		case 23:
		case 27:
		case 33:
		case 34:
		case 44:
			uint8_t tmpInt = 1;
			switch (type - type%10)
			{
			case 20:
				if (!rec20) {
					rec20 = true;
					GenericRecord record((uint8_t)(type - type%10), &tmpInt);
					recordList[0] = record;
					nvsWriteRecordList(0);
					if (gatts_conn) {
						sendRecordByIndicate(record);
					}
				}
				break;

			case 30:
				if (!rec30) {
					rec30 = true;
					GenericRecord record((uint8_t)(type - type%10), &tmpInt);
					recordList[1] = record;
					nvsWriteRecordList(1);
					if (gatts_conn) {
						sendRecordByIndicate(record);
					}
				}
				break;

			case 40:
				if (!rec40) {
					rec40 = true;
					GenericRecord record((uint8_t)(type - type%10), &tmpInt);
					recordList[2] = record;
					nvsWriteRecordList(2);
					if (gatts_conn) {
						sendRecordByIndicate(record);
					}
				}
				break;
			}
			break;
		}
	}
}

void generateYesNoRecords()
{
	uint8_t tmpInt = 0;
	GenericRecord record20(20, &tmpInt);
	GenericRecord record30(30, &tmpInt);
	GenericRecord record40(40, &tmpInt);
	addToRecordList(record20, true, NULL);
	addToRecordList(record30, true, NULL);
	addToRecordList(record40, true, NULL);

}

int findMatchingIndex (const GenericRecord record)
{
	GenericRecord tmpRec;
	for (int i = 0; i < recordListCounter; i++) {
		tmpRec = recordList[i];
		if (tmpRec.m_timePast == record.m_timePast) {
			if (tmpRec.m_eventNum == record.m_eventNum) {
				return i;
//				break;
			}
		}
	}
	return -1;
}

void updateRecordByIndex(int recordIndex, uint8_t* data) // Overwriting existed data
{
	ESP_LOGI("DEBUGGG", "foundRecord.m_data = %d, record.m_data = %d", recordList[recordIndex].m_data[0], data[0]);
	memcpy(recordList[recordIndex].m_data, data, recordList[recordIndex].m_dataLen);
	ESP_LOGI("DEBUGGG", "foundRecord.m_data = %d, record.m_data = %d", recordList[recordIndex].m_data[0], data[0]);
	nvsWriteRecordList(recordIndex);
}

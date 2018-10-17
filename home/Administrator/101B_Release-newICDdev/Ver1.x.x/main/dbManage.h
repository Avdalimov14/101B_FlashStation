/*
 * dbManage.h
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#ifndef MAIN_DBMANAGE_H_
#define MAIN_DBMANAGE_H_

#include "mainHeader.h"
#include "GenericRecord.h"

#define REC_HEAD_SIZE 4
#define MAX_REC_SIZE 24
class GenericRecord;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MESUREMENT_TYPE_HEARTRATE		=	81,
	MESUREMENT_TYPE_BLOODPRESURE	=	82,
	MESUREMENT_TYPE_SPO2			=	83,
	MESUREMENT_TYPE_RESPRATE		=	84,
	MESUREMENT_TYPE_VAS				=	85,
} record_type_t;

#define GEN_REC_CLASS_SIZE sizeof(GenericRecord)

extern const int RECORD_SIZE;// = sizeof(record_inst);
static GenericRecord* recordList;// = (genericRecord_inst*)malloc(300 * RECORD_SIZE);
//record_inst bleMeasurementsStack[4];
extern uint32_t recordListCounter;// = 0;
extern uint32_t recordSendCoutner;// = 0;
extern uint32_t mesurementSendCounter;// = 0;
//static uint32_t stackSendCounter = 0;
extern bool sendingRecordsToSmartphone;// = false;
extern bool sendingMeasurementsToSmartphone;// = false;
//static bool sendingStackToSmartphone = false;
//static bool scanIsOn = false; moved to extern in mainHeader
extern uint8_t ramRecordListLimit;// = false; // related to be able save more then 4-5k records


//GenericRecord makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t data, uint8_t charData[]); // recordType -> 0-Numeric 1-Char
bool addToRecordList (GenericRecord record, bool writeToNvs, uint32_t recordNum);
void printRecordList(int startFrom);
//bool checkRecordType(GenericRecord record); // not implemented
//recordType_t blePreSendRecord(record_inst record, int i);
//void bleSendRecord(recordType_t recordType);
//void bleSendRecordList();
//void bleSendMeasurements();
//void bleSendStack();
void init_db();
GenericRecord& getRecord(int recordNum);
void automaticRecordUpdate(uint8_t type);
void generateYesNoRecords();
int findMatchingIndex (const GenericRecord record);
void updateRecordByIndex(int recordIndex, uint8_t* data); // Overwriting existed data

#ifdef __cplusplus
}
#endif

#endif /* MAIN_DBMANAGE_H_ */

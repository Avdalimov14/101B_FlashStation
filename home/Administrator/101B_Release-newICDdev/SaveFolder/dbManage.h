/*
 * dbManage.h
 *
 *  Created on: Jun 14, 2018
 *      Author: albert
 */

#ifndef MAIN_DBMANAGE_H_
#define MAIN_DBMANAGE_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct numericRecord_inst {      // members of structures need to be sorted in ascending order
	uint32_t data;
	uint16_t timePast;
	uint8_t type;
	//uint8_t len; // or size may be optional, consider to delete if not necessary
} numericRecord_inst;

typedef struct charRecord_inst {
	uint8_t data[20];
	uint16_t timePast;
	uint8_t type;
	//uint8_t len;
} charRecord_inst;

typedef enum {
	NUMERIC_RECORD_TYPE	= 0,
	CHAR_RECORD_TYPE	= 1,
	NUMERIC_RECORD_TYPE2 = 2, // experimental
}recordType_t;

typedef struct record_inst {
	union {

		charRecord_inst* charRecord;
		numericRecord_inst* numericRecord;
	} recordInfo;
	recordType_t recordType;
} record_inst;

typedef enum {
    MESUREMENT_TYPE_HEARTRATE		=	81,
	MESUREMENT_TYPE_BLOODPRESURE	=	82,
	MESUREMENT_TYPE_SPO2			=	83,
	MESUREMENT_TYPE_RESPRATE		=	84,
	MESUREMENT_TYPE_VAS				=	85,
} record_type_t;

#define NUMERIC_RECORD_SIZE sizeof(numericRecord_inst)
#define CHAR_RECORD_SIZE sizeof(charRecord_inst)
#define NUMERIC_BLE_BUFF_SIZE 7
#define CHAR_BLE_BUFF_SIZE 23

//uint8_t bleNumericSendBuffer[NUMERIC_BLE_BUFF_SIZE];
//uint8_t bleCharSendBuffer[CHAR_BLE_BUFF_SIZE];

extern int RECORD_SIZE;// = sizeof(record_inst);
static record_inst* recordList;// = (record_inst*)malloc(300 * RECORD_SIZE);
//record_inst bleMeasurementsStack[4];
extern uint32_t recordListCounter;// = 0;
extern uint32_t recordSendCoutner;// = 0;
extern uint32_t mesurementSendCounter;// = 0;
//static uint32_t stackSendCounter = 0;
extern bool sendingRecordsToSmartphone;// = false;
extern bool sendingMeasurementsToSmartphone;// = false;
//static bool sendingStackToSmartphone = false;
static bool scanIsOn = false;

record_inst makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t data, uint8_t charData[]); // recordType -> 0-Numeric 1-Char
bool addToRecordList (record_inst record);//, bool writeToNvs, uint32_t recordNum);
void printRecordList();
bool checkRecordType(record_inst record); // not implemented
//recordType_t blePreSendRecord(record_inst record, int i);
//void bleSendRecord(recordType_t recordType);
//void bleSendRecordList();
//void bleSendMeasurements();
//void bleSendStack();
void init_db();
record_inst* getRecord(int recordNum);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_DBMANAGE_H_ */

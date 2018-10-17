/*
 * dataBaseManage.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_DBMANAGE_H_
#define MAIN_DBMANAGE_H_


// Data Base management section
//uint64_t debugTimer = 0;

#include "mainHeader.h"
#include "stdbool.h"

#ifdef __cplusplus
extern "C" {
#endif


// moved to mainHeader.h
//typedef struct numericRecord_inst {      // members of structures need to be sorted in ascending order
//	uint32_t data;
//	uint16_t timePast;
//	uint8_t type;
//	//uint8_t len; // or size may be optional, consider to delete if not necessary
//} numericRecord_inst;
//
//typedef struct charRecord_inst {
//	uint8_t data[20];
//	uint16_t timePast;
//	uint8_t type;
//	//uint8_t len;
//} charRecord_inst;
//
//typedef enum {
//	NUMERIC_RECORD_TYPE	= 0,
//	CHAR_RECORD_TYPE	= 1,
//}recordType_t;
//
//typedef struct record_inst {
//	union {
//
//		charRecord_inst* charRecord;
//		numericRecord_inst* numericRecord;
//	} recordInfo;
//	recordType_t recordType;
//} record_inst;
//
//const int NUMERIC_RECORD_SIZE = sizeof(numericRecord_inst);
//const int CHAR_RECORD_SIZE = sizeof(charRecord_inst);
//const int RECORD_SIZE = sizeof(record_inst);
//#define NUMERIC_BLE_BUFF_SIZE 7
//#define CHAR_BLE_BUFF_SIZE 23
typedef enum {
    MESUREMENT_TYPE_HEARTRATE		=	81,
	MESUREMENT_TYPE_BLOODPRESURE	=	82,
	MESUREMENT_TYPE_SPO2			=	83,
	MESUREMENT_TYPE_RESPRATE		=	84,
	MESUREMENT_TYPE_VAS				=	85,
} record_type_t;

record_inst makeRecord (bool recordType, uint16_t timePast, uint8_t type, uint32_t data, uint8_t charData[]); // recordType -> 0-Numeric 1-Char
bool addToRecordList (record_inst record, bool writeToNvs, uint32_t recordNum);//, record_inst* recordList, uint32_t* recordListCounter);
void printRecordList();//record_inst* recordList, uint32_t* recordListCounter);
//bool checkRecordType(record_inst record);
recordType_t blePreSendRecord(record_inst record, int i);//, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer);
void bleSendRecord(recordType_t recordType);//, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer);
void bleSendRecordList();//record_inst* recordList, uint8_t* bleNumericSendBuffer, uint8_t* bleCharSendBuffer, uint32_t* recordListCounter, uint32_t timePastInMinutes);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_DBMANAGE_H_ */

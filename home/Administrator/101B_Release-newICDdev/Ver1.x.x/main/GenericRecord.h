/*
 * GenericRecord.h
 *
 *  Created on: Aug 26, 2018
 *      Author: albert
 */

#ifndef MAIN_GENERICRECORD_H_
#define MAIN_GENERICRECORD_H_

#include "mainHeader.h"

class GenericRecord // Header: 7 bit eventNum, 17 bit timePast, 8bit type-> generic data
{
public:	// vars are here because we use them frequantly --> decided to not implement setters/getters
	uint8_t* m_data;
	uint32_t m_timePast;
	uint8_t m_eventNum;
	uint8_t m_type;
	uint8_t m_dataLen;

	GenericRecord();
	GenericRecord(uint8_t type);
	GenericRecord(uint8_t type, uint8_t* data);//, int dataLen);
	GenericRecord(const GenericRecord& record);
	~GenericRecord(void);

//	uint32_t getEventPlusTime();
	void getBytesEventPlusTime(uint8_t* array) const;
	uint8_t getBytesType() const { return m_type; };
	void getBytesData(uint8_t* dataArray) const;
	void getBytesRecord(uint8_t* recArray) const;// no need -> , int& recLen)
	int getRecordSize() const;

	void setBytesEventPlusTime(uint8_t* array);
	void setBytesType(uint8_t newType) {m_type = newType;};
	void setBytesData(uint8_t* dataArray);//, int dataLen); no need because findDataLen
	void setBytesRecord(uint8_t* recArray);//, int recLen); no need because findDataLen

	void printHeaderInfo();
	void printRecordHeaderHex();
	//void printDataDec(); // couldn"t think about good way to print it... --> make it esp_logi(..., "%d %d %d ... oh but it going to next line ffs
	void printDataChar();
	void printDataHex();
	void printRecordHex();

	void findDataLen();

	GenericRecord& operator=(const GenericRecord& record);
};




#endif /* MAIN_GENERICRECORD_H_ */

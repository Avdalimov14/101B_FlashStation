/*
 * GenericRecord.cpp
 *
 *  Created on: Aug 26, 2018
 *      Author: albert
 */
#include "GenericRecord.h"

GenericRecord::GenericRecord() :  m_timePast(0), m_eventNum(0), m_type(0), m_dataLen(0)
{
	// TODO: function that init and returns timePast + eventNum, for now it = 0
//	int tmpTimePast = timePastSeconds;
//	updateTimeParams();
//	if (timePastSeconds == tmpTimePast) {
//		m_eventNum = ++timePastEventNum;
//	}
//	else {
//		m_eventNum = timePastEventNum = 0;
//	}
//	m_timePast = timePastSeconds;

	if (m_dataLen) { // for debug
		m_data = new uint8_t[m_dataLen];
		if (m_data != NULL) {
			for (int i = 0; i < m_dataLen; i++)
				m_data[i] = 0x10 + i;
		}
	}
	else
		m_data = NULL;
}

GenericRecord::GenericRecord(uint8_t type) : m_type(type), m_dataLen(0)//, m_data(NULL)
{
	// TODO: function that init and returns timePast + eventNum, for now it = 0
	int tmpTimePast = timePastSeconds;
	updateTimeParams();
	if (timePastSeconds == tmpTimePast) {
		m_eventNum = ++timePastEventNum;
	}
	else {
		m_eventNum = timePastEventNum = 0;
	}
	m_timePast = timePastSeconds;

	m_data = NULL;
}

GenericRecord::GenericRecord(uint8_t type, uint8_t* data) : m_type(type)
{
	// TODO: function that init and returns timePast + eventNum, for now it = 0
	int tmpTimePast = timePastSeconds;
	updateTimeParams();
	if (timePastSeconds == tmpTimePast) {
		m_eventNum = ++timePastEventNum;
	}
	else {
		m_eventNum = timePastEventNum = 0;
	}
	m_timePast = timePastSeconds;

	//findDataLen(); inside setBytesData
	setBytesData(data);//, dataLen);

}

GenericRecord::GenericRecord(const GenericRecord& record)
{
	m_timePast = record.m_timePast;
	m_eventNum = record.m_eventNum;
	m_type = record.m_type;
	m_dataLen = record.m_dataLen;
	setBytesData(record.m_data);
}

GenericRecord::~GenericRecord()
{
    if (m_dataLen)
        delete[] m_data;
}

void GenericRecord::getBytesEventPlusTime(uint8_t* array) const
{
	array[0] = (m_eventNum << 1) | ((m_timePast >> 16) & 1);
	array[1] = (m_timePast >> 8) & 0xFF;
	array[2] = m_timePast & 0xFF;
}


void GenericRecord::getBytesData(uint8_t* dataArray) const
{
	if (m_dataLen) {
		for (int i = 0; i < m_dataLen; i++) {
			dataArray[i] = m_data[i];
		}
	}
	else
		ESP_LOGI(DBLOG, "No Data (m_dataLen = 0) !");
}

void GenericRecord::getBytesRecord(uint8_t* recArray) const// no need -> , int& recLen)
{
	int recLen = 0;
	getBytesEventPlusTime(recArray + recLen);
	recLen += 3;
	recArray[recLen++] = getBytesType();
	getBytesData(recArray + recLen);
	recLen += m_dataLen;

}

int GenericRecord::getRecordSize() const
{
	return REC_HEAD_SIZE + m_dataLen; // or dataLen
}

void GenericRecord::setBytesEventPlusTime(uint8_t* array)
{
	m_eventNum = array[0] >> 1;
	m_timePast = ((array[0] & 1) << 16) | (array[1] << 8) | array[2];
}

void GenericRecord::setBytesData(uint8_t* dataArray)//, int dataLen)
{
	//m_dataLen = dataLen;
	findDataLen();
	if (m_dataLen) {
		m_data = new uint8_t[m_dataLen];
		if (m_data != NULL) {
			for (int i = 0; i < m_dataLen; i++)
				m_data[i] = dataArray[i];
		}
	}
}

void GenericRecord::setBytesRecord(uint8_t* recArray)//, int recLen)
{
	setBytesEventPlusTime(recArray);
	setBytesType(recArray[3]);
	setBytesData(recArray + REC_HEAD_SIZE);//, recLen - REC_HEAD_SIZE);

}

void GenericRecord::printHeaderInfo()
{
	ESP_LOGI(DBLOG, "Printing record's header info (Dec):");
	ESP_LOGI(DBLOG, "eventNum = %d, timePast = %d, type = %d", m_eventNum, m_timePast, m_type);
}

void GenericRecord::printRecordHeaderHex()
{
	ESP_LOGI(DBLOG, "Printing record's header bytes (Hex):");
	uint8_t tmpArr[REC_HEAD_SIZE];
	getBytesEventPlusTime(tmpArr);
	tmpArr[REC_HEAD_SIZE - 1] = m_type;
	esp_log_buffer_hex(DBLOG, tmpArr, REC_HEAD_SIZE);

}

//void GenericRecord::printDataDec()
//{
//
//}

void GenericRecord::printDataChar()
{
	ESP_LOGI(DBLOG, "Printing record's data bytes (Char):");
	esp_log_buffer_char(DBLOG, m_data, m_dataLen);
}

void GenericRecord::printDataHex()
{
	ESP_LOGI(DBLOG, "Printing record's data bytes (Hex):");
	esp_log_buffer_hex(DBLOG, m_data, m_dataLen);
}

void GenericRecord::printRecordHex()
{
	uint8_t* tmpArr = new uint8_t[REC_HEAD_SIZE + m_dataLen];
	getBytesRecord(tmpArr);
	ESP_LOGI(DBLOG, "Printing record bytes (Hex):");
	esp_log_buffer_hex(DBLOG, tmpArr, REC_HEAD_SIZE + m_dataLen);
	delete[] tmpArr;
}

void GenericRecord::findDataLen()
{
	switch (m_type)
	{
	default:
		m_dataLen = 1;
		break;
	case 99:
		m_dataLen = 3;
		break;
	case 11:
	case 12:
		m_dataLen = 20;
		break;
	case 63:
		m_dataLen = 200;
		break;
	case 80:
		m_dataLen = 5;
		break;
	case 0: // used to be MacAddress
		m_dataLen = MAC_SIZE;
		break;
//	case 221:
//	case 222:
//		m_dataLen = 0;
//		break;
	}
}

GenericRecord& GenericRecord::operator=(const GenericRecord& record)
{
	m_timePast = record.m_timePast;
	m_eventNum = record.m_eventNum;
	m_type = record.m_type;
	m_dataLen = record.m_dataLen;
	setBytesData(record.m_data);

	return *this;
}




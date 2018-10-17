/*
 * nvsManage.h
 *
 *  Created on: Jun 27, 2018
 *      Author: albert
 */

#ifndef MAIN_NVSMANAGE_H_
#define MAIN_NVSMANAGE_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t MAX_RECORD_IN_NVS;// = 500;

extern nvs_handle storageNvsHandle;

void nvsWriteBbMacAddress(uint8_t* address);
bool nvsReadBbMacAddress(uint8_t* address);
void init_nvs();
void nvsTimeRead();
void nvsTimeWrite();
void nvsReadRecordListCounter();
void nvsReadMaxRecordParam();
void nvsWriteMaxRecordParam();
void nvsReadRamRecordListLimit();
void nvsWriteRamRecordListLimit();
void nvsWriteRecordListCounter();
void nvsReadRecordList(uint32_t recordNum);
void nvsWriteRecordList(uint32_t recordNum);


#ifdef __cplusplus
}
#endif


#endif /* MAIN_NVSMANAGE_H_ */

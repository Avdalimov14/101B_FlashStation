/*
 * nvsManage.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_NVSMANAGE_H_
#define MAIN_NVSMANAGE_H_					// need to add closing function

#include "mainHeader.h"
#include "Arduino.h"
#include "WString.h"
#include "dbManage.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_KEY_LEN 15
#define MAX_RECORD_IN_NVS 500
static nvs_handle storageNvsHandle;

void init_nvs();
void nvsTimeCheck();
void nvsReadRecordListCounter();//uint32_t* recordListCounter);
void nvsWriteRecordListCounter();//uint32_t* recordListCounter);
void nvsReadRecordList(uint32_t recordNum);//, record_inst* recordList, uint32_t* recordListCounter);
void nvsWriteRecordList(uint32_t recordNum);
void nvsWriteMinutesCounter();//uint32_t timePastMinutes);
esp_err_t eraseStorageNvsHandle();

//extern uint32_t timePastMinutes;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_NVSMANAGE_H_ */

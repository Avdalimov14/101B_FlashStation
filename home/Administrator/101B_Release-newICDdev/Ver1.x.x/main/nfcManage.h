/*
 * nfcManage.h
 *
 *  Created on: Jun 27, 2018
 *      Author: albert
 */

#ifndef MAIN_NFCMANAGE_H_
#define MAIN_NFCMANAGE_H_


#include "SPI.h"
#include "PN532_SPI.h"
#include "PN532.h"
#include "NfcAdapter.h"
#include "Arduino.h"


#define NFC_DECODE_SIZE 2
#define NFC_RF_PULSE_WIDTH 33
static PN532_SPI pn532spi(SPI, 5);
static NfcAdapter nfc = NfcAdapter(pn532spi);

#ifdef __cplusplus
extern "C" {
#endif

//static xQueueHandle rfFieldQueue;

bool init_nfc();
void nfcRfFieldTask(void *pvParameters);
void nfcHandleTask(void *pvParameters);
void readTagRecords();
bool checkIfHce();
bool tagValidation(int tagNum);
//void generateAutomaticRecord(uint8_t type); // moved to dbManage.h!

#ifdef __cplusplus
}
#endif

#endif /* MAIN_NFCMANAGE_H_ */

/*
 * nfcManage.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_NFCMANAGE_H_
#define MAIN_NFCMANAGE_H_

#include "SPI.h"
#include "PN532_SPI.h"
#include "PN532.h"
#include "NfcAdapter.h"
#include "Arduino.h"

// NFC

static PN532_SPI pn532spi(SPI, 5);
static NfcAdapter nfc = NfcAdapter(pn532spi);

#ifdef __cplusplus
extern "C" {
#endif


extern uint8_t smartphoneMacAddress[MAC_SIZE];
extern uint8_t measureDeviceBtMacAddrs[6];// = {0,0,0,0,0,0};//{0x78, 0xB6, 0x90, 0x35, 0xB6, 0x98};

#define NFC_DECODE_SIZE 2

// defines regards NFC HCE

bool checkIfHce();

void nfcHandleTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_NFCMANAGE_H_ */

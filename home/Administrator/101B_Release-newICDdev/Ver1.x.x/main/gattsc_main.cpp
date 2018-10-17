 /*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for gatt client. It can scan ble device, connect one device.
* Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. Then the two devices will exchange
* data.
*
****************************************************************************/

#include "mainHeader.h"

#include "gattc_related.h"
#include "gatts_related.h"
#include "nfcManage.h"
#include "nvsManage.h"
#include "ioManage.h"
#include "indicationManage.h"


// extern global
extern struct gattc_profile_inst gattc_gl_profile_tab[GATTC_PROFILE_NUM];

extern "C" {

int app_main(void);
uint8_t myDeviceBtMacAddrs[MAC_SIZE] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
uint8_t bbBtMacAddrs[MAC_SIZE] = {0};
uint8_t smartphoneMacAddress[MAC_SIZE] = {0};
uint8_t simulatorMacAddress[MAC_SIZE] = {0};
bool gattsInitDone = false; // in case of re-connecting to BioBeat
bool scanIsOn = false;
void init_gap_ble();
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

uint8_t measureDeviceBtMacAddrs[6] = {0};
int16_t rssiPassRange = START_RSSI_VAL; // change on nfcRfFieldTask

xQueueHandle indicationLedQueue;
}

int app_main(void)
{
#ifndef OLD_INDICATIONS
	indicationLedQueue = xQueueCreate(5, sizeof(indicationStatus_t));
#endif

	init_gpio();
	init_pwm();
	vTaskDelay(25 / portTICK_RATE_MS);
	init_nvs();
	init_db();
	if (recordListCounter == 0 && !ramRecordListLimit) {
		esp_read_mac(myDeviceBtMacAddrs, ESP_MAC_BT);
		// oldICD
//		record_inst macAddressRecord = makeRecord(1, timePastMinutes, 0, NULL, myDeviceBtMacAddrs);
//		addToRecordList(macAddressRecord, true, NULL);
//		printRecordList();
		GenericRecord macAddressRecord(0, myDeviceBtMacAddrs);//, MAC_SIZE);

		generateYesNoRecords();
	}
	else {
		if (!ramRecordListLimit) {
			for (int i = 0; i < recordListCounter; i++)
				nvsReadRecordList(i);

			printRecordList(recordListCounter);
		}
	}
	vTaskDelay(25 / portTICK_RATE_MS);
	init_nfc();
	vTaskDelay(25 / portTICK_RATE_MS);
	init_timers();
#ifdef OLD_INDICATIONS
	turnBlueLedForXmSec(2000);
#else
	xTaskCreatePinnedToCore(ledHandleTask, "LEDfunc", 2048, NULL, 2, NULL, 1);
	xTaskCreatePinnedToCore(nfcRfFieldTask, "RFfunc", 3074, NULL, 1, NULL, 0);
	sendIndicationStatusToQueue((indicationStatus_t)IND_BIT_SUCCESS);
#endif
	vTaskDelay(25 / portTICK_RATE_MS);
	BEEP;
	init_gap_ble();
	vTaskDelay(25 / portTICK_RATE_MS);
//	init_gattc_ble();
	init_gatts_ble(); //	init_gattc_ble(); is done after last event ( ESP_GATTS_ADD_CHAR_DESCR_EVT )
	vTaskDelay(25 / portTICK_RATE_MS);
//	init_gatts_ble(); // is done inside ESP_GATTC_WRITE_DESCR_EVT


	vTaskDelay(500 / portTICK_RATE_MS);
//	BEEP;


	xTaskCreatePinnedToCore(nfcHandleTask, "NFCfunc", 2048, NULL, 1, NULL, 0); // time handle func
	return 0;
}

void init_gap_ble()
{
	esp_err_t ret;

	// Initialize NVS.
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
		return;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
		return;
	}

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    uint8_t *adv_uuid_cmpl = NULL;
    uint8_t adv_uuid_cmpl_len = 0;

    uint8_t *adv_uuid_part = NULL;
    uint8_t adv_uuid_part_len = 0;


    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
    	ESP_LOGI(GATTC_TAG, "Trying to see if there is BB Mac Address in NVS...");
    	if (nvsReadBbMacAddress(bbBtMacAddrs)) {
//    		esp_ble_gap_set_prefer_conn_params(bbBtMacAddrs, 11, 11, 0, 10);
    		stop_advertise();
    		esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, bbBtMacAddrs, BLE_ADDR_TYPE_PUBLIC  , true);
			openBleConnAttempts = 10;
			openBleConnection = true;
			sendIndicationStatusToQueue((indicationStatus_t)IND_MONITOR_DISCONNECTED);
    	}
    	else {
    		uint32_t duration = 90;
    		esp_ble_gap_start_scanning(duration);
    	}
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");
        scanIsOn = true;
#ifndef OLD_INDICATIONS
        sendIndicationStatusToQueue((indicationStatus_t)IND_SCAN_START);
#endif
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
//            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
//            ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

            adv_uuid_cmpl = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_CMPL, &adv_uuid_cmpl_len);

            adv_uuid_part = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_PART, &adv_uuid_part_len);
            // service uuid is bb_serviceUUID

//            ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
//            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
//            ESP_LOGI(GATTC_TAG, "\n");
//            if (adv_name != NULL) {
//                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
//                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
//                    if (gattc_conn == false) {
//                        gattc_conn = true;
//                        ESP_LOGI(GATTC_TAG, "connect to the remote device. Mac Address:");
//                        esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
//                        esp_ble_gap_stop_scanning();
//                        esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
//                    }
//                }
//            }
//            ESP_LOGI("DEBUGGG", "rssiPassRange = %d", rssiPassRange);
            if (scan_result->scan_rst.rssi > rssiPassRange) {
                ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
                esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
                ESP_LOGI(GATTC_TAG, "\n");
                if (adv_name != NULL) {
                    if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
                        ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
                        if (gattc_conn == false) {
                            gattc_conn = true;
                            ESP_LOGI(GATTC_TAG, "connect to the remote device. Mac Address:");
                            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
                            esp_ble_gap_stop_scanning();
                            esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
                            memcpy(bbBtMacAddrs, scan_result->scan_rst.bda, MAC_SIZE);
                            nvsWriteBbMacAddress(bbBtMacAddrs);
                        }
                    }
                }

				if (adv_uuid_cmpl != NULL || adv_uuid_part != NULL) {
					if ((16 == adv_uuid_cmpl_len && (memcmp(adv_uuid_cmpl, bb_serviceUUID, adv_uuid_cmpl_len) == 0))
							|| (16 == adv_uuid_part_len && (memcmp(adv_uuid_part, bb_serviceUUID, adv_uuid_part_len)))) {
						ESP_LOGI(GATTC_TAG, "searched device\n");
						esp_log_buffer_hex(GATTC_TAG, bb_serviceUUID, 16);
						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_cmpl)\n");
						esp_log_buffer_hex(GATTC_TAG, adv_uuid_cmpl, 16);
						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_part)\n");
						if (gattc_conn == false) {
							gattc_conn = true;
							ESP_LOGI(GATTC_TAG, "connect to the remote device. Mac Address:");
							esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
							esp_ble_gap_stop_scanning();
							esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
							memcpy(bbBtMacAddrs, scan_result->scan_rst.bda, MAC_SIZE);
                            nvsWriteBbMacAddress(bbBtMacAddrs);
						}
					}
				}
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
        	ESP_LOGI(GATTS_TAG, "ESP_GAP_SEARCH_INQ_CMPL_EVT");
        	scanIsOn = false;
        	if (!gattc_conn) {
        		start_scan(90);
        	}
            break;
        default:
            break;
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
        scanIsOn = false;
        break;


#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done==0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
//        adv_config_done &= (~adv_config_flag);
//        if (adv_config_done == 0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
//        adv_config_done &= (~scan_rsp_config_flag);
//        if (adv_config_done == 0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
        break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
    	if (param->adv_start_cmpl.status == ESP_BT_STATUS_PENDING) {
			ESP_LOGI(GATTS_TAG, "Advertising start is pending, status = %d\n", param->adv_start_cmpl.status);
		}
    	else if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed, status = %d\n", param->adv_start_cmpl.status);
            if (param->adv_start_cmpl.status == 20) {
//            	adv_params.adv_int_min        = 0x20;
//            	adv_params.adv_int_max        = 0x40;
//            	adv_params.adv_type           = ADV_TYPE_IND;
//            	adv_params.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
//            	adv_params.channel_map        = ADV_CHNL_ALL;
//            	adv_params.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
//            	start_advertise();
            	// Pluster - fixed but still here for any case!
            	esp_restart();
            }
        }
        else {
        	advertisingOn = true;
        	ESP_LOGI(GATTS_TAG, "Advertising start success.\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        }
        else {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        	advertisingOn = false;
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTSC_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_PASSKEY_REQ_EVT:                           /* passkey request event */
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
//        esp_ble_passkey_reply(heart_rate_profile_tab[HEART_PROFILE_APP_IDX].remote_bda, true, 0x00);
		break;
	case ESP_GAP_BLE_OOB_REQ_EVT:                                /* OOB request event */
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
		break;
	case ESP_GAP_BLE_LOCAL_IR_EVT:                               /* BLE local IR event */
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_LOCAL_IR_EVT");
		break;
	case ESP_GAP_BLE_LOCAL_ER_EVT:                               /* BLE local ER event */
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_LOCAL_ER_EVT");
		break;
	case ESP_GAP_BLE_NC_REQ_EVT:
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_NC_REQ_EVT");
		break;
	case ESP_GAP_BLE_SEC_REQ_EVT:
		/* send the positive(true) security response to the peer device to accept the security request.
		If not accept the security request, should sent the security response with negative(false) accept value*/
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:  ///the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
		///show the passkey number to the user to input it in the peer deivce.
		ESP_LOGE(GATTS_TAG, "The passkey Notify number:%d", param->ble_security.key_notif.passkey);
		break;
	case ESP_GAP_BLE_KEY_EVT:
		//shows the ble key info share with peer device to the user.
		ESP_LOGI(GATTS_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
		break;
	case ESP_GAP_BLE_AUTH_CMPL_EVT: {
		esp_bd_addr_t bd_addr;
		memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
		ESP_LOGI(GATTS_TAG, "remote BD_ADDR: %08x%04x",\
				(bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
				(bd_addr[4] << 8) + bd_addr[5]);
		ESP_LOGI(GATTS_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
		ESP_LOGI(GATTS_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
		show_bonded_devices();
		break;
	}
	case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
		ESP_LOGD(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
		ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
		ESP_LOGI(GATTS_TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
		esp_log_buffer_hex(GATTS_TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
		ESP_LOGI(GATTS_TAG, "------------------------------------");
		break;
	}
    default:
        break;
    }
}

void start_scan(uint32_t duration)
{
//    stop_scan_done = false;
//    scanIsOn = true; // on scan complete event
//    uint32_t duration = 2;
	resetOpenBleConnVars();
	stop_advertise();
	rssiPassRange = START_RSSI_VAL;
    esp_ble_gap_start_scanning(duration);
}

void deinit_ble()
{
	esp_err_t ret = esp_bluedroid_disable();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s disable bluedroid failed, error code = %x\n", __func__, ret);
	ret = esp_bluedroid_deinit();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s deinit bluedroid failed, error code = %x\n", __func__, ret);
	ret = esp_bt_controller_disable();
	if (ret)
			ESP_LOGE(GATTC_TAG, "%s disable bt controller failed, error code = %x\n", __func__, ret);
}

bool sendIndicationStatusToQueue(indicationStatus_t status)
{
	if (! xQueueSend(indicationLedQueue, &status, 1000 / portTICK_RATE_MS)) {
		ESP_LOGE(INDLOG, "Failed to send to indicationLedQueue...");
		return false;
	}
	return true;
}

char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;

   }

   return key_str;
}

void show_bonded_devices(void)
{
    int dev_num = esp_ble_get_bond_device_num();

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    ESP_LOGI(GATTS_TAG, "Bonded devices number : %d\n", dev_num);

    ESP_LOGI(GATTS_TAG, "Bonded devices list : %d\n", dev_num);
    for (int i = 0; i < dev_num; i++) {
        esp_log_buffer_hex(GATTS_TAG, (void *)dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
    }

    free(dev_list);
}

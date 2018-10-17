/*
 * gatts_related.c
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */
#include "mainHeader.h"
#include "gatts_related.h"
#include "gattc_related.h"
#include "dbManage.h"
#include "ioManage.h"
#include "nvsManage.h"

// extern cars init
uint8_t indicateCharValue[16*9] = {0};
uint8_t writeCharValue[9 * 7] = {1};
uint8_t controlCharValue[20] = {0};
struct gatts_profile_inst gatts_gl_profile_tab[GATTS_PROFILE_NUM];
esp_ble_adv_params_t adv_params;
bool gatts_conn;
bool indicate_enabled;
bool notify_enabled;
bool advertisingOn;
bool sim_conn = false;

void setAdvertisingName(char *name)
{
	stop_advertise();

	esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(name);
	if (set_dev_name_ret){
		ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
	}
	else
		ESP_LOGI(GATTS_TAG, "set device name to %s succeed!", name);

    vTaskDelay(100 / portTICK_RATE_MS);
    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret){
        ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
    }
    vTaskDelay(100 / portTICK_RATE_MS);
    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);   // Check why need to do it! --> Gal looks for adv_data, nRF for scan_rsp_data
	if (ret){
		ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
	}
	start_advertise();
}

void changeIndicateCharValue(const GenericRecord& record)
{
	gatts_indicateCharValue.attr_len = record.getRecordSize();
	record.getBytesRecord(indicateCharValue);
	char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
	strcat(tmpBleSendBuffer_TAG, "[");
	char tmpItoa[5];
	if (record.m_type == 100){
		itoa(recordListCounter + 1, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
	}
	else {
		itoa(recordSendCoutner, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
	}
	strcat(tmpBleSendBuffer_TAG, "]");


	ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, indicateCharValue, gatts_indicateCharValue.attr_len, ESP_LOG_INFO);

// oldICD
//	gatts_indicateCharValue.attr_len = NUMERIC_BLE_BUFF_SIZE;
//	ESP_LOGI("DEGBUGGG", "recordType is %d", record->recordType);
//	if (record->recordType == NUMERIC_RECORD_TYPE){
//		indicateCharValue[0] = (record->recordInfo.numericRecord->timePast >> 8) & 0xFF;
//		indicateCharValue[1] = record->recordInfo.numericRecord->timePast & 0xFF;
//		indicateCharValue[2] = record->recordInfo.numericRecord->type;
//		for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
//			indicateCharValue[i] = (record->recordInfo.numericRecord->data >> (24 - 8*(i-3))) & 0xFF;
//		}
//
//		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
//		strcat(tmpBleSendBuffer_TAG, "[");
//		char tmpItoa[5];
//		if (record->recordInfo.numericRecord->type == 100){
//			itoa(recordListCounter + 1, tmpItoa, 10);
//			strcat(tmpBleSendBuffer_TAG, tmpItoa);
//		}
//		else {
//			itoa(recordSendCoutner, tmpItoa, 10);
//			strcat(tmpBleSendBuffer_TAG, tmpItoa);
//		}
//		strcat(tmpBleSendBuffer_TAG, "]");
//
//
//		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, indicateCharValue, NUMERIC_BLE_BUFF_SIZE, ESP_LOG_INFO);
//		return record->recordType;
//	}
//	else {
//		bleCharSendBuffer[0] = (record->recordInfo.charRecord->timePast >> 8) & 0xFF;
//		bleCharSendBuffer[1] = record->recordInfo.charRecord->timePast & 0xFF;
//		bleCharSendBuffer[2] = record->recordInfo.charRecord->type;
//		for (int i = 3; i < CHAR_BLE_BUFF_SIZE; i++) {
//			bleCharSendBuffer[i] = (record->recordInfo.charRecord->data[i-3]);
//		}
//
//		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
//		strcat(tmpBleSendBuffer_TAG, "[");
//		char tmpItoa[5];
//		itoa(5, tmpItoa, 10);
//		strcat(tmpBleSendBuffer_TAG, tmpItoa);
//		strcat(tmpBleSendBuffer_TAG, "]");
//
//		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleCharSendBuffer, CHAR_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
////		return record.recordType;
//	}
//	gatts_notifyCharValue.attr_value = &(record->recordInfo.numericRecord->data);
}

void changeIndicateCharValue_Nrecords(const GenericRecord* record, int n)
{
	gatts_indicateCharValue.attr_len = 0;
	for (int r = 0; r < n; r++) {
		record[r].getBytesRecord(indicateCharValue +  gatts_indicateCharValue.attr_len);

		gatts_indicateCharValue.attr_len += record[r].m_dataLen;

	//		return record->recordType;
	}

	char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
	strcat(tmpBleSendBuffer_TAG, "[");
	char tmpItoa[5];
//	ESP_LOGI("DEBUGGG", "sendingMeasurementsToSmartphone = %d", sendingMeasurementsToSmartphone);
	if (sendingMeasurementsToSmartphone){
		itoa(recordListCounter - n , tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
		strcat(tmpBleSendBuffer_TAG, "-");
		itoa(recordListCounter - 1, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
	}
	else {
		itoa(recordSendCoutner, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
		strcat(tmpBleSendBuffer_TAG, "-");
		itoa(recordSendCoutner + n - 1, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
	}
	strcat(tmpBleSendBuffer_TAG, "]");

	ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, indicateCharValue, gatts_indicateCharValue.attr_len, ESP_LOG_INFO);
}

void changeIndicateCharValue_2records(const GenericRecord* record)
{
	gatts_indicateCharValue.attr_len = 2 * 7;
//	for (int r = 0; r < 2; r++) {
//		ESP_LOGI("DEGBUGGG", "recordType is %d", record[r]->recordType);
//		if (record[r]->recordType == NUMERIC_RECORD_TYPE){
//			indicateCharValue[0 + r*7] = (record[r]->recordInfo.numericRecord->timePast >> 8) & 0xFF;
//			indicateCharValue[1 + r*7] = record[r]->recordInfo.numericRecord->timePast & 0xFF;
//			indicateCharValue[2 + r*7] = record[r]->recordInfo.numericRecord->type;
//			for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
//				indicateCharValue[i + r*7] = (record[r]->recordInfo.numericRecord->data >> (24 - 8*(i-3))) & 0xFF;
//			}
//
//			char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
//			strcat(tmpBleSendBuffer_TAG, "[");
//			char tmpItoa[5];
//			itoa(recordSendCoutner, tmpItoa, 10);
//			strcat(tmpBleSendBuffer_TAG, tmpItoa);
//			strcat(tmpBleSendBuffer_TAG, "]");
//
//			ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, indicateCharValue, NUMERIC_BLE_BUFF_SIZE, ESP_LOG_INFO);
//	//		return record->recordType;
//		}
	//	else {
	//		bleCharSendBuffer[0] = (record->recordInfo.charRecord->timePast >> 8) & 0xFF;
	//		bleCharSendBuffer[1] = record->recordInfo.charRecord->timePast & 0xFF;
	//		bleCharSendBuffer[2] = record->recordInfo.charRecord->type;
	//		for (int i = 3; i < CHAR_BLE_BUFF_SIZE; i++) {
	//			bleCharSendBuffer[i] = (record->recordInfo.charRecord->data[i-3]);
	//		}
	//
	//		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
	//		strcat(tmpBleSendBuffer_TAG, "[");
	//		char tmpItoa[5];
	//		itoa(5, tmpItoa, 10);
	//		strcat(tmpBleSendBuffer_TAG, tmpItoa);
	//		strcat(tmpBleSendBuffer_TAG, "]");
	//
	//		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, bleCharSendBuffer, CHAR_BLE_BUFF_SIZE, ESP_LOG_DEBUG);
	////		return record.recordType;
	//	}
	//	gatts_notifyCharValue.attr_value = &(record->recordInfo.numericRecord->data);
//	}
}

void stop_advertise()
{
	if (advertisingOn) {
		esp_ble_gap_stop_advertising();
		advertisingOn = false;
	}
}

void start_advertise()
{
	ESP_LOGI("DEBUGGG", "advertisingOn = %d", advertisingOn);
	if (!advertisingOn) {
		adv_config_done &= (~adv_config_flag);
		ESP_LOGI("DEBUGGG", "adv_config_done = %d adv_config_flag = %d", adv_config_done, adv_config_flag);
		if (adv_config_done == 0 || advConfigDone){ // Duplicated check...Consider to delete adv_config_done
			esp_ble_gap_start_advertising(&adv_params);
		}
		adv_config_done &= (~scan_rsp_config_flag);
		ESP_LOGI("DEBUGGG", "adv_config_done = %d scan_rsp_config_flag = %d", adv_config_done, scan_rsp_config_flag);
		if (adv_config_done == 0 || advConfigDone){ // Duplicated check...Consider to delete adv_config_done
			esp_ble_gap_start_advertising(&adv_params);
		}
		advertisingOn = true;
	}
}

void init_gatts_ble()
{
	adv_params.adv_int_min        = 0x20;
	adv_params.adv_int_max        = 0x40;
	adv_params.adv_type           = ADV_TYPE_IND;
	adv_params.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
	adv_params.channel_map        = ADV_CHNL_ALL;
	adv_params.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_cb = gatts_profile_a_event_handler;
	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if = ESP_GATT_IF_NONE;       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected = 0;
	gatts_conn = false;
	indicate_enabled = false;
	notify_enabled = false;
	advertisingOn = false;
	ESP_LOGI("DEBUGG", "there is %d  connections", gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected);

    esp_err_t ret;

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
//    ret = esp_ble_gap_register_callback(gap_event_handler);
//    if (ret){
//        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
//        return;
//    }
    ret = esp_ble_gatts_app_register(GATTS_PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
//    ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
//    if (ret){
//        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
//        return;
//    }
//    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(155); // not working...
//    if (local_mtu_ret){
//        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//    }
    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribut to you,
    and the response key means which key you can distribut to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribut to you,
    and the init key means which key you can distribut to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

}

//static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
//{
//    switch (event) {
//#ifdef CONFIG_SET_RAW_ADV_DATA
//    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
//        adv_config_done &= (~adv_config_flag);
//        if (adv_config_done==0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
//        break;
//    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
//        adv_config_done &= (~scan_rsp_config_flag);
//        if (adv_config_done==0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
//        break;
//#else
//    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
//        adv_config_done &= (~adv_config_flag);
//        if (adv_config_done == 0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
//        break;
//    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
//        adv_config_done &= (~scan_rsp_config_flag);
//        if (adv_config_done == 0){
//            esp_ble_gap_start_advertising(&adv_params);
//        }
//        break;
//#endif
//    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
//        //advertising start complete event to indicate advertising start successfully or failed
//        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
//        }
//        break;
//    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
//        }
//        else {
//            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
//        }
//        break;
//    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//         ESP_LOGI(GATTS_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
//                  param->update_conn_params.status,
//                  param->update_conn_params.min_int,
//                  param->update_conn_params.max_int,
//                  param->update_conn_params.conn_int,
//                  param->update_conn_params.latency,
//                  param->update_conn_params.timeout);
//        break;
//    default:
//        break;
//    }
//}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
    	memcpy(TEST_STRING, param->write.value, param->write.len);
        if (param->write.is_prep){
        	ESP_LOGI("DEBUGGGGG", "param->write.is_prep = %d", param->write.is_prep);
            if (prepare_write_env->prepare_buf == NULL) {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }
        else if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_handle == param->write.handle){
        	ESP_LOGI(GATTS_TAG, "prepare for sending void response");
			esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);

			if (err != ESP_OK)
				ESP_LOGE(GATTS_TAG, "Error sending void response for write request!");
        }
        else {
// Andorid is not getting any responses. ere we can see how response should be sent back.
//			ESP_LOGI(GATTS_TAG, "prepare for sending response with time value (2bytes)");
//			esp_gatt_rsp_t rsp;
//			memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
//            rsp.handle = param->write.handle;
//			rsp.attr_value.handle = param->write.handle;
//			rsp.attr_value.len = NUMERIC_BLE_BUFF_SIZE;
//			ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, sizeof(writeCharValue) %d\n", rsp.attr_value.len);
//			memcpy(rsp.attr_value.value, writeCharValue, sizeof(writeCharValue));
//			esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id,
//										ESP_GATT_OK, &rsp);
//			if (err != ESP_OK)
//				ESP_LOGE(GATTS_TAG, "Error sending response for write request!");
        	// empty resonse with GAT_OK
        	ESP_LOGI(GATTS_TAG, "prepare for sending void response");
			esp_err_t err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);

			if (err != ESP_OK)
				ESP_LOGE(GATTS_TAG, "Error sending void response for write request!");
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT: {
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.is_primary = true;
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_A;
//        memcpy(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128);

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
#ifdef CONFIG_SET_RAW_ADV_DATA
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret){
            ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        adv_config_done |= adv_config_flag;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret){
            ESP_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        adv_config_done |= scan_rsp_config_flag;
#else
        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;
        advConfigDone = true;

#endif
        esp_ble_gatts_create_service(gatts_if, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    }
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        if (param->read.handle == gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle) {
			rsp.attr_value.handle = param->read.handle;
			rsp.attr_value.len = sizeof(indicateCharValue);
			ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, sizeof(notifyCharValue) %d\n", rsp.attr_value.len);
			memcpy(rsp.attr_value.value, indicateCharValue, sizeof(indicateCharValue));
			esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
										ESP_GATT_OK, &rsp);
        }
        if (param->read.handle == gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_handle) {
			rsp.attr_value.handle = param->read.handle;
			rsp.attr_value.len = gatts_writeCharValue.attr_len;
			ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, gatts_writeCharValue.attr_len %d\n", rsp.attr_value.len);
			memcpy(rsp.attr_value.value, writeCharValue, sizeof(writeCharValue));
			esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
										ESP_GATT_OK, &rsp);
		}
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
            if (param->write.handle == gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_handle)
            {
            	gatts_writeCharValue.attr_len = 0;
            	while (gatts_writeCharValue.attr_len < param->write.len) {
					GenericRecord record;
					record.setBytesRecord((param->write.value) + gatts_writeCharValue.attr_len);
					if (/*record.m_timePast == 0x00FF || */record.m_timePast == 0) {
						ESP_LOGI("DEBUGGG", "I'm making new record...");
						int tmpTimePast = timePastSeconds;
						updateTimeParams();
						if (timePastSeconds == tmpTimePast) {
							record.m_eventNum = ++timePastEventNum;
						}
						else {
							record.m_eventNum = timePastEventNum = 0;
						}
						record.m_timePast = timePastSeconds;

//						record.getBytesEventPlusTime((gatts_writeCharValue.attr_value) + gatts_writeCharValue.attr_len);
						record.getBytesRecord((gatts_writeCharValue.attr_value) + gatts_writeCharValue.attr_len);

						addToRecordList(record, true, 0);
						automaticRecordUpdate(record.m_type);
					}
					else {
						// O(n) finding... should be better algorithm
						ESP_LOGI("DEBUGGG", "I'm updating existed record...");
						int recordIndex = findMatchingIndex(record);

						if (recordIndex != -1) { // Overwriting existed
							updateRecordByIndex(recordIndex, record.m_data);
						}
					}
					gatts_writeCharValue.attr_len += record.getRecordSize();

            	ESP_LOGI("DEBUGG", "Time = %d, Type = %d, DataLen = %d", record.m_timePast, record.m_type, record.m_dataLen);
				ESP_LOGI(GATTS_TAG, "gatts_writeCharValue.attr_value is: (LEN is %d)", gatts_writeCharValue.attr_len);  // DEBUGGG
				esp_log_buffer_hex(GATTS_TAG, gatts_writeCharValue.attr_value, gatts_writeCharValue.attr_len); // DEBUGGG

            	}

// oldICD
//            	ESP_LOGI(GATTS_TAG, "gatts_writeCharValue.attr_value is: (LEN is %d)", gatts_writeCharValue.attr_len); // DEBUGGG
//            	esp_log_buffer_hex(GATTS_TAG, gatts_writeCharValue.attr_value, gatts_writeCharValue.attr_len); // DEBUGGG
//            	gatts_writeCharValue.attr_len = param->write.len;
//            	uint8_t numOfRecords = param->write.len / 7;
//            	// need to re-move it for a function. this and the above one.
//            	uint16_t tmpTime;
//            	uint8_t tmpType;
//            	uint32_t tmpData;
//            	for (int r = 0; r < numOfRecords; r++) {
//                	tmpTime = (param->write.value[0 + 7 * r] << 8) | (param->write.value[1 + 7 * r]);
//                	if (tmpTime == 0x00FF || tmpTime==0) {
//                		tmpTime = timePastMinutes;
//
//                		gatts_writeCharValue.attr_value[0 + 7 * r] = (tmpTime >> 8) & 0xFF;
//                		gatts_writeCharValue.attr_value[1 + 7 * r] = tmpTime & 0xFF;
//                	}
//                	tmpType = param->write.value[2 + 7 * r];
//                	gatts_writeCharValue.attr_value[2 + 7 * r] =  param->write.value[2 + 7 * r];
//    				tmpData = 0;
//    				for (int i = 0; i < NUMERIC_BLE_BUFF_SIZE - 3; i++) {
//    					tmpData |= (param->write.value[(3 + i) + 7 * r]) << (24 - 8*i);
//    					gatts_writeCharValue.attr_value[(3 + i) + 7 * r] =  param->write.value[(3 + i) + 7 * r];
//    				}
//    				ESP_LOGI("DEBUGG", "tmpTime = %d, tmpType = %d, tmpData = %d", tmpTime, tmpType, tmpData);
//                	ESP_LOGI(GATTS_TAG, "gatts_writeCharValue.attr_value is: (LEN is %d)", gatts_writeCharValue.attr_len);  // DEBUGGG
//                	esp_log_buffer_hex(GATTS_TAG, gatts_writeCharValue.attr_value, NUMERIC_BLE_BUFF_SIZE); // DEBUGGG
//                	record_inst record = makeRecord(NUMERIC_RECORD_TYPE, tmpTime, tmpType, tmpData, NULL);
//                	addToRecordList(record, true, NULL);
//            	}

            	printRecordList(11);
            }

            if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_handle == param->write.handle) {// Medical Simulator Related
            	if (param->write.len == 2 || param->write.len == 3) {

            		if (!memcmp(param->write.value, "on", 2))
            			passMonitorMeasurements = false;
            		if (!memcmp(param->write.value, "off", 3))
            			passMonitorMeasurements = true;

            	}
            	else {
            		addManualMeasurements(param->write.value);// Medical Simulator Related
            	}
            }


            if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (indicateCharProperty & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        notify_enabled = true;
//                        uint8_t notify_data[15];
//                        for (int i = 0; i < sizeof(notify_data); ++i)
//                        {
//                            notify_data[i] = i%0xff;
//                        }
                        sendRecordListByNotify();
                        //the size of notify_data[] need less than MTU size
                        // no need to send before it past 10 sec....
//                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle,
//                                                sizeof(notify_data), notify_data, false);
                    }
                }else if (descr_value == 0x0002){
                    if (indicateCharProperty & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                        indicate_enabled = true;
//                        uint8_t indicate_data[15];
//                        for (int i = 0; i < sizeof(indicate_data); ++i)
//                        {
//                            indicate_data[i] = i%0xff;
//                        }
                        //the size of indicate_data[] need less than MTU size
//                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle,
//                                                sizeof(indicate_data), indicate_data, true);
                        sendRecordListByIndicate();
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    indicate_enabled = false;
                    notify_enabled = false;
                }else{
                    ESP_LOGE(GATTS_TAG, "unknown descr value");
                    esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
                }

            }
        }

        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);  // here should be sent a response!
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        //N_REC_IN_CHAR = param->mtu.mtu / 7;
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT: {
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_uuid.len = ESP_UUID_LEN_16;
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_uuid.uuid.uuid16 = GATTS_NOTIFY_CHAR_UUID_A;
//        memcpy(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].char_uuid.uuid.uuid128, bb_measurements_characteristicUUID, ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle);
        indicateCharProperty = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE;
        writeCharProperty = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_READ;
        controlCharProperty = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_uuid,
#ifdef SECURE_BLE
														ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
#else
														ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
#endif
                                                        indicateCharProperty,
														&gatts_indicateCharValue,
														NULL);
        if (add_char_ret){
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
        }

        break;
    }
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d (uuid is 0x%x), service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.char_uuid.uuid.uuid16, param->add_char.service_handle);
        if (param->add_char.char_uuid.uuid.uuid16 == GATTS_NOTIFY_CHAR_UUID_A) {
			gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle = param->add_char.attr_handle;
			gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
			gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
			esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
			if (get_attr_ret == ESP_FAIL){
				ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
			}

			ESP_LOGI(GATTS_TAG, "the gatts notify char length = 0x%x(%d d)\n", length, length);
			for(int i = 0; i < length; i++){
				ESP_LOGI(GATTS_TAG, "prf_char[0x%x] =0x%x\n",i,prf_char[i]);
			}
			esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid,
#ifdef SECURE_BLE
																	ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
#else
																	ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
#endif
																	NULL, NULL);
			if (add_descr_ret){
				ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
			}
        }
        else if (param->add_char.char_uuid.uuid.uuid16 == GATTS_WRITE_CHAR_UUID_A){
        	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_handle = param->add_char.attr_handle;
			esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
			if (get_attr_ret == ESP_FAIL){
				ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
			}

			ESP_LOGI(GATTS_TAG, "the gatts write char length = 0x%x(%d d)\n", length, length);
			for(int i = 0; i < length; i++){
				ESP_LOGI(GATTS_TAG, "prf_char[0x%x] =0x%x\n",i,prf_char[i]);
			}

	        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_uuid.len = ESP_UUID_LEN_16;
			gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_uuid.uuid.uuid16 = GATTS_CONTROL_CHAR_UUID_A;
			esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_uuid,
#ifdef SECURE_BLE
													ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
#else
													ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
#endif
													controlCharProperty,
													&gatts_controlCharValue,
													NULL);
        }
        else if (param->add_char.char_uuid.uuid.uuid16 == GATTS_CONTROL_CHAR_UUID_A) {
        	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].control_char_handle = param->add_char.attr_handle;
			esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
			if (get_attr_ret == ESP_FAIL){
				ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
			}

			ESP_LOGI(GATTS_TAG, "the gatts control char length = 0x%x(%d d)\n", length, length);
			for(int i = 0; i < length; i++){
				ESP_LOGI(GATTS_TAG, "prf_char[0x%x] =0x%x\n",i,prf_char[i]);
			}
//			start_advertise();
			gattsInitDone = true;
			init_gattc_ble(); //now here
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);

        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_uuid.len = ESP_UUID_LEN_16;
		gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_uuid.uuid.uuid16 = GATTS_WRITE_CHAR_UUID_A;
		esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].write_char_uuid,
#ifdef SECURE_BLE
														ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
#else
														ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
#endif
														writeCharProperty,
														&gatts_writeCharValue,
														NULL);
		if (add_char_ret){
			ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
		}

//        start_advertise();
//		init_gattc_ble(); // was here
        break;
    }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
    	if (param->connect.remote_bda[0] == 0x00 && param->connect.remote_bda[1] == 0x0b && param->connect.remote_bda[2] == 0x57)
    		break;
    	if (param->connect.remote_bda[0] == 0x30 && param->connect.remote_bda[1] == 0xae && param->connect.remote_bda[2] == 0xa4)
			break;
#ifdef MEDIC_SIM_OFF
    	simulatorDevice = false;
#endif
    	advertisingOn = false;
//        esp_ble_conn_update_params_t conn_params = {0};
//        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
//        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
//        conn_params.latency = 0;
//        conn_params.max_int = 0x18;    // max_int = 0x20*1.25ms = 40ms
//        conn_params.min_int = 0x18;    // min_int = 0x10*1.25ms = 20ms
//        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        if (!simulatorDevice) {// Medical Simulator Related
        	gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id = param->connect.conn_id; //00:0b:57:be:5d:84:
        	memcpy(smartphoneMacAddress, param->connect.remote_bda, MAC_SIZE);
    		gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected++;
        }
        else {// Medical Simulator Related
        	memcpy(simulatorMacAddress, param->connect.remote_bda, MAC_SIZE);
        	sim_conn = true;
        }

		if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected == 1 && !simulatorDevice) {
			gatts_conn = true;
			timer_start(TIMER_GROUP_0, TIMER_0);
			ESP_LOGI(GATTS_TAG, "gatts_conn = true, timer0(g0) started. attempt to change BioBeat sample rate");
		}
#ifdef SECURE_BLE
		if (!simulatorDevice) // Medical Simulator Related
		/* start security connect with peer device when receive the connect event sent by the master */
			esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
#endif
		simulatorDevice = true; // can be inside if(!simulatorDevice)...// Medical Simulator Related
#ifdef OLD_INDICATIONS
		turnBlueLed(true);
#endif
        //start sent the update connection parameters to the peer device.
//        esp_ble_gap_update_conn_params(&conn_params);
//        esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(155); // not my responsible
//        if (local_mtu_ret){
//            ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//        }
//        if (!gattc_conn && !scanIsOn)
//        	start_scan(90);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
    	if (param->disconnect.remote_bda[0] == 0x00 && param->disconnect.remote_bda[1] == 0x0b && param->disconnect.remote_bda[2] == 0x57)
				break;
		if (param->disconnect.remote_bda[0] == 0x30 && param->disconnect.remote_bda[1] == 0xae && param->disconnect.remote_bda[2] == 0xa4)
				break;
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d (conn_id = %d)", param->disconnect.reason, param->disconnect.conn_id);
        if (sim_conn && !memcmp(param->disconnect.remote_bda, simulatorMacAddress, MAC_SIZE)) {// Medical Simulator Related
        	ESP_LOGI(GATTS_TAG, "simulator disconnected");
        	memset(simulatorMacAddress, 0, MAC_SIZE);
        	sim_conn = false;
    		if (gattc_conn)
    			start_advertise();
        	break;
        }
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected--;
        if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected == 0) {
        	ESP_LOGI(GATTS_TAG, "connected = 0, timer0(g0) paused");
        	timer_pause(TIMER_GROUP_0, TIMER_0);
        	gatts_conn = false;
        	memset(smartphoneMacAddress, 0, MAC_SIZE);
        	ESP_LOGI(GATTS_TAG, "smartphone disconnected");
        	esp_log_buffer_hex(GATTS_TAG, smartphoneMacAddress, MAC_SIZE);
        	if (gattc_conn) {
#ifdef REAL_TIME_TEST_PROG
        		changeBiobeatSamplePeriod(120);
#else
				changeBiobeatSamplePeriod(5);
#endif
        	}
#ifdef OLD_INDICATIONS
        	turnBlueLed(false);
#endif
        }
        if (!gattc_conn && !scanIsOn && !openBleConnection)
        	start_scan(90);
        //esp_ble_gap_start_advertising(&adv_params);
		// Medical Simulator Related
		if (gattc_conn)
			start_advertise();
        break;
    case ESP_GATTS_CONF_EVT: {
    	ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
            ESP_LOGE(GATTS_TAG, "ESP_GATTS_CONF_EVT, ERRORRRRR");
            break;
//            recordSendCoutner -= N_REC_IN_CHAR;
//            ESP_LOGE(GATTS_TAG, "ESP_GATTS_CONF_EVT, recordSendCoutner -= N_REC_IN_CHAR !!");
        }
        if (gatts_conn) {
        	if (recordListCounter <= N_REC_IN_CHAR && sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)) {
        		if (recordListCounter == N_REC_IN_CHAR) {
        			// add type 99 - record list counter
					GenericRecord endOfSyncRecord(99);
					uint8_t endOfSyncData[3];
					endOfSyncData[0] = ((recordListCounter - 1) & 0xFF00) >> 8;
					endOfSyncData[1] = (recordListCounter - 1) & 0xFF;
					endOfSyncData[2] = getGattClientConnStatus();
					endOfSyncRecord.setBytesData(endOfSyncData);
					ESP_LOGI(GATTS_TAG, "Record Type 99 almost has been Sent!!!");
					changeIndicateCharValue(endOfSyncRecord);
					char tmpRecordBuffer_TAG[25] = "Record #";
					char tmpItoa[5];
					itoa(recordListCounter, tmpItoa, 10);
					strcat(tmpRecordBuffer_TAG, tmpItoa);
					strcat(tmpRecordBuffer_TAG, " - ");
					esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
																gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
					ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");
        		}
        		sendMeasurementDeviceStatus();
        		sendingRecordsToSmartphone = false;


		        if (!gattc_conn && !scanIsOn)
		        	start_scan(90);
#ifdef REAL_TIME_TEST_PROG
		        if (gattc_conn)
		        	changeBiobeatSamplePeriod(0x1E);
#endif
				if (gattc_conn)// Medical Simulator Related
					start_advertise();
        	}
        	else if ((recordSendCoutner + N_REC_IN_CHAR) < recordListCounter && sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)){
	        	GenericRecord* tmpNRecords = new GenericRecord[N_REC_IN_CHAR];
        		if (!tmpNRecords)
        			ESP_LOGE(GATTS_TAG, "FAILED MALLOC tmpNRecords")
        		for (int i = 0; i < N_REC_IN_CHAR; i++) {
        			tmpNRecords[i] = getRecord(recordSendCoutner + i);
        		}
        		changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
				esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
												gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
	//        	recordSendCoutner++;
				recordSendCoutner += N_REC_IN_CHAR;
				// if n = 2 so
	//        	if (recordSendCoutner >= recordListCounter) {
	//
	//
	//        	}
	//        	for (int i = 0; i < N_REC_IN_CHAR; i++) {
	//        		free(tmpNRecords[i]);
	//        	}
				delete[] tmpNRecords;
			}
			else if ((recordSendCoutner + N_REC_IN_CHAR) >= recordListCounter && sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)) {
	//        	int n = 3; // oldICD
//				record_inst** tmpNRecords;
	//			tmpNRecords = (record_inst**)malloc(sizeof(record_inst*) * n);
	//			*tmpNRecords = (record_inst*)malloc(RECORD_SIZE * n);
				ESP_LOGI("DEBUGGG_GATTS", "recordSendCoutner = %d, recordListCounter = %d.", recordSendCoutner, recordListCounter);
//				if (recordListCounter % N_REC_IN_CHAR == 1) { // || recordSendCoutner == recordListCounter
//					ESP_LOGI("DEBUGGG_GATTS", "1st type");
//					// send type 99 - record list counter
//					record_inst recordListCounterRecord;
//					uint32_t dataOfType99 = 0;
//		//			dataOfType99 = (recordListCounter & 0xFFFF) | ((uint32_t)evt.timer_counter_value << 16);
//		//				recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, dataOfType99, NULL); // when Gal will be ready get off from comment
//					recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
//					changeIndicateCharValue(&recordListCounterRecord);
//					// Client need to approve / authenticate that the phone got all records
//					// bleSendNumericRecord(numericRecordListCounterRecord);
//					char tmpRecordBuffer_TAG[25] = "Record #";
//					char tmpItoa[5];
//					itoa(recordListCounter, tmpItoa, 10);
//					strcat(tmpRecordBuffer_TAG, tmpItoa);
//					strcat(tmpRecordBuffer_TAG, " - ");
//					esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//																gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
//					ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");
//
//					recordSendCoutner++;
//				}
				/*else*/ if (recordListCounter % N_REC_IN_CHAR == 0) {
					ESP_LOGI("DEBUGGG_GATTS", "2nd type");
					GenericRecord* tmpNRecords = new GenericRecord[N_REC_IN_CHAR];
					if (!tmpNRecords)
						ESP_LOGE(GATTS_TAG, "FAILED MALLOC tmpNRecords")
					for (int i = 0; i < N_REC_IN_CHAR; i++) {
						tmpNRecords[i] = getRecord(recordSendCoutner + i);
					}
					changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
					esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);

					// send type 99 - record list counter
					GenericRecord endOfSyncRecord(99);
					uint8_t endOfSyncData[3];
					endOfSyncData[0] = ((recordListCounter - 1) & 0xFF00) >> 8;
					endOfSyncData[1] = (recordListCounter - 1) & 0xFF;
					endOfSyncData[2] = getGattClientConnStatus();
					endOfSyncRecord.setBytesData(endOfSyncData);
					ESP_LOGI(GATTS_TAG, "Record Type 99 almost has been Sent!!!");
					changeIndicateCharValue(endOfSyncRecord);
					// Client need to approve / authenticate that the phone got all records
					// bleSendNumericRecord(numericRecordListCounterRecord);
					char tmpRecordBuffer_TAG[25] = "Record #";
					char tmpItoa[5];
					itoa(recordListCounter, tmpItoa, 10);
					strcat(tmpRecordBuffer_TAG, tmpItoa);
					strcat(tmpRecordBuffer_TAG, " - ");
					esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
																gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
					ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

	//	        	for (int i = 0; i < N_REC_IN_CHAR; i++) {
	//	        		free(tmpNRecords[i]);
	//	        	}
					delete[] tmpNRecords;

				}
				else {//if (recordSendCoutner >= recordListCounter){ //Was recordSendCoutner <= recordListCounter;
					ESP_LOGI("DEBUGGG_GATTS", "3rd type");
					GenericRecord* tmpNRecords = new GenericRecord[(recordListCounter % N_REC_IN_CHAR) + 1];
					// all records to recordListCounter + another one of 99
					if (!tmpNRecords)
						ESP_LOGE(GATTS_TAG, "FAILED MALLOC tmpNRecords")
					for (int i = 0; i < (recordListCounter % N_REC_IN_CHAR); i++) {
						tmpNRecords[i] = getRecord(recordSendCoutner + i);
					}
					// add type 99 - record list counter
					GenericRecord endOfSyncRecord(99);
					uint8_t endOfSyncData[3];
					endOfSyncData[0] = ((recordListCounter - 1) & 0xFF00) >> 8;
					endOfSyncData[1] = (recordListCounter - 1) & 0xFF;
					endOfSyncData[2] = getGattClientConnStatus();
					endOfSyncRecord.setBytesData(endOfSyncData);
					tmpNRecords[(recordListCounter % N_REC_IN_CHAR)] = endOfSyncRecord;
					ESP_LOGI(GATTS_TAG, "Record Type 99 has been Sent!!!");
					changeIndicateCharValue_Nrecords(tmpNRecords, (recordListCounter % N_REC_IN_CHAR) + 1);
					esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
					char tmpRecordBuffer_TAG[25] = "Record #";
					char tmpItoa[5];
					itoa(recordListCounter, tmpItoa, 10);
					strcat(tmpRecordBuffer_TAG, tmpItoa);
					strcat(tmpRecordBuffer_TAG, " - ");
					ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

	//				for (int i = 0; i < (recordListCounter % N_REC_IN_CHAR); i++)
	//					free(tmpNRecords[i]);
					delete[] tmpNRecords;
				}

				sendMeasurementDeviceStatus();
				sendingRecordsToSmartphone = false;

		        if (!gattc_conn && !scanIsOn)
		        	start_scan(90);
#ifdef REAL_TIME_TEST_PROG
		        if (gattc_conn)
		        	changeBiobeatSamplePeriod(0x1E);
#endif
				if (gattc_conn)// Medical Simulator Related
					start_advertise();
			}

			if (mesurementSendCounter < validMeasurementsCounter && sendingMeasurementsToSmartphone) {
				ESP_LOGE(GATTS_TAG, "sendingMeasurementsToSmartphone is TRUE, Should not get here!");
//				ESP_LOGI(GATTS_TAG, "sending next measure %d", mesurementSendCounter);
//	//			changeIndicateCharValue(getRecord(recordListCounter - 4 + mesurementSendCounter));
//	//			esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//	//			        									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
//	//			mesurementSendCounter++;// recordListCounter - 4 + mesurementSendCounter
//
//				record_inst** tmpNRecords;
//				tmpNRecords = (record_inst**)malloc(sizeof(record_inst*) * N_REC_IN_CHAR);
//	//			for (int i = 0; i < N_REC_IN_CHAR; i++) {
//	//				tmpNRecords[i] = (record_inst*)malloc(RECORD_SIZE);
//	//			}
//	//			*tmpNRecords = (record_inst*)malloc(RECORD_SIZE * n);
//				for (int i = 0; i < N_REC_IN_CHAR; i++) {
//					tmpNRecords[i] = getRecord(recordListCounter - validMeasurementsCounter + mesurementSendCounter + i);
//				}
//				changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
//	//        	changeIndicateCharValue(getRecord(recordSendCoutner));
//				esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//												gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
//	//        	recordSendCoutner++;
//				mesurementSendCounter += N_REC_IN_CHAR;
//
//	//			for (int i = 0; i < (recordListCounter % N_REC_IN_CHAR); i++)
//	//				free(tmpNRecords[i]);
//				free(tmpNRecords);


			}
			else if (sendingMeasurementsToSmartphone) {
				sendingMeasurementsToSmartphone = false;
				mesurementSendCounter = 0;
				validMeasurementsCounter = 0;
			}
        }
        break;
    }
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < GATTS_PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gatts_gl_profile_tab[idx].gatts_if) {
                if (gatts_gl_profile_tab[idx].gatts_cb) {
                    gatts_gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void sendRecordByIndicate(const GenericRecord& record)
{
	ESP_LOGI(GATTS_TAG, "We're going to send by indicate one nfc record");
	changeIndicateCharValue(record);
	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);

}

void sendRecordListByIndicate()
{
	recordSendCoutner = 0;
	sendingRecordsToSmartphone = true;
	ESP_LOGI(GATTS_TAG, "We're going to send by indicate %d records", recordListCounter);
//	changeIndicateCharValue(getRecord(recordSendCoutner));
	// for sending 2 records in one Characteristics, un-comment following code
//	ESP_LOGI(GATTS_TAG, "We're going to send by indicate %d and %d records", recordListCounter, recordListCounter+1);
//	record_inst* tmp2Recs[2];
//	tmp2Recs[0] = getRecord(recordSendCoutner);
//	recordSendCoutner++;
//	tmp2Recs[1] = getRecord(recordSendCoutner);
//	changeIndicateCharValue_2records(tmp2Recs);
	// end on sending 2 records
//	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
//	recordSendCoutner++;

	// for sending N records in one Characteristics, un-comment following code
//	int n = 3; //n represent how many records in one char'
	ESP_LOGI(GATTS_TAG, "We're going to send by one indicate %d records in one characteristic", N_REC_IN_CHAR);
	GenericRecord* tmpNRecords = new GenericRecord[N_REC_IN_CHAR];
	for (int i = 0; i < N_REC_IN_CHAR; i++) {
		tmpNRecords[i] = getRecord(recordSendCoutner + i);
	}
	changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
										gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
	recordSendCoutner += N_REC_IN_CHAR;

	delete[] tmpNRecords;
}
void sendRecordListByNotify()
{
	recordSendCoutner = 0;
	sendingRecordsToSmartphone = true;
	ESP_LOGI(GATTS_TAG, "We're going to send by notify %d records", recordListCounter);

//	changeIndicateCharValue(getRecord(recordSendCoutner));
//	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, false);
//	recordSendCoutner++;

	// for sending N records in one Characteristics, un-comment following code
//	int n = 2; //n represent how many records in one char'
	if (recordListCounter == 0) {
		ESP_LOGI(GATTS_TAG, "there is %d records in one characteristic", recordListCounter);
		// send type 99 - record list counter
		GenericRecord endOfSyncRecord(99);
		uint8_t endOfSyncData[3];
		endOfSyncData[0] = ((recordListCounter - 1) & 0xFF00) >> 8;
		endOfSyncData[1] = (recordListCounter - 1) & 0xFF;
		endOfSyncData[2] = getGattClientConnStatus();
		endOfSyncRecord.setBytesData(endOfSyncData);
		changeIndicateCharValue(endOfSyncRecord);
		// Client need to approve / authenticate that the phone got all records
		// bleSendNumericRecord(numericRecordListCounterRecord);
		char tmpRecordBuffer_TAG[25] = "Record #";
		char tmpItoa[5];
		itoa(recordListCounter, tmpItoa, 10);
		strcat(tmpRecordBuffer_TAG, tmpItoa);
		strcat(tmpRecordBuffer_TAG, " - ");
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
		ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

		sendMeasurementDeviceStatus();
		sendingRecordsToSmartphone = false;


        if (!gattc_conn && !scanIsOn)
        	start_scan(90);
#ifdef REAL_TIME_TEST_PROG
        if (gattc_conn)
        	changeBiobeatSamplePeriod(0x1E);
#endif
		if (gattc_conn)// Medical Simulator Related
			start_advertise();
		return;
	}
	if (N_REC_IN_CHAR > 16) { // in case something goes wrong ---> actually it is a plaster to avoid conflits cause by sending records and measurements together.
		ESP_LOGI(GATTS_TAG, "N_REC_IN_CHAR = %d", N_REC_IN_CHAR);
		N_REC_IN_CHAR = 16;
	}
	ESP_LOGI(GATTS_TAG, "We're going to send by one notify %d records in one characteristic", N_REC_IN_CHAR);

	if (recordListCounter < N_REC_IN_CHAR) {
		GenericRecord* tmpNRecords = new GenericRecord[recordListCounter + 1];
		// all records to recordListCounter + another one of 99
		if (!tmpNRecords)
			ESP_LOGE(GATTS_TAG, "FAILED MALLOC tmpNRecords")
		for (int i = 0; i < recordListCounter; i++) {
			tmpNRecords[i] = getRecord(recordSendCoutner + i);
		}
		// add type 99 - record list counter
		GenericRecord endOfSyncRecord(99);
		uint8_t endOfSyncData[3];
		endOfSyncData[0] = ((recordListCounter - 1) & 0xFF00) >> 8;
		endOfSyncData[1] = (recordListCounter - 1) & 0xFF;
		endOfSyncData[2] = getGattClientConnStatus();
		endOfSyncRecord.setBytesData(endOfSyncData);
		tmpNRecords[recordListCounter] = endOfSyncRecord;
		ESP_LOGI(GATTS_TAG, "Record Type 99 has been Sent!!!");
		changeIndicateCharValue_Nrecords(tmpNRecords, recordListCounter + 1);
		sendingRecordsToSmartphone = false;
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, false);
		sendMeasurementDeviceStatus();


        if (!gattc_conn && !scanIsOn)
        	start_scan(90);
#ifdef REAL_TIME_TEST_PROG
        if (gattc_conn)
        	changeBiobeatSamplePeriod(0x1E);
#endif
		delete[] tmpNRecords;
		if (gattc_conn)// Medical Simulator Related
			start_advertise();
	}
	else if (recordListCounter == N_REC_IN_CHAR) {
		GenericRecord* tmpNRecords = new GenericRecord[recordListCounter];
		if (!tmpNRecords)
			ESP_LOGE(GATTS_TAG, "FAILED MALLOC tmpNRecords")
		for (int i = 0; i < recordListCounter; i++) {
			tmpNRecords[i] = getRecord(recordSendCoutner + i);
		}
		changeIndicateCharValue_Nrecords(tmpNRecords, recordListCounter);
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, false);

		delete[] tmpNRecords;
	}
	else {
		GenericRecord* tmpNRecords = new GenericRecord[N_REC_IN_CHAR];
		for (int i = 0; i < N_REC_IN_CHAR; i++) {
			tmpNRecords[i] = getRecord(recordSendCoutner + i);
		}
		changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
											gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, false);
		recordSendCoutner += N_REC_IN_CHAR;
		delete[] tmpNRecords;
	}
}

void sendMeasurementsByIndicate()
{
	ESP_LOGI(GATTS_TAG, "We're going to send by one %s %d measurements in one characteristic", indicate_enabled ? "indicate" : "notify", validMeasurementsCounter);
	sendingMeasurementsToSmartphone = true;

	if (validMeasurementsCounter) {
		if (validMeasurementsCounter > 5) {
			ESP_LOGE(GATTS_TAG, "validMeasurementsCounter = %d(> 5), shouldn't exceed 5!", validMeasurementsCounter);
			validMeasurementsCounter = 5;
		}
		changeIndicateCharValue(getRecord(recordListCounter - 1));
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
											gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);

		validMeasurementsCounter = 0;
		sendingMeasurementsToSmartphone = false;
	}

// oldICD
//	if (N_REC_IN_CHAR > 3) {
//		sendingMeasurementsToSmartphone = true;
//		ESP_LOGI(GATTS_TAG, "We're going to send by one %s %d measurements in one characteristic", indicate_enabled ? "indicate" : "notify", validMeasurementsCounter);
//		if (validMeasurementsCounter) {
//			if (validMeasurementsCounter > 4)
//				validMeasurementsCounter = 4;
//			record_inst** tmpNRecords;
//			tmpNRecords = (record_inst**)malloc(sizeof(record_inst*) * validMeasurementsCounter);
//	//		for (int i = 0; i < 4; i++) {
//	//			tmpNRecords[i] = (record_inst*)malloc(RECORD_SIZE);
//	//		}
//		//	*tmpNRecords = (record_inst*)malloc(RECORD_SIZE * n);
//			for (int i = 0; i < validMeasurementsCounter; i++) {
//				tmpNRecords[i] = getRecord(recordListCounter - validMeasurementsCounter + i);
//			}
//			changeIndicateCharValue_Nrecords(tmpNRecords, validMeasurementsCounter);
//			esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//												gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
//			free(tmpNRecords);
//			validMeasurementsCounter = 0;
//			sendingMeasurementsToSmartphone = false;
//		}
//		return;
//	}
//	mesurementSendCounter = 0;
//	sendingMeasurementsToSmartphone = true;
////	ESP_LOGI(GATTS_TAG, "We're going to send by indicate %d measurements records", 4);
////	changeIndicateCharValue(getRecord(recordListCounter - 4 + mesurementSendCounter));
////	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
////										gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
////	mesurementSendCounter++;
//
//// for sending N records in one Characteristics, un-comment following code
////	int n = 3; //n represent how many records in one char'
//	ESP_LOGI(GATTS_TAG, "We're going to send by one %s %d records in one characteristic", indicate_enabled ? "indicate" : "notify", N_REC_IN_CHAR);
//	record_inst** tmpNRecords;
//	tmpNRecords = (record_inst**)malloc(sizeof(record_inst*) * N_REC_IN_CHAR);
////	for (int i = 0; i < N_REC_IN_CHAR; i++) {
////		tmpNRecords[i] = (record_inst*)malloc(RECORD_SIZE);
////	}
////	*tmpNRecords = (record_inst*)malloc(RECORD_SIZE * n);
//	for (int i = 0; i < N_REC_IN_CHAR; i++) {
//		tmpNRecords[i] = getRecord(recordListCounter - 4 + mesurementSendCounter + i);
//	}
//	changeIndicateCharValue_Nrecords(tmpNRecords, N_REC_IN_CHAR);
//	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
//										gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
//	mesurementSendCounter += N_REC_IN_CHAR;
//	free(tmpNRecords);

}

void sendMeasurementDeviceStatus()
{
	if (gatts_conn) {
		uint8_t tmpGattClientConnStatus = getGattClientConnStatus();
		GenericRecord measureDeviceStatusRecord(100, &tmpGattClientConnStatus);
		changeIndicateCharValue(measureDeviceStatusRecord);
		// Client need to approve / authenticate that the phone got all records
		// bleSendNumericRecord(numericRecordListCounterRecord);
		char tmpRecordBuffer_TAG[25] = "Status Record";
		esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
													gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, indicate_enabled ? true : false);
		ESP_LOGI(tmpRecordBuffer_TAG, "Status Record = %d, has been Sent!!!", getGattClientConnStatus());
	}
	else
		ESP_LOGD("DEBUGGG", "gatts_conn = %s", gatts_conn ? "true" : "false");
}

void addManualMeasurements(uint8_t* dataString) // Medical Simulator Related
{
	ESP_LOGI(GATTC_TAG, "Manual measurements string = %s", dataString);
	char measurementMatrix[5][4] = {0};
	for (int i = 0, runIndex = 0; i < 5; i++) {
		for (int j = 0; j < 4; j++) {
			if (dataString[runIndex] == ' ' || dataString[runIndex] == 0) {
				validMeasurementsCounter++;
				ESP_LOGI(GATTC_TAG, "runIndex = %d", runIndex);
				runIndex++;
				break;
			}
			ESP_LOGI(GATTC_TAG, "runIndex = %d", runIndex);
			measurementMatrix[i][j] = dataString[runIndex];
			runIndex++;
		}
	}
	validMeasurementsCounter--; //beacuse BP is from 2 numberr
	int rr = atoi(measurementMatrix[0]);
	int spo2 = atoi(measurementMatrix[1]);
	int hr = atoi(measurementMatrix[2]);
	int sbp = atoi(measurementMatrix[3]);
	int dbp = atoi(measurementMatrix[4]);

	int monitorMeasurementsData[5] = {rr, spo2, hr, sbp, dbp};
	GenericRecord record(80, (uint8_t*)monitorMeasurementsData);
	ESP_LOGI(GATTC_TAG, "RR: %d", rr);

	ESP_LOGI(GATTC_TAG, "SPO2: %d", spo2);

	ESP_LOGI(GATTC_TAG, "HR: %d", hr);

	ESP_LOGI(GATTC_TAG, "SBP: %d", sbp);
	ESP_LOGI(GATTC_TAG, "DBP: %d", dbp);
	uint32_t tmpVal = ((sbp << 16) & 0xFFFF0000) | (dbp & 0x0000FFFF);

	addToRecordList(record, true, NULL);

	printRecordList(20);

	if (gatts_conn && !sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)) {
		sendMeasurementsByIndicate();
	}

}

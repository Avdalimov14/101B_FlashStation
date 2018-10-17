/*
 * gatts_related.c
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */
#include "gatts_related.h"
#include "dbManage.h"

// extern cars init
uint8_t indicateCharValue[MAX_CHAR_LENGTH] = {0};
uint8_t writeCharValue[MAX_CHAR_LENGTH] = {1};
struct gatts_profile_inst gatts_gl_profile_tab[GATTS_PROFILE_NUM];
bool gatts_conn;
bool indicate_enabled;

void changeIndicateCharValue(record_inst* record)
{
	gatts_indicateCharValue.attr_len = NUMERIC_BLE_BUFF_SIZE;
	ESP_LOGI("DEGBUGGG", "recordType is %d", record->recordType);
	if (record->recordType == NUMERIC_RECORD_TYPE){
		indicateCharValue[0] = (record->recordInfo.numericRecord->timePast >> 8) & 0xFF;
		indicateCharValue[1] = record->recordInfo.numericRecord->timePast & 0xFF;
		indicateCharValue[2] = record->recordInfo.numericRecord->type;
		for (int i = 3; i < NUMERIC_BLE_BUFF_SIZE; i++) {
			indicateCharValue[i] = (record->recordInfo.numericRecord->data >> (24 - 8*(i-3))) & 0xFF;
		}

		char tmpBleSendBuffer_TAG[25] = "BLE Send Buffer ";
		strcat(tmpBleSendBuffer_TAG, "[");
		char tmpItoa[5];
		itoa(recordSendCoutner, tmpItoa, 10);
		strcat(tmpBleSendBuffer_TAG, tmpItoa);
		strcat(tmpBleSendBuffer_TAG, "]");

		ESP_LOG_BUFFER_HEX_LEVEL(tmpBleSendBuffer_TAG, indicateCharValue, NUMERIC_BLE_BUFF_SIZE, ESP_LOG_INFO);
//		return record->recordType;
	}
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

void start_advertise()
{
    adv_config_done &= (~adv_config_flag);
    if (adv_config_done == 0){
        esp_ble_gap_start_advertising(&adv_params);
    }
    adv_config_done &= (~scan_rsp_config_flag);
    if (adv_config_done == 0){
        esp_ble_gap_start_advertising(&adv_params);
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
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(155); // not working...
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

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

        }else{
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
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
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep){
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
            if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_descr_handle == param->write.handle && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (indicateCharProperty & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
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
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
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
        writeCharProperty = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
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

			ESP_LOGI(GATTS_TAG, "the gatts demo char length = 0x%x(%d d)\n", length, length);
			for(int i = 0; i < length; i++){
				ESP_LOGI(GATTS_TAG, "prf_char[0x%x] =0x%x\n",i,prf_char[i]);
			}
			esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].service_handle, &gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].descr_uuid,
																	ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
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

			ESP_LOGI(GATTS_TAG, "the gatts demo char length = 0x%x(%d d)\n", length, length);
			for(int i = 0; i < length; i++){
				ESP_LOGI(GATTS_TAG, "prf_char[0x%x] =0x%x\n",i,prf_char[i]);
			}
			start_advertise();
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
														ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
														writeCharProperty,
														&gatts_writeCharValue,
														NULL);
		if (add_char_ret){
			ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
		}

//        start_advertise();
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
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id = param->connect.conn_id; //00:0b:57:be:5d:84:


		gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected++;
		if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected)
			gatts_conn = true;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(155);
        if (local_mtu_ret){
            ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        }
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT");
        gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected--;
        if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected == 0)
        	gatts_conn = false;
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT: {
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
            ESP_LOGE(GATTS_TAG, "ESP_GATTS_CONF_EVT, ERRORRRRR");
            break;
        }
        if (recordSendCoutner < recordListCounter && sendingRecordsToSmartphone){
        	changeIndicateCharValue(getRecord(recordSendCoutner));
        	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
        									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
        	recordSendCoutner++;
        }
        else if (recordSendCoutner == recordListCounter && sendingRecordsToSmartphone) {
        	// send type 99 - record list counter
			record_inst recordListCounterRecord;
			uint32_t dataOfType99 = 0;
//			dataOfType99 = (recordListCounter & 0xFFFF) | ((uint32_t)evt.timer_counter_value << 16);
//				recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, dataOfType99, NULL); // when Gal will be ready get off from comment
			recordListCounterRecord = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 99, recordListCounter, NULL);
			changeIndicateCharValue(&recordListCounterRecord);
			// Client need to approve / authenticate that the phone got all records
			// bleSendNumericRecord(numericRecordListCounterRecord);
			char tmpRecordBuffer_TAG[25] = "Record #";
			char tmpItoa[5];
			itoa(recordListCounter, tmpItoa, 10);
			strcat(tmpRecordBuffer_TAG, tmpItoa);
			strcat(tmpRecordBuffer_TAG, " - ");
			esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
			        									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
			ESP_LOGI(tmpRecordBuffer_TAG, "Record Type 99 has been Sent!!!");

			recordSendCoutner++;
			sendingRecordsToSmartphone = false;
        }
        if (mesurementSendCounter < 4 && sendingMeasurementsToSmartphone) {
			ESP_LOGI(GATTS_TAG, "sending next measure %d", mesurementSendCounter);
			changeIndicateCharValue(getRecord(recordListCounter - 4 + mesurementSendCounter));
			esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
			        									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
			mesurementSendCounter++;
		}
		else if (sendingMeasurementsToSmartphone) {
			sendingMeasurementsToSmartphone = false;
			mesurementSendCounter = 0;
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

//static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
//    switch (event) {
//    case ESP_GATTS_REG_EVT:
//        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_B;
//
//        esp_ble_gatts_create_service(gatts_if, &gatts_gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_B);
//        break;
//    case ESP_GATTS_READ_EVT: {
//        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
//        esp_gatt_rsp_t rsp;
//        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
//        rsp.attr_value.handle = param->read.handle;
//        rsp.attr_value.len = 4;
//        rsp.attr_value.value[0] = 0xde;
//        rsp.attr_value.value[1] = 0xed;
//        rsp.attr_value.value[2] = 0xbe;
//        rsp.attr_value.value[3] = 0xef;
//        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
//                                    ESP_GATT_OK, &rsp);
//        break;
//    }
//    case ESP_GATTS_WRITE_EVT: {
//        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
//        if (!param->write.is_prep){
//            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
//            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
//            if (gatts_gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2){
//                uint16_t descr_value= param->write.value[1]<<8 | param->write.value[0];
//                if (descr_value == 0x0001){
//                    if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
//                        ESP_LOGI(GATTS_TAG, "notify enable");
//                        uint8_t notify_data[15];
//                        for (int i = 0; i < sizeof(notify_data); ++i)
//                        {
//                            notify_data[i] = i%0xff;
//                        }
//                        //the size of notify_data[] need less than MTU size
//                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//                                                sizeof(notify_data), notify_data, false);
//                    }
//                }else if (descr_value == 0x0002){
//                    if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
//                        ESP_LOGI(GATTS_TAG, "indicate enable");
//                        uint8_t indicate_data[15];
//                        for (int i = 0; i < sizeof(indicate_data); ++i)
//                        {
//                            indicate_data[i] = i%0xff;
//                        }
//                        //the size of indicate_data[] need less than MTU size
//                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gatts_gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//                                                sizeof(indicate_data), indicate_data, true);
//                    }
//                }
//                else if (descr_value == 0x0000){
//                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
//                }else{
//                    ESP_LOGE(GATTS_TAG, "unknown value");
//                }
//
//            }
//        }
//        example_write_event_env(gatts_if, &b_prepare_write_env, param);
//        break;
//    }
//    case ESP_GATTS_EXEC_WRITE_EVT:
//        ESP_LOGI(GATTS_TAG,"ESP_GATTS_EXEC_WRITE_EVT");
//        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
//        example_exec_write_event_env(&b_prepare_write_env, param);
//        break;
//    case ESP_GATTS_MTU_EVT:
//        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
//        break;
//    case ESP_GATTS_UNREG_EVT:
//        break;
//    case ESP_GATTS_CREATE_EVT:
//        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_B;
//
//        esp_ble_gatts_start_service(gatts_gl_profile_tab[PROFILE_B_APP_ID].service_handle);
//        b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
//        esp_err_t add_char_ret =esp_ble_gatts_add_char( gatts_gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gatts_gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
//                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
//                                                        b_property,
//                                                        NULL, NULL);
//        if (add_char_ret){
//            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
//        }
//        break;
//    case ESP_GATTS_ADD_INCL_SRVC_EVT:
//        break;
//    case ESP_GATTS_ADD_CHAR_EVT:
//        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
//                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
//
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
//        esp_ble_gatts_add_char_descr(gatts_gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gatts_gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
//                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
//                                     NULL, NULL);
//        break;
//    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
//        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
//                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
//        break;
//    case ESP_GATTS_DELETE_EVT:
//        break;
//    case ESP_GATTS_START_EVT:
//        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
//                 param->start.status, param->start.service_handle);
//        break;
//    case ESP_GATTS_STOP_EVT:
//        break;
//    case ESP_GATTS_CONNECT_EVT:
//        ESP_LOGI(GATTS_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
//                 param->connect.conn_id,
//                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
//                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
//        gatts_gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
//        break;
//    case ESP_GATTS_CONF_EVT:
//        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT status %d", param->conf.status);
//        if (param->conf.status != ESP_GATT_OK){
//            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
//        }
//    break;
//    case ESP_GATTS_DISCONNECT_EVT:
//    case ESP_GATTS_OPEN_EVT:
//    case ESP_GATTS_CANCEL_OPEN_EVT:
//    case ESP_GATTS_CLOSE_EVT:
//    case ESP_GATTS_LISTEN_EVT:
//    case ESP_GATTS_CONGEST_EVT:
//    default:
//        break;
//    }
//}

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

void sendRecordListByIndicate()
{
	recordSendCoutner = 0;
	sendingRecordsToSmartphone = true;
	ESP_LOGI(GATTS_TAG, "We're going to send by indicate %d records", recordListCounter);
	changeIndicateCharValue(getRecord(recordSendCoutner));
	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
									gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
	recordSendCoutner++;
}

void sendMeasurementsByIndicate()
{
	mesurementSendCounter = 0;
	sendingMeasurementsToSmartphone = true;
	ESP_LOGI(GATTS_TAG, "We're going to send by indicate %d measurements records", 4);
	changeIndicateCharValue(getRecord(recordListCounter - 4 + mesurementSendCounter));
	esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].indicate_char_handle,
										gatts_indicateCharValue.attr_len, gatts_indicateCharValue.attr_value, true);
	mesurementSendCounter++;
}

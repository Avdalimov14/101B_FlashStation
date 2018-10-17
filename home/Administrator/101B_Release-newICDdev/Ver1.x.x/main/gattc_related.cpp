/*
 * gattc_related.c
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */
#include "gattc_related.h"
#include "gatts_related.h"
#include "ioManage.h"

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
struct gattc_profile_inst gattc_gl_profile_tab[GATTC_PROFILE_NUM];// = {
//    [GATTC_PROFILE_A_APP_ID] = {
//    		gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_cb = gattc_profile_event_handler,
//			gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
//    },
//};
bool gattc_conn = false;
bool passTheNotify = true;
bool passMonitorMeasurements = true;
uint8_t openBleConnAttempts = 0;
bool openBleConnection = false;
uint8_t validMeasurementsCounter = 0;

void init_gattc_ble()
{
	memcpy(remote_filter_service_uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128);
	memcpy(remote_filter_measurements_uuid.uuid.uuid128, bb_measurements_characteristicUUID, ESP_UUID_LEN_128);
	memcpy(remote_filter_profile_char_uuid.uuid.uuid128, bb_profile_characteristicUUID, ESP_UUID_LEN_128);
	memcpy(remote_filter_time_uuid.uuid.uuid128, bb_time_characteristicUUID, ESP_UUID_LEN_128);

	gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_cb = gattc_profile_event_handler;
	gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if = ESP_GATT_IF_NONE;       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */

	esp_err_t ret;
//    // Initialize NVS.
//    esp_err_t ret = nvs_flash_init();
//    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
//        ESP_ERROR_CHECK(nvs_flash_erase());
//        ret = nvs_flash_init();
//    }
//    ESP_ERROR_CHECK( ret );
//
//    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
//
//    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//    ret = esp_bt_controller_init(&bt_cfg);
//    if (ret) {
//        ESP_LOGE(GATTC_TAG, "%s initialize controller failed, error code = %x\n", __func__, ret);
//        return;
//    }
//
//    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//    if (ret) {
//        ESP_LOGE(GATTC_TAG, "%s enable controller failed, error code = %x\n", __func__, ret);
//        return;
//    }
//
//    ret = esp_bluedroid_init();
//    if (ret) {
//        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed, error code = %x\n", __func__, ret);
//        return;
//    }
//
//    ret = esp_bluedroid_enable();
//    if (ret) {
//        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed, error code = %x\n", __func__, ret);
//        return;
//    }

    //register the  callback function to the gap module
//    ret = esp_ble_gap_register_callback(esp_gap_cb);
//    if (ret){
//        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
//        return;
//    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return;
    }

    ret = esp_ble_gattc_app_register(GATTC_PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT: {
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    }
    case ESP_GATTC_CONNECT_EVT:{
    	if (compareToBbMacAdd(param->connect.remote_bda))
    	{
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
//        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id); // no need to change MTU
//        if (mtu_ret){
//            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
//        }
//        start_advertise();
        break;
    	}
    	break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
//            if ((bbBtMacAddrs[0] == 0 && bbBtMacAddrs[1] == 0 && bbBtMacAddrs[2] == 0) && !scanIsOn) { // if bb mac addrss is null so cancel adv and start scan BB
//				stop_advertise();
//				start_scan(30);
//			}
//			else if (openBleConnAttempts < MAX_OPEN_CONN_ATT || !scanIsOn){ // if there is already mac address try to re-connect
//				ESP_LOGE(GATTC_TAG, "trying to re-open connection with BioBeat (#%d)", openBleConnAttempts);
//				stop_advertise();
//				esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, bbBtMacAddrs,BLE_ADDR_TYPE_PUBLIC  , true);
//				openBleConnAttempts++;
//				openBleConnection = true;
//			}
//			else if (!scanIsOn){
//				stop_advertise();
//				start_scan(30);
//			}
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        resetOpenBleConnVars();
        gattc_conn = true;
        sendMeasurementDeviceStatus();
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
//        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_RES_EVT");
        esp_gatt_srvc_id_t *srvc_id =(esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
        if (srvc_id->id.uuid.len == ESP_UUID_LEN_128 && !memcmp(srvc_id->id.uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128)) {
            ESP_LOGI(GATTC_TAG, "service found");
            get_server = true;
            gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(GATTC_TAG, "UUID128: ");
            esp_log_buffer_hex(GATTC_TAG, srvc_id->id.uuid.uuid.uuid128, ESP_UUID_LEN_128);
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = 0x%x", p_data->search_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle,
                                                                     gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{


                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle,
															 remote_filter_profile_char_uuid,
                                                             char_elem_result,
                                                             &count);
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
                        gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].profile_char_handle = char_elem_result[0].char_handle;
                        ESP_LOGI(GATTC_TAG, "profile_char_handle =  %d", gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].profile_char_handle);
                        uint8_t writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};
//                        esp_ble_gattc_write_char(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id, char_elem_result[0].char_handle, sizeof(bb_writeToBiobeatProfile),
//                        		bb_writeToBiobeatProfile, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
                    }

                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle,
															 remote_filter_measurements_uuid,
                                                             char_elem_result,
                                                             &count);
                    ESP_LOGI(GATTC_TAG, "found %d chars", count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle = char_elem_result[0].char_handle;
                        ESP_LOGI(GATTC_TAG, "measurements_char_handle =  %d", gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle);
//                        esp_ble_gattc_register_for_notify (gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                    }

                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle,
                                                             gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle,
															 remote_filter_time_uuid,
                                                             char_elem_result,
                                                             &count);
                    ESP_LOGI(GATTC_TAG, "found %d chars", count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 /*&& (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)*/){
                        gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].time_char_handle = char_elem_result[0].char_handle;
                        ESP_LOGI(GATTC_TAG, "time_char_handle =  %d", gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].time_char_handle);
//                        changeBiobeatSamplePeriod(0x1F);
//                        uint8_t tmpWriteArray[2] = {0x00, 0x1E};
//                        esp_ble_gattc_write_char(gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id, char_elem_result[0].char_handle, sizeof(tmpWriteArray),
//                        		tmpWriteArray, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
//                vTaskDelay(1000 / portTICK_RATE_MS);
                uint8_t writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};
				esp_ble_gattc_write_char(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].profile_char_handle,
						sizeof(bb_writeToBiobeatProfile), bb_writeToBiobeatProfile, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
//                esp_ble_gattc_register_for_notify(gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_start_handle,
                                                                         gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].service_end_handle,
                                                                         gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = (esp_gattc_descr_elem_t*)malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
																		 gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle,//p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                    	ESP_LOGI(GATTC_TAG, "char handle used is =  %d", gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle);
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }

                    /* Erery char have only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    	gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_desc_handle = descr_elem_result[0].handle;
                        ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_NO_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                    	ESP_LOGE(GATTC_TAG, "descr handle used is =  %d", descr_elem_result[0].handle);
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }
                    else {
#ifdef OLD_INDICATIONS
                    	turnGreenLed(true);  // old QA and DEV indication
#else
                    	sendIndicationStatusToQueue((indicationStatus_t)IND_SCAN_SUCESS);

#endif
                    }

                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value: (conn_id = %d)", p_data->notify.conn_id);
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value: (conn_id = %d)", p_data->notify.conn_id);
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);

        if (passMonitorMeasurements) {
//			record_inst record;
        	uint8_t monitorMeasurementsData[5];
			int i = 5;
#ifndef REAL_BIOBEAT_DEVICE
			if (passTheNotify) {
#endif
				ESP_LOGI(GATTC_TAG, "RR: %d", p_data->notify.value[i]);
				monitorMeasurementsData[i - 5] = p_data->notify.value[i];
#ifndef ZERO_FROM_BIOBEAT_TEST
				if (p_data->notify.value[i]) {
#endif

					validMeasurementsCounter++;
#ifndef ZERO_FROM_BIOBEAT_TEST
				}
#endif
				i++;

				ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i]);
				monitorMeasurementsData[i - 5] = p_data->notify.value[i];
#ifndef ZERO_FROM_BIOBEAT_TEST
				if (p_data->notify.value[i]) {
#endif

					validMeasurementsCounter++;
#ifndef ZERO_FROM_BIOBEAT_TEST
				}
#endif
				i++;

				ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i]);
				monitorMeasurementsData[i - 5] = p_data->notify.value[i];
#ifndef ZERO_FROM_BIOBEAT_TEST
				if (p_data->notify.value[i]) {
#endif

				validMeasurementsCounter++;
#ifndef ZERO_FROM_BIOBEAT_TEST
				}
#endif
				i++;

				ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i]);
				monitorMeasurementsData[i - 5] = p_data->notify.value[i];
				uint32_t tmpHighVal = p_data->notify.value[i++];


				ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i]);
				monitorMeasurementsData[i - 5] = p_data->notify.value[i];
#ifndef ZERO_FROM_BIOBEAT_TEST
				if (tmpHighVal || p_data->notify.value[i]) {
#endif
				uint32_t tmpLowVal = p_data->notify.value[i];
				uint32_t  tmpVal = 0;
				tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;

				validMeasurementsCounter++;
#ifndef ZERO_FROM_BIOBEAT_TEST
				}
#endif
				i++;

				GenericRecord record(80, monitorMeasurementsData);
				addToRecordList(record, true, NULL);

				printRecordList(15);

				if (gatts_conn && !sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)) {
					sendMeasurementsByIndicate();
				}
				passTheNotify = false;
#ifndef REAL_BIOBEAT_DEVICE
			}
#endif
        }
// oldICD
//        if (passMonitorMeasurements) {
//			record_inst record;
//			int i = 5;
//#ifndef REAL_BIOBEAT_DEVICE
//			if (passTheNotify) {
//#endif
//				ESP_LOGI(GATTC_TAG, "RR: %d", p_data->notify.value[i]);
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				if (p_data->notify.value[i]) {
//#endif
//					record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_RESPRATE, (uint32_t)(p_data->notify.value[i]), NULL);
//					addToRecordList(record, true, NULL);
//					validMeasurementsCounter++;
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				}
//#endif
//				i++;
//
//				ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i]);
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				if (p_data->notify.value[i]) {
//#endif
//					record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_SPO2, (uint32_t)(p_data->notify.value[i]), NULL);
//					addToRecordList(record, true, NULL);
//					validMeasurementsCounter++;
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				}
//#endif
//				i++;
//
//				ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i]);
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				if (p_data->notify.value[i]) {
//#endif
//				record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_HEARTRATE, (uint32_t)(p_data->notify.value[i]), NULL);
//				addToRecordList(record, true, NULL);
//				validMeasurementsCounter++;
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				}
//#endif
//				i++;
//
//				ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i]);
//				uint32_t tmpHighVal = p_data->notify.value[i++];
//
//
//				ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i]);
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				if (tmpHighVal || p_data->notify.value[i]) {
//#endif
//				uint32_t tmpLowVal = p_data->notify.value[i];
//				uint32_t  tmpVal = 0;
//				tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
//				record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_BLOODPRESURE, (uint32_t)tmpVal, NULL); //p_data->read.value
//				addToRecordList(record, true, NULL);
//				validMeasurementsCounter++;
//#ifndef ZERO_FROM_BIOBEAT_TEST
//				}
//#endif
//				i++;
//	// i = 5 => RaspRate
//	//			ESP_LOGI(GATTC_TAG, "TEMP: %f", (float)(p_data->notify.value[16] + 200) / 10);
//	//			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_RESPRATE, 0/*(uint32_t)p_data->notify.value[16]*/, NULL); //p_data->read.value
//	//			addToRecordList(record, true, NULL);
//
//				printRecordList(recordListCounter - 50);
//
//				if (gatts_conn && !sendingRecordsToSmartphone && (indicate_enabled || notify_enabled)) {
//					sendMeasurementsByIndicate();
//				}
//				passTheNotify = false;
//#ifndef REAL_BIOBEAT_DEVICE
//			}
//#endif
//        }
        break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
        registeredToNotify = true;
#ifdef REAL_TIME_TEST_PROG
        if (gatts_conn) {
        	changeBiobeatSamplePeriod(30);
        } else
        	changeBiobeatSamplePeriod(120);
#else
        changeBiobeatSamplePeriod(5);
#endif

        if (!gattsInitDone)
        	init_gatts_ble();
//        start_advertise(); // For EMC Tests
        // Medical Simulator related
        start_advertise();
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
        vTaskDelay(50 / portTICK_RATE_MS);
        if (p_data->write.handle == gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].profile_char_handle)
        	esp_ble_gattc_register_for_notify (gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_char_handle);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
    	if (compareToBbMacAdd(param->disconnect.remote_bda))
    	{
#ifdef OLD_INDICATIONS
			turnGreenLed(false); // old QA and DEV indication
#else
        	sendIndicationStatusToQueue((indicationStatus_t)IND_MONITOR_DISCONNECTED);
#endif
			gattc_conn = false;
			sendMeasurementDeviceStatus();
			get_server = false;
			ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d (conn_id = %d)", p_data->disconnect.reason, p_data->disconnect.conn_id);
			if (p_data->disconnect.reason != ESP_GATT_CONN_TIMEOUT) {

				stop_advertise();
				start_scan(30);
				registeredToNotify = false;
				break;
			}
			if ((bbBtMacAddrs[0] == 0 && bbBtMacAddrs[1] == 0 && bbBtMacAddrs[2] == 0) && !scanIsOn) {
				stop_advertise();
				start_scan(30);
			}
			else if (!scanIsOn) {
				ESP_LOGE(GATTC_TAG, "trying to re-open connection with BioBeat (#%d)", openBleConnAttempts);
				stop_advertise();
				esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, bbBtMacAddrs,BLE_ADDR_TYPE_PUBLIC  , true);
				openBleConnAttempts++;
				openBleConnection = true;
			}
			registeredToNotify = false;
    	}
        break;
    }
    case ESP_GATTC_READ_DESCR_EVT:
    	ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_DESCR_EVT");
    	if (param->read.status != ESP_GATT_OK) {
    		ESP_LOGE(GATTC_TAG, "Error! status = 0x%X", param->read.status);
    		break;
    	}
    	esp_log_buffer_hex(GATTC_TAG, param->read.value, param->read.value_len);
    	if (param->read.value[0] == 0) {
    		uint16_t notify_en = 1;
    		esp_gatt_status_t ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                          gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
														  gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_desc_handle,
                                                          sizeof(notify_en),
                                                          (uint8_t *)&notify_en,
                                                          ESP_GATT_WRITE_TYPE_NO_RSP,
                                                          ESP_GATT_AUTH_REQ_NONE);
    	}

    	break;
    case ESP_GATTC_CANCEL_OPEN_EVT:
    	ESP_LOGE("DEBUGGGG", "ESP_GATTC_CANCEL_OPEN_EVT");
    	break;
    case ESP_GATTC_EXEC_EVT:
    	ESP_LOGE("DEBUGGGG", "ESP_GATTC_CANCEL_OPEN_EVT");
    	break;
    default:
        break;
    }
}

//static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
//{
//    uint8_t *adv_name = NULL;
//    uint8_t adv_name_len = 0;
//
//    uint8_t *adv_uuid_cmpl = NULL;
//    uint8_t adv_uuid_cmpl_len = 0;
//
//    uint8_t *adv_uuid_part = NULL;
//    uint8_t adv_uuid_part_len = 0;
//
//    switch (event) {
//    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
//        //the unit of the duration is second
//        uint32_t duration = 30;
//        esp_ble_gap_start_scanning(duration);
//        break;
//    }
//    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
//        //scan start complete event to indicate scan start successfully or failed
//        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
//            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
//            break;
//        }
//        ESP_LOGI(GATTC_TAG, "scan start success");
//
//        break;
//    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
//        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
//        switch (scan_result->scan_rst.search_evt) {
//        case ESP_GAP_SEARCH_INQ_RES_EVT:
//            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
//            ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
//            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
//                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
//
//            adv_uuid_cmpl = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
//            		ESP_BLE_AD_TYPE_128SRV_CMPL, &adv_uuid_cmpl_len);
//
//            adv_uuid_part = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
//            		ESP_BLE_AD_TYPE_128SRV_PART, &adv_uuid_part_len);
//            // service uuid is bb_serviceUUID
//
//            ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
//            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
//            ESP_LOGI(GATTC_TAG, "\n");
//            if (adv_name != NULL) {
//                if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0) {
//                    ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
//                    if (gattc_conn == false) {
//                        gattc_conn = true;
//                        ESP_LOGI(GATTC_TAG, "connect to the remote device.");
//                        esp_ble_gap_stop_scanning();
//                        esp_ble_gattc_open(gattc_GATTC_PROFILE_A_APP_ID[GATTC_PROFILE_A_APP_ID].gattc_if, myDeviceBtMacAddrs/*scan_result->scan_rst.bda*/,BLE_ADDR_TYPE_PUBLIC  , true);
//                    }
//                }
//            }
//
//            if (scan_result->scan_rst.rssi > -40) {
//				if (adv_uuid_cmpl != NULL || adv_uuid_part != NULL) {
//					if ((16 == adv_uuid_cmpl_len && (memcmp(adv_uuid_cmpl, bb_serviceUUID, adv_uuid_cmpl_len) == 0))
//							|| (16 == adv_uuid_part_len && (memcmp(adv_uuid_part, bb_serviceUUID, adv_uuid_part_len)))) {
//						ESP_LOGI(GATTC_TAG, "searched device\n");
//						esp_log_buffer_hex(GATTC_TAG, bb_serviceUUID, 16);
//						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_cmpl)\n");
//						esp_log_buffer_hex(GATTC_TAG, adv_uuid_cmpl, 16);
//						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_part)\n");
//						if (gattc_conn == false) {
//							gattc_conn = true;
//							ESP_LOGI(GATTC_TAG, "connect to the remote device.");
//							esp_ble_gap_stop_scanning();
//							esp_ble_gattc_open(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
//						}
//					}
//				}
//            }
//            break;
//        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
//            break;
//        default:
//            break;
//        }
//        break;
//    }
//
//    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
//        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
//            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
//            break;
//        }
//        ESP_LOGI(GATTC_TAG, "stop scan successfully");
//        break;
//
//    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
//            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
//            break;
//        }
//        ESP_LOGI(GATTC_TAG, "stop adv successfully");
//        break;
//    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//         ESP_LOGI(GATTC_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
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

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gattc_gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < GATTC_PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gattc_gl_profile_tab[idx].gattc_if) {
                if (gattc_gl_profile_tab[idx].gattc_cb) {
                    gattc_gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

bool getGattClientConnStatus()
{
	return gattc_conn;

}

void changeBiobeatSamplePeriod(uint16_t seconds)
{
	ESP_LOGI(GATTC_TAG, "trying to update sample time to %d...(time_char_handle = %d, conn_id = %d)", seconds, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].time_char_handle, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id);
	uint8_t tmpWriteArray[2] = {0};
	tmpWriteArray[0] = (seconds >> 8) & 0xFF;
	tmpWriteArray[1] = (seconds) & 0xFF;
	esp_log_buffer_hex(GATTC_TAG, tmpWriteArray, sizeof(tmpWriteArray));
	esp_err_t err = esp_ble_gattc_write_char(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
			gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].time_char_handle, sizeof(tmpWriteArray), tmpWriteArray, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
	ESP_LOGI(GATTC_TAG, "err is %d", err);
}

void resetOpenBleConnVars()
{
	openBleConnection = false;
	openBleConnAttempts = 0;
}

bool compareToBbMacAdd(uint8_t* macAdd)
{
	if ((macAdd[0] == 0x00 && macAdd[1] == 0x0b && macAdd[2] == 0x57)
	    ||	 (macAdd[0] == 0x30 && macAdd[1] == 0xae && macAdd[2] == 0xa4))
		return true;
	return false;
}

void checkMeasurementDescriptor()
{
	if (!registeredToNotify) {
		esp_err_t err = esp_ble_gattc_read_char_descr(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
				gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].measurements_desc_handle, ESP_GATT_AUTH_REQ_NONE);
		if (err != ESP_OK) {
			ESP_LOGE(GATTC_TAG, "esp_ble_gattc_read_char_descr Error! status = 0x%X", err);
		}
	}
}

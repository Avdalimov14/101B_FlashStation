/*
 * gattc_related.c
 *
 *  Created on: Jun 4, 2018
 *      Author: albert
 */
#include "gattc_related.h"
#include "gatts_related.h"

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
struct gattc_profile_inst gattc_gl_profile_tab[GATTC_PROFILE_NUM];// = {
//    [GATTC_PROFILE_A_APP_ID] = {
//    		gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_cb = gattc_profile_event_handler,
//			gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
//    },
//};

void start_advertise();

void init_gattc_ble()
{
	memcpy(remote_filter_service_uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128);
	memcpy(remote_filter_measurements_uuid.uuid.uuid128, bb_characteristicUUID, ESP_UUID_LEN_128);
	memcpy(remote_filter_profile_char_uuid.uuid.uuid128, bb_profile_characteristicUUID, ESP_UUID_LEN_128);


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
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }

//        start_advertise();
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
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
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
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
                        esp_ble_gattc_write_char(gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id, char_elem_result[0].char_handle, 13, writeToBiobeatProfile, ESP_GATT_WRITE_TYPE_NO_RSP,
                                ESP_GATT_AUTH_REQ_NONE);
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
                        esp_ble_gattc_register_for_notify (gattc_if, gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].remote_bda, char_elem_result[0].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
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
                        ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                    	ESP_LOGE(GATTC_TAG, "descr handle used is =  %d", descr_elem_result[0].handle);
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
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
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        record_inst record;
        int i = 6;
        ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i]);
		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_SPO2, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
		addToRecordList(record);//, true, NULL);

		ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i]);
		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_HEARTRATE, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
		addToRecordList(record);//, true, NULL);

		ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i]);
		uint32_t tmpHighVal = p_data->notify.value[i++];


		ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i]);
		uint32_t tmpLowVal = p_data->notify.value[i++];
		uint32_t  tmpVal = 0;
		tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_BLOODPRESURE, (uint32_t)tmpVal, NULL); //p_data->read.value
		addToRecordList(record);//, true, NULL);

		ESP_LOGI(GATTC_TAG, "TEMP: %f", (float)(p_data->notify.value[16] + 200) / 10);
		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_RESPRATE, (uint32_t)p_data->notify.value[16], NULL); //p_data->read.value
		addToRecordList(record);//, true, NULL);

		printRecordList();

		if (gatts_conn && !sendingRecordsToSmartphone && indicate_enabled) {
			sendMeasurementsByIndicate();
		}
//        notifyCharValue[i] = p_data->notify.value[i];
//        ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i++]);
//
//        notifyCharValue[i] = p_data->notify.value[i];
//        ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i++]);
//
//        notifyCharValue[i] = p_data->notify.value[i];
//        ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i++]);
//
//        notifyCharValue[i] = p_data->notify.value[i];
//        ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i++]);
//
//        notifyCharValue[16] = p_data->notify.value[16];
//        ESP_LOGI(GATTC_TAG, "TEMP: %f", (float)(p_data->notify.value[16] + 200) / 10);
//
//        if (gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].connected > 0) {
//			static int j = 0;
//			gpio_set_level(2, 1);
//			esp_ble_gatts_send_indicate(gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].gatts_if, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].conn_id, gatts_gl_profile_tab[GATTS_PROFILE_A_APP_ID].char_handle,
//					gatts_notifyCharValue.attr_len, gatts_notifyCharValue.attr_value, false);
//        }
//		vTaskDelay(pdMS_TO_TICKS(200));
//		gpio_set_level(2, 0);
//		vTaskDelay(pdMS_TO_TICKS(19800));
        break;
    }
    case ESP_GATTC_WRITE_DESCR_EVT: {
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write descr success ");
//        uint8_t write_char_data[35];
//        for (int i = 0; i < sizeof(write_char_data); ++i)
//        {
//            write_char_data[i] = i % 256;
//        }
//        esp_ble_gattc_write_char( gattc_if,
//                                  gattc_gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].conn_id,
//                                  gattc_gattc_gl_profile_tab[GATTC_PROFILE_A_APP_ID].char_handle,
//                                  sizeof(write_char_data),
//                                  write_char_data,
//                                  ESP_GATT_WRITE_TYPE_RSP,
//                                  ESP_GATT_AUTH_REQ_NONE);

    	init_gatts_ble();
        break;
    }
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        gattc_conn = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
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


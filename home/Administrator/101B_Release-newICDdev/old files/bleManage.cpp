/*
 * bleManage.c
 *
 *  Created on: Apr 24, 2018
 *      Author: albert
 */

#include "mainHeader.h"
#include "bleManage.h"
#include "dbManage.h"
#include "driver/gpio.h"
#include "ioManage.h"
#include "Arduino.h"
#include "string.h"


/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_a_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
	[PROFILE_B_APP_ID] = {
		.gattc_cb = gattc_profile_b_event_handler,
		.gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
	},
};

bool getConnDeviceA()
{return conn_device_a;}

bool getConnDeviceB()
{return conn_device_b;}

void setConnDeviceA(bool status)
{conn_device_a = status;}

void setConnDeviceB(bool status)
{conn_device_b = status;}

void start_scan(void)
{
    stop_scan_done = false;
    //Isconnecting = false;
    uint32_t duration = 10;
    esp_ble_gap_start_scanning(duration);
}

void connectToSmartphoneProcedure(char* name, uint8_t length)
{
	strcpy((char*)remote_device_name[1], name);
	ESP_LOG_BUFFER_HEX_LEVEL(GATTC_TAG, remote_device_name[1], length, ESP_LOG_DEBUG);
	remote_device_name[1][(int)length-2] = 0;
	remote_device_name[1][(int)length-1] = 0;
	//				for (int i = 0; i < responseLength; i++)
	//					remote_device_name[1][i] = (char)response[i];
	ESP_LOGD(GATTC_TAG, "Trying to establish BLE connection. Response Length: %d, Device name: %s", length, remote_device_name[1]);
	ESP_LOGI(NFCLOG, "Trying to establish BLE connection with current smartphone.");

//	runNfcFunction = false;
	start_scan();
}

bool connectToMeasurementDevice(uint8_t* measureDeviceBtMacAddrs)
{
	if (conn_device_a == false) {
		conn_device_a = true;
		ESP_LOGI(GATTC_TAG, "connect to the ESP BLE device via NFC tag.");  // --------> here I can make a record of connection if needed
//						esp_ble_gap_stop_scanning();
		esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, measureDeviceBtMacAddrs/*scan_result->scan_rst.bda*/,BLE_ADDR_TYPE_PUBLIC  , true);
		return conn_device_a;
	}
	else
		return false;
}

void readFromMeasurementDevice()
{
	esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
							gl_profile_tab[PROFILE_A_APP_ID].conn_id,
							gl_profile_tab[PROFILE_A_APP_ID].char_handle,
							ESP_GATT_AUTH_REQ_NONE);
}

static void gattc_profile_a_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
	esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

	switch (event) {
	case ESP_GATTC_REG_EVT: {

		}
		break;

	/* one device connect successfully, all profiles callback function will get the ESP_GATTC_CONNECT_EVT,
 	 so must compare the mac address to check which device is connected, so it is a good choice to use ESP_GATTC_OPEN_EVT. */
	case ESP_GATTC_CONNECT_EVT:
		break;

	case ESP_GATTC_OPEN_EVT: {
		if (p_data->open.status != ESP_GATT_OK){
			//open failed, ignore the first device, connect the second device
			ESP_LOGE(GATTC_TAG, "connect device failed, status %d", p_data->open.status);
			conn_device_a = false;
			//start_scan();
			break;
		}
		memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->open.remote_bda, 6);
		gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->open.conn_id;
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
		ESP_LOGD(GATTC_TAG, "REMOTE BDA:");
		esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
		esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
		if (mtu_ret){
			ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
		}
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_G, 0);
#else
		gpio_set_level(GPIO_OUTPUT_G, 1);
#endif
		}
		break;

	case ESP_GATTC_CFG_MTU_EVT:
		if (param->cfg_mtu.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
		}
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
		esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_a_filter_service_uuid);
		break;
	case ESP_GATTC_SEARCH_RES_EVT: {
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_SEARCH_RES_EVT");
		esp_gatt_srvc_id_t *srvc_id =(esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
		ESP_LOGD(GATTC_TAG, "SEARCH RES: conn_id = %x", p_data->search_res.conn_id);
		if (srvc_id->id.uuid.len == ESP_UUID_LEN_128 && !memcmp(srvc_id->id.uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128)) {
			ESP_LOGD(GATTC_TAG, "service found");
			get_service_a = true;
			gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
			gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
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
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_A_SEARCH_CMPL_EVT");

		if (get_service_a){
			uint16_t count = 0;
			esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
																	 p_data->search_cmpl.conn_id,
																	 ESP_GATT_DB_CHARACTERISTIC,
																	 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
																	 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
																	 INVALID_HANDLE,
																	 &count);
			if (status != ESP_GATT_OK){
				ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
			}

			ESP_LOGD("GATTC- Debug", "Testing char of first service count: %d\n", count); //Debugging
			if (count > 0){
				ESP_LOGD("GATTC- Debug", "Testing char of first service count: %d\n", count); //Debugging
				char_elem_result_a = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
				if (!char_elem_result_a){
					ESP_LOGE(GATTC_TAG, "gattc no mem");
				}else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
															 remote_a_filter_profile_char_uuid,
                                                             char_elem_result_a,
                                                             &count);
                    if (count > 0 && (char_elem_result_a[0].properties & ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
                        gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result_a[1].char_handle;
                        uint8_t writeToBiobeatProfile[13] = {1,95,1,150 ,70,4,200, 0,70, 0,120, 0,80};
                        esp_ble_gattc_write_char(gattc_if, gl_profile_tab[PROFILE_A_APP_ID].conn_id, char_elem_result_a[0].char_handle, 13, writeToBiobeatProfile, ESP_GATT_WRITE_TYPE_NO_RSP,
                                ESP_GATT_AUTH_REQ_NONE);
                    }

					status = esp_ble_gattc_get_char_by_uuid( gattc_if,
															 p_data->search_cmpl.conn_id,
															 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
															 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
															 remote_a_filter_measurements_char_uuid,
															 char_elem_result_a,
															 &count);
					if (status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
					}

					/*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result_a' */
					if (count > 0 && (char_elem_result_a[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
						gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result_a[0].char_handle;
						esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result_a[0].char_handle);
					}
					else {
						ESP_LOGE(GATTC_TAG, "char_elem_result_a[index].properties not have notify permission");

					}
				}
				/* free char_elem_result_a */
				free(char_elem_result_a);
			}else{
				ESP_LOGE(GATTC_TAG, "no char found");
			}
		}
		 break;
	case ESP_GATTC_REG_FOR_NOTIFY_EVT: { // Need to handle
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
		if (p_data->reg_for_notify.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
		}else{
			uint16_t count = 0;
			uint16_t notify_en = 1;
			ESP_LOGD("GATTC- Debug", "Testing: char_handle is: %d\n", gl_profile_tab[PROFILE_A_APP_ID].char_handle);
			esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
																		 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																		 ESP_GATT_DB_DESCRIPTOR,
																		 gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
																		 gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
																		 gl_profile_tab[PROFILE_A_APP_ID].char_handle,
																		 &count);
			if (ret_status != ESP_GATT_OK){
				ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
			}
			if (count > 0){
				descr_elem_result_a = (esp_gattc_descr_elem_t*)malloc(sizeof(esp_gattc_descr_elem_t) * count); // --------- NEED TO CAST MALLOC
				if (!descr_elem_result_a){
					ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
				}else{
					ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
																		 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																		 p_data->reg_for_notify.handle,
																		 notify_descr_uuid,
																		 descr_elem_result_a,
																		 &count);
					if (ret_status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
					}

					/* Erery char have only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result_a' */
					ESP_LOGD("GATTC- Debug", "Testing descr count: %d\n", count); //Debugging
					if (count > 0 && descr_elem_result_a[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_a[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
						ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
																	 gl_profile_tab[PROFILE_A_APP_ID].conn_id,
																	 descr_elem_result_a[0].handle,
																	 sizeof(notify_en),
																	 (uint8_t *)&notify_en,
																	 ESP_GATT_WRITE_TYPE_NO_RSP,
																	 ESP_GATT_AUTH_REQ_NONE);
					}

					if (ret_status != ESP_GATT_OK){
						ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");

					}

					/* free descr_elem_result_a */
					free(descr_elem_result_a);
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
        static int notifyCounter = 0;
//        if (notifyCounter == 11) {
			record_inst record;
			int i = 6;

			ESP_LOGI(GATTC_TAG, "SPO2: %d", p_data->notify.value[i]);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_SPO2, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "HR: %d", p_data->notify.value[i]);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_HEARTRATE, (uint32_t)(p_data->notify.value[i++]), NULL); // random dofek
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "SBP: %d", p_data->notify.value[i]);
			uint32_t tmpHighVal = p_data->notify.value[i++];


			ESP_LOGI(GATTC_TAG, "DBP: %d", p_data->notify.value[i]);
			uint32_t tmpLowVal = p_data->notify.value[i++];
			uint32_t  tmpVal = 0;
			tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_BLOODPRESURE, (uint32_t)tmpVal, NULL); //p_data->read.value
			addToRecordList(record, true, NULL);

			ESP_LOGI(GATTC_TAG, "TEMP: %f", (float)(p_data->notify.value[16] + 200) / 10);
			record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, MESUREMENT_TYPE_RESPRATE, (uint32_t)p_data->notify.value[16], NULL); //p_data->read.value
			addToRecordList(record, true, NULL);

			printRecordList();

			if (conn_device_b) {
				for (int j = 1; j < 5; j++){
					bleSendRecord(blePreSendRecord(recordList[recordListCounter - j], recordListCounter - j));
				}
			}
			notifyCounter = 0;
//        }
//        else
//        	notifyCounter++;
		// Blink GPIO 14 corresponding to Gatt_server's pulses
//		esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
//		if (p_data->notify.value[0])  --> Not relevant by now
//			gpio_set_level(GPIO_OUTPUT_RRGB1, NULL);
//		else
//			gpio_set_level(GPIO_OUTPUT_RRGB1, 1ULL);
//		record_inst record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 222, p_data->notify.value[0], NULL); // numericRecord_inst record = makeNumericRecord(timePastMinutes, 222, toggleCounter);
//		addToRecordList(record, true, NULL); // addToNumericRecordList(record);
		}
		break;
	case ESP_GATTC_WRITE_DESCR_EVT: {
		if (p_data->write.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			break;
		}
		ESP_LOGD(GATTC_TAG, "write descr success ");

		uint8_t write_char_data[20];
		for (int i = 0; i < sizeof(write_char_data); ++i)
		{
			write_char_data[i] = i % 256;
		}
//		esp_ble_gattc_write_char( gattc_if,
//								  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//								  gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//								  sizeof(write_char_data),
//								  write_char_data,
//								  ESP_GATT_WRITE_TYPE_NO_RSP,
//								  ESP_GATT_AUTH_REQ_NONE);

		break;
	}
	case ESP_GATTC_SRVC_CHG_EVT: {
		esp_bd_addr_t bda;
		memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
		ESP_LOGD(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
		esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
		break;
	}
	case ESP_GATTC_WRITE_CHAR_EVT:
		if (p_data->write.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
//			// Read Attempt --->  No need for this profile
//			ESP_LOGE("GATTC", "need ot reed now");
//			esp_ble_gattc_read_char(gattc_if,
//									gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//									gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//									ESP_GATT_AUTH_REQ_NONE);
			break;
		}

		ESP_LOGD(GATTC_TAG, "write char success ");
//		No need for this profile
//		// Read Attempt
//		ESP_LOGE("GATTC", "need ot reed now");
//		esp_ble_gattc_read_char(gattc_if,
//								gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//								gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//								ESP_GATT_AUTH_REQ_NONE);
		break;
	case ESP_GATTC_READ_CHAR_EVT: {
		if (p_data->read.status != ESP_GATT_OK){
			ESP_LOGE(GATTC_TAG, "read char failed, error status = %x", p_data->read.status);
			break;
		}
		ESP_LOGD(GATTC_TAG, "read char success. value len = %d, value data:", p_data->read.value_len);
		esp_log_buffer_hex(GATTC_TAG, p_data->read.value, sizeof(esp_bd_addr_t));


		// generate random HR and BP
//		record_inst record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 81, (uint32_t)(random(50, 150)), NULL); // random dofek
//		addToRecordList(record, true, NULL);
//		uint32_t tmpLowVal = random(50, 90); // random lahatz dam
//		uint32_t tmpHighVal = random(80, 120);
//		uint32_t  tmpVal = 0;
//		tmpVal = ((tmpHighVal << 16) & 0xFFFF0000) | (tmpLowVal & 0x0000FFFF) ;
//		record = makeRecord(NUMERIC_RECORD_TYPE, timePastMinutes, 82, (uint32_t)tmpVal, NULL); //p_data->read.value
//		addToRecordList(record, true, NULL);
//		printRecordList();
//
//		if (conn_device_b) {
//			bleSendRecord(blePreSendRecord(recordList[recordListCounter - 2], recordListCounter - 2));
//			bleSendRecord(blePreSendRecord(recordList[recordListCounter - 1], recordListCounter - 1));
//		}
		// old workflow
//		if (timePastMinutes % 2 == 0 && conn_device_a) {
//			// past 2 minutes so need to sample BioBeat
//			esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
//										gl_profile_tab[PROFILE_A_APP_ID].conn_id,
//										gl_profile_tab[PROFILE_A_APP_ID].char_handle,
//										ESP_GATT_AUTH_REQ_NONE);
//		}
//		int i;
//		for(i=0; i<(p_data->read.value_len); i++)
//			printf("0x%X ", p_data->read.value[i]);
//		printf("and now?");

		break;
	}
	case ESP_GATTC_DISCONNECT_EVT:
		if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, 6) == 0){
			ESP_LOGI(GATTC_TAG, "device 'A' disconnect");
			conn_device_a = false;
			get_service_a = false;
            if (!conn_device_b)
            	Isconnecting = false;
			// so turn off green led
#ifdef ACTIVE_LOW_LEDS
    		gpio_set_level(GPIO_OUTPUT_G, 1);
#else
    		gpio_set_level(GPIO_OUTPUT_G, 0);
#endif
		}
		break;
	default:
		break;
	}
}

static void gattc_profile_b_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:{
		ESP_LOGD(GATTC_TAG, "REG_EVT");
		esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
		if (scan_ret){
			ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
		}
		}
        break;

    case ESP_GATTC_CONNECT_EVT:
        break;

    case ESP_GATTC_OPEN_EVT: {
        if (p_data->open.status != ESP_GATT_OK){
            //open failed, ignore the second device, connect the third device
            ESP_LOGE(GATTC_TAG, "connect device 2 failed, status %d", p_data->open.status);
            conn_device_b = false;
            //start_scan();
            break;
        }
        memcpy(gl_profile_tab[PROFILE_B_APP_ID].remote_bda, p_data->open.remote_bda, 6);
        gl_profile_tab[PROFILE_B_APP_ID].conn_id = p_data->open.conn_id;
        esp_ble_gatt_set_local_mtu(27);
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_OPEN_EVT conn_id %d, if %d, status %d, mtu %d", p_data->open.conn_id, gattc_if, p_data->open.status, p_data->open.mtu);
        ESP_LOGD(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, p_data->open.remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->open.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
//        esp_ble_gattc_search_service(gattc_if, param->open.conn_id, &remote_b_filter_service_uuid);
#ifdef ACTIVE_LOW_LEDS
		gpio_set_level(GPIO_OUTPUT_B, 0);
#else
		gpio_set_level(GPIO_OUTPUT_B, 1);
#endif
    	}
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"Config mtu failed");
        }
        ESP_LOGD(GATTC_TAG, "Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        // moved one case above
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_b_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        esp_gatt_srvc_id_t *srvc_id = (esp_gatt_srvc_id_t *)&p_data->search_res.srvc_id;
        ESP_LOGD(GATTC_TAG, "SEARCH RES: conn_id = %x", p_data->search_res.conn_id);
        if (srvc_id->id.uuid.len == ESP_UUID_LEN_16 && srvc_id->id.uuid.uuid.uuid16 == REMOTE_B_SERVICE_UUID) {
        	ESP_LOGD(GATTC_TAG, "service found");
            ESP_LOGD(GATTC_TAG, "UUID16: %x", srvc_id->id.uuid.uuid.uuid16);
            get_service_b = true;
            gl_profile_tab[PROFILE_B_APP_ID].service_start_handle = p_data->search_res.start_handle;
            gl_profile_tab[PROFILE_B_APP_ID].service_end_handle = p_data->search_res.end_handle;
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_B_SEARCH_CMPL_EVT");
		esp_ble_conn_update_params_t testingParam;
		testingParam.min_int = 16;
		testingParam.max_int = 64;
		testingParam.latency = 10;
		testingParam.timeout = 400;
		esp_ble_gap_update_conn_params(&testingParam);
        if (get_service_b){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
            	ESP_LOGD("GATTC- Debug", "Testing char of service count: %d\n", count); //Debugging
                char_elem_result_b = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result_b){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
                    status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                             remote_b_filter_char_uuid,
                                                             char_elem_result_b,
                                                             &count);
                    if (status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
                    ESP_LOGD("GATTC- Debug", "Testing char found count: %d\n", count); //Debugging
                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    // from here it should register for notify and write to descriptor, meanwhile we don't register for notify ------- NEED TO DO!!!
                    if (count > 0 && (char_elem_result_b[0].properties & ESP_GATT_CHAR_PROP_BIT_WRITE)){
                        gl_profile_tab[PROFILE_B_APP_ID].char_handle = char_elem_result_b[0].char_handle;
//                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, char_elem_result_b[0].char_handle); //$$$$$$
                        // send test array...
//                        uint8_t write_char_data[26];
//						for (int i = 0; i < sizeof(write_char_data); ++i)
//						{
//							write_char_data[i] = i + 65;
//						}
//                        esp_ble_gattc_write_char( gattc_if,
//                                                          gl_profile_tab[PROFILE_B_APP_ID].conn_id,
//                                                          gl_profile_tab[PROFILE_B_APP_ID].char_handle,
//                                                          sizeof(write_char_data),
//                                                          write_char_data,
//														  ESP_GATT_WRITE_TYPE_NO_RSP,
//                                                          ESP_GATT_AUTH_REQ_NONE);
                        // end of send test array|
                        bleSendRecordList();
                    }
                }
                /* free char_elem_result */
                free(char_elem_result_b);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {

        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "reg notify failed, error status =%x", p_data->reg_for_notify.status);
            break;
        }
        uint16_t count = 0;
        uint16_t notify_en = 1;
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                     ESP_GATT_DB_DESCRIPTOR,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].service_end_handle,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                                                     &count);
        if (ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
        }
        if (count > 0){
            descr_elem_result_b = (esp_gattc_descr_elem_t *)malloc(sizeof(esp_gattc_descr_elem_t) * count);
            if (!descr_elem_result_b){
                ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
            }else{
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                     gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                     p_data->reg_for_notify.handle,
                                                                     notify_descr_uuid,
                                                                     descr_elem_result_b,
                                                                     &count);
                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                }

                /* Erery char have only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                ESP_LOGD("GATTC- Debug", "Testing descr count: %d\n", count); //Debugging
                if (count > 0 && descr_elem_result_b[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result_b[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                    ret_status = (esp_gatt_status_t)esp_ble_gattc_write_char_descr( gattc_if,
                                                                 gl_profile_tab[PROFILE_B_APP_ID].conn_id,
                                                                 descr_elem_result_b[0].handle,
                                                                 sizeof(notify_en),
                                                                 (uint8_t *)&notify_en,
                                                                 ESP_GATT_WRITE_TYPE_NO_RSP,
                                                                 ESP_GATT_AUTH_REQ_NONE);
                }

                if (ret_status != ESP_GATT_OK){
                    ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                }

                /* free descr_elem_result */
                free(descr_elem_result_b);
            }
        }
        else{
            ESP_LOGE(GATTC_TAG, "decsr not found");
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, Receive notify value:");
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGD(GATTC_TAG, "write descr success");

		if (conn_device_b)
			bleSendRecordList(); //bleSendNumericRecordList()

        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "Write char failed, error status = %x", p_data->write.status);
        }else{
            ESP_LOGD(GATTC_TAG, "Write char success");
        }
//        start_scan();
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGD(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:%08x%04x",(bda[0] << 24) + (bda[1] << 16) + (bda[2] << 8) + bda[3],
                 (bda[4] << 8) + bda[5]);
        break;
    }
    case ESP_GATTC_DISCONNECT_EVT:
        if (memcmp(p_data->disconnect.remote_bda, gl_profile_tab[PROFILE_B_APP_ID].remote_bda, 6) == 0){
            ESP_LOGI(GATTC_TAG, "device 'B' disconnect");
            conn_device_b = false;
            get_service_b = false;
            if (!conn_device_a)
                Isconnecting = false;
    		// so turn off blue led

            // try to connect up to 3 times
            static int smartphoneConnectAttempt = 0;
            if (smartphoneConnectAttempt < 3) {
            	start_scan();
            	smartphoneConnectAttempt++;
            }
            else {
            	smartphoneConnectAttempt = 0;
//            	runNfcFunction =true;
            }
#ifdef ACTIVE_LOW_LEDS
    		gpio_set_level(GPIO_OUTPUT_B, 1);
#else
    		gpio_set_level(GPIO_OUTPUT_B, 0);
#endif
        }
        break;
    default:
        break;
    }
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;

    uint8_t *adv_uuid_cmpl = NULL;
    uint8_t adv_uuid_cmpl_len = 0;

    uint8_t *adv_uuid_part = NULL;
    uint8_t adv_uuid_part_len = 0;

    switch (event) {
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connetion params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        uint32_t duration = 10;
//        esp_ble_gap_start_scanning(duration);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(GATTC_TAG, "Scan start success");
        }else{
            ESP_LOGE(GATTC_TAG, "Scan start failed");
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(GATTC_TAG, "Searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

            adv_uuid_cmpl = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_CMPL, &adv_uuid_cmpl_len);

            adv_uuid_part = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
            		ESP_BLE_AD_TYPE_128SRV_PART, &adv_uuid_part_len);
            // service uuid is bb_serviceUUID

            ESP_LOGI(GATTC_TAG, "Searched Device Name Len %d", adv_name_len);
            esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
            ESP_LOGI(GATTC_TAG, "\n");
//            if (Isconnecting){   ----> idiot mi she sam at ze po
//                break;
//            }
//            if (conn_device_a && conn_device_b && !stop_scan_done){  // shouldn't be on comment by may cause bugs...
//                stop_scan_done = true;
//                esp_ble_gap_stop_scanning();
//                ESP_LOGI(GATTC_TAG, "all devices are connected");
//                break;
//            }
            if (adv_name != NULL) {

                if (strlen(remote_device_name[0]) == adv_name_len && strncmp((char *)adv_name, remote_device_name[0], adv_name_len) == 0) {
                    if (conn_device_a == false) {
                        conn_device_a = true;
                        ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[0]);
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, BLE_ADDR_TYPE_PUBLIC, true);
                        Isconnecting = true;
                    }
                    break;
                }
                ESP_LOGD("FEBUGGGGGGGG", "IM NOT NULL adv_name");
                if (/*strlen(remote_device_name[1]) == adv_name_len &&*/ strncmp((char *)adv_name, remote_device_name[1], adv_name_len) == 0) {
                    if (conn_device_b == false) {
                        conn_device_b = true;
                        ESP_LOGI(GATTC_TAG, "Searched device %s", remote_device_name[1]);
                        esp_ble_gap_stop_scanning();
                        esp_ble_gattc_open(gl_profile_tab[PROFILE_B_APP_ID].gattc_if, scan_result->scan_rst.bda, BLE_ADDR_TYPE_PUBLIC, true);
                        Isconnecting = true;
                        //vTaskDelay(10000/portTICK_RATE_MS);  // debug for assertion failing

                    }
                }
            }

            if (scan_result->scan_rst.rssi > -40) {
//                ESP_LOGI(GATTC_TAG, "Close Device!!! Name Len %d", adv_name_len);
//                esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
//                esp_log_buffer_hex(GATTC_TAG, adv_uuid_cmpl, adv_uuid_cmpl_len);
//                esp_log_buffer_hex(GATTC_TAG, adv_uuid_part, adv_uuid_part_len);
//                esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.ble_adv, 31);
//                esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.ble_adv+31, 31);


                ESP_LOGI(GATTC_TAG, "\n");
				if (adv_uuid_cmpl != NULL || adv_uuid_part != NULL) {
					if ((16 == adv_uuid_cmpl_len && (memcmp(adv_uuid_cmpl, bb_serviceUUID, adv_uuid_cmpl_len) == 0))
							|| (16 == adv_uuid_part_len && (memcmp(adv_uuid_part, bb_serviceUUID, adv_uuid_part_len)))) {
						ESP_LOGI(GATTC_TAG, "searched device\n");
						esp_log_buffer_hex(GATTC_TAG, bb_serviceUUID, 16);
						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_cmpl)\n");
						esp_log_buffer_hex(GATTC_TAG, adv_uuid_cmpl, 16);
						ESP_LOGI(GATTC_TAG, "found following device (adv_uuid_part)\n");
						if (conn_device_a == false) {
							conn_device_a = true;
							ESP_LOGI(GATTC_TAG, "connect to the remote device.");
							esp_ble_gap_stop_scanning();
							esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda,BLE_ADDR_TYPE_PUBLIC  , true);
						}
					}
				}
            }
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Scan stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop scan successfully");

        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "Adv stop failed");
            break;
        }
        ESP_LOGI(GATTC_TAG, "Stop adv successfully");
        break;

    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    //ESP_LOGI(GATTC_TAG, "EVT %d, gattc if %d, app_id %d", event, gattc_if, param->reg.app_id);

    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "Reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

void init_ble()
{
	memcpy(remote_a_filter_service_uuid.uuid.uuid128, bb_serviceUUID, ESP_UUID_LEN_128);
	memcpy(remote_a_filter_measurements_char_uuid.uuid.uuid128, bb_measurements_characteristicUUID, ESP_UUID_LEN_128);
	memcpy(remote_a_filter_profile_char_uuid.uuid.uuid128, bb_profile_characteristicUUID, ESP_UUID_LEN_128);

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s initialize controller failed\n", __func__);
		return;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s enable controller failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s init bluetooth failed\n", __func__);
		return;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed\n", __func__);
		return;
	}

	//register the  callback function to the gap module
	ret = esp_ble_gap_register_callback(esp_gap_cb);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gap register error, error code = %x", ret);
		return;
	}

	//register the callback function to the gattc module
	ret = esp_ble_gattc_register_callback(esp_gattc_cb);
	if(ret){
		ESP_LOGE(GATTC_TAG, "gattc register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gattc_app_register(PROFILE_B_APP_ID);
	if (ret){
		ESP_LOGE(GATTC_TAG, "gattc app register error, error code = %x", ret);
		return;
	}

	ret = esp_ble_gatt_set_local_mtu(200);
	if (ret){
		ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", ret);
	}
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



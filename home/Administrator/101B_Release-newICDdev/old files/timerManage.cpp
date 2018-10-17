/*
 * timerManage.cpp
 *
 *  Created on: May 14, 2018
 *      Author: albert
 */
#include "timerManage.h"
#include "bleManage.h"
#include "nvsManage.h"


uint32_t timePastMinutes;// = 0;
RTC_DATA_ATTR uint32_t timePastSeconds;// = 0; // consider to typdef a struct timePast

static void inline print_timer_counter(uint64_t counter_value)
{
    ESP_LOGI(TIMERLOG, "Counter: 0x%08x%08x\n", (uint32_t) (counter_value >> 32),
                                    (uint32_t) (counter_value));
    ESP_LOGI(TIMERLOG, "Time   : %.8f s\n", (double) counter_value / TIMER_SCALE);
}

void IRAM_ATTR timer_group0_isr(void *para)
{
    int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

    /* Prepare basic event data
       that will be then sent back to the main program task */
    timer_event_t evt;
    evt.timer_group = TIMER_GROUP_0;
    evt.timer_idx = (timer_idx_t)timer_idx;
    evt.timer_counter_value = timer_counter_value;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        evt.type = TEST_WITHOUT_RELOAD;
        TIMERG0.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
        TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
    } else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1) {
        evt.type = TEST_WITH_RELOAD;
        TIMERG0.int_clr_timers.t1 = 1;
    } else {
        evt.type = -1; // not supported even type
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

    /* Now just send the event data back to the main program task */
    xQueueSendFromISR(timer_queue, &evt, NULL);
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void example_tg0_timer_init(timer_idx_t timer_idx, bool auto_reload, double timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

void init_timers() {
	timer_queue = xQueueCreate(30, sizeof(timer_event_t));
	    example_tg0_timer_init(TIMER_0, TEST_WITHOUT_RELOAD, TIMER_INTERVAL0_SEC);
	    example_tg0_timer_init(TIMER_1, TEST_WITH_RELOAD,    TIMER_INTERVAL1_SEC);
}

void checkForTimerISR ()
{
	timer_event_t evt;
	// Check if ISR sent Queue
	ESP_LOGD(TIMERLOG, "I'm checking if there ISR request..");

	if (xQueueReceive(timer_queue, &evt, 5)) {
		/* Print information that the timer reported an event */
		if (evt.type == TEST_WITHOUT_RELOAD) {
			ESP_LOGD(TIMERLOG, "\n    Example timer without reload");
		} else if (evt.type == TEST_WITH_RELOAD) {
			ESP_LOGD(TIMERLOG, "\n    Example timer with auto reload");

			ESP_LOGI(TIMERLOG, "it past %d minutes from last restart", ++timePastMinutes);

			// check if need to fetch measures ---- > old version workflow
//			if (timePastMinutes % 2 == 0 && getConnDeviceA()) { // was && conn_device_a
//				// past 2 minutes so need to sample BioBeat
////					esp_ble_gattc_read_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
////												gl_profile_tab[PROFILE_A_APP_ID].conn_id,
////												gl_profile_tab[PROFILE_A_APP_ID].char_handle,
////												ESP_GATT_AUTH_REQ_NONE);
//				esp_log_buffer_hex(NFCLOG, measureDeviceBtMacAddrs, MAC_SIZE); // from main
//				if (connectToMeasurementDevice(measureDeviceBtMacAddrs) == false) {
//					readFromMeasurementDevice();
//				}
//			}
			nvsWriteMinutesCounter();//timePastMinutes);
		} else {
			ESP_LOGD(TIMERLOG, "\n    UNKNOWN EVENT TYPE\n");
		}
		ESP_LOGD(TIMERLOG, "Group[%d], timer[%d] alarm event\n", evt.timer_group, evt.timer_idx);

		/* Print the timer values passed by event */
		ESP_LOGD(TIMERLOG, "------- EVENT TIME --------\n");
		print_timer_counter(evt.timer_counter_value);

		/* Print the timer values as visible by this task */
		ESP_LOGD(TIMERLOG, "-------- TASK TIME --------\n");
		uint64_t task_counter_value;
		timer_get_counter_value(evt.timer_group, evt.timer_idx, &task_counter_value);
		print_timer_counter(task_counter_value);

	}
	else {
		ESP_LOGD(TIMERLOG, "there is no ISR request..\n");
	}

}

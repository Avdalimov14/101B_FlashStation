/*
 * timerRelated.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_TIMERMANAGE_H_
#define MAIN_TIMERMANAGE_H_


// Timer configurations section
#ifdef __cplusplus
extern "C" {
#endif

#include "mainHeader.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_log.h"

#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

#ifdef REAL_TIME_TEST_PROG
/*#define*/static float TIMER_INTERVAL0_SEC =  30; // sample test interval for the first timer
/*#define*/static float TIMER_INTERVAL1_SEC =  60;  // sample test interval for the second timer
#else
#define TIMER_INTERVAL0_SEC   15 // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   5  // sample test interval for the second timer
#endif

#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

#define TIMERLOG "Timer Log"
/*
 * A sample structure to pass events
 * from the timer interrupt handler to the main program.
 */
typedef struct {
    int type;  // the type of timer's event
    timer_group_t timer_group;
    timer_idx_t timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;


extern xQueueHandle timer_queue;
//uint32_t timePastMinutes;// = 0;
//RTC_DATA_ATTR uint32_t timePastSeconds;// = 0; // consider to typdef a struct timePast
#define NUM_OF_SEC_DEF 60

void  print_timer_counter(uint64_t counter_value); // may be not static?
void IRAM_ATTR timer_group0_isr(void *para);
static void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec);

void init_timers();
void checkForTimerISR (void *pvParameters);
void updateTimeParams();

// extern vars
extern uint8_t measureDeviceBtMacAddrs[6];
extern uint32_t timePastMinutes;
extern uint32_t timePastSeconds;
extern uint8_t timePastEventNum;
RTC_DATA_ATTR static time_t rtcLastFromResetInSeconds = 0;

#ifdef __cplusplus
}
#endif

#endif /* MAIN_TIMERMANAGE_H_ */

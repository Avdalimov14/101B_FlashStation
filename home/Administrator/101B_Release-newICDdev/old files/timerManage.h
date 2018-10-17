/*
 * timerRelated.h
 *
 *  Created on: Apr 23, 2018
 *      Author: albert
 */

#ifndef MAIN_TIMERMANAGE_H_
#define MAIN_TIMERMANAGE_H_


#include "mainHeader.h"

// Timer configurations section
#ifdef __cplusplus
extern "C" {
#endif


#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (60.9) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC   (15)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

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


static xQueueHandle timer_queue;
//uint32_t timePastMinutes;// = 0;
//RTC_DATA_ATTR uint32_t timePastSeconds;// = 0; // consider to typdef a struct timePast
#define NUM_OF_SEC_DEF 60

static void inline print_timer_counter(uint64_t counter_value); // may be not static?
void IRAM_ATTR timer_group0_isr(void *para);
static void example_tg0_timer_init(int timer_idx, bool auto_reload, double timer_interval_sec);

void init_timers();
void checkForTimerISR ();

// extern vars
extern uint8_t measureDeviceBtMacAddrs[6];

#ifdef __cplusplus
}
#endif

#endif /* MAIN_TIMERMANAGE_H_ */

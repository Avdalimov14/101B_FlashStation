/*
 * ioManage.h
 *
 *  Created on: Jun 28, 2018
 *      Author: albert
 */

#ifndef MAIN_IOMANAGE_H_
#define MAIN_IOMANAGE_H_

#include "mainHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

// PWM Defines

#define LEDC_HS_TIMER         	LEDC_TIMER_0
#define LEDC_HS_MODE          	LEDC_HIGH_SPEED_MODE
//#define LEDC_HS_CH0_R       (GPIO_NUM_33)
//#define LEDC_HS_CH0_CHANNEL   	LEDC_CHANNEL_0
//#define LEDC_HS_CH1_G       (GPIO_NUM_26)
//#define LEDC_HS_CH1_CHANNEL		LEDC_CHANNEL_1
//#define LEDC_HS_CH2_B 		(GPIO_NUM_32)
//#define LEDC_HS_CH2_CHANNEL		LEDC_CHANNEL_2
#define LEDC_HS_CH3_BZZR		(GPIO_NUM_13)
#define LEDC_HS_CH3_CHANNEL		LEDC_CHANNEL_3

#define LEDC_TEST_CH_NUM       (1)
#define LEDC_FULL_DUTY			(8191)
#define LEDC_ZERO_DUTY         (0)

// PWM global variables
static ledc_timer_config_t ledc_timer;
static ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM];


// GPIO Defines
#ifdef BoardV3
#define GPIO_OUTPUT_G   	GPIO_NUM_25//14
#define GPIO_OUTPUT_B   	GPIO_NUM_26//12
#define GPIO_OUTPUT_R   	GPIO_NUM_27//27
#else
#define GPIO_OUTPUT_G   	GPIO_NUM_14//14
#define GPIO_OUTPUT_B   	GPIO_NUM_12//12
#define GPIO_OUTPUT_R   	GPIO_NUM_27//27
#endif
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_R) | (1ULL<<GPIO_OUTPUT_G) | (1ULL<<GPIO_OUTPUT_B))
#define GPIO_INPUT_IO_0     GPIO_NUM_0//0
//#define GPIO_INPUT_IRQ		GPIO_NUM_35 - not in use this version.
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)

// Declare of init functions
void init_gpio();
void init_pwm();
void flickLed();
void flickLedTwice();
#define FLICK flickLed()
#define FLICK_T flickLedTwice()
void turnBlueLed(bool on);
void turnGreenLed(bool on);
void turnRedLed(bool on);

// Beep section
static int BEEP_FREQ = 5000;
#define BEEP_MS 100
void beep();
#define BEEP beep()


#ifdef __cplusplus
}
#endif


#endif /* MAIN_IOMANAGE_H_ */


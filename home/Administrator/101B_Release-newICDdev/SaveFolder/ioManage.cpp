/*
 * ioManage.cpp
 *
 *  Created on: May 16, 2018
 *      Author: albert
 */
#include "ioManage.h"

void init_gpio()
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
#ifdef ACTIVE_LOW_LEDS
	gpio_set_level(GPIO_OUTPUT_R, 1ULL);
	gpio_set_level(GPIO_OUTPUT_G, 1ULL);
	gpio_set_level(GPIO_OUTPUT_B, 1ULL);
#else
	gpio_set_level(GPIO_OUTPUT_R, NULL);
	gpio_set_level(GPIO_OUTPUT_G, NULL);
	gpio_set_level(GPIO_OUTPUT_B, NULL);
#endif

	//bit mask of the pins
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpio_config(&io_conf);
}

void init_pwm(bool buzzerOnly)
{
	/*
	 * Prepare and set configuration of timers
	 * that will be used by LED Controller
	 */
	ledc_timer.duty_resolution = LEDC_TIMER_13_BIT; // resolution of PWM duty
	ledc_timer.freq_hz = BEEP_FREQ;                      // frequency of PWM signal
	ledc_timer.speed_mode = LEDC_HS_MODE;           // timer mode
	ledc_timer.timer_num = LEDC_HS_TIMER;            // timer index

	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);

	if (buzzerOnly) {
		ledc_channel[3].channel = LEDC_HS_CH3_CHANNEL;
		ledc_channel[3].duty		= LEDC_ZERO_DUTY;
		ledc_channel[3].gpio_num	= LEDC_HS_CH3_BZZR;
		ledc_channel[3].speed_mode	= LEDC_HS_MODE;
		ledc_channel[3].timer_sel	= LEDC_HS_TIMER;

		ledc_channel_config(&ledc_channel[3]);

	} else {
		ledc_channel[0].channel    = LEDC_HS_CH0_CHANNEL,
		ledc_channel[0].duty       = LEDC_FULL_DUTY,
		ledc_channel[0].gpio_num   = LEDC_HS_CH0_R,
		ledc_channel[0].speed_mode = LEDC_HS_MODE,
		ledc_channel[0].timer_sel  = LEDC_HS_TIMER;

		ledc_channel[1].channel    = LEDC_HS_CH1_CHANNEL;
		ledc_channel[1].duty       = LEDC_FULL_DUTY;
		ledc_channel[1].gpio_num   = LEDC_HS_CH1_G;
		ledc_channel[1].speed_mode = LEDC_HS_MODE;
		ledc_channel[1].timer_sel  = LEDC_HS_TIMER;

		ledc_channel[2].channel	= LEDC_HS_CH2_CHANNEL;
		ledc_channel[2].duty		= LEDC_FULL_DUTY;
		ledc_channel[2].gpio_num	= LEDC_HS_CH2_B;
		ledc_channel[2].speed_mode	= LEDC_HS_MODE;
		ledc_channel[2].timer_sel	= LEDC_HS_TIMER;

		ledc_channel[3].channel = LEDC_HS_CH3_CHANNEL;
		ledc_channel[3].duty		= LEDC_ZERO_DUTY;
		ledc_channel[3].gpio_num	= LEDC_HS_CH3_BZZR;
		ledc_channel[3].speed_mode	= LEDC_HS_MODE;
		ledc_channel[3].timer_sel	= LEDC_HS_TIMER;

		// Set LED Controller with previously prepared configuration
		for (int ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
			ledc_channel_config(&ledc_channel[ch]);
		}

		// Initialize fade service.
		ledc_fade_func_install(0);
	}
}

void beep()
{
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_FULL_DUTY / 4);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
	vTaskDelay(BEEP_MS / portTICK_RATE_MS);
	ledc_set_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel, LEDC_ZERO_DUTY);
	ledc_update_duty(ledc_channel[3].speed_mode, ledc_channel[3].channel);
	vTaskDelay(BEEP_MS / portTICK_RATE_MS);
}


#include <atmel_start.h>
#include "led_driver.h"
#include <math.h>

#define LED_OFF			0

int prev[4],current[4],status,reset,prev_reset;
int button_led[4]={128,64,32,16};
uint8_t led_stat;

void wTouch_button_read(void)
{
	for (uint8_t i = 2;i<4; i++)
	{
		uint8_t check_button = get_sensor_state(i) & KEY_TOUCHED_MASK;
		if(check_button!=0)
		{
			prev[i]=current[i];
			current[i]=1;
			led_gpio_update(button_led[i],LED_BUTTON);
			
		}
		else
		{
			if(status==0)
			{
				led_gpio_update(LED_OFF,LED_BUTTON);
				current[i]=0;
			}
			else
			{
				led_gpio_update(button_led[0],LED_BUTTON);
				current[i]=0;
			}
			
		}
		if(current[i]==1 && prev[i]==0)
		{
			if(i==3)
			printf("Cool -\r\n");
			else
			printf("Cool +\r\n");
		}
	}
}

void oled_display(void)
{
	int i=0;
	uint8_t check_button = get_sensor_state(i) & KEY_TOUCHED_MASK;
	if(check_button!=0)
	{
		prev[i]=current[i];
		current[i]=1;
	}
	else
	{
		current[i]=0;
	}
	if(current[i]==1 && prev[i]==0)
	{
		touch_detected(0);
	}
}

void touch_detected(int i)
{
	if(status==1)
	{
		printf("DisOFF\r\n");
		led_gpio_update(LED_OFF,LED_BUTTON);
		status=0;
	}
	else
	{
		printf("Dis_ON\r\n");
		led_gpio_update(button_led[i],LED_BUTTON);
		status=1;
	}
}

void factory_reset(void)
{
	uint8_t check_button0 = get_sensor_state(0) & KEY_TOUCHED_MASK;
	uint8_t check_button2 = get_sensor_state(2) & KEY_TOUCHED_MASK;
	uint8_t check_button3 = get_sensor_state(3) & KEY_TOUCHED_MASK;
	if(check_button0!=0 && check_button2!=0 && check_button3!=0)
	{
		for(int i=0;i<30000;i++);
		printf("ResetF\r\n");
		led_gpio_update(button_led[0]|button_led[1]|button_led[2]|button_led[3],LED_BUTTON);
		reset=1;
	}
	if(reset==1)
	{
		led_gpio_update(LED_OFF,LED_BUTTON);
		status=0;
		reset=0;
	}
}

int main(void)
{
	/* Initializes MCU, drivers and middle ware */
	atmel_start_init();
	#if ENABLE_LED== 1u
	init_led_driver();
	led_reset();
	#endif
	status=1;
	led_gpio_update(button_led[0],LED_BUTTON);
	/* Replace with your application code */
	while (1)
	{
		touch_process();
		wTouch_button_read();
		oled_display();
		factory_reset();
	}
}
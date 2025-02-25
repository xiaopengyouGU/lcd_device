#ifndef __KEY_H__
#define __KEY_H__
	
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>
//����PIN����
#define KEY0_PIN	GET_PIN(E,4)
#define KEY1_PIN	GET_PIN(E,3)
#define KEY_UP_PIN		GET_PIN(A,0)		

#define KEY0	rt_pin_read(KEY0_PIN)
#define KEY1	rt_pin_read(KEY1_PIN)
#define KEY_UP	rt_pin_read(KEY_UP_PIN)

#define KEY0_PRES	1
#define KEY1_PRES	2
#define KEY_UP_PRES	3

void soft_delay_ms(rt_uint32_t cnt);	//��������ʱ
void key_init(void);
rt_uint8_t key_scan(rt_uint8_t mode);	//����ɨ��


#endif

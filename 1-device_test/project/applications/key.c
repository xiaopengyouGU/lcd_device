#include "key.h"

void key_init(void)
{
	rt_pin_mode(KEY0_PIN,PIN_MODE_INPUT_PULLUP);	//������Ĭ�ϸߵ�ƽ
	rt_pin_mode(KEY1_PIN,PIN_MODE_INPUT_PULLUP);	//
	rt_pin_mode(KEY_UP_PIN,PIN_MODE_INPUT_PULLDOWN);	//������Ĭ�ϵ͵�ƽ
}

//��������ʱ��
void soft_delay_ms(rt_uint32_t cnt)
{
	for(rt_uint32_t cnt_ms = 0; cnt_ms < cnt;cnt_ms++)
	for(rt_uint16_t i = 0;i < 70; i++)
		for(rt_uint16_t j = 0; j < 1000; j++);	//ȥ����,Ԥ��cnt ms
}
rt_uint8_t key_scan(rt_uint8_t mode)	//����ɨ��
{
	static rt_uint8_t key_up = 1;	//�����ɿ���־
	rt_uint8_t key_val = 0;
	
	if(mode) key_up = 1;		//֧������
	
	if(key_up && (KEY0 == 0 || KEY1 == 0 ||KEY_UP == 1))	//��һ������������
	{
		rt_thread_mdelay(10);
		key_up = 0;
		if(KEY0 == 0) key_val = KEY0_PRES;
		if(KEY1 == 0) key_val = KEY1_PRES;
		if(KEY_UP == 1) key_val = KEY_UP_PRES;
	}
	else if(KEY0 == 1 && KEY1 == 1 && KEY_UP == 0)	//û�а�������
	{
		key_up = 1;
	}
	
	return key_val;
}

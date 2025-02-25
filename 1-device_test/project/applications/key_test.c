#include "key.h"

rt_align(RT_ALIGN_SIZE)	//�ֽڶ���
//�������¼�
#define EVENT_KEY0	(1 << 3)
#define EVENT_KEY1  (1 << 5)
#define EVENT_KEY_UP (1 << 7)


//��������
static rt_thread_t thread1;
static rt_thread_t thread2;

//�¼���
static rt_event_t my_event = RT_NULL;

//�߳���ں���
static void rt_thread1_entry(void *parameter)
{
	rt_uint32_t e;
	
	if(rt_event_recv(my_event,(EVENT_KEY0 | EVENT_KEY1 | EVENT_KEY_UP),
							RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
							RT_WAITING_FOREVER,&e) == RT_EOK)
	{
		rt_kprintf("thread1: OR recv event 0x%x\n",e);
	}
	
	rt_kprintf("thread1: delay 1s to prepare the second event\n");
	rt_thread_mdelay(1000);
	
	//�ڶ��ν��գ��������������²��ܴ����߳�
	if(rt_event_recv(my_event,(EVENT_KEY0 | EVENT_KEY1 | EVENT_KEY_UP),
						RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
						RT_WAITING_FOREVER,&e) == RT_EOK)
	{
		rt_kprintf("thread1: AND recv event 0x%x\n",e);
	}
	//ִ������¼�����ɾ��������������ظ���ʼ��������
	rt_event_delete(my_event);
	my_event = RT_NULL;
	rt_kprintf("thread1 leave.\n");
}

static void rt_thread2_entry(void *parameter)
{
	rt_uint8_t key_val;
	
	while(1)
	{
		key_val = key_scan(0);
		if(key_val == KEY0_PRES)
		{
			rt_kprintf("thread2: KEY0 pres\n");
			rt_event_send(my_event,EVENT_KEY0);
		}
		if(key_val == KEY1_PRES)
		{
			rt_kprintf("thread2: KEY1 pres\n");
			rt_event_send(my_event,EVENT_KEY1);
		}
		if(key_val == KEY_UP_PRES)
		{
			rt_kprintf("thread2: KEY_UP pres\n");
			rt_event_send(my_event,EVENT_KEY_UP);
		}
		rt_thread_mdelay(10);	//��ʱ
	}
}
void key_test(void)
{
	key_init();			//��ʼ�������豸
	my_event = rt_event_create("my_event",RT_IPC_FLAG_PRIO);
	if(my_event == RT_NULL)
	{
		rt_kprintf("create event failed!\n");
		return;
	}
	
	thread1 = rt_thread_create("thread1",
								rt_thread1_entry,
								RT_NULL,
								512,
								5,
								20);
	if(thread1 != RT_NULL)
	{
		rt_thread_startup(thread1);	//�����߳�
	}
	
	thread2 = rt_thread_create("thread2",
								rt_thread2_entry,
								RT_NULL,
								512,
								3,				//������Ӧ���ȼ����ߣ�
								20);
	if(thread2 != RT_NULL)
	{
		rt_thread_startup(thread2);	//�����߳�
	}
	
	
}
//������ msh �����б���
MSH_CMD_EXPORT(key_test, key test);

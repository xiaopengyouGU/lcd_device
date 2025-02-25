#include "key.h"

rt_align(RT_ALIGN_SIZE)	//字节对齐
//测试用事件
#define EVENT_KEY0	(1 << 3)
#define EVENT_KEY1  (1 << 5)
#define EVENT_KEY_UP (1 << 7)


//测试例程
static rt_thread_t thread1;
static rt_thread_t thread2;

//事件集
static rt_event_t my_event = RT_NULL;

//线程入口函数
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
	
	//第二次接收，三个按键都按下才能触发线程
	if(rt_event_recv(my_event,(EVENT_KEY0 | EVENT_KEY1 | EVENT_KEY_UP),
						RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
						RT_WAITING_FOREVER,&e) == RT_EOK)
	{
		rt_kprintf("thread1: AND recv event 0x%x\n",e);
	}
	//执行完该事件集后，删除掉，避免出现重复初始化的问题
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
		rt_thread_mdelay(10);	//延时
	}
}
void key_test(void)
{
	key_init();			//初始化按键设备
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
		rt_thread_startup(thread1);	//启动线程
	}
	
	thread2 = rt_thread_create("thread2",
								rt_thread2_entry,
								RT_NULL,
								512,
								3,				//按键响应优先级更高！
								20);
	if(thread2 != RT_NULL)
	{
		rt_thread_startup(thread2);	//启动线程
	}
	
	
}
//导出到 msh 命令列表中
MSH_CMD_EXPORT(key_test, key test);

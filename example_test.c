/*
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-18     Lvtou      the first version
 */
#include "drv_touch.h"
/* this is a test for touch device */
rt_align(RT_ALIGN_SIZE)
static rt_thread_t touch_thread;
static rt_device_t touch;
static rt_sem_t xpt_2046_sem;
struct rt_touch_data read_data;

void draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);       /* ÖÐÐÄµã */
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}

static void touch_thread_entry(void *parameter)
{
	/* we try to start adjust in this thread */
	rt_device_control(touch,RT_TOUCH_CTRL_START_ADJUST,RT_NULL);
	
	rt_uint8_t i;
	rt_uint16_t x_pos,y_pos;
	rt_err_t result;
	
	for(i = 0; i < 5; i++)
	{
		rt_sem_take(xpt_2046_sem,RT_WAITING_FOREVER);
		rt_device_read(touch,0,&read_data,sizeof(read_data));
		if(read_data.event == RT_TOUCH_EVENT_DOWN)
		{
			rt_kprintf("%d %d %d %d %d\n",
                      i,
                      read_data.x_coordinate,
                      read_data.y_coordinate,
                      read_data.timestamp,
                      read_data.width);
			draw_big_point(read_data.x_coordinate,read_data.y_coordinate,TOUCH_COLOR);
			rt_device_control(touch,RT_TOUCH_CTRL_ENABLE_INT,RT_NULL);/* enable interrupt */
		}
		rt_thread_mdelay(10);
	}
	
	/* we start polling and hand write */
	rt_device_control(touch,RT_TOUCH_CTRL_DISABLE_INT,RT_NULL);	
	lcd_show_string(30,100,"start hand write!!!\n",TOUCH_COLOR);
	rt_thread_mdelay(1000);
	lcd_clear(WHITE);
	lcd_show_string(LCD_WIDTH - 24, 0, "RST", BLUE); 		  /* show clear screen pos */
	/* if touch "RST" , clear the screen */
	while(1)
	{
		result = rt_device_read(touch,0,&read_data,sizeof(read_data));
		if(result == RT_EOK)
		{
			x_pos = read_data.x_coordinate;
			y_pos = read_data.y_coordinate;
			if(x_pos > (LCD_WIDTH - 24) && y_pos < 16)
			{
				lcd_clear(WHITE);
				lcd_show_string(LCD_WIDTH - 24, 0, "RST", BLUE); 		  /* show clear screen pos */
			}
			else
			{
				draw_big_point(read_data.x_coordinate,read_data.y_coordinate,TOUCH_COLOR);	/* support hand write */
			}
		}
		rt_thread_mdelay(10);
	}
	
}

/* receive callback function */
static rt_err_t rx_callback(rt_device_t dev, rt_size_t size)
{
	rt_sem_release(xpt_2046_sem);
	return RT_EOK;
}

void touch_test(void)
{
	touch = rt_device_find("touch");
	rt_device_open(touch,RT_DEVICE_FLAG_INT_RX);				/* open touch device */	
	
	if(touch == RT_NULL)
	{
		rt_kprintf("find touch device failed!\n");
		return;
	}
	rt_device_set_rx_indicate(touch,rx_callback);	/* set callback function */
	
	xpt_2046_sem = rt_sem_create("_2046",0,RT_IPC_FLAG_PRIO);
	if(xpt_2046_sem == RT_NULL)
	{
		rt_kprintf("create xpt_2046_sem failed!\n");
		return;
	}
	
	touch_thread = rt_thread_create("touch_thread",touch_thread_entry,RT_NULL,512,9,20);
	if(touch_thread == RT_NULL)
	{
		rt_kprintf("create touch thread failed!\n");
		return;
	}
	rt_thread_startup(touch_thread);
		
}
MSH_CMD_EXPORT(touch_test,touch device test);

/*
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-24     Lvtou      the first version
 */

#include "drv_touch.h"
#include "stdlib.h"

/* we support RT_Thread touch frame */
#ifdef TOUCH_FINISH_ADJUST
#ifndef TOUCH_SUPPORT_FRAME
static struct touch_info _info = {
			X_RATIO_FACTOR,
			Y_RATIO_FACTOR,
			X_CENTER,
			Y_CENTER,
			0,
			0,
			TOUCH_ADJUSTED,
									};
#else
static struct touch_info _info = {
			X_RATIO_FACTOR,
			Y_RATIO_FACTOR,
			X_CENTER,
			Y_CENTER,
			0,
			0,
			TOUCH_ADJUSTED,
									};
#endif									
#else										
static struct touch_info _info;
#endif

/*************  inner functions  *******************/
static void _write_byte(rt_uint8_t data);
static rt_uint16_t _read_ad(rt_uint8_t cmd);							
static void _draw_touch_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);
/*************  inner functions  *******************/

void touch_read_pos(rt_uint8_t mode)	/* 0: logical position, 1: physical position */
{
	rt_uint16_t x_pos, y_pos;

	for(rt_uint8_t i = 0; i < TOUCH_READ_TIMES; i++)
	{
		x_pos += _read_ad(TOUCH_READ_X);
		y_pos += _read_ad(TOUCH_READ_Y);
	}
        
	/* get average value */
	x_pos /= TOUCH_READ_TIMES;
	y_pos /= TOUCH_READ_TIMES;
	
	_info.x_pos = x_pos;
	_info.y_pos = y_pos;
	/* transform physical position to logical position */
	if(!mode)
	{
	/* transform physical position to logical position */
	_info.x_pos = (float)(_info.x_pos - _info.x_center + 0.5f) / _info.x_ratio + LCD_WIDTH/ 2;
	_info.y_pos = (float)(_info.y_pos - _info.y_center + 0.5f) / _info.y_ratio + LCD_HEIGHT / 2;
	}
}

/* use 5 point to adjust the touchscreen */
void touch_adjust(struct touch_info* info)
{
	rt_uint8_t  cnt = info->adjusted;
	switch(cnt)
	{
		case 0:		/* start adjusted */
			lcd_clear(WHITE);
			lcd_show_string(40,40,"5 points adjust method!",RED);
			_draw_touch_point(20,20,RED);								
			break;
		case 1:	
			_draw_touch_point(20, 20, WHITE);              				/* clear point 1 */
			_draw_touch_point(LCD_WIDTH - 20, 20, RED);    				/* draw point 2 */
			break;
		case 2:
			_draw_touch_point(LCD_WIDTH - 20, 20, WHITE); 			 	/* clear point 2 */
			_draw_touch_point(20, LCD_HEIGHT - 20, RED);   				/* draw point 3 */
			break;
		case 3:
			_draw_touch_point(20, LCD_HEIGHT - 20, WHITE); 				/* clear point 3 */
			_draw_touch_point(LCD_WIDTH - 20, LCD_HEIGHT - 20, RED);    /* draw point 4 */
			break;
		case 4:
			lcd_clear(WHITE);   										/* clear screen */
			_draw_touch_point(LCD_WIDTH / 2, LCD_HEIGHT / 2, RED);  	/* draw point 5 */
			break;
		case 5:
		{
			info->xy_pos[cnt - 1][0] = info->x_pos;      /* X axis, physical center pos */
			info->xy_pos[cnt - 1][1] = info->y_pos;      /* Y axis, physical center pos */
			short s1,s2,s3,s4;
			double px,py;
			s1 = info->xy_pos[1][0] - info->xy_pos[0][0]; /* X pos diff of point 2 and point 1(AD value) */
			s3 = info->xy_pos[3][0] - info->xy_pos[2][0]; /* X pos diff of point 4 and point 3(AD value) */
			s2 = info->xy_pos[3][1] - info->xy_pos[1][1]; /* Y pos diff of point 4 and point 2(AD value) */
			s4 = info->xy_pos[2][1] - info->xy_pos[0][1]; /* Y pos diff of point 3 and point 1(AD value) */
			px = (double)s1 / s3;       /* factor in X axis */
			py = (double)s2 / s4;       /* factor in Y axis */

			if (px < 0)px = -px;        /* get abs(px) */
			if (py < 0)py = -py;        /* get abs(py) */
			
			if (px < 0.95 || px > 1.05 || py < 0.95 || py > 1.05 ||     /* unqulified */
					abs(s1) > 4095 || abs(s2) > 4095 || abs(s3) > 4095 || abs(s4) > 4095 || /* unqulified */
					abs(s1) == 0 || abs(s2) == 0 || abs(s3) == 0 || abs(s4) == 0            /* unqulified */
			   )
			{
				_draw_touch_point(LCD_WIDTH/ 2, LCD_HEIGHT / 2, WHITE); /* clear point 5 */
				touch_show_adjust_info(info->xy_pos, px, py);   /* show information to find problem */
				_draw_touch_point(20, 20, RED);   				/* redraw point 1 */
				info->adjusted = 1;
				return;											/* wait a new adjust */
			}
			info->adjusted = TOUCH_ADJUSTED;					/* adjusted successfully! */
			
			info->x_ratio = (float)(s1 + s3) / (2 * (LCD_WIDTH - 40));
			info->y_ratio = (float)(s2 + s4) / (2 * (LCD_HEIGHT - 40));

			info->x_center = info->x_pos;      /* X axis, physical center pos */
			info->y_center = info->y_pos;      /* Y axis, physical center pos */

			lcd_clear(WHITE);   				/* clear screen */
			lcd_show_string(35, 110,"Touch Screen Adjust OK!", BLUE); /* adjust finish */
			
			/* then we close the IRQ and start polling */
			rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_DISABLE);								
			/* show adjust info */
			char str[35];
			lcd_show_string(35,140,"touch adjust info:",RED);
			int temp0 = (int)info->x_ratio, temp2;
			float temp1 = (info->x_ratio - temp0);
			temp2 = (int)(temp1*100);
			rt_sprintf(str,"x_ratio_factor(.00)-> %d, %d",temp0,abs(temp2));
			lcd_show_string(35,160,str,RED);
			
			temp0 = (int)info->y_ratio;
			temp1 = (int)(info->y_ratio - temp0);
			temp2 = (rt_uint16_t)(temp1*100);
			rt_sprintf(str,"y_ratio_factor(.00)-> %d, %d",temp0,abs(temp2));
			lcd_show_string(35,180,str,RED);
			rt_sprintf(str,"x_center:%d",info->x_center);
			lcd_show_string(35,200,str,RED);
			rt_sprintf(str,"y_center:%d",info->y_center);
			lcd_show_string(35,220,str,RED);
			
			rt_thread_mdelay(1500);									  /* delay 1500 ms*/
			lcd_clear(WHITE);/* clear */
			return;/* screen */
		}
			
	}
	if(cnt > 0)
	{
		info->xy_pos[cnt - 1][0] = info->x_pos;
		info->xy_pos[cnt - 1][1] = info->y_pos;
	}
	info->adjusted += 1;
	rt_thread_mdelay(120);		/* here we need a 120 ms delay to debounce! */
}
/* use 5 point to adjust the touchscreen */

/**
 * @brief       show adjust info
 * @param       xy[5][2]: 5 physical pos
 * @param       px,py   : pos ratio factor(closer to 1, the better)
 */
void touch_show_adjust_info(rt_uint16_t xy[5][2], double px, double py)
{
    rt_uint8_t i;
    char sbuf[20];

    for (i = 0; i < 5; i++)   /* show 5 physical position */
    {
        rt_sprintf(sbuf, "x%d:%d", i + 1, xy[i][0]);
        lcd_show_string(40, 160 + (i * 20), sbuf, RED);
        rt_sprintf(sbuf, "y%d:%d", i + 1, xy[i][1]);
        lcd_show_string(40 + 80, 160 + (i * 20),sbuf, RED);
    }

    /* show factor in x/y direction */
    lcd_color_fill(40, 160 + (i * 20), LCD_WIDTH - 1, 16, WHITE);  /* clear previous px and py */
    rt_sprintf(sbuf, "px:0.%d", px*100);
    sbuf[7] = 0; /* add end */
    lcd_show_string(40, 160 + (i * 20), sbuf, RED);
    rt_sprintf(sbuf, "py:0.%d", py*100);
    sbuf[7] = 0; /* add end */
    lcd_show_string(40 + 80, 160 + (i * 20),sbuf, RED);
}

/*************  inner functions *******************/
/* here we use software to simulate SPI */

/* SPI write a byte data*/
static void _write_byte(rt_uint8_t data)
{
    rt_uint8_t count = 0;
	
    for (count = 0; count < 8; count++)
    {
        if (data & 0x80)    /* send 1 */
        {
            T_MOSI(1);
        }
        else                /* send 0 */
        {
            T_MOSI(0);
        }
        data <<= 1;
        T_SCK(0);
        delay_us(1);
        T_SCK(1);           /* RISING */
    }
}

/* read scan ad value */
static rt_uint16_t _read_ad(rt_uint8_t cmd)
{
    rt_uint8_t count = 0;
    rt_uint16_t num = 0;
    T_SCK(0);           /* lower clock */
    T_MOSI(0);          /* lower data */
    T_CS(0);            /* select IC */
    _write_byte(cmd); 	/* send cmd */
    delay_us(6);        /* the longest transform time of ADS7846 is 6 us */
    T_SCK(0);
    delay_us(1);
    T_SCK(1);           /* clear busy */
    delay_us(1);
    T_SCK(0);

    for (count = 0; count < 16; count++)    /* read 16 bits */
    {
        num <<= 1;
        T_SCK(0);       /* falling valid */
        delay_us(1);
        T_SCK(1);

        if (T_MISO)num++;
    }

    num >>= 4;          /* higher 12 bit are valid. */
    T_CS(1);            /* release */
    return num;
}

/* draw a cross for adjusting */
static void _draw_touch_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); /* horizontal */
    lcd_draw_line(x, y - 12, x, y + 13, color); /* vertical */
    lcd_draw_point(x + 1, y + 1, color);
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color);            /* draw center circle */
}

/*************  inner functions  *******************/

/***********  support RT_Thread touch frame  *************************/
rt_align(RT_ALIGN_SIZE)
static rt_touch_t touch_dev;
static rt_thread_t _adjust_thread;
static rt_sem_t _wait_adjust;		/* wait adjusting */
static rt_uint8_t _adjust_flag;

static void adjust_thread_entry(void *parameter)
{
#ifdef TOUCH_SUPPORT_FRAME
	rt_device_t touch = (rt_device_t)touch_dev;				/* this two lines can't be deleted when using Touch frame */
	struct rt_touch_data* data = (struct rt_touch_data *)touch_dev->parent.user_data;
#endif
	while(1)
	{
		if(_adjust_flag)
		{
			touch_read_pos(1);								/* read physical pos */
			touch_adjust(&_info);		
			_adjust_flag = 0;			
			if(_info.adjusted == TOUCH_ADJUSTED	)
			{
				_adjust_flag = 1;
				break;
			}
		}
		rt_thread_mdelay(5);
	}
	rt_sem_release(_wait_adjust);
	
}


static void _adjust_callback(void *parameter)
{
	_adjust_flag = 1;
}

static void touch_start_adjust(void)				/* start adjust */
{
	_wait_adjust = rt_sem_create("wait",0,RT_IPC_FLAG_PRIO);
	if(_wait_adjust == RT_NULL)
	{
		rt_kprintf("create wait sem failed! \n");
		return;
	}
	
	_adjust_thread = rt_thread_create("_adj_t",adjust_thread_entry,RT_NULL,512,9,20);
	if(_adjust_thread == RT_NULL)
	{
		rt_kprintf("create adjust thread failed! \n");
		return;
	}
	/* enable interrupt */
	rt_pin_mode(IRQ_PIN,PIN_MODE_INPUT_PULLUP);
	rt_pin_detach_irq(IRQ_PIN);					/* very important line!!! */
	rt_pin_attach_irq(IRQ_PIN,PIN_IRQ_MODE_FALLING,_adjust_callback, RT_NULL);
	rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_ENABLE);
	/* start adjust thread*/
	rt_thread_startup(_adjust_thread);
}

static void _irq_callback(void *parameter)		/* irq callback function */
{
	rt_touch_t touch = (rt_touch_t)parameter;
	rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_DISABLE);	/* disable interrupt */
	rt_hw_touch_isr(touch);
}

static rt_size_t _touch_readpoint(rt_touch_t touch, void*buf,rt_size_t touch_num)
{
	struct rt_touch_data *read_data = (struct rt_touch_data *)buf;
	if(!T_IRQ)
	{
		rt_uint16_t tmp = _info.x_center;
		_info.x_center = _info.y_center;	/* very abstract!!! */
		
		touch_read_pos(0);
		read_data->event = RT_TOUCH_EVENT_DOWN;
		read_data->x_coordinate = _info.x_pos;
		read_data->y_coordinate = _info.y_pos;		
		read_data->timestamp = rt_touch_get_ts();
		
		_info.x_center = tmp;
		return 1;
	}
	else
	{
		read_data->event = RT_TOUCH_EVENT_UP;
	}
	
	return 0;
}

static rt_err_t _touch_control(struct rt_touch_device *touch, int cmd, void*arg)
{
	switch(cmd)
	{
		case RT_TOUCH_CTRL_ENABLE_INT:
		{
			rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_ENABLE);
			break;
		}
		case RT_TOUCH_CTRL_DISABLE_INT:
		{
			rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_DISABLE);
			break;
		}
		case RT_TOUCH_CTRL_GET_INFO:
		{
			struct rt_touch_info * info = (struct rt_touch_info *)arg;
			info->point_num = 1;
			info->range_x = LCD_WIDTH;
			info->range_y = LCD_HEIGHT;
			info->type = RT_TOUCH_TYPE_RESISTANCE;
			info->vendor = RT_TOUCH_VENDOR_UNKNOWN;
			break;
		}
		case RT_TOUCH_CTRL_GET_ID:
		{
			rt_uint16_t *id = (rt_uint16_t *)arg;
			*id = 2046;
			break;
		}			
		case RT_TOUCH_CTRL_START_ADJUST:
		{
#ifndef TOUCH_FINISH_ADJUST
			touch_start_adjust();
			rt_sem_take(_wait_adjust,RT_WAITING_FOREVER);
			rt_sem_delete(_wait_adjust);	
			rt_pin_detach_irq(IRQ_PIN);		/* very important line!!! */
			rt_pin_attach_irq(IRQ_PIN,PIN_IRQ_MODE_FALLING,_irq_callback,touch_dev);
#endif		
			break;
		}
		
		default:break;
	}
	return RT_EOK;
}


struct rt_touch_ops _ops = 
{
	_touch_readpoint,
	_touch_control,
};

static int touch_hw_register(void)
{
	
	touch_dev = (rt_touch_t)(rt_malloc(sizeof(struct rt_touch_device)));
	/* here we use PIN device */
	rt_pin_mode(MISO_PIN,PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(CS_PIN,PIN_MODE_OUTPUT);
	rt_pin_mode(SCK_PIN,PIN_MODE_OUTPUT);
	rt_pin_mode(MOSI_PIN,PIN_MODE_OUTPUT);
	/* using IRQ */
	rt_pin_mode(IRQ_PIN,PIN_MODE_INPUT_PULLUP);
	rt_pin_attach_irq(IRQ_PIN,PIN_IRQ_MODE_FALLING,_irq_callback,touch_dev);	/* add callback functions */

	touch_dev->irq_handle = RT_NULL;
	touch_dev->ops = &_ops;
	touch_dev->config.dev_name = "xpt_2046";
	touch_dev->info.point_num = 1;
	touch_dev->info.range_x = LCD_WIDTH;
	touch_dev->info.range_y = LCD_HEIGHT;
	touch_dev->info.type = RT_TOUCH_TYPE_RESISTANCE;
	touch_dev->info.vendor = RT_TOUCH_VENDOR_UNKNOWN;
	
	return rt_hw_touch_register(touch_dev,"touch",RT_DEVICE_FLAG_STANDALONE,&_info);	/* another important line! */
}
INIT_DEVICE_EXPORT(touch_hw_register);
/***********  support RT_Thread touch frame  *************************/

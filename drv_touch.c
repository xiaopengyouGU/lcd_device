#include "drv_touch.h"
#include "drv_lcd.h"
#include "stdlib.h"
rt_align(RT_ALIGN_SIZE)

/* we support RT_Thread touch frame */
static struct touch_info _touch_info;
/* inner functions */
static void _write_byte(rt_uint8_t data);
static rt_uint16_t _read_ad(rt_uint8_t cmd);
static void _draw_touch_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);
static void _pos_transform(void);		/* this function transform physical position to logical position */

rt_thread_t _read_thread;
rt_uint8_t _read_flag = 0; 
static void read_thread_entry(void *parameter)
{
	while(1)
	{
		if(_read_flag)	/* read pos */
		{	
			if(_touch_info.adjusted == TOUCH_ADJUSTED)
			{	
				if(!T_IRQ)		/* screen is touched */
				{
					touch_read_pos(0);
					if(_touch_info.x_pos > (LCD_WIDTH - 24) && _touch_info.y_pos < 16)
					{
						lcd_clear(WHITE);			/* if touch "RST" , clear the screen */
						lcd_show_string(LCD_WIDTH - 24, 0, "RST", BLUE); 		  /* show clear screen pos */
					}
					else
					touch_draw_big_point(_touch_info.x_pos,_touch_info.y_pos,TOUCH_COLOR);
				}	
			}
			else
			{
				touch_read_pos(1);
				touch_adjust();
				_read_flag = 0;			/* change flag */
				if(_touch_info.adjusted == TOUCH_ADJUSTED)	/* adjusted finish, start polling */
				_read_flag = 1;		
			}
		}
		rt_thread_mdelay(5);		/* delay 10 ms */
	}
}


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
	_touch_info.x_pos = x_pos;
	_touch_info.y_pos = y_pos;
	/* transform physical position to logical position */
	if(!mode)
	{
		_pos_transform();	/*transform physical position to logical position */
	}
		
}

/* use 5 point to adjust the touchscreen */
void touch_adjust(void)
{
    rt_uint8_t  cnt = _touch_info.adjusted;
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
			_touch_info.xy_pos[cnt - 1][0] = _touch_info.x_pos;      /* X axis, physical center pos */
			_touch_info.xy_pos[cnt - 1][1] = _touch_info.y_pos;      /* Y axis, physical center pos */
			short s1,s2,s3,s4;
			double px,py;
			s1 = _touch_info.xy_pos[1][0] - _touch_info.xy_pos[0][0]; /* X pos diff of point 2 and point 1(AD value) */
			s3 = _touch_info.xy_pos[3][0] - _touch_info.xy_pos[2][0]; /* X pos diff of point 4 and point 3(AD value) */
			s2 = _touch_info.xy_pos[3][1] - _touch_info.xy_pos[1][1]; /* Y pos diff of point 4 and point 2(AD value) */
			s4 = _touch_info.xy_pos[2][1] - _touch_info.xy_pos[0][1]; /* Y pos diff of point 3 and point 1(AD value) */
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
				touch_show_adjust_info(_touch_info.xy_pos, px, py);   /* show information to find problem */
				_draw_touch_point(20, 20, RED);   /* redraw point 1 */
				_touch_info.adjusted = 1;
				return;			/* wait a new adjust */
			}
			_touch_info.adjusted = TOUCH_ADJUSTED;			/* adjusted successfully! */
			
			_touch_info.x_ratio_factor = (float)(s1 + s3) / (2 * (LCD_WIDTH - 40));
			_touch_info.y_ratio_factor = (float)(s2 + s4) / (2 * (LCD_HEIGHT - 40));

			_touch_info.x_center = _touch_info.x_pos;      /* X axis, physical center pos */
			_touch_info.y_center = _touch_info.y_pos;      /* Y axis, physical center pos */

			lcd_clear(WHITE);   /* clear screen */
			lcd_show_string(35, 110,"Touch Screen Adjust OK!", BLUE); /* adjust finish */
			
			/* then we close the IRQ and start polling */
			rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_DISABLE);								
			rt_thread_mdelay(1000);									  /* delay 1000 ms*/
			lcd_clear(WHITE);/* clear */
			lcd_show_string(LCD_WIDTH - 24, 0, "RST", BLUE); 		  /* show clear screen pos */
			return;/* screen */
		}
			
	}
	if(cnt > 0)
	{
		_touch_info.xy_pos[cnt - 1][0] = _touch_info.x_pos;
		_touch_info.xy_pos[cnt - 1][1] = _touch_info.y_pos;
	}
	_touch_info.adjusted += 1;
	rt_thread_mdelay(120);		/* here we need a 100 ms delay to debounce! */
}

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
    rt_sprintf(sbuf, "px:%0.2f", px);
    sbuf[7] = 0; /* add end */
    lcd_show_string(40, 160 + (i * 20), sbuf, RED);
    rt_sprintf(sbuf, "py:%0.2f", py);
    sbuf[7] = 0; /* add end */
    lcd_show_string(40 + 80, 160 + (i * 20),sbuf, RED);
}


/**
 * @brief       touchscreen init
 */
void touch_init(void)
{
	/* here we use PIN device */
	rt_pin_mode(MISO_PIN,PIN_MODE_INPUT_PULLUP);
	rt_pin_mode(CS_PIN,PIN_MODE_OUTPUT);
	rt_pin_mode(SCK_PIN,PIN_MODE_OUTPUT);
	rt_pin_mode(MOSI_PIN,PIN_MODE_OUTPUT);
	/* using IRQ */
	rt_pin_mode(IRQ_PIN,PIN_MODE_INPUT_PULLUP);
	rt_pin_attach_irq(IRQ_PIN,PIN_IRQ_MODE_FALLING,touch_irq_handle,RT_NULL);	/* add callback functions */
	rt_pin_irq_enable(IRQ_PIN,PIN_IRQ_ENABLE);									/* enable interrupt */				
	
	_read_thread = rt_thread_create("read_pos",read_thread_entry,RT_NULL,512,5,20);
	if(_read_thread != RT_NULL)
	{
		rt_kprintf("create read thread successfully!\n");
		rt_thread_startup(_read_thread);
	}
}

/* draw a big point */
void touch_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color);       /* ÖÐÐÄµã */
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}


void touch_irq_handle(void *args)
{
	_read_flag = 1;
}


/* software simulates SPI */

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

static void _pos_transform(void)
{
	/* transform physical position to logical position */
	_touch_info.x_pos = (signed short)(_touch_info.x_pos - _touch_info.x_center) / _touch_info.x_ratio_factor + LCD_WIDTH/ 2;
	_touch_info.y_pos = (signed short)(_touch_info.y_pos - _touch_info.y_center) / _touch_info.y_ratio_factor + LCD_HEIGHT / 2;
}

MSH_CMD_EXPORT(touch_init, init touchscreen )

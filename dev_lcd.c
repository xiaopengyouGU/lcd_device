#include "dev_lcd.h"

static rt_err_t rt_lcd_init(rt_device_t dev)
{
	rt_lcd_t lcd = (rt_lcd_t)dev;	
	/* check parameters */
	RT_ASSERT(lcd != RT_NULL);
	
	if(lcd->parent.flag != RT_DEVICE_FLAG_ACTIVATED)
		lcd->parent.flag = RT_DEVICE_FLAG_ACTIVATED;	/* activated the lcd! */
	else 
		return RT_EOK;
	
}

static rt_err_t rt_lcd_open(rt_device_t dev,rt_uint16_t oflag)
{
	rt_lcd_t lcd = (rt_lcd_t)dev;	
	/* check parameters */
	RT_ASSERT(lcd != RT_NULL);
	rt_lcd_init(dev);		/* if the device isn't init ,then init! */
	
	lcd->parent.open_flag = oflag;
}

static rt_err_t rt_lcd_close(rt_device_t dev)
{
	rt_lcd_t lcd = (rt_lcd_t)dev;
	/* check parameters */
	RT_ASSERT(lcd != RT_NULL);
	
	lcd->parent.flag = RT_DEVICE_FLAG_DEACTIVATE;
}


static rt_err_t rt_lcd_control(rt_device_t dev,int cmd, void* args)
{
	rt_lcd_t lcd = (rt_lcd_t)dev;
	/* check parameters */
	RT_ASSERT(lcd != RT_NULL);
	
	rt_lcd_pos_t pos = (rt_lcd_pos_t)args;	/* type transform */
	switch (cmd)
	{
		case LCD_CTRL_WRITE_POINT:
		{
			//rt_lcd_pos_t pos = (rt_lcd_pos_t)args;	/* type transform */
			rt_uint16_t *color =(rt_uint16_t *)pos->user_data; 
			lcd->ops->write_point(pos->x_pos,pos->y_pos,*color);
			break;
		}
		case LCD_CTRL_READ_POINT:
		{
			//rt_lcd_pos_t pos = (rt_lcd_pos_t)args;	/* type transform */
			rt_uint16_t color; 
			lcd->ops->read_point(pos->x_pos,pos->y_pos,&color);
			break;
		}
		case LCD_CTRL_DRAW_LINE:
		{
			rt_uint16_t x_start,y_start;
			rt_uint16_t x_end,y_end;
			
			x_start = pos->x_pos;
			y_start = pos->y_pos;
			rt_lcd_pos_t pos2 = (rt_lcd_pos_t)pos->user_data;
			x_end = pos2->x_pos;
			y_end = pos2->y_pos;
			lcd->ops->draw_line(x_start,y_start,x_end,y_end);
			break;
		}
		case LCD_CTRL_SHOW_NUM:
		{
			rt_uint32_t *num = (rt_uint32_t *)(pos->user_data);
			lcd->ops->show_num(pos->x_pos,pos->y_pos,*num);
			break;
		}
		case LCD_CTRL_SHOW_CHAR:
		{
			char *ch = (char *)(pos->user_data);
			lcd->ops->show_char(pos->x_pos,pos->y_pos,*ch);
			break;
		}
		case LCD_CTRL_SHOW_STRING:
		{
			char *str = (char *)(pos->user_data);
			lcd->ops->show_string(pos->x_pos,pos->y_pos,str);
			break;
		}
#if defined LCD_ALL_FUNCTIONS		
		case LCD_CTRL_DRAW_HOR_LINE:
		{
			rt_uint16_t *length = (rt_uint16_t *)(pos->user_data);
			lcd->ops->draw_hor_line(pos->x_pos,pos->y_pos,*length);
			break;
		}
		case LCD_CTRL_DRAW_VER_LINE:
		{
			rt_uint16_t *length = (rt_uint16_t *)(pos->user_data);
			lcd->ops->draw_ver_line(pos->x_pos,pos->y_pos,*length);
			break;
		}
		case LCD_CTRL_DRAW_RECT:
		{
			rt_lcd_pos_t info = (rt_lcd_pos_t)(pos->user_data);
			rt_uint16_t length = info->x_pos, width = info->y_pos;
			lcd->ops->draw_rect(pos->x_pos,pos->y_pos,length,width);
			break;
		}
		case LCD_CTRL_DRAW_CIRCLE:
		{
			rt_uint16_t *radius = (rt_uint16_t *)(pos->user_data);
			lcd->ops->draw_circle(pos->x_pos,pos->y_pos,*radius);
			break;
		}
#endif	
		default: break;
	}
	return RT_EOK;
}

static rt_ssize_t rt_lcd_read(rt_device_t dev,rt_off_t pos, void *buffer, rt_size_t size)
{
	return (rt_ssize_t)rt_lcd_control(dev,LCD_CTRL_READ_POINT,buffer);
}
static rt_ssize_t rt_lcd_write(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
	return (rt_ssize_t)rt_lcd_control(dev,LCD_CTRL_WRITE_POINT,buffer);
}

/* PIN device and SPI device would be using in this file */
//static void using_spi
//static void uing_pin

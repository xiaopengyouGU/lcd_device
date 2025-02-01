#ifndef __DEV_LCD_H__
#define __DEV_LCD_H__
	
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>

/**
 * @brief lcd device structure
 */
#define LCD_BASIC_FUNCTIONS 0x00
#define LCD_ALL_FUNCTIONS 0x01
struct rt_device_lcd
{
	struct rt_device parent;
	
	struct  rt_lcd_info * info;					/* lcd infomatino */
	
	const struct rt_lcd_ops * ops;				/* some lcd operators! */
};
typedef struct rt_device_lcd * rt_lcd_t;


/* lcd info */
struct rt_lcd_info
{
	rt_uint8_t pixel_format;
	rt_uint8_t	touch_enabled; 
	
	rt_uint16_t width;
	rt_uint16_t height;
	
	//rt_uint32_t lcd_ram;
	//rt_uint32_t lcd_addr;
};
typedef struct rt_lcd_info * rt_lcd_info_t;

/* */
/**
 * @brief LCD operators
 */
struct rt_lcd_ops
{
	/* functions need to be inplemented! */
	void (*control)(int cmd, void *args);
	void (*open)(void);
	void (*init)(void);
	void (*close)(void);
#if defined LCD_BASIC_FUNCTIONS	
	void (*write_point)(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t color);
	void (*read_point)(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t *color);
	void (*draw_line)(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t x_end,rt_uint16_t y_end);
	void (*show_num)(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num);
	void (*show_char)(rt_uint16_t x_pos,rt_uint16_t y_pos,char ch);
	void (*show_string)(rt_uint16_t x_pos,rt_uint16_t y_pos,const char* str);
#endif
	
#if defined LCD_ALL_FUNCTIONS	
	void (*draw_ver_line)(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length);
	void (*draw_hor_line)(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length);
	void (*draw_rect)(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t length, rt_uint16_t width);
	void (*draw_circle)(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t radius);
#endif
	
};

/* */
/**
 * @brief LCD position struct
 */
struct rt_lcd_pos
{
	rt_uint16_t x_pos;
	rt_uint16_t y_pos;
	void *user_data;		/* a little code skill */
};
typedef struct rt_lcd_pos * rt_lcd_pos_t;


/* control comand */
#define LCD_CTRL_WRITE_POINT 	0x00
#define LCD_CTRL_READ_POINT		0x01
#define LCD_CTRL_DRAW_LINE		0x02
#define LCD_CTRL_SHOW_NUM		0x03
#define LCD_CTRL_SHOW_CHAR		0x04
#define LCD_CTRL_SHOW_STRING 	0x05
#define LCD_CTRL_DRAW_VER_LINE	0x06
#define LCD_CTRL_DRAW_HOR_LINE	0x07
#define LCD_CTRL_DRAW_RECT		0x08
#define LCD_CTRL_DRAW_CIRCLE	0x09


#endif

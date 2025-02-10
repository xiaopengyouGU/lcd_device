#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__
	
#include "drv_lcd.h"
#include "drivers/dev_touch.h"

/* Here we support RT_Thread Touch frame! */
/* We use PIN device to simulate SPI */
#ifdef BSP_USING_LCD_TOUCH
#define MOSI_PIN	GET_PIN(F,9)
#define MISO_PIN	GET_PIN(B,2)
#define CS_PIN		GET_PIN(F,11)
#define SCK_PIN		GET_PIN(B,1)
#define IRQ_PIN		GET_PIN(F,10)	/* interupt trigger pin io*/

#define T_IRQ		rt_pin_read(IRQ_PIN)
#define T_SCK(x)	rt_pin_write(SCK_PIN,x)
#define T_MISO		rt_pin_read(MISO_PIN)
#define T_MOSI(x)	rt_pin_write(MOSI_PIN,x)
#define T_CS(x)		rt_pin_write(CS_PIN,x)
#endif

/* Here we support vertical screen */
#define TOUCH_READ_X	0XD0
#define TOUCH_READ_Y	0X90
#define TOUCH_READ_TIMES   3       	/* read times */
#define TOUCH_ADJUSTED	   0X10	  	/* we ues 5 points adjust! */
#define TOUCH_UNADJUSTED	0
#define TOUCH_COLOR			RED
#define delay_us(x) rt_hw_us_delay(x)



void touch_init(void);
void touch_adjust(void);
void touch_read_pos(rt_uint8_t mode);	/* 0: logical position, 1: physical position */
void touch_show_adjust_info(rt_uint16_t xy[5][2], double px, double py);	/* px(y) closer to 1,the better */
void touch_draw_big_point(rt_uint16_t x_pos, rt_uint16_t y_pos, rt_uint16_t color);
void touch_irq_handle(void *args);		/* interupt callback functions */
/* touch info struct */
struct touch_info
{
	/* touch adjust info */
	float x_ratio_factor;			
	float y_ratio_factor;			
	short x_center;					/*ad value 0~4095*/
	short y_center;					/*ad value 0~4095*/
	/* touch point position info */
	rt_uint16_t x_pos ;		
	rt_uint16_t y_pos ;		
	rt_uint8_t 	adjusted;			/* touch screen adjusted */
	rt_uint16_t xy_pos[5][2];     	/* save info of 5 points adjust */
};
#endif

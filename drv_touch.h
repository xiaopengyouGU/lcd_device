/*
 * Change Logs:
 * Date           Author       Notes
 * 2025-02-18     Lvtou      the first version
 */
#ifndef __DRV_TOUCH_H__
#define __DRV_TOUCH_H__
	
#include "drv_lcd.h"
#include "drivers/dev_touch.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Here we support RT_Thread Touch frame! 
 * We use PIN device to simulate SPI
 * Touchsreen IC is XPT2046, capacity touchscreen IC's control way is similar
 * This driver is based on drv_lcd files
 * When you first touch the screen, it will start adjusting
 * When adjusted successfully, the screen start polling, then you can draw by hand!
 * 
 */

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

/* Here we support vertical screen */
/* change the content of READ_X and READ_Y if you use horizontal screen */
#define TOUCH_READ_X	0XD0
#define TOUCH_READ_Y	0X90
#define TOUCH_READ_TIMES   3       	/* read times */
#define TOUCH_ADJUSTED	   0X10	  	/* we ues 5 points adjust! */
#define TOUCH_COLOR			RED
#define RT_TOUCH_CTRL_START_ADJUST			RT_TOUCH_CTRL_GET_STATUS + 1/* start adjust */
#define delay_us(x) rt_hw_us_delay(x)

#define TOUCH_SUPPORT_FRAME			/* for some abstract reason, Touch frame can't operate as being expected, so define this macro */
//#define TOUCH_FINISH_ADJUST			/* if you don't adjust the lcd device, define this macro */
/* If the touch device is adjusted before, we don't need to adjust it,
 * we just need to change follow parameters to our first adjust info */
#ifdef TOUCH_FINISH_ADJUST 
#ifndef TOUCH_SUPPORT_FRAME
#define X_RATIO_FACTOR	-12.14
#define Y_RATIO_FACTOR	-7.00
#define X_CENTER		2072
#define Y_CENTER		18323
#else
#define X_RATIO_FACTOR	-12.34
#define Y_RATIO_FACTOR	-8.00
#define X_CENTER		4768
#define Y_CENTER		2532
#endif
#endif
/* touch info struct */
struct touch_info
{
	/* touch adjust info */
	float x_ratio;			
	float y_ratio;			
	rt_uint16_t x_center;					/*ad value 0~4095*/
	rt_uint16_t y_center;					/*ad value 0~4095*/
	/* touch point position info */
	rt_uint16_t x_pos ;		
	rt_uint16_t y_pos ;	
	rt_uint8_t adjusted;			/* touch screen adjusted */
	rt_uint16_t xy_pos[5][2];     	/* save info of 5 points adjust */
};

void touch_adjust(struct touch_info* info);
void touch_show_adjust_info(rt_uint16_t xy[5][2], double px, double py);	/* px(y) closer to 1,the better */
void touch_read_pos(rt_uint8_t mode);	/* 0: logical position, 1: physical position */
void touch_test(void);
void draw_big_point(rt_uint16_t x, rt_uint16_t y, rt_uint16_t color);

rt_uint8_t touch_get_flag(void);
#ifdef __cplusplus
}
#endif

#endif

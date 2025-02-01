#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__
	
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>

/* #define RST_PIN		GET_PIN(E,4) */	
#define	BL_PIN			GET_PIN(B,0)	/* PWM output or GPIO output*/	

#if defined BSP_USING_LCD_DEFAULT_TOUCH
#define MOSI_PIN	GET_PIN(F,9)
#define MISO_PIN	GET_PIN(B,2)
#define CS_PIN		GET_PIN(F,11)
#define SCK_PIN		GET_PIN(B,1)
#endif

void stm32_hw_lcd_init(void);
void stm32_lcd_init(void);
rt_err_t stm32_lcd_open(void);
rt_ssize_t stm32_lcd_write(void);
rt_ssize_t stm32_lcd_read(void);
rt_err_t   stm32_lcd_control();


#endif

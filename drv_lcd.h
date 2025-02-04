#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__
	
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>
#include "dev_lcd.h"

/* 	In this frame, we use STM32 FSMC to control the TFTLCD!!!
*  	The LCD drive IC type is NT35310 with resolution 320 * 480 
*  	However, you can can transplant this frame to your device by modifying just a little:
* 		1.LCD set up table 
* 		2.LCD pin definition
*		3.LCD transplant functions
*		4.LCD device struct and hw init
*		5.MX_FSMC_Init	(if not default set up)
*		6.drv_lcd_font_ic.h (change IC init function)
*	The parts need to be modified are easy to find as you can see,
*	among which 1,3,4,6 are closed related to LCD drive IC
*  	The set up of FSMC is finished in STM32cubeMX
*  	but you must move it to drv_lcd.c if you don't use default set up!!!
*  	STM32 HAL library is used here but not much, remember stm32fxxx_hal_sram.c and stm32fxxx_ll_fsmc.c files
* 
*/

/****** LCD set up table ***********/
#define LCD_BASIC_FUNCTIONS 0x00
#define LCD_ADVANCED_FUNCTIONS 0x01
#define LCD_SUPPORT_FINSH 0x02
//#define BSP_USING_LCD_DEFAULT_TOUCH
#define LCD_WIDTH 320
#define LCD_HEIGHT 480


/****** LCD set up  table ***********/

#ifdef BSP_USING_LCD_DEFAULT_TOUCH
#define MOSI_PIN	GET_PIN(F,9)
#define MISO_PIN	GET_PIN(B,2)
#define CS_PIN		GET_PIN(F,11)
#define SCK_PIN		GET_PIN(B,1)
#endif

/*************************	LCD pin definition	**********************************************/
/* LCD pin definition  RST/WR/RD/BL/CS/RS */
/* RESET shares place with SYSTEM reset, so you can ignore it  */
//#define LCD_RST_GPIO_PORT               GPIOx
//#define LCD_RST_GPIO_PIN                SYS_GPIO_PINx
//#define LCD_RST_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOx_CLK_ENABLE(); }while(0)   /* Reset */

#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_PIN_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* Write IO*/

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_PIN_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* Read IO */

#define LCD_BL_GPIO_PORT                GPIOB
#define LCD_BL_GPIO_PIN                 GPIO_PIN_0
#define LCD_BL_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* backlight IO*/

#define LCD_CS_GPIO_PORT                GPIOG
#define LCD_CS_GPIO_PIN                 GPIO_PIN_12
#define LCD_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)

#define LCD_RS_GPIO_PORT                GPIOG
#define LCD_RS_GPIO_PIN                 GPIO_PIN_0
#define LCD_RS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0) 

/* LCD backlight control */
#define LCD_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)

#define LCD_FSMC_NEX         4              /* use FSMC_NE4 to connect LCD_CS, range: 1~4 */
#define LCD_FSMC_AX          10             /* use FSMC_A10 to connect LCD_RS, range: 0 ~ 25 */
#define LCD_BASE        (uint32_t)((0X60000000 + (0X4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((LCD_TypeDef *) LCD_BASE)
/*************************	LCD pin definition	**********************************************/
					 
struct lcd_device
{
	rt_uint16_t width;
	rt_uint16_t height;
	rt_uint16_t dir;
	rt_uint16_t wramcmd;
	rt_uint16_t setxcmd;
	rt_uint16_t setycmd;
};

/* LCD address struct */
typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;


/* scan directions */
#define L2R_U2D         0           /* From Left to Right and from up to down*/
#define L2R_D2U         1          
#define R2L_U2D         2         
#define R2L_D2U         3           

#define U2D_L2R         4          
#define U2D_R2L         5          
#define D2U_L2R         6         
#define D2U_R2L         7          
#define DFT_SCAN_DIR    L2R_U2D     /* default scan directions */

/* common pan color */
#define WHITE           0xFFFF     
#define BLACK           0x0000      
#define RED             0xF800     
#define GREEN           0x07E0      
#define BLUE            0x001F      
#define MAGENTA         0XF81F      /* BLUE + RED */
#define YELLOW          0XFFE0      /* GREEN + RED */
#define CYAN            0X07FF      /* GREEN + BLUE */  

/* uncommon pan color */
#define BROWN           0XBC40      
#define BRRED           0XFC07     
#define GRAY            0X8430      
#define DARKBLUE        0X01CF     
#define LIGHTBLUE       0X7D7C      
#define GRAYBLUE        0X5458       
#define LIGHTGREEN      0X841F      

/* functions need to be inplemented! */
#define delay_ms(x) rt_thread_mdelay(x)
void lcd_init(void);
/* lcd system functions! */
void lcd_wr_data(volatile uint16_t data);            /* LCD write data */
void lcd_wr_regno(volatile uint16_t regno);          /* LCD write register number or address */
void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCD write register value */

void lcd_clear(rt_uint16_t color);	

/***************** LCD transplant functions ********************/
void lcd_backlight_set(rt_uint8_t pwm);				/* this function needs to be modified if you change backlight IO */		
void lcd_scan_dir(rt_uint8_t dir);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_read_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t *color);
void lcd_set_window(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t width, rt_uint16_t height);
//	static void _lcd_set_cursor(rt_uint16_t x_pos,rt_uint16_t y_pos);  /* remember this function */
/***************** LCD transplant functions ********************/

/* lcd basic functions */
#ifdef LCD_BASIC_FUNCTIONS	
	void lcd_draw_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t color);
	void lcd_draw_line(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t x_end,rt_uint16_t y_end,rt_uint16_t color);
	void lcd_show_num(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num,rt_uint16_t color);
	void lcd_show_xnum(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num,rt_uint16_t color);
	void lcd_show_char(rt_uint16_t x_pos,rt_uint16_t y_pos,char ch,rt_uint16_t color);
	void lcd_show_string(rt_uint16_t x_pos,rt_uint16_t y_pos,const char* str,rt_uint16_t color);
#endif
/* lcd advanced functions */	
#ifdef LCD_ADVANCED_FUNCTIONS
	void lcd_color_fill(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t width, rt_uint16_t height,rt_uint16_t color);
	void lcd_draw_ver_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length,rt_uint16_t color);
	void lcd_draw_hor_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length,rt_uint16_t color);
	void lcd_draw_rect(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t width, rt_uint16_t height,rt_uint16_t color);
	void lcd_draw_circle(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t radius,rt_uint16_t color);
#endif


#endif

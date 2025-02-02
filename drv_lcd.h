#ifndef __DRV_LCD_H__
#define __DRV_LCD_H__
	
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_gpio.h>
#include "dev_lcd.h"

#if defined BSP_USING_LCD_DEFAULT_TOUCH
#define MOSI_PIN	GET_PIN(F,9)
#define MISO_PIN	GET_PIN(B,2)
#define CS_PIN		GET_PIN(F,11)
#define SCK_PIN		GET_PIN(B,1)
#endif

/* In this driver, we use stm32 FSMC to control the TFTLCD
*  the chip name is SSD1693
*  the set up of FSMC is finished in STM32cubeMX
*  
*/

/* In this driver, we use HAL */
#define delay_ms(x) rt_thread_mdelay(x)

/* LCD pins definition  RST/WR/RD/BL/CS/RS */
/* RESET ��ϵͳ��λ�Ź��� �������ﲻ�ö��� RESET���� */
//#define LCD_RST_GPIO_PORT               GPIOx
//#define LCD_RST_GPIO_PIN                SYS_GPIO_PINx
//#define LCD_RST_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOx_CLK_ENABLE(); }while(0)   /* ����IO��ʱ��ʹ�� */

#define LCD_WR_GPIO_PORT                GPIOD
#define LCD_WR_GPIO_PIN                 GPIO_PIN_5
#define LCD_WR_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* ����IO��ʱ��ʹ�� */

#define LCD_RD_GPIO_PORT                GPIOD
#define LCD_RD_GPIO_PIN                 GPIO_PIN_4
#define LCD_RD_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0)   /* ����IO��ʱ��ʹ�� */

#define LCD_BL_GPIO_PORT                GPIOB
#define LCD_BL_GPIO_PIN                 GPIO_PIN_0
#define LCD_BL_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)   /* ��������IO��ʱ��ʹ�� */

/* LCD_CS(��Ҫ����LCD_FSMC_NEX������ȷ��IO��) �� LCD_RS(��Ҫ����LCD_FSMC_AX������ȷ��IO��) ���� ���� */
#define LCD_CS_GPIO_PORT                GPIOG
#define LCD_CS_GPIO_PIN                 GPIO_PIN_12
#define LCD_CS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /* ����IO��ʱ��ʹ�� */

#define LCD_RS_GPIO_PORT                GPIOG
#define LCD_RS_GPIO_PIN                 GPIO_PIN_0
#define LCD_RS_GPIO_CLK_ENABLE()        do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /* ����IO��ʱ��ʹ�� */

/* FSMC��ز��� ���� 
 * ע��: ����Ĭ����ͨ��FSMC��1������LCD, ��1��4��Ƭѡ: FSMC_NE1~4
 *
 * �޸�LCD_FSMC_NEX, ��Ӧ��LCD_CS_GPIO�������Ҳ�ø�
 * �޸�LCD_FSMC_AX , ��Ӧ��LCD_RS_GPIO�������Ҳ�ø�
 */
#define LCD_FSMC_NEX         4              /* ʹ��FSMC_NE4��LCD_CS,ȡֵ��Χֻ����: 1~4 */
#define LCD_FSMC_AX          10             /* ʹ��FSMC_A10��LCD_RS,ȡֵ��Χ��: 0 ~ 25 */

#define LCD_FSMC_BCRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2]       /* BCR�Ĵ���,����LCD_FSMC_NEX�Զ����� */
#define LCD_FSMC_BTRX        FSMC_Bank1->BTCR[(LCD_FSMC_NEX - 1) * 2 + 1]   /* BTR�Ĵ���,����LCD_FSMC_NEX�Զ����� */
#define LCD_FSMC_BWTRX       FSMC_Bank1E->BWTR[(LCD_FSMC_NEX - 1) * 2]      /* BWTR�Ĵ���,����LCD_FSMC_NEX�Զ����� */

struct stm32_lcd_device
{
	rt_uint16_t width;
	rt_uint16_t height;
	rt_uint16_t dir;
	rt_uint16_t wramcmd;
	rt_uint16_t setxcmd;
	rt_uint16_t setycmd;
};

/* LCD backlight control */
#define LCD_BL(x)   do{ x ? \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_SET) : \
                      HAL_GPIO_WritePin(LCD_BL_GPIO_PORT, LCD_BL_GPIO_PIN, GPIO_PIN_RESET); \
                     }while(0)
/* LCD��ַ�ṹ�� */
typedef struct
{
    volatile uint16_t LCD_REG;
    volatile uint16_t LCD_RAM;
} LCD_TypeDef;


/* LCD_BASE����ϸ���㷽��:
 * ����һ��ʹ��FSMC�Ŀ�1(BANK1)������TFTLCDҺ����(MCU��), ��1��ַ��Χ�ܴ�СΪ256MB,���ֳ�4��:
 * �洢��1(FSMC_NE1)��ַ��Χ: 0X6000 0000 ~ 0X63FF FFFF
 * �洢��2(FSMC_NE2)��ַ��Χ: 0X6400 0000 ~ 0X67FF FFFF
 * �洢��3(FSMC_NE3)��ַ��Χ: 0X6800 0000 ~ 0X6BFF FFFF
 * �洢��4(FSMC_NE4)��ַ��Χ: 0X6C00 0000 ~ 0X6FFF FFFF
 *
 * ������Ҫ����Ӳ�����ӷ�ʽѡ����ʵ�Ƭѡ(����LCD_CS)�͵�ַ��(����LCD_RS)
 * ս��F103������ʹ��FSMC_NE4����LCD_CS, FSMC_A10����LCD_RS ,16λ������,���㷽������:
 * ����FSMC_NE4�Ļ���ַΪ: 0X6C00 0000;     NEx�Ļ�ַΪ(x=1/2/3/4): 0X6000 0000 + (0X400 0000 * (x - 1))
 * FSMC_A10��Ӧ��ֵַ: 2^10 * 2 = 0X800;    FSMC_Ay��Ӧ�ĵ�ַΪ(y = 0 ~ 25): 2^y * 2
 *
 * LCD->LCD_REG,��ӦLCD_RS = 0(LCD�Ĵ���); LCD->LCD_RAM,��ӦLCD_RS = 1(LCD����)
 * �� LCD->LCD_RAM�ĵ�ַΪ:  0X6C00 0000 + 2^10 * 2 = 0X6C00 0800
 *    LCD->LCD_REG�ĵ�ַ����Ϊ LCD->LCD_RAM֮��������ַ.
 * ��������ʹ�ýṹ�����LCD_REG �� LCD_RAM(REG��ǰ,RAM�ں�,��Ϊ16λ���ݿ��)
 * ��� �ṹ��Ļ���ַ(LCD_BASE) = LCD_RAM - 2 = 0X6C00 0800 -2
 *
 * ����ͨ�õļ��㹫ʽΪ((Ƭѡ��FSMC_NEx)x=1/2/3/4, (RS�ӵ�ַ��FSMC_Ay)y=0~25):
 *          LCD_BASE = (0X6000 0000 + (0X400 0000 * (x - 1))) | (2^y * 2 -2)
 *          ��Ч��(ʹ����λ����)
 *          LCD_BASE = (0X6000 0000 + (0X400 0000 * (x - 1))) | ((1 << y) * 2 -2)
 */
#define LCD_BASE        (uint32_t)((0X60000000 + (0X4000000 * (LCD_FSMC_NEX - 1))) | (((1 << LCD_FSMC_AX) * 2) -2))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

/******************************************************************************************/
/* LCDɨ�跽�����ɫ ���� */

/* ɨ�跽���� */
#define L2R_U2D         0           /* ������,���ϵ��� */
#define L2R_D2U         1           /* ������,���µ��� */
#define R2L_U2D         2           /* ���ҵ���,���ϵ��� */
#define R2L_D2U         3           /* ���ҵ���,���µ��� */

#define U2D_L2R         4           /* ���ϵ���,������ */
#define U2D_R2L         5           /* ���ϵ���,���ҵ��� */
#define D2U_L2R         6           /* ���µ���,������ */
#define D2U_R2L         7           /* ���µ���,���ҵ��� */

#define DFT_SCAN_DIR    L2R_U2D     /* Ĭ�ϵ�ɨ�跽�� */

/* ���û�����ɫ */
#define WHITE           0xFFFF      /* ��ɫ */
#define BLACK           0x0000      /* ��ɫ */
#define RED             0xF800      /* ��ɫ */
#define GREEN           0x07E0      /* ��ɫ */
#define BLUE            0x001F      /* ��ɫ */ 
#define MAGENTA         0XF81F      /* Ʒ��ɫ/�Ϻ�ɫ = BLUE + RED */
#define YELLOW          0XFFE0      /* ��ɫ = GREEN + RED */
#define CYAN            0X07FF      /* ��ɫ = GREEN + BLUE */  

/* �ǳ�����ɫ */
#define BROWN           0XBC40      /* ��ɫ */
#define BRRED           0XFC07      /* �غ�ɫ */
#define GRAY            0X8430      /* ��ɫ */ 
#define DARKBLUE        0X01CF      /* ����ɫ */
#define LIGHTBLUE       0X7D7C      /* ǳ��ɫ */ 
#define GRAYBLUE        0X5458      /* ����ɫ */ 
#define LIGHTGREEN      0X841F      /* ǳ��ɫ */  
#define LGRAY           0XC618      /* ǳ��ɫ(PANNEL),���屳��ɫ */ 
#define LGRAYBLUE       0XA651      /* ǳ����ɫ(�м����ɫ) */ 
#define LBBLUE          0X2B12      /* ǳ����ɫ(ѡ����Ŀ�ķ�ɫ) */ 

/******************************************************************************************/
/* SSD1963������ò���(һ�㲻�ø�) */

/* LCD�ֱ������� */ 
#define SSD_HOR_RESOLUTION      320     /* LCDˮƽ�ֱ��� */ 
#define SSD_VER_RESOLUTION      480     /* LCD��ֱ�ֱ��� */ 

/* LCD������������ */ 
#define SSD_HOR_PULSE_WIDTH     1       /* ˮƽ���� */ 
#define SSD_HOR_BACK_PORCH      46      /* ˮƽǰ�� */ 
#define SSD_HOR_FRONT_PORCH     210     /* ˮƽ���� */ 

#define SSD_VER_PULSE_WIDTH     1       /* ��ֱ���� */ 
#define SSD_VER_BACK_PORCH      23      /* ��ֱǰ�� */ 
#define SSD_VER_FRONT_PORCH     22      /* ��ֱǰ�� */ 

/* ���¼����������Զ����� */ 
#define SSD_HT          (SSD_HOR_RESOLUTION + SSD_HOR_BACK_PORCH + SSD_HOR_FRONT_PORCH)
#define SSD_HPS         (SSD_HOR_BACK_PORCH)
#define SSD_VT          (SSD_VER_RESOLUTION + SSD_VER_BACK_PORCH + SSD_VER_FRONT_PORCH)
#define SSD_VPS         (SSD_VER_BACK_PORCH)
#endif


/* functions declaration */
void lcd_wr_data(volatile uint16_t data);            /* LCDд���� */
void lcd_wr_regno(volatile uint16_t regno);          /* LCDд�Ĵ������/��ַ */
void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCDд�Ĵ�����ֵ */

/* functions need to be inplemented! */
void lcd_control(int cmd, void *args);
void lcd_open(void);
void lcd_init(void);
void lcd_close(void);

#if defined LCD_BASIC_FUNCTIONS	
	void lcd_draw_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t color);
	void lcd_read_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t *color);
	void lcd_draw_line(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t x_end,rt_uint16_t y_end,rt_uint16_t color);
	void lcd_show_num(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num,rt_uint16_t color);
	void lcd_show_char(rt_uint16_t x_pos,rt_uint16_t y_pos,char ch,rt_uint16_t color);
	void lcd_show_string(rt_uint16_t x_pos,rt_uint16_t y_pos,const char* str,rt_uint16_t color);
#endif
	
#if defined LCD_ALL_FUNCTIONS
	void lcd_display_on(void);
	void lcd_display_off(void);
	void lcd_scan_dir(rt_uint8_t dir);
	void lcd_color_fill(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t length, rt_uint16_t width,rt_uint16_t color);
	void lcd_backlight_set(rt_uint8_t pwm);
	void lcd_clear(rt_uint16_t color);
	void lcd_set_window(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t length, rt_uint16_t width);
	void lcd_draw_ver_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length,rt_uint16_t color);
	void lcd_draw_hor_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length,rt_uint16_t color);
	void lcd_draw_rect(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t length, rt_uint16_t width,rt_uint16_t color);
	void lcd_draw_circle(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t radius,rt_uint16_t color);
#endif



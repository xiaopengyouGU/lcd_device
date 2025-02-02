#include "drv_lcd.h"
#include "drv_lcd_font.h"
#include "board.h"

struct stm32_lcd_device _lcd_dev = {
	SSD_HOR_RESOLUTION,
	SSD_VER_RESOLUTION,
	0,	/* vertical */
	0X2C,	/* Write GRAM*/
	0X2B,	/* Set x pos cmd */
	0X2A,	/* Set y pos cmd */
};

rt_uint16_t g_back_color = WHITE;	/* background color */
rt_uint16_t g_pan_color = RED;		/* pan color */

static void _lcd_set_cursor(rt_uint16_t x_pos,rt_uint16_t y_pos);
static rt_uint16_t _lcd_rd_data(void);
static void _lcd_write_ram_prepare(void);
static void lcd_ex_ssd1963_reginit(void);
static rt_uint32_t _lcd_pow(rt_uint8_t m, rt_uint8_t n);

void stm32_hw_lcd_init(void)
{
	LCD_CS_GPIO_CLK_ENABLE();   /* LCD_CS��ʱ��ʹ�� */
    LCD_WR_GPIO_CLK_ENABLE();   /* LCD_WR��ʱ��ʹ�� */
    LCD_RD_GPIO_CLK_ENABLE();   /* LCD_RD��ʱ��ʹ�� */
    LCD_RS_GPIO_CLK_ENABLE();   /* LCD_RS��ʱ��ʹ�� */
    LCD_BL_GPIO_CLK_ENABLE();   /* LCD_BL��ʱ��ʹ�� */
    
	GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* ���츴�� */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* ���� */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* ���� */
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_CS���� */

    gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_WR���� */

    gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_RD���� */

    gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* ��ʼ��LCD_RS���� */

    gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* ������� */
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* LCD_BL����ģʽ����(�������) */
	
	/**************************************************************************************** */
	lcd_ex_ssd1963_reginit();			/* init the IC */
	
	lcd_backlight_set(100);		/* the lightest */
	lcd_scan_dir(DFT_SCAN_DIR);	/* default scan direction */
	LCD_BL(1);							/* light lcd */
	lcd_clear(WHITE);				/* white background */
	
}

/* functions need to be inplemented! */
void lcd_control(int cmd, void *args);
void lcd_open(void);
void lcd_init(void);
void lcd_close(void);

#if defined LCD_BASIC_FUNCTIONS	
	void lcd_draw_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t color)
	{
		_lcd_set_cursor(x_pos,y_pos);
		_lcd_write_ram_prepare();
		LCD->LCD_RAM = color;
	}
	
	void lcd_read_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t *color)
	{
		rt_uint16_t res;
		
		if(x_pos >= _lcd_dev.width || y_pos >=_lcd_dev.height) return;
		
		_lcd_set_cursor(x_pos,y_pos); //lcd set cursor
		lcd_wr_regno(0X2E); /* send read GRAM CMD */
		res = _lcd_rd_data();	/* true read */
		*color = res;
	}
	
	void lcd_draw_line(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t x_end,rt_uint16_t y_end,rt_uint16_t color)
	{
		rt_uint16_t t;
		int xerr = 0, yerr = 0, delta_x, delta_y, distance;
		int incx, incy, row, col;
		delta_x = x_end - x_start;          /* ������������ */
		delta_y = y_end - y_start;
		row = x_start;
		col = y_start;

		if (delta_x > 0)incx = 1;   /* ���õ������� */
		else if (delta_x == 0)incx = 0; /* ��ֱ�� */
		else
		{
			incx = -1;
			delta_x = -delta_x;
		}

		if (delta_y > 0)incy = 1;
		else if (delta_y == 0)incy = 0; /* ˮƽ�� */
		else
		{
			incy = -1;
			delta_y = -delta_y;
		}

		if ( delta_x > delta_y)distance = delta_x;  /* ѡȡ�������������� */
		else distance = delta_y;

		for (t = 0; t <= distance + 1; t++ )   /* ������� */
		{
			lcd_draw_point(row, col, color); /* ���� */
			xerr += delta_x ;
			yerr += delta_y ;

			if (xerr > distance)
			{
				xerr -= distance;
				row += incx;
			}

			if (yerr > distance)
			{
				yerr -= distance;
				col += incy;
			}
		}
	}
	
	void lcd_show_num(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num,rt_uint16_t color)
	{
		rt_uint8_t t,tmp;
		rt_uint8_t end_show = 0,length = 1;
		rt_uint32_t rec = num;
		while(rec > 0)
		{
			if(rec / 10) length++;
			rec /= 10;
		}
		
		for(t = 0;t < length;t++)
		{
			tmp = (num / _lcd_pow(10, length - t - 1)) % 10;   /* ��ȡ��Ӧλ������ */

			if (end_show == 0 && t < (length - 1))   /* û��ʹ����ʾ,�һ���λҪ��ʾ */
			{
				if (tmp == 0)
				{
					lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, ' ',color);/* ��ʾ�ո�,ռλ */
					continue;   /* �����¸�һλ */
				}
				else
				{
					end_show = 1; /* ʹ����ʾ */
				}

			}

			lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, tmp + '0', color); /* ��ʾ�ַ� */
		}
	}
	
	void lcd_show_xnum(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint32_t num,rt_uint16_t color)
	{
		rt_uint8_t t, temp;
		rt_uint8_t enshow = 0,len = 1;
		rt_uint32_t rec = num;
		
		while(rec > 0)
		{
			if(rec / 16) len++;
			rec /= 16;
		}
	
		for (t = 0; t < len; t++)   /* ������ʾλ��ѭ�� */
		{
			temp = (num / _lcd_pow(16, len - t - 1)) % 16;    /* ��ȡ��Ӧλ������ */

			if (enshow == 0 && t < (len - 1))   /* û��ʹ����ʾ,�һ���λҪ��ʾ */
			{
				if (temp == 0)
				{
					lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, ' ',color);  /* �ÿո�ռλ */					
					continue;
				}
				else
				{
					enshow = 1; /* ʹ����ʾ */
				}
			}

			lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, temp + '0',color);
		}
	}
	
	void lcd_show_char(rt_uint16_t x_pos,rt_uint16_t y_pos,char ch,rt_uint16_t color)
	{
		rt_uint8_t tmp, t1,t;
		rt_uint8_t csize = (FONT_SIZE/8 + ((FONT_SIZE % 8)? 1:0))*(FONT_SIZE);/* (size/8 + ((size % 8)? 1:0))*(size/2); */
		rt_uint8_t *pfont;
		rt_uint16_t y0 = y_pos;
		
		ch = ch - ' ';			/* get index */
		pfont = (rt_uint8_t *)char_font[ch];	/* 16*8 font */
		
		for(t = 0; t < csize; t++)
		{
			tmp = pfont[t];
			
			for(t1 = 0; t1 < 8;t1++)
			{
				if(tmp & 0X80) /* effective point,show */
				{
					lcd_draw_point(x_pos,y_pos,color);
				}
				else
				{
					lcd_draw_point(x_pos,y_pos,g_back_color);
				}
				
				tmp <<= 1;
				y_pos++;
				
				if(y_pos >= _lcd_dev.height)return;	/* pos out of range */
				
				if((y_pos - y0)==FONT_SIZE)
				{
					y_pos = y0;
					x_pos++;
					
					if(x_pos >= _lcd_dev.width)return; /* pos out of range */
					break;
				}
			}
		}
		
	}
	
	void lcd_show_string(rt_uint16_t x_pos,rt_uint16_t y_pos,const char* str,rt_uint16_t color)
	{
		const char *ptr = str;
		rt_uint16_t i = 0;
		while((*ptr)!= '\0')
		{
			lcd_show_char(x_pos+i,y_pos,*ptr,color);
			ptr++;
			i++;
		}
	}
#endif
	
#if defined LCD_ALL_FUNCTIONS	
	void lcd_display_on(void)
	{
		lcd_wr_regno(0X29);
	}
	
	void lcd_display_off(void)
	{
		lcd_wr_regno(0X28);
	}
	
	
	void lcd_scan_dir(rt_uint8_t dir)
{
    rt_uint16_t regval = 0;
    rt_uint16_t dirreg = 0;
    rt_uint16_t temp;

    /* ����ʱ����1963���ı�ɨ�跽������IC�ı�ɨ�跽������ʱ1963�ı䷽������IC���ı�ɨ�跽�� */
    if (_lcd_dev.dir == 0)
    {
        switch (dir)   /* ����ת�� */
        {
            case 0:
                dir = 6;
                break;

            case 1:
                dir = 7;
                break;

            case 2:
                dir = 4;
                break;

            case 3:
                dir = 5;
                break;

            case 4:
                dir = 1;
                break;

            case 5:
                dir = 0;
                break;

            case 6:
                dir = 3;
                break;

            case 7:
                dir = 2;
                break;
        }
    }


	/* ����ɨ�跽ʽ ���� 0X36/0X3600 �Ĵ��� bit 5,6,7 λ��ֵ */
	switch (dir)
	{
		case L2R_U2D:/* ������,���ϵ��� */
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;

		case L2R_D2U:/* ������,���µ��� */
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;

		case R2L_U2D:/* ���ҵ���,���ϵ��� */
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;

		case R2L_D2U:/* ���ҵ���,���µ��� */
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;

		case U2D_L2R:/* ���ϵ���,������ */
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;

		case U2D_R2L:/* ���ϵ���,���ҵ��� */
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;

		case D2U_L2R:/* ���µ���,������ */
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;

		case D2U_R2L:/* ���µ���,���ҵ��� */
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
	}

	dirreg = 0X36;  /* �Ծ��󲿷�����IC, ��0X36�Ĵ������� */

	lcd_write_reg(dirreg, regval);

	/* ������ʾ����(����)��С */
	lcd_wr_regno(_lcd_dev.setxcmd);
	lcd_wr_data(0);
	lcd_wr_data(0);
	lcd_wr_data((_lcd_dev.width - 1) >> 8);
	lcd_wr_data((_lcd_dev.width - 1) & 0XFF);
	lcd_wr_regno(_lcd_dev.setycmd);
	lcd_wr_data(0);
	lcd_wr_data(0);
	lcd_wr_data((_lcd_dev.height - 1) >> 8);
	lcd_wr_data((_lcd_dev.height - 1) & 0XFF);
	
    }

	void lcd_color_fill(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t length, rt_uint16_t width,rt_uint16_t color)
	{
		rt_uint16_t i,j;
		
		for(j = y_start;i < (y_start + width); j++)
		{
			_lcd_set_cursor(x_start,j);
			_lcd_write_ram_prepare();
			for(i = 0; i < length;i++)
			{
				LCD->LCD_RAM = color;
			}
		}
	}
	
	void lcd_backlight_set(rt_uint8_t pwm)
	{
		lcd_wr_regno(0xBE);         /* ����PWM��� */
		lcd_wr_data(0x05);          /* 1����PWMƵ�� */
		lcd_wr_data(pwm * 2.55);    /* 2����PWMռ�ձ� */
		lcd_wr_data(0x01);          /* 3����C */
		lcd_wr_data(0xFF);          /* 4����D */
		lcd_wr_data(0x00);          /* 5����E */
		lcd_wr_data(0x00);          /* 6����F */
	}
	
	void lcd_clear(rt_uint16_t color)
	{
		rt_uint32_t index = 0;
		rt_uint32_t totalpoint = _lcd_dev.width;
		totalpoint *= _lcd_dev.height;    /* �õ��ܵ��� */
		_lcd_set_cursor(0x00, 0x0000);   /* ���ù��λ�� */
		_lcd_write_ram_prepare();        /* ��ʼд��GRAM */

		for (index = 0; index < totalpoint; index++)
		{
			LCD->LCD_RAM = color;
		}
	}
	
	void lcd_set_window(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t length, rt_uint16_t width)
	{
		   
		rt_uint16_t tlength, twidth;
		tlength = x_start + length - 1;
		twidth = y_start + width - 1;

   
   if (_lcd_dev.dir != 1)     /* 1963�������⴦�� */
    {
        x_start = _lcd_dev.width - width - x_start;
        width = y_start + width - 1;
        lcd_wr_regno(_lcd_dev.setxcmd);
        lcd_wr_data(x_start >> 8);
        lcd_wr_data(x_start & 0XFF);
        lcd_wr_data((x_start + width - 1) >> 8);
        lcd_wr_data((x_start + width - 1) & 0XFF);
        lcd_wr_regno(_lcd_dev.setycmd);
        lcd_wr_data(y_start >> 8);
        lcd_wr_data(y_start & 0XFF);
        lcd_wr_data(width >> 8);
        lcd_wr_data(width & 0XFF);
    }
    else    /* 9341/5310/7789/1963���� �� ���ô��� */
    {
        lcd_wr_regno(_lcd_dev.setxcmd);
        lcd_wr_data(x_start >> 8);
        lcd_wr_data(x_start & 0XFF);
        lcd_wr_data(tlength >> 8);
        lcd_wr_data(tlength & 0XFF);
        lcd_wr_regno(_lcd_dev.setycmd);
        lcd_wr_data(y_start >> 8);
        lcd_wr_data(y_start & 0XFF);
        lcd_wr_data(twidth >> 8);
        lcd_wr_data(twidth & 0XFF);
    }
	}
	
	void lcd_draw_ver_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t width,rt_uint16_t color)
	{
		if(x_pos >= _lcd_dev.width || y_pos >= _lcd_dev.height || width == 0) return;
		lcd_color_fill(x_pos,y_pos,1,width,color);
	}
	
	void lcd_draw_hor_line(rt_uint16_t x_pos,rt_uint16_t y_pos, rt_uint16_t length,rt_uint16_t color)
	{
		if(x_pos >= _lcd_dev.width || y_pos >= _lcd_dev.height || length == 0) return;
		lcd_color_fill(x_pos,y_pos,length,1,color);
	}
	
	void lcd_draw_rect(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t length, rt_uint16_t width,rt_uint16_t color)
	{
		lcd_draw_line(x_pos,y_pos,x_pos + length,y_pos,color);
		lcd_draw_line(x_pos,y_pos,x_pos,y_pos + width,color);
		lcd_draw_line(x_pos,y_pos + width,x_pos + length,y_pos + width,color);
		lcd_draw_line(x_pos + length,y_pos,x_pos + length,y_pos + width,color);
	}
	void lcd_draw_circle(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t radius,rt_uint16_t color)
	{
		int a, b;
		int di;
		a = 0;
		b = radius;
		di = 3 - (radius << 1);       /* �ж��¸���λ�õı�־ */

		while (a <= b)
		{
			lcd_draw_point(x_pos + a, y_pos - b, color);  /* 5 */
			lcd_draw_point(x_pos + b, y_pos - a, color);  /* 0 */
			lcd_draw_point(x_pos + b, y_pos + a, color);  /* 4 */
			lcd_draw_point(x_pos + a, y_pos + b, color);  /* 6 */
			lcd_draw_point(x_pos - a, y_pos + b, color);  /* 1 */
			lcd_draw_point(x_pos - b, y_pos + a, color);
			lcd_draw_point(x_pos - a, y_pos - b, color);  /* 2 */
			lcd_draw_point(x_pos - b, y_pos - a, color);  /* 7 */
			a++;

			/* ʹ��Bresenham�㷨��Բ */
			if (di < 0)
			{
				di += 4 * a + 6;
			}
			else
			{
				di += 10 + 4 * (a - b);
				b--;
			}
		}
	}
#endif


/* */

/* taken from ATK */
static void lcd_ex_ssd1963_reginit(void)
{
    lcd_wr_regno(0xE2); /* Set PLL with OSC = 10MHz (hardware),	Multiplier N = 35, 250MHz < VCO < 800MHz = OSC*(N+1), VCO = 300MHz */
    lcd_wr_data(0x1D);  /* parameter 1 */
    lcd_wr_data(0x02);  /* parameter 2 Divider M = 2, PLL = 300/(M+1) = 100MHz */
    lcd_wr_data(0x04);  /* parameter 3 Validate M and N values */
    delay_ms(1);		/* the only changes! */
    lcd_wr_regno(0xE0); /*  Start PLL command */
    lcd_wr_data(0x01);  /*  enable PLL */
    delay_ms(10);
    lcd_wr_regno(0xE0); /*  Start PLL command again */
    lcd_wr_data(0x03);  /*  now, use PLL output as system clock */
    delay_ms(12);
    lcd_wr_regno(0x01); /* soft reset */
    delay_ms(10);

    lcd_wr_regno(0xE6); /* set pixel frequency,33Mhz */
    lcd_wr_data(0x2F);
    lcd_wr_data(0xFF);
    lcd_wr_data(0xFF);

    lcd_wr_regno(0xB0); /* ����LCDģʽ */
    lcd_wr_data(0x20);  /* 24λģʽ */
    lcd_wr_data(0x00);  /* TFT ģʽ */

    lcd_wr_data((SSD_HOR_RESOLUTION - 1) >> 8); /* ����LCDˮƽ���� */
    lcd_wr_data(SSD_HOR_RESOLUTION - 1);
    lcd_wr_data((SSD_VER_RESOLUTION - 1) >> 8); /* ����LCD��ֱ���� */
    lcd_wr_data(SSD_VER_RESOLUTION - 1);
    lcd_wr_data(0x00);  /* RGB���� */

    lcd_wr_regno(0xB4); /* Set horizontal period */
    lcd_wr_data((SSD_HT - 1) >> 8);
    lcd_wr_data(SSD_HT - 1);
    lcd_wr_data(SSD_HPS >> 8);
    lcd_wr_data(SSD_HPS);
    lcd_wr_data(SSD_HOR_PULSE_WIDTH - 1);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_regno(0xB6); /* Set vertical perio */
    lcd_wr_data((SSD_VT - 1) >> 8);
    lcd_wr_data(SSD_VT - 1);
    lcd_wr_data(SSD_VPS >> 8);
    lcd_wr_data(SSD_VPS);
    lcd_wr_data(SSD_VER_FRONT_PORCH - 1);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xF0); /* ����SSD1963��CPU�ӿ�Ϊ16bit */
    lcd_wr_data(0x03);  /* 16-bit(565 format) data for 16bpp */

    lcd_wr_regno(0x29); /* ������ʾ */
    /* ����PWM���  ����ͨ��ռ�ձȿɵ� */
    lcd_wr_regno(0xD0); /* �����Զ���ƽ��DBC */
    lcd_wr_data(0x00);  /* disable */

    lcd_wr_regno(0xBE); /* ����PWM��� */
    lcd_wr_data(0x05);  /* 1����PWMƵ�� */
    lcd_wr_data(0xFE);  /* 2����PWMռ�ձ� */
    lcd_wr_data(0x01);  /* 3����C */
    lcd_wr_data(0x00);  /* 4����D */
    lcd_wr_data(0x00);  /* 5����E */
    lcd_wr_data(0x00);  /* 6����F */

    lcd_wr_regno(0xB8); /* ����GPIO���� */
    lcd_wr_data(0x03);  /* 2��IO�����ó���� */
    lcd_wr_data(0x01);  /* GPIOʹ��������IO���� */
    lcd_wr_regno(0xBA);
    lcd_wr_data(0X01);  /* GPIO[1:0]=01,����LCD���� */
}

/**
 * @brief       LCDд����
 * @param       data: Ҫд�������
 * @retval      ��
 */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;            /* ʹ��-O2�Ż���ʱ��,����������ʱ */
    LCD->LCD_RAM = data;
}

/**
 * @brief       LCDд�Ĵ������/��ַ����
 * @param       regno: �Ĵ������/��ַ
 * @retval      ��
 */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;          /* ʹ��-O2�Ż���ʱ��,����������ʱ */
    LCD->LCD_REG = regno;   /* д��Ҫд�ļĴ������ */
}

/**
 * @brief       LCDд�Ĵ���
 * @param       regno:�Ĵ������/��ַ
 * @param       data:Ҫд�������
 * @retval      ��
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD->LCD_REG = regno;   /* д��Ҫд�ļĴ������ */
    LCD->LCD_RAM = data;    /* д������ */
}

static rt_uint16_t _lcd_rd_data(void)
{
	volatile rt_uint16_t ram;
	for(rt_uint8_t i = 3; i >= 1;i--);	/*soft delay*/
	ram = LCD->LCD_RAM;
	return ram;
}

static void _lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = _lcd_dev.wramcmd;
}

static void _lcd_set_cursor(rt_uint16_t x_pos,rt_uint16_t y_pos)
{
	if(_lcd_dev.dir == 0)/* vertical */
	{
		x_pos = _lcd_dev.width - 1 -x_pos; /* ??? */
		lcd_wr_regno(_lcd_dev.setxcmd);
		lcd_wr_data(0);
		lcd_wr_data(0);
		lcd_wr_data(x_pos >> 8);
		lcd_wr_data(x_pos & 0XFF);
	}
	else
	{
		lcd_wr_regno(_lcd_dev.setxcmd);
		lcd_wr_data(x_pos >> 8);
		lcd_wr_data(x_pos & 0XFF);
		lcd_wr_data((_lcd_dev.width - 1) >> 8);
		lcd_wr_data((_lcd_dev.width - 1) & 0XFF);
		
	}
	lcd_wr_regno(_lcd_dev.setycmd);
	lcd_wr_data(y_pos >> 8);
	lcd_wr_data(y_pos & 0XFF);
	lcd_wr_data((_lcd_dev.height - 1) >> 8);
	lcd_wr_data((_lcd_dev.height - 1) & 0XFF);
	
}

static rt_uint32_t _lcd_pow(rt_uint8_t m, rt_uint8_t n)
{
	rt_uint32_t result = 1;
	
	while(n--)result *= m;
	
	return result;
}
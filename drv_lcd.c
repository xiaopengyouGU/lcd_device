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
	LCD_CS_GPIO_CLK_ENABLE();   /* LCD_CS脚时钟使能 */
    LCD_WR_GPIO_CLK_ENABLE();   /* LCD_WR脚时钟使能 */
    LCD_RD_GPIO_CLK_ENABLE();   /* LCD_RD脚时钟使能 */
    LCD_RS_GPIO_CLK_ENABLE();   /* LCD_RS脚时钟使能 */
    LCD_BL_GPIO_CLK_ENABLE();   /* LCD_BL脚时钟使能 */
    
	GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                /* 推挽复用 */
    gpio_init_struct.Pull = GPIO_PULLUP;                    /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          /* 高速 */
    HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_CS引脚 */

    gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
    HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_WR引脚 */

    gpio_init_struct.Pin = LCD_RD_GPIO_PIN;
    HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RD引脚 */

    gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
    HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);     /* 初始化LCD_RS引脚 */

    gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;            /* 推挽输出 */
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);     /* LCD_BL引脚模式设置(推挽输出) */
	
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
		delta_x = x_end - x_start;          /* 计算坐标增量 */
		delta_y = y_end - y_start;
		row = x_start;
		col = y_start;

		if (delta_x > 0)incx = 1;   /* 设置单步方向 */
		else if (delta_x == 0)incx = 0; /* 垂直线 */
		else
		{
			incx = -1;
			delta_x = -delta_x;
		}

		if (delta_y > 0)incy = 1;
		else if (delta_y == 0)incy = 0; /* 水平线 */
		else
		{
			incy = -1;
			delta_y = -delta_y;
		}

		if ( delta_x > delta_y)distance = delta_x;  /* 选取基本增量坐标轴 */
		else distance = delta_y;

		for (t = 0; t <= distance + 1; t++ )   /* 画线输出 */
		{
			lcd_draw_point(row, col, color); /* 画点 */
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
			tmp = (num / _lcd_pow(10, length - t - 1)) % 10;   /* 获取对应位的数字 */

			if (end_show == 0 && t < (length - 1))   /* 没有使能显示,且还有位要显示 */
			{
				if (tmp == 0)
				{
					lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, ' ',color);/* 显示空格,占位 */
					continue;   /* 继续下个一位 */
				}
				else
				{
					end_show = 1; /* 使能显示 */
				}

			}

			lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, tmp + '0', color); /* 显示字符 */
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
	
		for (t = 0; t < len; t++)   /* 按总显示位数循环 */
		{
			temp = (num / _lcd_pow(16, len - t - 1)) % 16;    /* 获取对应位的数字 */

			if (enshow == 0 && t < (len - 1))   /* 没有使能显示,且还有位要显示 */
			{
				if (temp == 0)
				{
					lcd_show_char(x_pos + (FONT_SIZE / 2)*t, y_pos, ' ',color);  /* 用空格占位 */					
					continue;
				}
				else
				{
					enshow = 1; /* 使能显示 */
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

    /* 横屏时，对1963不改变扫描方向！其他IC改变扫描方向！竖屏时1963改变方向，其他IC不改变扫描方向 */
    if (_lcd_dev.dir == 0)
    {
        switch (dir)   /* 方向转换 */
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


	/* 根据扫描方式 设置 0X36/0X3600 寄存器 bit 5,6,7 位的值 */
	switch (dir)
	{
		case L2R_U2D:/* 从左到右,从上到下 */
			regval |= (0 << 7) | (0 << 6) | (0 << 5);
			break;

		case L2R_D2U:/* 从左到右,从下到上 */
			regval |= (1 << 7) | (0 << 6) | (0 << 5);
			break;

		case R2L_U2D:/* 从右到左,从上到下 */
			regval |= (0 << 7) | (1 << 6) | (0 << 5);
			break;

		case R2L_D2U:/* 从右到左,从下到上 */
			regval |= (1 << 7) | (1 << 6) | (0 << 5);
			break;

		case U2D_L2R:/* 从上到下,从左到右 */
			regval |= (0 << 7) | (0 << 6) | (1 << 5);
			break;

		case U2D_R2L:/* 从上到下,从右到左 */
			regval |= (0 << 7) | (1 << 6) | (1 << 5);
			break;

		case D2U_L2R:/* 从下到上,从左到右 */
			regval |= (1 << 7) | (0 << 6) | (1 << 5);
			break;

		case D2U_R2L:/* 从下到上,从右到左 */
			regval |= (1 << 7) | (1 << 6) | (1 << 5);
			break;
	}

	dirreg = 0X36;  /* 对绝大部分驱动IC, 由0X36寄存器控制 */

	lcd_write_reg(dirreg, regval);

	/* 设置显示区域(开窗)大小 */
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
		lcd_wr_regno(0xBE);         /* 配置PWM输出 */
		lcd_wr_data(0x05);          /* 1设置PWM频率 */
		lcd_wr_data(pwm * 2.55);    /* 2设置PWM占空比 */
		lcd_wr_data(0x01);          /* 3设置C */
		lcd_wr_data(0xFF);          /* 4设置D */
		lcd_wr_data(0x00);          /* 5设置E */
		lcd_wr_data(0x00);          /* 6设置F */
	}
	
	void lcd_clear(rt_uint16_t color)
	{
		rt_uint32_t index = 0;
		rt_uint32_t totalpoint = _lcd_dev.width;
		totalpoint *= _lcd_dev.height;    /* 得到总点数 */
		_lcd_set_cursor(0x00, 0x0000);   /* 设置光标位置 */
		_lcd_write_ram_prepare();        /* 开始写入GRAM */

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

   
   if (_lcd_dev.dir != 1)     /* 1963竖屏特殊处理 */
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
    else    /* 9341/5310/7789/1963横屏 等 设置窗口 */
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
		di = 3 - (radius << 1);       /* 判断下个点位置的标志 */

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

			/* 使用Bresenham算法画圆 */
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

    lcd_wr_regno(0xB0); /* 设置LCD模式 */
    lcd_wr_data(0x20);  /* 24位模式 */
    lcd_wr_data(0x00);  /* TFT 模式 */

    lcd_wr_data((SSD_HOR_RESOLUTION - 1) >> 8); /* 设置LCD水平像素 */
    lcd_wr_data(SSD_HOR_RESOLUTION - 1);
    lcd_wr_data((SSD_VER_RESOLUTION - 1) >> 8); /* 设置LCD垂直像素 */
    lcd_wr_data(SSD_VER_RESOLUTION - 1);
    lcd_wr_data(0x00);  /* RGB序列 */

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

    lcd_wr_regno(0xF0); /* 设置SSD1963与CPU接口为16bit */
    lcd_wr_data(0x03);  /* 16-bit(565 format) data for 16bpp */

    lcd_wr_regno(0x29); /* 开启显示 */
    /* 设置PWM输出  背光通过占空比可调 */
    lcd_wr_regno(0xD0); /* 设置自动白平衡DBC */
    lcd_wr_data(0x00);  /* disable */

    lcd_wr_regno(0xBE); /* 配置PWM输出 */
    lcd_wr_data(0x05);  /* 1设置PWM频率 */
    lcd_wr_data(0xFE);  /* 2设置PWM占空比 */
    lcd_wr_data(0x01);  /* 3设置C */
    lcd_wr_data(0x00);  /* 4设置D */
    lcd_wr_data(0x00);  /* 5设置E */
    lcd_wr_data(0x00);  /* 6设置F */

    lcd_wr_regno(0xB8); /* 设置GPIO配置 */
    lcd_wr_data(0x03);  /* 2个IO口设置成输出 */
    lcd_wr_data(0x01);  /* GPIO使用正常的IO功能 */
    lcd_wr_regno(0xBA);
    lcd_wr_data(0X01);  /* GPIO[1:0]=01,控制LCD方向 */
}

/**
 * @brief       LCD写数据
 * @param       data: 要写入的数据
 * @retval      无
 */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;            /* 使用-O2优化的时候,必须插入的延时 */
    LCD->LCD_RAM = data;
}

/**
 * @brief       LCD写寄存器编号/地址函数
 * @param       regno: 寄存器编号/地址
 * @retval      无
 */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;          /* 使用-O2优化的时候,必须插入的延时 */
    LCD->LCD_REG = regno;   /* 写入要写的寄存器序号 */
}

/**
 * @brief       LCD写寄存器
 * @param       regno:寄存器编号/地址
 * @param       data:要写入的数据
 * @retval      无
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD->LCD_REG = regno;   /* 写入要写的寄存器序号 */
    LCD->LCD_RAM = data;    /* 写入数据 */
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
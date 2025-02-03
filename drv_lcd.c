#include "drv_lcd.h"
#include "drv_lcd_font.h"
#include "stdlib.h"

struct lcd_device _lcd_dev = {
	LCD_WIDTH,
	LCD_HEIGHT,
	0,	/* vertical */
	0X2C,	/* Write GRAM*/
	0X2A,	/* Set x pos cmd */
	0X2B,	/* Set y pos cmd */
};

rt_uint16_t g_back_color = WHITE;	/* background color */
rt_uint16_t g_pan_color = RED;		/* pan color */
/* functions declaration */
static void MX_FSMC_Init(void);
static void lcd_wr_data(volatile uint16_t data);            /* LCD写数据 */
static void lcd_wr_regno(volatile uint16_t regno);          /* LCD写寄存器编号/地址 */
static void lcd_write_reg(uint16_t regno, uint16_t data);   /* LCD写寄存器的值 */

static void _lcd_set_cursor(rt_uint16_t x_pos,rt_uint16_t y_pos);
static rt_uint16_t _lcd_rd_data(void);
static void _lcd_write_ram_prepare(void);
static void _lcd_ic_init(void);
static rt_uint32_t _lcd_pow(rt_uint8_t m, rt_uint8_t n);

void stm32_hw_lcd_init(void)
{
/*******************************************************************************************/
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
	MX_FSMC_Init();											/* init FSMC,taken from STM32cubeMX !*/
	
/*******************************************************************************************/
	_lcd_ic_init();			/* init the IC */
	
	lcd_backlight_set(100);		/* the lightest */
	lcd_scan_dir(DFT_SCAN_DIR);	/* default scan direction */
	LCD_BL(1);						/* light lcd */
	lcd_clear(WHITE);				/* white background */
	
}

///* functions need to be inplemented! */
//void lcd_control(int cmd, void *args)
//{
//	
//}
//void lcd_open(void);
void lcd_init(void)
{
	stm32_hw_lcd_init();
}
//void lcd_close(void);

#if defined LCD_BASIC_FUNCTIONS	
	void lcd_draw_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t color)
	{
		_lcd_set_cursor(x_pos,y_pos);
		_lcd_write_ram_prepare();
		LCD->LCD_RAM = color;
	}
	
	void lcd_read_point(rt_uint16_t x_pos,rt_uint16_t y_pos,rt_uint16_t *color)
	{
		rt_uint16_t r = 0, g = 0, b = 0;
		
		if(x_pos >= _lcd_dev.width || y_pos >=_lcd_dev.height) return;
		
		_lcd_set_cursor(x_pos,y_pos); //lcd set cursor
		lcd_wr_regno(0X2E); /* send read GRAM CMD */
		r = _lcd_rd_data();	/* dummy read */
		r = _lcd_rd_data();	/* true read */
		b = _lcd_rd_data();
		g = r&0XFF;
		g <<= 8;
		*color = (((r >> 11) << 11) | ((g >> 10) << 5)|(b >> 11));
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
		rt_uint8_t i = 0;
		while((*ptr)!= '\0')
		{
			lcd_show_char(x_pos+(FONT_SIZE / 2)*i,y_pos,*ptr,color);
			ptr++;
			i++;
		}
	}
#endif
#ifdef LCD_ADVANCED_FUNCTIONS	
	void lcd_color_fill(rt_uint16_t x_start,rt_uint16_t y_start,rt_uint16_t length, rt_uint16_t width,rt_uint16_t color)
	{
		rt_uint16_t i,j;
		
		for(i = 0;i < width ; i++)
		{
			_lcd_set_cursor(x_start,y_start+i);
			_lcd_write_ram_prepare();
			for(j = 0; j < length;j++)
			{
				LCD->LCD_RAM = color;
			}
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

	/* horizontal change dir*/
	if (_lcd_dev.dir == 1)
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
	
	if(regval &0X20)
	{
		if(_lcd_dev.width < _lcd_dev.height)
		{
			rt_uint16_t tmp = _lcd_dev.width;
			_lcd_dev.width = _lcd_dev.height;
			_lcd_dev.height = tmp;
		}
	}
	else
	{
		if(_lcd_dev.width > _lcd_dev.height)
		{
			rt_uint16_t tmp = _lcd_dev.width;
			_lcd_dev.width = _lcd_dev.height;
			_lcd_dev.height = tmp;
		}
	}

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
	  /* 9341/5310/7789/1963横屏 等 设置窗口 */
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

/*********************************************************************************************/
/* IC initialization and some inner functions */
void lcd_ex_nt35310_reginit();
static void _lcd_ic_init(void)
{
	lcd_ex_nt35310_reginit();
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
static void lcd_write_reg(uint16_t regno, uint16_t data)
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

/* main part of draw point function*/
static void _lcd_set_cursor(rt_uint16_t x_pos,rt_uint16_t y_pos)
{
	lcd_wr_regno(_lcd_dev.setxcmd);
	lcd_wr_data(x_pos >> 8);
	lcd_wr_data(x_pos & 0XFF);
	lcd_wr_regno(_lcd_dev.setycmd);
	lcd_wr_data(y_pos >> 8);
	lcd_wr_data(y_pos & 0XFF);
}

static rt_uint32_t _lcd_pow(rt_uint8_t m, rt_uint8_t n)
{
	rt_uint32_t result = 1;
	
	while(n--)result *= m;
	
	return result;
}

/*lcd device supports finsh */
#ifdef RT_USING_FINSH
#if defined LCD_FUNCTIONS_TEST			/* test lcd main functions! */
static void lcd(int argc, char **argv)
{
	//int result = RT_EOK;
	//static rt_lcd_device_t lcd_device = RT_NULL;
	rt_uint16_t x_pos,y_pos,color;
	
	if(argc > 1)
	{
		if(!rt_strcmp(argv[1],"draw_point"))
		{
			if(argc == 5)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				color = atoi(argv[4]);
				lcd_draw_point(x_pos,y_pos,color);
				rt_kprintf("lcd draw point in (%d, %d), color: %x \n",x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd draw point in (x, y), color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"read_point"))
		{
			if(argc == 4)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				lcd_read_point(x_pos,y_pos,&color);
				rt_kprintf("lcd read point in (%d, %d), color: %x \n",x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd read point in (x, y), color: \n");
			}
		}
		else if(!rt_strcmp(argv[1],"show_char"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				char ch = *(argv[4]);
				color = atoi(argv[5]);
				lcd_show_char(x_pos,y_pos,ch,color);
				rt_kprintf("lcd show char: %c in (%d, %d), color: %x \n",ch,x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd show char:  in (x, y), color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"draw_line"))
		{
			if(argc == 7)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t x_end = atoi(argv[4]),y_end = atoi(argv[5]);
				color = atoi(argv[6]);
				lcd_draw_line(x_pos,y_pos,x_end,y_end,color);
				rt_kprintf("lcd draw line from (%d, %d) to (%d, %d), color: %x\n",x_pos,y_pos,x_end,y_end,color);
			}
			else
			{
				rt_kprintf("lcd draw line from (x0, y0) to (x1, y1), color: \n");
			}
		}
		else if(!rt_strcmp(argv[1],"clear"))
		{
			rt_kprintf("lcd clear screen\n");
			lcd_clear(g_back_color);
		}
		else if(!rt_strcmp(argv[1],"show_num"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint32_t num = atoi(argv[4]);
				color = atoi(argv[5]);
				lcd_show_num(x_pos,y_pos,num,color);
				rt_kprintf("lcd show num: %d in (%d, %d), color: %x \n",num,x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd show num:  in (x, y), color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"show_xnum"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint32_t num = atoi(argv[4]);
				color = atoi(argv[5]);
				lcd_show_xnum(x_pos,y_pos,num,color);
				rt_kprintf("lcd show xnum: %d in (%d, %d), color: %x \n",num,x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd show xnum:  in (x, y), color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"show_string"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				color = atoi(argv[5]);
				lcd_show_string(x_pos,y_pos,argv[4],color);
				rt_kprintf("lcd show string: %s in (%d, %d), color: %x \n",argv[4],x_pos,y_pos,color);
			}
			else
			{
				rt_kprintf("lcd show string:  in (x, y), color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"backlight"))
		{
			if(argc == 3)
			{
				rt_uint8_t light = atoi(argv[2]);
				if(light > 100) light = 100;
				lcd_backlight_set(light);
				rt_kprintf("lcd set backlight : %d\n",light);
			}
			else
			{
				rt_kprintf("lcd set backlight : \n");
			}
		}
		else if(!rt_strcmp(argv[1],"draw_rect"))
		{
			if(argc == 7)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t length = atoi(argv[4]),width = atoi(argv[5]);
				color = atoi(argv[6]);
				lcd_draw_rect(x_pos,y_pos,length,width,color);
				rt_kprintf("lcd draw rect ,start point(%d, %d)  length: %d, width: %d, color: %x\n",x_pos,y_pos,length,width,color);
			}
			else
			{
				rt_kprintf("lcd draw rect ,start point(x, y)  length: , width: , color: \n");
			}
		}
		else if(!rt_strcmp(argv[1],"draw_circle"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t radius = atoi(argv[4]);
				color = atoi(argv[5]);
				lcd_draw_circle(x_pos,y_pos,radius,color);
				rt_kprintf("lcd draw circle: center in (%d, %d), radius: %d, color: %x \n",x_pos,y_pos,radius,color);
			}
			else
			{
				rt_kprintf("lcd draw circle: center in (x, y), radius: , color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"set_window"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t length = atoi(argv[4]),width = atoi(argv[5]);
				lcd_set_window(x_pos,y_pos,length,width);
				rt_kprintf("lcd set window,start point(%d, %d)  length: %d, width: %d \n",x_pos,y_pos,length,width);
			}
			else
			{
				rt_kprintf("lcd set window,start point(x, y)  length: , width:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"draw_vline"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t length = atoi(argv[4]);
				color = atoi(argv[5]);
				lcd_draw_ver_line(x_pos,y_pos,length,color);
				rt_kprintf("lcd draw vertical line, start point(%d, %d)  length: %d, color: %x \n",x_pos,y_pos,length,color);
			}
			else
			{
				rt_kprintf("lcd draw vertical line, start point(x, y)  length: , color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"draw_hline"))
		{
			if(argc == 6)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t length = atoi(argv[4]);
				color = atoi(argv[5]);
				lcd_draw_hor_line(x_pos,y_pos,length,color);
				rt_kprintf("lcd draw horizontal line, start point(%d, %d)  length: %d, color: %x \n",x_pos,y_pos,length,color);
			}
			else
			{
				rt_kprintf("lcd draw vertical line, start point(x, y)  length: , color:  \n");
			}
		}
		else if(!rt_strcmp(argv[1],"fill"))
		{
			if(argc == 7)
			{
				x_pos = atoi(argv[2]);
				y_pos = atoi(argv[3]);
				rt_uint16_t length = atoi(argv[4]),width = atoi(argv[5]);
				color = atoi(argv[6]);
				lcd_draw_rect(x_pos,y_pos,length,width,color);
				rt_kprintf("lcd color fill ,start point(%d, %d)  length: %d, width: %d, color: %x\n",x_pos,y_pos,length,width,color);
			}
			else
			{
				rt_kprintf("lcd color fill ,start point(x, y)  length: , width: , color: \n");
			}
		}
		else if(!rt_strcmp(argv[1],"on"))
		{
			lcd_display_on();
			rt_kprintf("lcd display on\n");
		}
		else if(!rt_strcmp(argv[1],"off"))
		{
			lcd_display_off();
			rt_kprintf("lcd display off\n");
		}
		else if(!rt_strcmp(argv[1],"scan_dir"))
		{
			if(argc == 3)
			{
				x_pos = atoi(argv[2]);
				lcd_scan_dir(x_pos);
				rt_kprintf("lcd scan dir:%d\n",x_pos);
			}
			else
			{
				rt_kprintf("lcd scan dir: \n");
			}
		}
		else if(!rt_strcmp(argv[1],"help"))
		{
			if(argc == 3)
			{
				if(!rt_strcmp(argv[2],"func"))
				{
					rt_kprintf("lcd device support functions and call formats: \n");
#ifdef LCD_BASIC_FUNCTIONS			
					rt_kprintf("*********** basic functions ***********\n");
					rt_kprintf("lcd draw_point x y color \n");
					rt_kprintf("lcd read_point x y \n");
					rt_kprintf("lcd draw_line x y length width color \n");
					rt_kprintf("lcd show_num x y num color \n");
					rt_kprintf("lcd show_xnum x y num color \n");
					rt_kprintf("lcd show_char x y ch color \n");
					rt_kprintf("lcd show_string x y str color \n");
					rt_kprintf("*********** basic functions ***********\n\n");
#endif
#ifdef LCD_ADVANCED_FUNCTIONS
					rt_kprintf("*********** advanced functions ***********\n");
					rt_kprintf("lcd draw_rect x y length width color \n");
					rt_kprintf("lcd draw_circle x y radius color \n");
					rt_kprintf("lcd draw_vline x y length color \n");
					rt_kprintf("lcd draw_hline x y length color \n");
					rt_kprintf("lcd fill x y length width color \n");
					rt_kprintf("*********** advanced functions ***********\n\n");
#endif
					rt_kprintf("lcd on \n");
					rt_kprintf("lcd off \n");
					rt_kprintf("lcd backlight light \n");
					rt_kprintf("lcd clear \n");
					rt_kprintf("lcd scan_dir dir \n");
					rt_kprintf("lcd set_window x y length width color \n");
					
				}
				else if(!rt_strcmp(argv[2],"color"))
				{
					rt_kprintf("lcd support colors and call numbers: \n");
					rt_kprintf("*********** common colors *********** \n");
					rt_kprintf("WHITE = %d \n",WHITE);
					rt_kprintf("BLACK = %d \n",BLACK);
					rt_kprintf("RED = %d \n",RED);
					rt_kprintf("GREEN = %d \n",GREEN);
					rt_kprintf("BLUE = %d \n",BLUE);
					rt_kprintf("MAGENTA = %d \n",MAGENTA);
					rt_kprintf("YELLOW = %d \n",YELLOW);
					rt_kprintf("CYAN = %d \n",CYAN);
					rt_kprintf("*********** common colors *********** \n\n");
					rt_kprintf("*********** uncommon colors *********** \n");
					rt_kprintf("BROWN = %d \n",BROWN);
					rt_kprintf("BRRED = %d \n",BRRED);
					rt_kprintf("GRAY = %d \n",GRAY);
					rt_kprintf("DARKBLUE = %d \n",DARKBLUE);
					rt_kprintf("GRAYBLUE = %d \n",GRAYBLUE);
					rt_kprintf("LIGHTBLUE = %d \n",LIGHTBLUE);
					rt_kprintf("*********** uncommon colors *********** \n\n");
					}
				else if(!rt_strcmp(argv[2],"dir"))
				{
					rt_kprintf("lcd support directions and call numbers: \n");
					rt_kprintf("L2R_U2D  = 0 \n");
					rt_kprintf("L2R_D2U  = 1 \n");
					rt_kprintf("R2L_U2D  = 2 \n");
					rt_kprintf("R2L_D2U  = 3 \n");
					rt_kprintf("U2D_L2R  = 4 \n");
					rt_kprintf("U2D_R2L  = 5 \n");
					rt_kprintf("D2U_L2R  = 6 \n");
					rt_kprintf("D2U_R2L  = 7 \n");
				}
			}
			else
			{
				rt_kprintf("you must input 'lcd init' before you start this journey!!!\n");
				rt_kprintf("lcd help call format: \n");
				rt_kprintf("lcd help func \n");
				rt_kprintf("lcd help color \n");
				rt_kprintf("lcd help dir \n");
			}
		}
		else if(!rt_strcmp(argv[1],"init"))
		{
			stm32_hw_lcd_init();	/* init the device */
			rt_kprintf("lcd device is inited successfully!!! \n");
		}
	}
	else
	{
		rt_kprintf("This lcd device supports finsh,you can call like this 'lcd help' to get help: \n");
		rt_kprintf("you must input 'lcd init' to init lcd device before you start this journey!!!\n");
		rt_kprintf("many functions can be called, see details by inputting 'lcd help func'\n");
		rt_kprintf("you may be confused with  what 'color' means, see details by inputting ' lcd help color' \n");
		rt_kprintf("by inputting 'lcd help dir' to see supported directions! \n");
		rt_kprintf("Hope you enjoy it!!! \n\n");
	}		
	
}
MSH_CMD_EXPORT(lcd, lcd functions!);
#endif
#endif

void lcd_ex_nt35310_reginit(void)
{
    lcd_wr_regno(0xED);
    lcd_wr_data(0x01);
    lcd_wr_data(0xFE);

    lcd_wr_regno(0xEE);
    lcd_wr_data(0xDE);
    lcd_wr_data(0x21);

    lcd_wr_regno(0xF1);
    lcd_wr_data(0x01);
    lcd_wr_regno(0xDF);
    lcd_wr_data(0x10);

    /* VCOMvoltage */
    lcd_wr_regno(0xC4);
    lcd_wr_data(0x8F);  /* 5f */

    lcd_wr_regno(0xC6);
    lcd_wr_data(0x00);
    lcd_wr_data(0xE2);
    lcd_wr_data(0xE2);
    lcd_wr_data(0xE2);
    lcd_wr_regno(0xBF);
    lcd_wr_data(0xAA);

    lcd_wr_regno(0xB0);
    lcd_wr_data(0x0D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x0D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x11);
    lcd_wr_data(0x00);
    lcd_wr_data(0x19);
    lcd_wr_data(0x00);
    lcd_wr_data(0x21);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x5D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x5D);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB1);
    lcd_wr_data(0x80);
    lcd_wr_data(0x00);
    lcd_wr_data(0x8B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x96);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB2);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x02);
    lcd_wr_data(0x00);
    lcd_wr_data(0x03);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB3);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB4);
    lcd_wr_data(0x8B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x96);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA1);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB5);
    lcd_wr_data(0x02);
    lcd_wr_data(0x00);
    lcd_wr_data(0x03);
    lcd_wr_data(0x00);
    lcd_wr_data(0x04);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB6);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB7);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3F);
    lcd_wr_data(0x00);
    lcd_wr_data(0x5E);
    lcd_wr_data(0x00);
    lcd_wr_data(0x64);
    lcd_wr_data(0x00);
    lcd_wr_data(0x8C);
    lcd_wr_data(0x00);
    lcd_wr_data(0xAC);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDC);
    lcd_wr_data(0x00);
    lcd_wr_data(0x70);
    lcd_wr_data(0x00);
    lcd_wr_data(0x90);
    lcd_wr_data(0x00);
    lcd_wr_data(0xEB);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDC);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xB8);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xBA);
    lcd_wr_data(0x24);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC1);
    lcd_wr_data(0x20);
    lcd_wr_data(0x00);
    lcd_wr_data(0x54);
    lcd_wr_data(0x00);
    lcd_wr_data(0xFF);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC2);
    lcd_wr_data(0x0A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x04);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC3);
    lcd_wr_data(0x3C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x39);
    lcd_wr_data(0x00);
    lcd_wr_data(0x37);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x36);
    lcd_wr_data(0x00);
    lcd_wr_data(0x32);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2F);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x29);
    lcd_wr_data(0x00);
    lcd_wr_data(0x26);
    lcd_wr_data(0x00);
    lcd_wr_data(0x24);
    lcd_wr_data(0x00);
    lcd_wr_data(0x24);
    lcd_wr_data(0x00);
    lcd_wr_data(0x23);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x36);
    lcd_wr_data(0x00);
    lcd_wr_data(0x32);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2F);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x29);
    lcd_wr_data(0x00);
    lcd_wr_data(0x26);
    lcd_wr_data(0x00);
    lcd_wr_data(0x24);
    lcd_wr_data(0x00);
    lcd_wr_data(0x24);
    lcd_wr_data(0x00);
    lcd_wr_data(0x23);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC4);
    lcd_wr_data(0x62);
    lcd_wr_data(0x00);
    lcd_wr_data(0x05);
    lcd_wr_data(0x00);
    lcd_wr_data(0x84);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF0);
    lcd_wr_data(0x00);
    lcd_wr_data(0x18);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA4);
    lcd_wr_data(0x00);
    lcd_wr_data(0x18);
    lcd_wr_data(0x00);
    lcd_wr_data(0x50);
    lcd_wr_data(0x00);
    lcd_wr_data(0x0C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x17);
    lcd_wr_data(0x00);
    lcd_wr_data(0x95);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);
    lcd_wr_data(0xE6);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC5);
    lcd_wr_data(0x32);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x65);
    lcd_wr_data(0x00);
    lcd_wr_data(0x76);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC6);
    lcd_wr_data(0x20);
    lcd_wr_data(0x00);
    lcd_wr_data(0x17);
    lcd_wr_data(0x00);
    lcd_wr_data(0x01);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC7);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC8);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xC9);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE0);
    lcd_wr_data(0x16);
    lcd_wr_data(0x00);
    lcd_wr_data(0x1C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x21);
    lcd_wr_data(0x00);
    lcd_wr_data(0x36);
    lcd_wr_data(0x00);
    lcd_wr_data(0x46);
    lcd_wr_data(0x00);
    lcd_wr_data(0x52);
    lcd_wr_data(0x00);
    lcd_wr_data(0x64);
    lcd_wr_data(0x00);
    lcd_wr_data(0x7A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x8B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA8);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB9);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC4);
    lcd_wr_data(0x00);
    lcd_wr_data(0xCA);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD9);
    lcd_wr_data(0x00);
    lcd_wr_data(0xE0);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE1);
    lcd_wr_data(0x16);
    lcd_wr_data(0x00);
    lcd_wr_data(0x1C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x22);
    lcd_wr_data(0x00);
    lcd_wr_data(0x36);
    lcd_wr_data(0x00);
    lcd_wr_data(0x45);
    lcd_wr_data(0x00);
    lcd_wr_data(0x52);
    lcd_wr_data(0x00);
    lcd_wr_data(0x64);
    lcd_wr_data(0x00);
    lcd_wr_data(0x7A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x8B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA8);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB9);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC4);
    lcd_wr_data(0x00);
    lcd_wr_data(0xCA);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD8);
    lcd_wr_data(0x00);
    lcd_wr_data(0xE0);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE2);
    lcd_wr_data(0x05);
    lcd_wr_data(0x00);
    lcd_wr_data(0x0B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x1B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x34);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x4F);
    lcd_wr_data(0x00);
    lcd_wr_data(0x61);
    lcd_wr_data(0x00);
    lcd_wr_data(0x79);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);
    lcd_wr_data(0x97);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA6);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB7);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC7);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD1);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD6);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDD);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);
    lcd_wr_regno(0xE3);
    lcd_wr_data(0x05);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA);
    lcd_wr_data(0x00);
    lcd_wr_data(0x1C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x33);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x50);
    lcd_wr_data(0x00);
    lcd_wr_data(0x62);
    lcd_wr_data(0x00);
    lcd_wr_data(0x78);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);
    lcd_wr_data(0x97);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA6);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB7);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC7);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD1);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD5);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDD);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE4);
    lcd_wr_data(0x01);
    lcd_wr_data(0x00);
    lcd_wr_data(0x01);
    lcd_wr_data(0x00);
    lcd_wr_data(0x02);
    lcd_wr_data(0x00);
    lcd_wr_data(0x2A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x4B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x5D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x74);
    lcd_wr_data(0x00);
    lcd_wr_data(0x84);
    lcd_wr_data(0x00);
    lcd_wr_data(0x93);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB3);
    lcd_wr_data(0x00);
    lcd_wr_data(0xBE);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC4);
    lcd_wr_data(0x00);
    lcd_wr_data(0xCD);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD3);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDD);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);
    lcd_wr_regno(0xE5);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x02);
    lcd_wr_data(0x00);
    lcd_wr_data(0x29);
    lcd_wr_data(0x00);
    lcd_wr_data(0x3C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x4B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x5D);
    lcd_wr_data(0x00);
    lcd_wr_data(0x74);
    lcd_wr_data(0x00);
    lcd_wr_data(0x84);
    lcd_wr_data(0x00);
    lcd_wr_data(0x93);
    lcd_wr_data(0x00);
    lcd_wr_data(0xA2);
    lcd_wr_data(0x00);
    lcd_wr_data(0xB3);
    lcd_wr_data(0x00);
    lcd_wr_data(0xBE);
    lcd_wr_data(0x00);
    lcd_wr_data(0xC4);
    lcd_wr_data(0x00);
    lcd_wr_data(0xCD);
    lcd_wr_data(0x00);
    lcd_wr_data(0xD3);
    lcd_wr_data(0x00);
    lcd_wr_data(0xDC);
    lcd_wr_data(0x00);
    lcd_wr_data(0xF3);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE6);
    lcd_wr_data(0x11);
    lcd_wr_data(0x00);
    lcd_wr_data(0x34);
    lcd_wr_data(0x00);
    lcd_wr_data(0x56);
    lcd_wr_data(0x00);
    lcd_wr_data(0x76);
    lcd_wr_data(0x00);
    lcd_wr_data(0x77);
    lcd_wr_data(0x00);
    lcd_wr_data(0x66);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0xBB);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0x66);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);
    lcd_wr_data(0x45);
    lcd_wr_data(0x00);
    lcd_wr_data(0x43);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE7);
    lcd_wr_data(0x32);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);
    lcd_wr_data(0x76);
    lcd_wr_data(0x00);
    lcd_wr_data(0x66);
    lcd_wr_data(0x00);
    lcd_wr_data(0x67);
    lcd_wr_data(0x00);
    lcd_wr_data(0x67);
    lcd_wr_data(0x00);
    lcd_wr_data(0x87);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0xBB);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0x77);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x56);
    lcd_wr_data(0x00);
    lcd_wr_data(0x23);
    lcd_wr_data(0x00);
    lcd_wr_data(0x33);
    lcd_wr_data(0x00);
    lcd_wr_data(0x45);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE8);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0x87);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);
    lcd_wr_data(0x77);
    lcd_wr_data(0x00);
    lcd_wr_data(0x66);
    lcd_wr_data(0x00);
    lcd_wr_data(0x88);
    lcd_wr_data(0x00);
    lcd_wr_data(0xAA);
    lcd_wr_data(0x00);
    lcd_wr_data(0xBB);
    lcd_wr_data(0x00);
    lcd_wr_data(0x99);
    lcd_wr_data(0x00);
    lcd_wr_data(0x66);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x44);
    lcd_wr_data(0x00);
    lcd_wr_data(0x55);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xE9);
    lcd_wr_data(0xAA);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0x00);
    lcd_wr_data(0xAA);

    lcd_wr_regno(0xCF);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xF0);
    lcd_wr_data(0x00);
    lcd_wr_data(0x50);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xF3);
    lcd_wr_data(0x00);

    lcd_wr_regno(0xF9);
    lcd_wr_data(0x06);
    lcd_wr_data(0x10);
    lcd_wr_data(0x29);
    lcd_wr_data(0x00);

    lcd_wr_regno(0x3A);
    lcd_wr_data(0x55);  /* 66 */

    lcd_wr_regno(0x11);
    delay_ms(100);
    lcd_wr_regno(0x29);
    lcd_wr_regno(0x35);
    lcd_wr_data(0x00);

    lcd_wr_regno(0x51);
    lcd_wr_data(0xFF);
    lcd_wr_regno(0x53);
    lcd_wr_data(0x2C);
    lcd_wr_regno(0x55);
    lcd_wr_data(0x82);
    lcd_wr_regno(0x2c);
}

/***************************************************************************************/
/* FSMC init functions */
SRAM_HandleTypeDef hsram1;
/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};
  FSMC_NORSRAM_TimingTypeDef ExtTiming = {0};

  /* USER CODE BEGIN FSMC_Init 1 */

  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK4;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 24;
  Timing.BusTurnAroundDuration = 0;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  /* ExtTiming */
  ExtTiming.AddressSetupTime = 8;
  ExtTiming.AddressHoldTime = 15;
  ExtTiming.DataSetupTime = 8;
  ExtTiming.BusTurnAroundDuration = 0;
  ExtTiming.CLKDivision = 16;
  ExtTiming.DataLatency = 17;
  ExtTiming.AccessMode = FSMC_ACCESS_MODE_A;

  HAL_SRAM_Init(&hsram1, &Timing, &ExtTiming);
}
/***********************************************************************************************/
#include "DL_ST7735.h"

#define DELAY 0x80

static const uint8_t
  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      ST7735_ROTATION,        //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

#if (defined(ST7735_IS_128X128) || defined(ST7735_IS_160X128))
  init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127
#endif // ST7735_IS_128X128

#ifdef ST7735_IS_160X80
  init_cmds2[] = {            // Init for 7735S, part 2 (160x80 display)
    3,                        //  3 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x4F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F ,            //     XEND = 159
    ST7735_INVON, 0 },        //  3: Invert colors
#endif

  init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Gamma Adjustments (pos. polarity), 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Gamma Adjustments (neg. polarity), 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay

static uint8_t DL_intToStr(char* str, int32_t num)
{
    uint8_t start = 0;
    uint8_t end = 0;

    if(num < 0)
    {
        str[0] = '-';

        num = -num;

        start = 1;
        end = 1;
    }

    do
    {
        str[end] = (char)(num % 10) + '0';
        end++;
        num /= 10;
    }
    while(num != 0);

    uint8_t len = end;

    str[end] = '\0';

    end--;

    for(; start < end; start++, end--)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
    }

    return len;
}

static uint8_t DL_floatToStr(char* str, float num, uint8_t precision)
{
    if(precision > 6) precision = 6;

    float addition = 0.51;

    for(uint8_t i = 0; i < precision; i++)
        addition /= 10;

    num += addition;

    uint8_t len = 0;
    int32_t whole_part = 0;

    if(num < 0)
    {
        str[0] = '-';
        num = -num;
        whole_part = (int32_t)num;
        len = DL_intToStr(str + 1, whole_part) + 1;
    }
    else
    {
        whole_part = (int32_t)num;
        len = DL_intToStr(str, whole_part);
    }

    if(precision > 0)
    {
        str[len] = '.';
        len++;
    }
    else
    {
        str[len] = '\0';
        return len;
    }

    float temp = num - whole_part;

    for(uint8_t i = 0; i < precision; i++, len++)
    {
        temp *= 10;
        int32_t frac_part = (int32_t)temp;
        DL_intToStr(str + len, frac_part);
        temp -= frac_part;
    }

    str[len] = '\0';

    return len;
}

static void ST7735_Select()
{
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET);
}

void ST7735_Unselect()
{
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET);
}

static void ST7735_Reset()
{
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
}

static void ST7735_WriteCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);

    /* probably faster
    SPI5->CR1 |= SPI_CR1_SPE;

    while (!(SPI5->SR & SPI_SR_TXE));

    SPI5->DR = cmd;

    while (SPI5->SR & SPI_SR_BSY);
    */
}

static void ST7735_WriteData(uint8_t* buff, size_t buff_size)
{
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);

    /* probably faster
    for(uint16_t i = 0; i < buff_size; i++)
    {
        SPI5->CR1 |= SPI_CR1_SPE;

        while (!(SPI5->SR & SPI_SR_TXE));

        SPI5->DR = buff[i];

        while (SPI5->SR & SPI_SR_BSY);
    }
    */
}

static void ST7735_ExecuteCommandList(const uint8_t *addr)
{
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while(numCommands--)
    {
        uint8_t cmd = *addr++;
        ST7735_WriteCommand(cmd);

        numArgs = *addr++;

        ms = numArgs & DELAY;
        numArgs &= ~DELAY;

        if(numArgs)
        {
            ST7735_WriteData((uint8_t*)addr, numArgs);
            addr += numArgs;
        }

        if(ms)
        {
            ms = *addr++;
            if(ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    // column address set
    ST7735_WriteCommand(ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
    ST7735_WriteData(data, sizeof(data));

    // row address set
    ST7735_WriteCommand(ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    ST7735_WriteData(data, sizeof(data));

    // write to RAM
    ST7735_WriteCommand(ST7735_RAMWR);
}

void ST7735_Init()
{
    ST7735_Select();
    ST7735_Reset();
    ST7735_ExecuteCommandList(init_cmds1);
    ST7735_ExecuteCommandList(init_cmds2);
    ST7735_ExecuteCommandList(init_cmds3);
    ST7735_Unselect();
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT))
        return;

    ST7735_Select();

    ST7735_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_WriteData(data, sizeof(data));

    ST7735_Unselect();
}

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color)
{
    uint32_t i, b, j;

    for(i = 0; i < font.height; i++)
    {
        b = font.data[(ch - 32) * font.height + i];

        for(j = 0; j < font.width; j++)
            if((b << j) & 0x8000)
                ST7735_DrawPixel(x + j, y + i, color);
    }
}
void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color)
{
    ST7735_Select();

    while(*str)
    {
        if(x + font.width >= ST7735_WIDTH)
        {
            x = 0;
            y += font.height;
            if(y + font.height >= ST7735_HEIGHT)
                break;

            if(*str == ' ')
            {
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color);
        x += font.width;
        str++;
    }

    ST7735_Unselect();
}
void ST7735_WriteInt(uint16_t x, uint16_t y, int32_t num, FontDef font, uint16_t color)
{
	char buff[32] = {0};

	DL_intToStr(buff, num);

	ST7735_WriteString(x, y, buff, font, color);
}
void ST7735_WriteFloat(uint16_t x, uint16_t y, float num, uint8_t precision, FontDef font, uint16_t color)
{
	char buff[32] = {0};

	DL_floatToStr(buff, num, precision);

	ST7735_WriteString(x, y, buff, font, color);
}

static void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // clipping
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
    if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ST7735_Unselect();
}
static void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // clipping
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) w = ST7735_WIDTH - x;
    if((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

    // Prepare whole line in a single buffer
    uint8_t pixel[] = { color >> 8, color & 0xFF };
    uint8_t *line = malloc(w * sizeof(pixel));
    for(x = 0; x < w; ++x)
    	memcpy(line + x * sizeof(pixel), pixel, sizeof(pixel));

    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--)
        HAL_SPI_Transmit(&ST7735_SPI_PORT, line, w * sizeof(pixel), HAL_MAX_DELAY);

    free(line);
    ST7735_Unselect();
}

void ST7735_FillScreen(uint16_t color)
{
    ST7735_FillRectangle(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}
void ST7735_FillScreenFast(uint16_t color)
{
    ST7735_FillRectangleFast(0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data)
{
    if((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT)) return;
    if((x + w - 1) >= ST7735_WIDTH) return;
    if((y + h - 1) >= ST7735_HEIGHT) return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);
    ST7735_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ST7735_Unselect();
}

void ST7735_InvertColors(uint8_t invert)
{
    ST7735_Select();
    ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
    ST7735_Unselect();
}

void ST7735_SetGamma(GammaDef gamma)
{
	ST7735_Select();
	ST7735_WriteCommand(ST7735_GAMSET);
	ST7735_WriteData((uint8_t *) &gamma, sizeof(gamma));
	ST7735_Unselect();
}

void ST7735_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
	int32_t dx =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int32_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;

	int32_t err = dx + dy;

	for (;;)
	{
		ST7735_DrawPixel(x0, y0, color);

		if(x0 == x1 && y0 == y1) break;

		if(2 * err >= dy)
		{
			err += dy;
			x0 += sx;
		}
		else if(2 * err <= dx)
		{
			err += dx;
			y0 += sy;
		}
	}
}

void ST7735_DrawRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
{
	ST7735_DrawLine(x0, y0, x0 + w - 1, y0, color);
	ST7735_DrawLine(x0, y0, x0, y0 + h - 1, color);
	ST7735_DrawLine(x0, y0 + h - 1, x0 + w - 1, y0 + h - 1, color);
	ST7735_DrawLine(x0 + w - 1, y0, x0 + w - 1, y0 + h - 1, color);
}
void ST7735_DrawFillRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t color)
{
	ST7735_FillRectangleFast(x0, y0, w, h, color);
}

void ST7735_DrawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	ST7735_DrawPixel(x0, y0 + radius, color);
	ST7735_DrawPixel(x0, y0 - radius, color);
	ST7735_DrawPixel(x0 + radius, y0, color);
	ST7735_DrawPixel(x0 - radius, y0, color);

	while(x < y)
	{
		if(f >= 0)
		{
		  y--;
		  ddF_y += 2;
		  f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x;

		ST7735_DrawPixel(x0 + x, y0 + y, color);
		ST7735_DrawPixel(x0 - x, y0 + y, color);
		ST7735_DrawPixel(x0 + x, y0 - y, color);
		ST7735_DrawPixel(x0 - x, y0 - y, color);
		ST7735_DrawPixel(x0 + y, y0 + x, color);
		ST7735_DrawPixel(x0 - y, y0 + x, color);
		ST7735_DrawPixel(x0 + y, y0 - x, color);
		ST7735_DrawPixel(x0 - y, y0 - x, color);
	}
}
void ST7735_DrawFillCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color)
{
	for(uint16_t i = 0; i < 360; i++)
	{
		uint16_t x1 = x0 + round( (float)radius * cosf( i * M_PI / 180 ) );
		uint16_t y1 = y0 + round( (float)radius * sinf( i * M_PI / 180 ) );
		ST7735_DrawLine(x0, y0, x1, y1, color);
	}
}

void ST7735_DrawArc(uint16_t x0, uint16_t y0, uint16_t radius, int16_t angle_start, int16_t angle_end, uint16_t color)
{
	int16_t step = angle_end > angle_start ? 1 : -1;

	while(angle_start != angle_end)
	{
		uint16_t xp1 = x0 + round( (float)radius * cosf( angle_start * M_PI / 180 ) );
		uint16_t yp1 = y0 + round( (float)radius * sinf( angle_start * M_PI / 180 ) );
		uint16_t xp2 = x0 + round( (float)radius * cosf( (angle_start + step) * M_PI / 180 ) );
		uint16_t yp2 = y0 + round( (float)radius * sinf( (angle_start + step) * M_PI / 180 ) );

		ST7735_DrawLine(xp1, yp1, xp2, yp2, color);

		angle_start += step;
	}
}

void ST7735_DrawSector(uint16_t x0, uint16_t y0, uint16_t radius, int16_t angle_start, int16_t angle_end, uint16_t color)
{
	uint16_t x1 = x0 + round( (float)radius * cosf( angle_start * M_PI / 180 ) );
	uint16_t y1 = y0 + round( (float)radius * sinf( angle_start * M_PI / 180 ) );

	ST7735_DrawLine(x0, y0, x1, y1, color);

	uint16_t x2 = x0 + round( (float)radius * cosf( angle_end * M_PI / 180 ) );
	uint16_t y2 = y0 + round( (float)radius * sinf( angle_end * M_PI / 180 ) );

	ST7735_DrawLine(x0, y0, x2, y2, color);

	ST7735_DrawArc(x0, y0, radius, angle_start, angle_end, color);
}
void ST7735_DrawFillSector(uint16_t x0, uint16_t y0, uint16_t radius, int16_t angle_start, int16_t angle_end, uint16_t color)
{
	int16_t step = angle_end > angle_start ? 1 : -1;

	while(angle_start != angle_end)
	{
		uint16_t x1 = x0 + round( (float)radius * cosf( angle_start * M_PI / 180 ) );
		uint16_t y1 = y0 + round( (float)radius * sinf( angle_start * M_PI / 180 ) );

		ST7735_DrawLine(x0, y0, x1, y1, color);

		angle_start += step;
	}
}

void ST7735_DrawRoundedRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
	ST7735_DrawLine(x0 + radius, y0, x0 + w - 1 - radius, y0, color);
	ST7735_DrawLine(x0, y0 + radius, x0, y0 + h - 1 - radius, color);
	ST7735_DrawLine(x0 + radius, y0 + h - 1, x0 + w - 1 - radius, y0 + h - 1, color);
	ST7735_DrawLine(x0 + w - 1, y0 + radius, x0 + w - 1, y0 + h - 1 - radius, color);

	ST7735_DrawArc(x0 + radius, y0 + radius, radius, 180, 270, color);
	ST7735_DrawArc(x0 + radius, y0 + h - 1 - radius, radius, 90, 180, color);
	ST7735_DrawArc(x0 + w - 1 - radius, y0 + radius, radius, 0, -90, color);
	ST7735_DrawArc(x0 + w - 1 - radius, y0 + h - 1 - radius, radius, 0, 90, color);
}
void ST7735_DrawFillRoundedRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color)
{
	ST7735_DrawFillRect(x0 + radius, y0, w - 2*radius, h, color);
	ST7735_DrawFillRect(x0, y0 + radius, radius, h - 2*radius, color);
	ST7735_DrawFillRect(x0 + w - radius, y0 + radius, radius, h - 2*radius, color);

	ST7735_DrawFillSector(x0 + radius, y0 + radius, radius, 180, 270, color);
	ST7735_DrawFillSector(x0 + radius, y0 + h - radius - 1, radius, 90, 180, color);
	ST7735_DrawFillSector(x0 + w - radius - 1, y0 + radius, radius, 0, -90, color);
	ST7735_DrawFillSector(x0 + w - radius - 1, y0 + h - radius - 1, radius, 0, 90, color);
}

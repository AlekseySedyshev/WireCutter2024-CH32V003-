#include "ST7565R.h"
unsigned char Contrast_level = 0x1A; // 0x15;
unsigned char lcd_X = 0, lcd_Y = 0;  // Две глобальные переменные расположения курсора

void SPI_Wr(uint8_t data, unsigned char value)
{
    if (data == CMD)
    {
        CMD_();
    }
    else
    {
        DAT_();
    }
    CS_LO();
    while (!(SPI1->STATR & SPI_STATR_TXE))
        ;
    SPI1->DATAR = value;
    while (SPI1->STATR & SPI_STATR_BSY)
        ;
    CS_HI();
}
void lcd_Initial_Dispay_Line(unsigned char line)
{
    //  Адрес первой строки дисплея
    // Specify DDRAM line for COM0 0~63
    line |= 0x40;
    SPI_Wr(CMD, line);
}
void lcd_Power_Control(unsigned char vol)
{
    //  Управление питанием
    // Power_Control   4 (internal converte ON) + 2 (internal regulor ON) + 1 (internal follower ON)

    SPI_Wr(CMD, (0x28 | vol));
}
void lcd_Regulor_Resistor_Select(unsigned char r)
{
    //  Regulor resistor select
    //            1+Rb/Ra  Vo=(1+Rb/Ra)Vev    Vev=(1-(63-a)/162)Vref   2.1v
    //            0  3.0       4  5.0(default)
    //            1  3.5       5  5.5
    //            2  4         6  6
    //            3  4.5       7  6.4
    SPI_Wr(CMD, (0x20 | r));
}
void lcd_Set_Contrast_Control_Register(unsigned char mod)
{
    //  Установка контрастности дисплея
    //  Input : mod - контрастность от 0 до 63
    // a(0-63) 32default   Vev=(1-(63-a)/162)Vref   2.1v
    SPI_Wr(CMD, 0x81);
    SPI_Wr(CMD, mod);
}
void lcd_init(void)
{
    //  инициализация дисплея
    CS_HI();
    SPI_Wr(CMD, 0xe2); // RESET();
    RES_OFF();
    DelayMs(100);
    RES_ON();
    DelayMs(500);
    RES_OFF();
    DelayMs(100);

#if (INVERT_MODE == 1)
    SPI_Wr(CMD, 0xa1); //  Reverce disrect
    SPI_Wr(CMD, 0xc0); // Reverce SHL 1,COM0-COM63              SET_SHL();
#else
    SPI_Wr(CMD, 0xa0); //  Normal disrect (SEG0-SEG131) CLEAR_ADC();
    SPI_Wr(CMD, 0xc8); // SHL 1,COM63-COM0              SET_SHL();
#endif
    SPI_Wr(CMD, 0xa2);       //  bias 0                     CLEAR_BIAS();
    lcd_Power_Control(0x07); // voltage regulator internal Resistor ratio set   // write_cmd(0x2f);
    lcd_Regulor_Resistor_Select(0x06);
    lcd_Set_Contrast_Control_Register(Contrast_level);
    lcd_Initial_Dispay_Line(0x00);
    SPI_Wr(CMD, 0xaf); //  Display on                   DISPLAY_ON();

    DelayMs(100);
    SPI_Wr(CMD, 0xa6); //  Инвертировать все точки - выкл - нормальный режим
                       //   SPI_Wr(CMD,0xa7);   //  Инвертировать все точки - вкл
    SPI_Wr(CMD, 0xa4); //  Зажечь все точки - выкл - норальный режим
                       //   SPI_Wr(CMD,0xa5);   //  Зажечь все точки - вкл

    lcd_X = 0;
    lcd_Y = 0;
    /*
        SPI_Wr(CMD, 0xC8); // P:C0
        SPI_Wr(CMD, 0x24);
        SPI_Wr(CMD, 0x81);      // set contrast
        SPI_Wr(CMD, 40);        // contrast value 30
        SPI_Wr(CMD, 0x40);
        SPI_Wr(CMD, 0x2C);
        DelayMs(100); // delay 100ms
        SPI_Wr(CMD, 0x2E);
        DelayMs(100); // delay 100ms
        SPI_Wr(CMD, 0x2F);
        DelayMs(100); // delay 100ms
        SPI_Wr(CMD, 0xAF);
        */
}
void lcd_clear(void)
{
    //  очистка дисплея
    unsigned char i, j;

    for (i = 0; i < 9; i++) // clear page 0~8
    {
        SPI_Wr(CMD, 0xB0 + i);    // set page
        SPI_Wr(CMD, 0X00);        // set column
        SPI_Wr(CMD, 0X10);        // set column
        for (j = 0; j < 132; j++) // clear all columns upto 130
        {
            SPI_Wr(DAT, 0x00);
        }
    }
}
void lcd_gotoxy(unsigned char x, unsigned char y)
{
    //  Установка курсора
    //  Input : x,y - координаты символа
    lcd_X = x;
    lcd_Y = y;
#if (INVERT_MODE == 1)
    x++;
#endif
    x = x * 6 - 2;
    SPI_Wr(CMD, (0xB0 | (y & 0x0F)));
    SPI_Wr(CMD, 0x10 | (x >> 4));
    SPI_Wr(CMD, 0x00 | (x & 0x0f));
}
unsigned char lcd_symbol_decode(unsigned int c)
{
    //  Преобразование символов через таблицу символов для вывода на дисплей
    //  Input : с - символ в ASCII кодировке
    if (32 <= c && c <= '~')
    {
        c = c - 32;
    }
    else
    {
        if (192 <= c && c <= 255)
        {
            c = c - 97;
        }
        else
        {
            c = 255;
        }
    }
    return c;
}
void lcd_putch(unsigned char c)
{
    //  Отправка символа на дисплей
    //  Input : с - символ в ASCII кодировке
    c = lcd_symbol_decode(c); //    декодирование ASCII символа
    if (c == 255)
    {
        return;
    }
    SPI_Wr(DAT, lcd_font[c][0]);
    SPI_Wr(DAT, lcd_font[c][1]);
    SPI_Wr(DAT, lcd_font[c][2]);
    SPI_Wr(DAT, lcd_font[c][3]);
    SPI_Wr(DAT, lcd_font[c][4]);
    SPI_Wr(DAT, 0x00);
    lcd_X++;
    if (lcd_X == 21)
    {
        if (lcd_Y == 7)
        {
            lcd_gotoxy(0, 0);
        }
        else
        {
            lcd_gotoxy(0, lcd_Y + 1);
        }
    }
}
void lcd_putch_inv(unsigned char c)
{
    //  Отправка инвертированного символа на дисплей
    //  Input : с - символ в ASCII кодировке
    c = lcd_symbol_decode(c); //    декодирование ASCII символа
    if (c == 255)
    {
        return;
    }
    SPI_Wr(DAT, 0xFF - lcd_font[c][0]);
    SPI_Wr(DAT, 0xFF - lcd_font[c][1]);
    SPI_Wr(DAT, 0xFF - lcd_font[c][2]);
    SPI_Wr(DAT, 0xFF - lcd_font[c][3]);
    SPI_Wr(DAT, 0xFF - lcd_font[c][4]);
    SPI_Wr(DAT, 0xFF);
    lcd_X++;
    if (lcd_X == 21)
    {
        if (lcd_Y == 7)
        {
            lcd_gotoxy(0, 0);
        }
        else
        {
            lcd_gotoxy(0, lcd_Y + 1);
        }
    }
}
void lcd_puts(unsigned char *s)
{
    //  Отправка строки на дисплей
    //  Input : s - строка
    while (*s)
    {
        lcd_putch(*s);
        ++s;
    }
}
void lcd_puts_inv(char *s)
{
    //  Отправка инвертированной строки на дисплей
    //  Input : s - строка
    while (*s)
    {
        lcd_putch_inv(*s);
        ++s;
    }
}
void lcd_test(void)
{
    //  Тестовое заполнение дисплея подряд идущим символами
    unsigned char i;
    lcd_clear();
    lcd_gotoxy(0, 0);
    for (i = 32; i < 127; i++)
    {
        lcd_putch(i);
    }
    for (i = 192; i != 0; i++)
    {
        lcd_putch(i);
    }
}
void lcd_putch_big_prototype(unsigned char col, unsigned char row, char c, unsigned char inv)
{
    //  Основной прототип функции - Вывод символа на дисплей х2,4,8 размера
    //  Input : col - ширина, row - высота, c - символ в ASCII, inv - 0-норма,1-инверсия
    //  значения ширины и высоты должны быть кратны 2: 1,2,4,8
    unsigned char i, j, k, l, variable1, shift1, variable2, shift2, shift3, t_X, t_Y;
    unsigned int sym, sym2;
    t_X = lcd_X;
    t_Y = lcd_Y;
    c = lcd_symbol_decode(c);
    if (c == 255)
    {
        return;
    }
    lcd_gotoxy(lcd_X, lcd_Y);
    variable1 = 0;
    shift1 = 8 / row;
    variable2 = 1;
    switch (row)
    {
    case 1:
    {
        shift2 = 1;
        shift3 = 1;
    }
    break;
    case 2:
    {
        shift2 = 16;
        shift3 = 2;
    }
    break;
    case 4:
    {
        shift2 = 4;
        shift3 = 8;
    }
    break;
    case 8:
    {
        shift2 = 2;
        shift3 = 32;
    }
    break;
    }
    for (i = 0; i < row; i++)
    {
        for (j = 0; j < 5; j++)
        {
            sym = 0;
            l = 1;
            for (k = variable1; k < variable1 + shift1; k++)
            {
                sym += (lcd_font[c][j] & (1 << k)) * l;
                l *= shift3;
            }
            sym = sym / variable2;

            sym2 = 0;
            for (k = 0; k < row; k++)
            {
                sym2 = sym2 + sym;
                sym *= 2;
            }
            for (k = 0; k < col; k++)
            {
                if (inv)
                    SPI_Wr(DAT, 255 - sym2);
                else
                    SPI_Wr(DAT, sym2);
            }
        }
        for (k = 0; k < col; k++)
            if (inv)
                SPI_Wr(DAT, 255);
            else
                SPI_Wr(DAT, 0);
        lcd_gotoxy(lcd_X, lcd_Y + 1);
        variable1 += shift1;
        variable2 *= shift2;
    }
    lcd_X = t_X + col;
    lcd_Y = t_Y;
}
void lcd_putch_big(char c)
{
    //  Вывод символа на дисплей х2
    //  Input : c - символ в ASCII
    lcd_putch_big_prototype(2, 2, c, 0);
}
void lcd_puts_big(char *s)
{
    //  Вывод строки на дисплей х2
    //  Input : *s - ссыла на строку
    while (*s)
    {
        lcd_putch_big(*s);
        ++s;
    }
}
void lcd_puts_int2(unsigned char v)
{
    //  Отправка числа (из 2-х цифр) на дисплей
    //  Input : v - число
    v = v % 100;
    lcd_putch(v / 10 + 48);
    lcd_putch(v % 10 + 48);
}
void lcd_puts_int(int v)
{
    //  Отправка числа на дисплей
    //  Input : v - число
    int i = 10000;
    int t;
    unsigned char flag = 0;
    if (v < 0)
    {
        lcd_putch('-');
        v = -v;
    }
    v = v % 100000;
    while (i > 0)
    {
        t = v / i;
        if (t > 0)
        {
            flag = 1;
        }
        v = v % i;
        i = i / 10;
        if (flag)
        {
            lcd_putch(t + 48);
        }
    }
    if (!flag)
    {
        lcd_putch(48);
    }
}
void lcd_puts_long(unsigned long v)
{
    //  Отправка длинного числа на дисплей (внимание, расходует много памяти!)
    //  Input : v - число
    unsigned long i = 1000000000;
    unsigned long t;
    unsigned char flag = 0;
    if (v < 0)
    {
        lcd_putch('-');
        v = -v;
    }
    v = v % 10000000000;
    while (i > 0)
    {
        t = v / i;
        if (t > 0)
        {
            flag = 1;
        }
        v = v % i;
        i = i / 10;
        if (flag)
        {
            lcd_putch(t + 48);
        }
    }
    if (!flag)
    {
        lcd_putch(48);
    }
}
void lcd_xline(uint8_t xa, uint8_t xb, uint8_t yy) // yy = 0...63, x = 0..127
{
    uint8_t ii;

    SPI_Wr(CMD, (0xB0 | ((yy / 8) & 0x0F))); // Vertical Page select

    SPI_Wr(CMD, 0x10 | (xa >> 4));   // horisontal line select
    SPI_Wr(CMD, 0x00 | (xa & 0x0f)); // horisontal line select
    for (ii = xa; ii <= xb; ii++)
    {
        SPI_Wr(DAT, (1 << (yy % 8)));
    }
}
void lcd_yline(uint8_t ya, uint8_t yb, uint8_t xx)
{
    uint8_t ii, top, bot, pre, post;

    pre = ya / 8;
    top = ya % 8;

    post = yb / 8;
    bot = yb % 8;
    if (top > 0)
    {
        SPI_Wr(CMD, 0x10 | (xx >> 4));
        SPI_Wr(CMD, 0x00 | (xx & 0x0f));

        SPI_Wr(CMD, (0xB0 | (pre & 0x0F)));
        SPI_Wr(DAT, 0xff - (1 << top));
    }
    if (pre < post)
        for (ii = pre; ii <= post; ii++)
        {
            SPI_Wr(CMD, 0x10 | (xx >> 4));
            SPI_Wr(CMD, 0x00 | (xx & 0x0f));

            SPI_Wr(CMD, (0xB0 | (ii & 0x0F)));
            SPI_Wr(DAT, 0xff);
        }
    if (bot > 0)
    {
        SPI_Wr(CMD, 0x10 | (xx >> 4));
        SPI_Wr(CMD, 0x00 | (xx & 0x0f));

        SPI_Wr(CMD, (0xB0 | (post & 0x0F)));
        SPI_Wr(DAT, bot);
    }
}
void lcd_puts_UTF8(unsigned char *s)
{
    //  Отправка строки на дисплей
    //  Input : s - строка
    // win1251
    // A - 0xC0,..... Я - 0xDF      Ё = 0xA8,   П = 0xCF(207), Р = 0xD0(208)
    // а - 0xE0,..... я - 0xFF      ё = 0xB8,   п = 0xEF(239), р = 0xF0(240)
    // UTF8
    // A - 0x0410,... Я - 0x042f    Ё - 0x0401,  П = 0x041F,  Р = 0x0420
    // а - 0x0430,... я - 0x044f    ё - 0x0451   п = 0x043F,  р = 0x0440
    unsigned char symb = 0;
    while (*s)
    {
        s++;
        symb = (*s + 0x30);
        if (symb < 192)
            symb += 64;

        if ((*s != 0x01) || (*s != 0x51))
        {
            lcd_putch((unsigned char)symb);
        }
        else
        {
            if (*s == 0x01)
            {
                lcd_putch(0xA8);
            }
            if (*s == 0x51)
            {
                lcd_putch(0xB8);
            }
        }

        ++s;
    }
}

void lcd_wire_center(uint8_t yy, uint8_t wire_mode) // yy = 0...7, x = 0..127
{
    uint8_t ii;

    SPI_Wr(CMD, (0xB0 | (yy & 0x0F))); // Vertical Page select

    SPI_Wr(CMD, 0x10);
    SPI_Wr(CMD, 0x00); // 0..127
    if (((wire_mode >> 4) & 0xf) == 0)
    {
        for (ii = 0; ii <= 30; ii++)
        {
            SPI_Wr(DAT, 0x1C);
        }
    }
    if (((wire_mode >> 4) & 0xf) == 1)
    {
        SPI_Wr(DAT, 0x7f);
        for (ii = 1; ii <= 27; ii++)
        {
            SPI_Wr(DAT, 0x41);
        }
        SPI_Wr(DAT, 0x7f);
        SPI_Wr(DAT, 0x1C);
        SPI_Wr(DAT, 0x1C);
    }
    if (((wire_mode >> 4) & 0xf) == 2)
    {
        SPI_Wr(DAT, 0x7f);
        for (ii = 1; ii <= 15; ii++)
        {
            SPI_Wr(DAT, 0x41);
        }
        SPI_Wr(DAT, 0x7f);
        for (ii = 17; ii <= 30; ii++)
        {
            SPI_Wr(DAT, 0x1C);
        }
    }
    // центральная часть
    SPI_Wr(DAT, 0x7f); // 31
    for (ii = 32; ii <= 94; ii++)
    {
        SPI_Wr(DAT, 0x41);
    }
    SPI_Wr(DAT, 0x7f); // 95
                       // конец центральной части
    if ((wire_mode & 0xf) == 0)
    {
        for (ii = 96; ii <= 127; ii++)
        {
            SPI_Wr(DAT, 0x1C);
        }
    }
    if ((wire_mode & 0xf) == 1)
    {
        SPI_Wr(DAT, 0x1C);
        SPI_Wr(DAT, 0x1C);
        SPI_Wr(DAT, 0x7f);
        for (ii = 99; ii <= 126; ii++)
        {
            SPI_Wr(DAT, 0x41);
        }
        SPI_Wr(DAT, 0x7f);
    }
    if ((wire_mode & 0xf) == 2)
    {
        for (ii = 96; ii <= 111; ii++)
        {
            SPI_Wr(DAT, 0x1C);
        }
        SPI_Wr(DAT, 0x7f);
        for (ii = 113; ii <= 126; ii++)
        {
            SPI_Wr(DAT, 0x41);
        }
        SPI_Wr(DAT, 0x7f);
    }
}
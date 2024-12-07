﻿//#include <stdint.h>
//#include <ch32v00x.h>
#include "main.h"

#define INVERT_MODE 0 //  перевернуть дисплей

#define CMD_() GPIOD->OUTDR &= (~GPIO_OUTDR_ODR2) // A0->0
#define DAT_() GPIOD->OUTDR |= GPIO_OUTDR_ODR2    // A0->1

#define RES_ON() GPIOC->OUTDR &= (~GPIO_OUTDR_ODR7) // RES->0
#define RES_OFF() GPIOC->OUTDR |= GPIO_OUTDR_ODR7   // RES->1

#define CS_HI() GPIOC->OUTDR |= GPIO_OUTDR_ODR4    // CS->1
#define CS_LO() GPIOC->OUTDR &= (~GPIO_OUTDR_ODR4) // CS->0

#define CMD 0
#define DAT 1

static const char lcd_font[][5] =
    {
        {0x00, 0x00, 0x00, 0x00, 0x00}, // sp
        {0x00, 0x00, 0x2f, 0x00, 0x00}, // !
        {0x00, 0x07, 0x00, 0x07, 0x00}, // "
        {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
        {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
        {0x23, 0x13, 0x08, 0x64, 0x62}, // %

        {0x36, 0x49, 0x55, 0x22, 0x50}, // &
        {0x0E, 0x11, 0x11, 0x11, 0x0E}, // ' = символ градуса           //{0x00, 0x05, 0x03, 0x00, 0x00}, // '
        {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
        {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
        {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
        {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
        {0x00, 0x00, 0x50, 0x30, 0x00}, // ,
        {0x08, 0x08, 0x08, 0x08, 0x08}, // -
                                        //  { 0x10, 0x10, 0x10, 0x10, 0x10 },  // -
        {0x00, 0x60, 0x60, 0x00, 0x00}, // .
        {0x20, 0x10, 0x08, 0x04, 0x02}, // /
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
        {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
        {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
        {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
        {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
        {0x00, 0x36, 0x36, 0x00, 0x00}, // :
        {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
        {0x07, 0x07, 0x07, 0x07, 0x07}, // < // Символ верхнего бункера  {0x08, 0x14, 0x22, 0x41, 0x00}
        {0x14, 0x14, 0x14, 0x14, 0x14}, // =
        {0x70, 0x70, 0x70, 0x70, 0x70}, // > // Символ нижнего бункера   {0x00, 0x41, 0x22, 0x14, 0x08}
        {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
        {0x32, 0x49, 0x59, 0x51, 0x3E}, // @
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
        {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
        {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
        {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
        {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
        {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
        {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
        {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
        {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
        {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
        {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
        {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
        {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
        {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
        {0x46, 0x49, 0x49, 0x49, 0x31}, // S
        {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
        {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
        {0x63, 0x14, 0x08, 0x14, 0x63}, // X
        {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
        {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
        {0x00, 0x7F, 0x41, 0x41, 0x00}, // [
        {0x02, 0x04, 0x08, 0x10, 0x20}, // / наоборот
                                        //  { 0x55, 0x2A, 0x55, 0x2A, 0x55 },  // 55
        {0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
        {0x04, 0x02, 0x01, 0x02, 0x04}, // ^
        {0x40, 0x40, 0x40, 0x40, 0x40}, // _
        {0x00, 0x01, 0x02, 0x04, 0x00}, // '
        {0x20, 0x54, 0x54, 0x54, 0x78}, // a
        {0x7F, 0x48, 0x44, 0x44, 0x38}, // b
        {0x38, 0x44, 0x44, 0x44, 0x20}, // c
        {0x38, 0x44, 0x44, 0x48, 0x7F}, // d
        {0x38, 0x54, 0x54, 0x54, 0x18}, // e
        {0x08, 0x7E, 0x09, 0x01, 0x02}, // f
        {0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
        {0x7F, 0x08, 0x04, 0x04, 0x78}, // h
        {0x00, 0x44, 0x7D, 0x40, 0x00}, // i
        {0x20, 0x40, 0x44, 0x3D, 0x00}, // j
        {0x7F, 0x10, 0x28, 0x44, 0x00}, // k
        {0x00, 0x41, 0x7F, 0x40, 0x00}, // l
        {0x7C, 0x04, 0x18, 0x04, 0x78}, // m
        {0x7C, 0x08, 0x04, 0x04, 0x78}, // n
        {0x38, 0x44, 0x44, 0x44, 0x38}, // o
        {0x7C, 0x14, 0x14, 0x14, 0x08}, // p
        {0x08, 0x14, 0x14, 0x18, 0x7C}, // q
        {0x7C, 0x08, 0x04, 0x04, 0x08}, // r
        {0x48, 0x54, 0x54, 0x54, 0x20}, // s
        {0x04, 0x3F, 0x44, 0x40, 0x20}, // t
        {0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
        {0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
        {0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
        {0x44, 0x28, 0x10, 0x28, 0x44}, // x
        {0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
        {0x44, 0x64, 0x54, 0x4C, 0x44}, // z

        {0x08, 0x08, 0x36, 0x41, 0x41}, // {
        {0x00, 0x00, 0x7F, 0x00, 0x00}, // |
        {0x41, 0x41, 0x36, 0x08, 0x08}, // }
        {0x02, 0x01, 0x02, 0x02, 0x01}, // ~

        {0x7E, 0x11, 0x11, 0x11, 0x7E}, // А
        {0x7F, 0x49, 0x49, 0x49, 0x33}, // Б
        {0x7F, 0x49, 0x49, 0x49, 0x36}, // В
        {0x7F, 0x01, 0x01, 0x01, 0x03}, // Г
        {0xE0, 0x51, 0x4F, 0x41, 0xFF}, // Д
        {0x7F, 0x49, 0x49, 0x49, 0x49}, // Е
        {0x77, 0x08, 0x7F, 0x08, 0x77}, // Ж
        {0x49, 0x49, 0x49, 0x49, 0x36}, // З
        {0x7F, 0x10, 0x08, 0x04, 0x7F}, // И
        {0x7C, 0x21, 0x12, 0x09, 0x7C}, // Й
        {0x7F, 0x08, 0x14, 0x22, 0x41}, // К
        {0x20, 0x41, 0x3F, 0x01, 0x7F}, // Л
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // М
        {0x7F, 0x08, 0x08, 0x08, 0x7F}, // Н
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // О
        {0x7F, 0x01, 0x01, 0x01, 0x7F}, // П
        {0x7F, 0x09, 0x09, 0x09, 0x06}, // Р
        {0x3E, 0x41, 0x41, 0x41, 0x22}, // С
        {0x01, 0x01, 0x7F, 0x01, 0x01}, // Т
        {0x27, 0x48, 0x48, 0x48, 0x3F}, // У
        {0x1C, 0x22, 0x7F, 0x22, 0x1C}, // Ф
        {0x63, 0x14, 0x08, 0x14, 0x63}, // Х
        {0x7F, 0x40, 0x40, 0x40, 0xFF}, // Ц
        {0x07, 0x08, 0x08, 0x08, 0x7F}, // Ч
        {0x7F, 0x40, 0x7F, 0x40, 0x7F}, // Ш
        {0x7F, 0x40, 0x7F, 0x40, 0xFF}, // Щ
        {0x01, 0x7F, 0x48, 0x48, 0x30}, // Ъ
        {0x7F, 0x48, 0x30, 0x00, 0x7F}, // Ы
        {0x7F, 0x48, 0x48, 0x30, 0x00}, // Ь
        {0x22, 0x41, 0x49, 0x49, 0x3E}, // Э
        {0x7F, 0x08, 0x3E, 0x41, 0x3E}, // Ю
        {0x46, 0x29, 0x19, 0x09, 0x7F}, // Я
        {0x20, 0x54, 0x54, 0x54, 0x78}, // а
        {0x3C, 0x4A, 0x4A, 0x49, 0x31}, // б
        {0x7C, 0x54, 0x54, 0x28, 0x00}, // в
        {0x7C, 0x04, 0x04, 0x04, 0x0C}, // г
        {0xE0, 0x54, 0x4C, 0x44, 0xFC}, // д
        {0x38, 0x54, 0x54, 0x54, 0x08}, // е
        {0x6C, 0x10, 0x7C, 0x10, 0x6C}, // ж
        {0x44, 0x44, 0x54, 0x54, 0x28}, // з
        {0x7C, 0x20, 0x10, 0x08, 0x7C}, // и
        {0x78, 0x42, 0x24, 0x12, 0x78}, // й
        {0x7C, 0x10, 0x28, 0x44, 0x00}, // к
        {0x20, 0x44, 0x3C, 0x04, 0x7C}, // л
        {0x7C, 0x08, 0x10, 0x08, 0x7C}, // м
        {0x7C, 0x10, 0x10, 0x10, 0x7C}, // н
        {0x38, 0x44, 0x44, 0x44, 0x38}, // о
        {0x7C, 0x04, 0x04, 0x04, 0x7C}, // п
        {0x7C, 0x14, 0x14, 0x14, 0x08}, // р
        {0x38, 0x44, 0x44, 0x44, 0x44}, // с
        {0x04, 0x04, 0x7C, 0x04, 0x04}, // т
        {0x0C, 0x50, 0x50, 0x50, 0x3C}, // у
        {0x18, 0x24, 0x7E, 0x24, 0x18}, // ф
        {0x44, 0x28, 0x10, 0x28, 0x44}, // х
        {0x7C, 0x40, 0x40, 0x40, 0xFC}, // ц
        {0x0C, 0x10, 0x10, 0x10, 0x7C}, // ч
        {0x7C, 0x40, 0x7C, 0x40, 0x7C}, // ш
        {0x7C, 0x40, 0x7C, 0x40, 0xFC}, // щ
        {0x04, 0x7C, 0x50, 0x50, 0x20}, // ъ
        {0x7C, 0x50, 0x20, 0x00, 0x7C}, // ы
        {0x7C, 0x50, 0x50, 0x20, 0x00}, // ь
        {0x28, 0x44, 0x54, 0x54, 0x38}, // э
        {0x7C, 0x10, 0x38, 0x44, 0x38}, // ю
        {0x08, 0x54, 0x34, 0x14, 0x7C}  // я
};

void SPI_Wr(uint8_t data, unsigned char value);

void lcd_Initial_Dispay_Line(unsigned char line);
//	Управление питанием
void lcd_Power_Control(unsigned char vol);
//	Установка контрастности дисплея
//	Input : mod - контрастность от 0 до 63
void lcd_Set_Contrast_Control_Register(unsigned char mod);
void lcd_Regulor_Resistor_Select(unsigned char r);

//	инициализация дисплея
void lcd_init(void);

//	очистка дисплея
void lcd_clear(void);
//	Установка курсора
// 	Input : x,y - координаты символа
void lcd_gotoxy(unsigned char x, unsigned char y);
//	Преобразование символов через таблицу символов для вывода на дисплей
//	Input : с - символ в ASCII кодировке
unsigned char lcd_symbol_decode(unsigned int c);
//	Отправка символа на дисплей
//	Input : с - символ в ASCII кодировке
void lcd_putch(unsigned char c);
//	Отправка инвертированного символа на дисплей
//	Input : с - символ в ASCII кодировке
void lcd_putch_inv(unsigned char c);
//	Отправка строки на дисплей
//	Input : s - строка
void lcd_puts(unsigned char *s);
//	Отправка инвертированной строки на дисплей
//	Input : s - строка
void lcd_puts_inv(char *s);
//	Тестовое заполнение дисплея подряд идущим символами
void lcd_test(void);
//	Основной прототип функции - Вывод символа на дисплей х2,4,8 размера
//	Input : col - ширина, row - высота, c - символ в ASCII, inv - 0-норма,1-инверсия
//	значения ширины и высоты должны быть кратны 2: 1,2,4,8
void lcd_putch_big_prototype(unsigned char col, unsigned char row, char c, unsigned char inv);
//	Вывод символа на дисплей х2
//	Input : c - символ в ASCII
void lcd_putch_big(char c);
//	Вывод строки на дисплей х2
//	Input : *s - ссыла на строку
void lcd_puts_big(char *s);
//	Отправка числа (из 2-х цифр) на дисплей
//	Input : v - число
void lcd_puts_int2(unsigned char v);
//	Отправка числа на дисплей
//	Input : v - число
void lcd_puts_int(int v);
//	Отправка длинного 4Б на дисплей  (внимание, расходует много памяти!)
//	Input : v - число
void lcd_puts_long(unsigned long v);
void lcd_xline(uint8_t xa, uint8_t xb, uint8_t yy);
void lcd_yline(uint8_t ya, uint8_t yb, uint8_t xx);
void lcd_puts_UTF8(unsigned char *s);
void lcd_pict(unsigned char xx, unsigned char yy, unsigned char pict_number);
void lcd_wire_center(uint8_t yy, uint8_t wire_mode);
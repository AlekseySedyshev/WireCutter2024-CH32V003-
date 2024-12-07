// Project wire cut started 01/10/2024
//  Copyright (c) 2022, Aleksey Sedyshev
//  https://github.com/AlekseySedyshev

// LCD:     CS - PC4, SCL-PC5, A0-PD2, MOSI- PC6, RST - PC7
// Buttons: Key_Up - PD0, Key_Right - PD4,  Key_Stop - PD5
//          Key_Down - PC0, Key_Start - PC1, Key_Left - PC2
// Input: PC3 - Wire End
// Prog:    PD1 - SWIO, PD7 - NRST
// Output:  PD6-Motor Enable, PA1 - Motor Step, PA2 - Motor Dir, PD3 - Servo PWM

#include "main.h"
#include "ST7565R.h"

// #define IWDG_ON

extern uint32_t SystemCoreClock;

uint16_t TimingDelay;
//-------------LCD-------------------

bool lcd_out_enable = false;
bool backlight = true;
bool scan_on = true;
uint8_t liq_lev_count = 0;

uint16_t count_lcd = 0, but_press = 0, blink = 0;
uint8_t menu_x = 0, menu_y = 0;

int16_t *EEP_REG = (int16_t *)(EEP_ADDR);
//-----------Определения-----------------
mode_param work_ = MODE_OFF;
set_param set;
//----------Timers---------------
volatile uint32_t step_counter = 0, dest_count = 0;
bool tim_flag = 0;
//--------------------------------------------------------
void DelayMs(uint16_t Delay_time)
{
    TimingDelay = Delay_time;
    while (TimingDelay > 0x00)
    {
    };
}

void init(void)
{
    { //-------------Systic init---------------
        SysTick->SR &= ~(1 << 0);
        SysTick->CMP = (SystemCoreClock / 1000) - 1; // 1ms
        SysTick->CNT = 0;
        SysTick->CTLR = 0xF;

        NVIC_SetPriority(SysTicK_IRQn, 1);
        NVIC_EnableIRQ(SysTicK_IRQn);
    }
    { //-------------Clock On------------------
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
        RCC->APB2PCENR |= RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO;
        RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
    }
    { // LCD:      MOSI- PC6, SCL-PC5,  PC4 - CS, RST - PC7,  PD2 - A0,
        GPIOC->CFGLR &= (~GPIO_CFGLR_CNF4) & (~GPIO_CFGLR_CNF5) & (~GPIO_CFGLR_CNF6) & (~GPIO_CFGLR_CNF7);
        GPIOC->CFGLR |= GPIO_CFGLR_MODE4 | GPIO_CFGLR_MODE5 | GPIO_CFGLR_MODE6 | GPIO_CFGLR_MODE7; // Speed Max 50 MHz
        GPIOC->CFGLR |= GPIO_CFGLR_CNF5_1 | GPIO_CFGLR_CNF6_1;                                     // PC5, PC6 - Alternative

        GPIOD->CFGLR &= (~GPIO_CFGLR_CNF2); // Reset mode
        GPIOD->CFGLR |= GPIO_CFGLR_MODE2;   // PD2- Output mode 50 MHZ

        SPI1->CTLR1 |= SPI_CTLR1_BR_0; // FCLK / 4
        SPI1->CTLR1 |= SPI_CTLR1_BIDIMODE | SPI_CTLR1_BIDIOE;
        SPI1->CTLR1 |= SPI_CTLR1_SSM | SPI_CTLR1_SSI;
        SPI1->CTLR1 |= SPI_CTLR1_MSTR | SPI_CTLR1_SPE; //
    }
    { // Buttons: Key_Down - PC0, Key_Start - PC1, Key_Left - PC2
      //          Key_Up - PD0, Key_Right - PD4,  Key_Stop - PD5
        GPIOC->CFGLR &= (~GPIO_CFGLR_CNF0) & (~GPIO_CFGLR_CNF1) & (~GPIO_CFGLR_CNF2);
        GPIOC->OUTDR |= GPIO_OUTDR_ODR0 | GPIO_OUTDR_ODR1 | GPIO_OUTDR_ODR2;       // Pull_Up Enable
        GPIOC->CFGLR |= GPIO_CFGLR_CNF0_1 | GPIO_CFGLR_CNF1_1 | GPIO_CFGLR_CNF2_1; // Input_Pull_Up

        GPIOD->CFGLR &= (~GPIO_CFGLR_CNF0) & (~GPIO_CFGLR_CNF4) & (~GPIO_CFGLR_CNF5);
        GPIOD->OUTDR |= GPIO_OUTDR_ODR0 | GPIO_OUTDR_ODR4 | GPIO_OUTDR_ODR5;       // Pull_Up Enable
        GPIOD->CFGLR |= GPIO_CFGLR_CNF0_1 | GPIO_CFGLR_CNF4_1 | GPIO_CFGLR_CNF5_1; // Input_Pull_Up
    }
    { // In Digit:  Wire_End
        GPIOC->CFGLR &= (~GPIO_CFGLR_CNF3);
        GPIOC->OUTDR |= GPIO_OUTDR_ODR3;
        GPIOC->CFGLR |= GPIO_CFGLR_CNF3_1; // Input Mode Pull Up
    }

    { // Servo PWM:   PD3 - Servo PWM
        GPIOD->CFGLR &= (~GPIO_CFGLR_CNF3);
        GPIOD->CFGLR |= GPIO_CFGLR_MODE3 | GPIO_CFGLR_CNF3_1;

        TIM2->CCER |= TIM_CC2E;
        TIM2->CHCTLR1 |= TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
        TIM2->PSC = 1200 - 1;
        TIM2->ATRLR = 200;
        TIM2->CH2CVR = KNIFE_OPEN;

        TIM2->CTLR1 |= TIM_ARPE | TIM_CEN; // start
    }
    { // Step driver pins configuration
        // PD6 - Step_Driver ~Enable,
        GPIOD->CFGLR &= (~GPIO_CFGLR_CNF6); // Reset mode
        GPIOD->CFGLR |= GPIO_CFGLR_MODE6;   // PD6 - Output mode 50 MHZ

        // PA2 - Step_Driver - DIR
        GPIOA->CFGLR &= (~GPIO_CFGLR_CNF2); // Reset mode
        GPIOA->CFGLR |= GPIO_CFGLR_MODE2;   // PA2 - Output mode 50 MHZ

        // PA1 - Motor Step,
        GPIOA->CFGLR &= (~GPIO_CFGLR_CNF1);
        GPIOA->CFGLR |= GPIO_CFGLR_MODE1 | GPIO_CFGLR_CNF1_1; // Альтернативные выход

        DRV_OFF; // DRV_ON
        DIR_FWD; // DIR_BWD

        TIM1->PSC = 24000 - 1; // 15000     // Тут можно настроить скорость под себя
        TIM1->ATRLR = 2;
        TIM1->CHCTLR1 |= TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2M_0; // PWM
        TIM1->CCER |= TIM_CC2E;
        TIM1->CH2CVR = 1;
        TIM1->DMAINTENR |= TIM_UIE;
        TIM1->BDTR |= TIM_MOE;
        NVIC_SetPriority(TIM1_UP_IRQn, 4);
        NVIC_EnableIRQ(TIM1_UP_IRQn);
    }
}

void ReadEEPROM(void)
{
    set.wires_a = (uint16_t)EEP_REG[EEP_WIRE_A];
    set.wires_b = (uint16_t)EEP_REG[EEP_WIRE_B];
    set.wires_c = (uint16_t)EEP_REG[EEP_WIRE_C];
    set.wires_qnt = (uint16_t)EEP_REG[EEP_WIRE_QNT];
    set.knife = (uint16_t)EEP_REG[EEP_KNIFE];
    set.knife_open = (uint16_t)EEP_REG[EEP_KNIFE_OPEN];
    set.knife_close = (uint16_t)EEP_REG[EEP_KNIFE_CLOSE];

    if (set.wires_a > 1000)
        set.wires_a = 5;
    if (set.wires_b > 10000)
        set.wires_b = 185;
    if (set.wires_c > 1000)
        set.wires_c = 10;
    if (set.wires_qnt > 1000)
        set.wires_qnt = 1;

    if (set.knife > 200)
        set.knife = KNIFE_DEF_CUT;
    if (set.knife_open > 200)
        set.knife_open = KNIFE_OPEN;
    if (set.knife_close > 200)
        set.knife_close = KNIFE_CLOSE;
}
void WriteEEPROM(void)
{
    FLASH_Unlock_Fast();
    FLASH_ErasePage_Fast(EEP_ADDR);

    FLASH_ProgramHalfWord(EEP_ADDR + 0, (uint16_t)set.wires_a);      // EEP_REG[EEP_FREEZ_DENCITY];
    FLASH_ProgramHalfWord(EEP_ADDR + 2, (uint16_t)set.wires_b);      // EEP_REG[EEP_FREEZ_TEMP_TOP];
    FLASH_ProgramHalfWord(EEP_ADDR + 4, (uint16_t)set.wires_c);      // EEP_REG[EEP_FREEZ_TEMP_BOT];
    FLASH_ProgramHalfWord(EEP_ADDR + 6, (uint16_t)set.wires_qnt);    // EEP_REG[EEP_WASH_TIME];
    FLASH_ProgramHalfWord(EEP_ADDR + 8, (uint16_t)set.knife);        // EEP_REG[EEP_NIGHT_TOP_TEMP];
    FLASH_ProgramHalfWord(EEP_ADDR + 10, (uint16_t)set.knife_open);  // EEP_REG[EEP_WASH_TIME];
    FLASH_ProgramHalfWord(EEP_ADDR + 12, (uint16_t)set.knife_close); // EEP_REG[EEP_NIGHT_TOP_TEMP];
    FLASH_Lock_Fast();
}

void key_scan(void)
{
    if (!UP())
    {
        if (but_press >= SHORT_PRESS && but_press <= SEC2_PRESS && (but_press % INC_PRESS == 0 || but_press == SHORT_PRESS))
        {
            //--------------Начало провода----------------
            if (menu_x == 1 && menu_y == 1 && set.wires_a < 100)
            {
                set.wires_a += 1;
            }
            if (menu_x == 1 && menu_y == 2 && set.wires_b < 1000)
            {
                set.wires_b += 1;
            }

            if (menu_x == 1 && menu_y == 3 && set.wires_c < 100)
            {
                set.wires_c += 1;
            }
            if (menu_x == 1 && menu_y == 4 && set.wires_qnt < 1000)
            {
                set.wires_qnt += 1;
            }
            if (menu_x == 1 && menu_y == 5 && set.knife < 200)
            {
                set.knife += 1;
            }
            if (menu_x == 1 && menu_y == 6 && set.knife_open < 200)
            {
                set.knife_open += 1;
            }
            if (menu_x == 1 && menu_y == 7 && set.knife_close < 200)
            {
                set.knife_close += 1;
            }
            if (menu_x == 1 && menu_y == 8)
            {
                WriteEEPROM();
                ReadEEPROM();
                menu_x = 0;
                menu_y = 0;
                work_ = MODE_OFF;
            }
        }
        if (but_press >= SEC2_PRESS && but_press < 0xffff && but_press % INC_PRESS == 0)
        {
            //--------------Начало провода----------------
            if (menu_x == 1 && menu_y == 1 && set.wires_a <= 90)
            {
                set.wires_a += 10;
            }
            if (menu_x == 1 && menu_y == 2 && set.wires_b <= 990)
            {
                set.wires_b += 10;
            }

            if (menu_x == 1 && menu_y == 3 && set.wires_c <= 90)
            {
                set.wires_c += 10;
            }
            if (menu_x == 1 && menu_y == 4 && set.wires_qnt <= 990)
            {
                set.wires_qnt += 10;
            }
            if (menu_x == 1 && menu_y == 5 && set.knife <= 189)
            {
                set.knife += 10;
            }
            if (menu_x == 1 && menu_y == 6 && set.knife_open <= 189)
            {
                set.knife_open += 10;
            }
            if (menu_x == 1 && menu_y == 7 && set.knife_close <= 189)
            {
                set.knife_close += 10;
            }
        }
        if (but_press > 50)
            blink = 0;
        lcd_out_enable = true;
    }
    if (!DOWN())
    {
        if (but_press >= SHORT_PRESS && but_press <= SEC2_PRESS && (but_press % INC_PRESS == 0 || but_press == SHORT_PRESS))
        {
            //------------начало провода----------------
            if (menu_x == 1 && menu_y == 1 && set.wires_a > 0)
            {
                set.wires_a -= 1;
            }
            if (menu_x == 1 && menu_y == 2 && set.wires_b > 0)
            {
                set.wires_b -= 1;
            }
            if (menu_x == 1 && menu_y == 3 && set.wires_c > 0)
            {
                set.wires_c -= 1;
            }
            if (menu_x == 1 && menu_y == 4 && set.wires_qnt > 1)
            {
                set.wires_qnt -= 1;
            }
            if (menu_x == 1 && menu_y == 5 && set.knife > 1)
            {
                set.knife -= 1;
            }
            if (menu_x == 1 && menu_y == 6 && set.knife_open > 1)
            {
                set.knife_open -= 1;
            }
            if (menu_x == 1 && menu_y == 7 && set.knife_close > 1)
            {
                set.knife_close -= 1;
            }
            if (menu_x == 1 && menu_y == 8)
            {
                WriteEEPROM();
                ReadEEPROM();
                menu_x = 0;
                menu_y = 0;
                work_ = MODE_OFF;
            }
        }
        if (but_press > SEC2_PRESS && but_press < 0xffff && but_press % INC_PRESS == 0)
        {
            if (menu_x == 1 && menu_y == 1 && set.wires_a > 9)
            {
                set.wires_a -= 10;
            }
            if (menu_x == 1 && menu_y == 2 && set.wires_b > 10)
            {
                set.wires_b -= 10;
            }
            if (menu_x == 1 && menu_y == 3 && set.wires_c > 9)
            {
                set.wires_c -= 10;
            }
            if (menu_x == 1 && menu_y == 4 && set.wires_qnt > 10)
            {
                set.wires_qnt -= 10;
            }
            if (menu_x == 1 && menu_y == 5 && set.knife > 10)
            {
                set.knife -= 10;
            }
            if (menu_x == 1 && menu_y == 6 && set.knife_open > 10)
            {
                set.knife_open -= 10;
            }
            if (menu_x == 1 && menu_y == 7 && set.knife_close > 10)
            {
                set.knife_close -= 10;
            }
        }
        if (but_press > 50)
            blink = 0;
        lcd_out_enable = true;
    }
    if (!LEFT())
    {
        if (but_press > SHORT_PRESS && but_press < 0xffff && work_ == MODE_OFF)
        {
            menu_x = 1;
            if (menu_y > 1)
                menu_y--;
            else
                menu_y = 8;

            but_press = 0xffff;
        }
        lcd_out_enable = true;
    }
    if (!RIGHT())
    {
        if (but_press > SHORT_PRESS && but_press < 0xffff && work_ == MODE_OFF)
        {
            menu_x = 1;
            if (menu_y < 8)
                menu_y++;
            else
                menu_y = 1;

            but_press = 0xffff;
        }

        lcd_out_enable = true;
    }
    if (!STOP())
    {
        if (but_press > SHORT_PRESS)
        {
            menu_x = 0;
            menu_y = 0;
            if (work_ != MODE_OFF)
                lcd_clear();
            TIM2->CH2CVR = set.knife_open;
            work_ = MODE_OFF;
        }

        lcd_out_enable = true;
    }
    if (!START())
    {
        if (but_press > SHORT_PRESS && work_ == MODE_OFF && but_press < 0xffff)
        {
            lcd_clear();
            TIM2->CH2CVR = set.knife_open;
            menu_x = 0;
            menu_y = 0;
            work_ = MODE_CUT;
            but_press = 0;
        }
        lcd_out_enable = true;
        work_ = MODE_CUT;
    }
    if (UP() && RIGHT() && DOWN() && START() && STOP() && LEFT())
    {
    }
    scan_on = false;
}
void lcd_routine(void)
{
    if (lcd_out_enable == 1)
    {

        // if (work_ == MODE_OFF)
        // {
        lcd_wire_center(0, 0x11);
        lcd_gotoxy(2, 1);
        if (menu_y != 1 || (menu_x == 1 && menu_y == 1 && blink < 500))
        {
            lcd_puts_int(set.wires_a);
            lcd_puts("  ");
        }
        if (menu_x == 1 && menu_y == 1 && blink >= 500)
        {
            lcd_puts("     ");
        }
        lcd_gotoxy(10, 1);
        if (menu_y != 2 || (menu_x == 1 && menu_y == 2 && blink < 500))
        {
            lcd_puts_int(set.wires_b);
            lcd_puts("  ");
        }
        if (menu_x == 1 && menu_y == 2 && blink >= 500)
        {
            lcd_puts("     ");
        }
        lcd_gotoxy(18, 1);
        if (menu_y != 3 || (menu_x == 1 && menu_y == 3 && blink < 500))
        {
            lcd_puts_int(set.wires_c);
            lcd_puts("  ");
        }
        if (menu_x == 1 && menu_y == 3 && blink >= 500)
        {
            lcd_puts("     ");
        }
        lcd_gotoxy(1, 3);
        lcd_puts_UTF8("Кол");
        lcd_puts("-");
        lcd_puts_UTF8("во");
        lcd_puts(": ");
        if (menu_y != 4 || (menu_x == 1 && menu_y == 4 && blink < 500))
        {
            lcd_puts_int(set.wires_qnt);
            lcd_puts(" ");
        }
        if (menu_x == 1 && menu_y == 4 && blink >= 500)
        {
            lcd_puts("     ");
        }
        lcd_gotoxy(12, 3);
        lcd_puts_UTF8("Нож");
        lcd_puts(": ");
        if (menu_y != 5 || (menu_x == 1 && menu_y == 5 && blink < 500))
        {
            lcd_puts_int(set.knife);
            lcd_puts(" ");
        }
        if (menu_x == 1 && menu_y == 5 && blink >= 500)
        {
            lcd_puts("     ");
        }
        if (work_ == MODE_OFF)
        {
            lcd_gotoxy(6, 4);
            lcd_puts_UTF8("Нож");
            lcd_puts(" ");
            lcd_puts_UTF8("открыт");
            lcd_puts(" ");
            if (menu_y != 6 || (menu_x == 1 && menu_y == 6 && blink < 500))
            {
                lcd_puts_int(set.knife_open);
                lcd_puts(" ");
            }
            if (menu_x == 1 && menu_y == 6 && blink >= 500)
            {
                lcd_puts("             ");
            }
            lcd_gotoxy(6, 5);
            lcd_puts_UTF8("Нож");
            lcd_puts(" ");
            lcd_puts_UTF8("закрыт");
            lcd_puts(" ");
            if (menu_y != 7 || (menu_x == 1 && menu_y == 7 && blink < 500))
            {
                lcd_puts_int(set.knife_close);
                lcd_puts(" ");
            }
            if (menu_x == 1 && menu_y == 7 && blink >= 500)
            {
                lcd_puts("       ");
            }
            lcd_gotoxy(1, 7);
            if (menu_y != 8 || (menu_x == 1 && menu_y == 8 && blink < 500))
            {
                lcd_puts_UTF8("Сохранить");
                lcd_puts(" ");
                lcd_puts_UTF8("настройки");
            }
            if (menu_x == 1 && menu_y == 8 && blink >= 500)
            {
                lcd_puts("                   ");
            }
        }
    }
}
void work_routine(void)
{
    if (menu_x == 1 && menu_y == 5)
    {
        TIM2->CH2CVR = set.knife;
    }
    if (menu_x == 1 && (menu_y == 6 || menu_y == 8 || menu_y < 5))
    {
        TIM2->CH2CVR = set.knife_open;
    }
    if (menu_x == 1 && menu_y == 7)
    {
        TIM2->CH2CVR = set.knife_close;
    }
}

void action(void)
{
    uint16_t qq;
    lcd_clear();
    if (set.wires_b == 0 || set.wires_qnt == 0)
        return;
    DRV_ON;
    DelayMs(1);

    TIM2->CH2CVR = set.knife_open;
    DelayMs(KNIFE_TIME);
    lcd_gotoxy(4, 1);
    lcd_puts_UTF8("Работаю");
    lcd_puts("...");
    for (qq = set.wires_qnt; qq > 0; qq--)
    {
        if (!WIRE_END()) // если провод закончился
        {
            DRV_OFF;
            TIM2->CH2CVR = set.knife_open;
            lcd_clear();
            lcd_gotoxy(7, 0);
            lcd_puts_UTF8("Внимание");
            lcd_gotoxy(1, 2);
            lcd_puts_UTF8("Провод");
            lcd_puts(" ");
            lcd_puts_UTF8("закончился");
            lcd_puts("!!!");
            lcd_gotoxy(1, 4);
            lcd_puts("[");
            lcd_puts_UTF8("Старт");
            lcd_puts("] - ");
            lcd_puts_UTF8("Продолжить");
            lcd_gotoxy(1, 5);
            lcd_puts("[");
            lcd_puts_UTF8("Стоп");
            lcd_puts("]  - ");
            lcd_puts_UTF8("Отмена");

            while (STOP() && START())
            {
            };
            if (!START())
            {
                DRV_ON;
                lcd_clear();
                lcd_gotoxy(4, 1);
                lcd_puts_UTF8("Работаю");
                lcd_puts("...");
            }
        }
        if (!STOP())
        {
            DRV_OFF;
            TIM2->CH2CVR = set.knife_open;
            work_ = MODE_OFF;
            // DelayMs(1000);
            lcd_clear();
            break;
            // return;
        }
        DIR_FWD;

        DelayMs(1);

        if (set.wires_a > 0)
        {
            dest_count = (uint32_t)((float)set.wires_a * (float)6.25);
            tim_flag = 0;
            TIM1->CTLR1 |= TIM_CEN;
            while (tim_flag == 0)
                ;
            TIM2->CH2CVR = set.knife;
            DelayMs(KNIFE_TIME);
            TIM2->CH2CVR = set.knife_open;
            DelayMs(KNIFE_TIME);
        }

        DelayMs(10);
        if (set.wires_b > 0)
        {
            dest_count = (uint32_t)(float)(set.wires_b * (float)6.25);
            tim_flag = 0;
            TIM1->CTLR1 |= TIM_CEN;
            while (tim_flag == 0)
                ;
        }
        DelayMs(10);
        if (set.wires_c > 0)
        {
            TIM2->CH2CVR = set.knife;
            DelayMs(KNIFE_TIME);
            TIM2->CH2CVR = set.knife_open;
            DelayMs(KNIFE_TIME);

            dest_count = (uint32_t)((float)set.wires_c * (float)6.25);
            tim_flag = 0;
            TIM1->CTLR1 |= TIM_CEN;
            while (tim_flag == 0)
                ;
        }
        TIM2->CH2CVR = set.knife_close;
        DelayMs(KNIFE_TIME);
        TIM2->CH2CVR = set.knife_open;
        //-------------Go backwar------
        DIR_BWD;
        DelayMs(10);

        dest_count = (uint32_t)((float)BACK_CONST * (float)6.25);
        tim_flag = 0;
        TIM1->CTLR1 |= TIM_CEN;
        while (tim_flag == 0)
            ;
        //-------------Go forward------
        DIR_FWD;
        DelayMs(50);
        dest_count = (uint32_t)((float)BACK_CONST * (float)6.25);
        tim_flag = 0;
        TIM1->CTLR1 |= TIM_CEN;
        while (tim_flag == 0)
            ;
        DelayMs(10);

        lcd_gotoxy(1, 2);
        lcd_puts_UTF8("Осталось");
        lcd_puts(": ");
        lcd_puts_int(qq - 1);

        lcd_puts(" ");
        lcd_gotoxy(1, 3);
        lcd_puts_UTF8("Сделано");
        lcd_puts(": ");
        lcd_puts_int((set.wires_qnt - qq) + 1);
        lcd_puts(" ");
    }
    DRV_OFF;
    work_ = MODE_OFF;
    lcd_gotoxy(1, 5);
    lcd_puts_UTF8("Вкалывают");
    lcd_puts(" ");
    lcd_puts_UTF8("роботы");
    lcd_puts(" ");
    lcd_gotoxy(1, 6);
    lcd_puts_UTF8("счастлив");
    lcd_puts(" ");
    lcd_puts_UTF8("инженер");
    DelayMs(2000);
    lcd_clear();
}

int main(void)
{
    init();
    lcd_init();
    lcd_clear();

    // out_port.out_motor = false;
    lcd_gotoxy(4, 3);

    lcd_puts_UTF8("Версия");
    lcd_puts(" ");
    lcd_puts_UTF8("ПО");

    lcd_puts(" ");
    lcd_puts(SW_REVISION);
    //---------------Вывод даты
    lcd_gotoxy(6, 4);
    lcd_puts(SW_DATE);
    if (!STOP())
    {
        lcd_gotoxy(4, 6);

        lcd_puts_UTF8("Сброс");
        lcd_puts(" ");
        lcd_puts_UTF8("настроек");

        set.wires_a = WIRE_DEF_A;
        set.wires_b = WIRE_DEF_B;
        set.wires_c = WIRE_DEF_C;
        set.wires_qnt = WIRE_DEF_QNT;
        set.knife = KNIFE_DEF_CUT;
        set.knife_open = KNIFE_OPEN;
        set.knife_close = KNIFE_CLOSE;

        WriteEEPROM();
    }
    DelayMs(3000);
    lcd_clear();
    ReadEEPROM();
    TIM2->CH2CVR = set.knife_open;

    while (1)
    {
        if (scan_on)
            key_scan();
        lcd_routine();
        work_routine();
        if (work_ == MODE_CUT)
            action();
    }
}

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    if (TimingDelay > 0)
    {
        TimingDelay--;
    }
    if (blink < 1000)
    {
        blink++;
    }
    else
    {
        blink = 0;
    }
    if (count_lcd < LCD_RESCAN)
    {
        count_lcd++;
    }
    else
    {
        count_lcd = 0;
        lcd_out_enable = true;
    }

    if ((!UP()) || (!RIGHT()) || (!DOWN()) || (!START()) || (!STOP()) || (!LEFT()))
    {
        if (but_press < 0xffff)
            but_press++;
    }
    else
    {
        but_press = 0;
    }
    scan_on = true;
    SysTick->SR = 0;
}

void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI0_IRQHandler(void)
{
}
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF)
    {
        step_counter++;
        TIM1->INTFR &= (~TIM_UIF);
        if (step_counter >= dest_count)
        {
            TIM1->CTLR1 &= (~TIM_CEN);
            tim_flag = 1;
            step_counter = 0;
            dest_count = 0;
            TIM1->CNT = 0;
        }
    }
}
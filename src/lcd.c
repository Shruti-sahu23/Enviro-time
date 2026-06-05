/******************************************************************************
 * File Name    : lcd.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file contains all LCD interface functions used in the
 * EnviroTime project.
 *
 * Features:
 * - LCD initialization
 * - Command transmission
 * - Character display
 * - String display
 * - Integer display
 * - Floating point display
 * - Line clearing utilities
 *
 * Hardware:
 * - 16x2 Character LCD
 * - 8-bit data interface
 *
 * LCD Connections:
 * D0-D7  -> P0.8 to P0.15
 * RS     -> P0.16
 * RW     -> P0.17
 * EN     -> P0.18
 *
 ******************************************************************************/

#include <LPC21xx.h>
#include "lcd.h"
#include "lcd_defines.h"
#include "defines.h"
#include "types.h"
#include "delays.h"
#include "pin_connections.h"

// --------------------------------------------------
// WriteLCD()
// Sends a raw byte to the LCD data bus and
// generates the enable pulse required for
// latching the data.
// --------------------------------------------------
void WriteLCD(u8 byte)
{
    IOCLR0 = (0xFF << LCD_DATA);

    IOSET0 = (byte << LCD_DATA);

    IOSET0 = (1 << LCD_EN);

    delay_us(1);

    IOCLR0 = (1 << LCD_EN);

    delay_ms(2);
}

// --------------------------------------------------
// CmdLCD()
// Sends a command instruction to the LCD.
// Examples:
// 0x01 -> Clear display
// 0x0C -> Display ON
// 0x80 -> First line start
// --------------------------------------------------
void CmdLCD(u8 cmd)
{
    IOCLR0 = 1 << LCD_RS;

    WriteLCD(cmd);
}

// --------------------------------------------------
// InitLCD()
// Configures LCD control/data pins and performs
// the standard LCD initialization sequence.
// --------------------------------------------------
void InitLCD(void)
{
    WRITEBYTE(IODIR0, LCD_DATA, 0xFF);

    SETBIT(IODIR0, LCD_RS);
    SETBIT(IODIR0, LCD_RW);
    SETBIT(IODIR0, LCD_EN);

    delay_ms(20);
    CmdLCD(0x38);
    CmdLCD(0x0C);
    CmdLCD(0x01);
    CmdLCD(0x06);
}

// --------------------------------------------------
// CharLCD()
// Displays a single ASCII character at the
// current cursor position.
// --------------------------------------------------
void CharLCD(u8 asciiVal)
{
    IOSET0 = 1 << LCD_RS;
    WriteLCD(asciiVal);
}

// --------------------------------------------------
// StrLCD()
// Displays a null-terminated string on the LCD.
// Characters are displayed sequentially from the
// current cursor position.
// --------------------------------------------------
void StrLCD(char *s)
{
    while(*s)
    {
        CharLCD(*s++);
    }
}

// --------------------------------------------------
// U32LCD()
// Displays an unsigned 32-bit integer value on
// the LCD by converting it into ASCII digits.
// --------------------------------------------------
void U32LCD(u32 n)
{
    s32 i = 0;

    u8 a[10];

    if(n == 0)
    {
        CharLCD('0');
        return;
    }

    while(n > 0)
    {
        a[i++] = (n % 10) + '0';

        n /= 10;
    }

    for(i = i - 1; i >= 0; i--)
    {
        CharLCD(a[i]);
    }
}

// --------------------------------------------------
// S32LCD()
// Displays a signed 32-bit integer value.
// Negative values are prefixed with '-'
// before displaying the magnitude.
// --------------------------------------------------
void S32LCD(s32 n)
{
    if(n < 0)
    {
        CharLCD('-');

        n = -n;
    }

    U32LCD(n);
}

// --------------------------------------------------
// F32LCD()
// Displays a floating point value with the
// specified number of decimal places.
//
// Example:
// F32LCD(28.56,1) -> 28.5
// F32LCD(28.56,2) -> 28.56
// --------------------------------------------------
void F32LCD(f32 fn,u8 nDP)
{
    u32 n,i;

    if(fn < 0.0)
    {
        CharLCD('-');

        fn = -fn;
    }

    n = fn;

    U32LCD(n);

    CharLCD('.');

    for(i=0;i<nDP;i++)
    {
        fn=(fn-n)*10;

        n=fn;

        CharLCD(n+48);
    }
}

// --------------------------------------------------
// ClearLineLCD()
// Clears the specified LCD line by overwriting
// all positions with blank spaces.
//
// line = 1 -> First line
// line = 2 -> Second line
// --------------------------------------------------
void ClearLineLCD(u8 line)
{
    u8 i;

    if(line == 1)
        CmdLCD(GOTO_LINE1_POS0);
    else
        CmdLCD(GOTO_LINE2_POS0);

    for(i = 0; i < 16; i++)
    {
        CharLCD(' ');
    }
}

// --------------------------------------------------
// LCD_Print()
// Moves the cursor to a specified LCD position
// and prints the supplied string.
// --------------------------------------------------
void LCD_Print(u8 pos, char *str)
{
    CmdLCD(pos);

    StrLCD(str);
}

// --------------------------------------------------
// LCD_ShowStars()
// Displays a specified number of '*' characters.
// Used mainly for password masking.
// --------------------------------------------------
void LCD_ShowStars(u8 count)
{
    u8 i;

    for(i=0;i<count;i++)
    {
        CharLCD('*');
    }
}


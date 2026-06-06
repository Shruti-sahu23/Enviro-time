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
    //clear prev data on LCD lines
    IOCLR0 = (0xFF << LCD_DATA);

    //Put new byte on LCD Bus
    IOSET0 = (byte << LCD_DATA);

    //Generate enable pulse
    IOSET0 = (1 << LCD_EN);

    delay_us(1);

    //Complete enable pulse
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
    //Select command register
    IOCLR0 = 1 << LCD_RS;

    //Send Command to LCD
    WriteLCD(cmd);
}

// --------------------------------------------------
// InitLCD()
// Configures LCD control/data pins and performs
// the standard LCD initialization sequence.
// --------------------------------------------------
void InitLCD(void)
{
    //configure LCD Data pins as outputs
    WRITEBYTE(IODIR0, LCD_DATA, 0xFF);

    //Configure RS,RW and EN pins as output
    SETBIT(IODIR0, LCD_RS);
    SETBIT(IODIR0, LCD_RW);
    SETBIT(IODIR0, LCD_EN);

    //LCD power-up delay
    delay_ms(20);
    CmdLCD(0x38);//Function set:MODE_8BIT_2LINE
    CmdLCD(0x0C);//Command: DSP_ON_CUR_OFF
    CmdLCD(0x01);//Command: CLEAR_LCD
    CmdLCD(0x06);//Command: SHIFT_CUR_RIGHT 
}

// --------------------------------------------------
// CharLCD()
// Displays a single ASCII character at the
// current cursor position.
// --------------------------------------------------
void CharLCD(u8 asciiVal)
{
    //Select Data Reg
    IOSET0 = 1 << LCD_RS;

    //Send ASCII character to LCD
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
    //Display chars until string ends
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

    //Special case for zero
    if(n == 0)
    {
        CharLCD('0');
        return;
    }

    //Extract digits from number
    while(n > 0)
    {
        //store digits in reverse order
        a[i++] = (n % 10) + '0';
        n /= 10;
    }

    //Display digits in correct order
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
    //Display negative sign if number is negative
    if(n < 0)
    {
        CharLCD('-');
        
        //make number positive
        n = -n;
    }

    //Display magnitude of number
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

    //Display sign for negative float values
    if(fn < 0.0)
    {
        CharLCD('-');

        //make float number positive
        fn = -fn;
    }

    //store float in integer variable
    n = fn;

    //Display int portion
    U32LCD(n);

    //Display dot(.)
    CharLCD('.');

    //Display fractional digits
    for(i=0;i<nDP;i++)
    {
        fn=(fn-n)*10;

        n=fn;

        // Display decimal digit
        CharLCD(n+48)
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

    // Position cursor at requested line
    if(line == 1)
        CmdLCD(GOTO_LINE1_POS0);
    else
        CmdLCD(GOTO_LINE2_POS0);

    // Overwrite all characters with spaces
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
    // Move cursor to requested location
    CmdLCD(pos);
    // Display string
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
    // Display required number of stars
    for(i=0;i<count;i++)
    {
        CharLCD('*');
    }
}


/******************************************************************************
 * File Name    : input.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file contains generic user input functions used throughout
 * the EnviroTime project.
 *
 * Features:
 * - Numeric input through keypad
 * - Input validation based on required digit count
 * - Backspace/Clear functionality
 * - Enter key processing
 *
 * These functions are reused by RTC, Alarm and Password modules
 * to simplify keypad-based data entry.
 *
 ******************************************************************************/

#include <LPC21xx.h>
#include "input2.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "keypad.h"
#include "alarm.h"
#include "delays.h"
#include "types.h"

// --------------------------------------------------
// InputDigits()
//
// Captures a fixed number of numeric digits from
// the keypad and stores them in the supplied buffer.
//
// Parameters:
// msg    -> Prompt displayed on LCD
// buf    -> Buffer to store entered digits
// digits -> Required number of digits
//
// Key Functions:
// 0-9 -> Enter digit
// C   -> Delete previous digit
// =   -> Confirm input
//
// Returns:
// 1 -> Valid input received
// --------------------------------------------------
u8 InputDigits(char *msg, u8 *buf, u8 digits)
{
	//current pos within input buffer
    u8 idx = 0;

	//Stores keypad key value
    char key;

	// Continuously display prompt and wait for keypad input until valid data is entered.
    while(1)
    {
        u8 i;

		//Clear both lines of LCD
        ClearLineLCD(1);
        ClearLineLCD(2);

		//Display prompt message
        LCD_Print(GOTO_LINE1_POS0, msg);
        
		CmdLCD(GOTO_LINE2_POS0);

		//Re display all previously entered digits
        for(i=0;i<idx;i++)
        {
            CharLCD(buf[i]);
        }

		//wait for keypad input
        key = GetKey();
		
		// Accept numeric digit if buffer is not full.
        if((key >= '0') && (key <= '9'))
        {	
			//store digits if buffer still has space
            if(idx < digits)
            {
                buf[idx++] = key;
            }
        }

        // CLEAR / BACKSPACE
		// Remove previously entered digit.
        else if(key == 'C')
        {
            if(idx > 0)
            {
                idx--;
            }
        }

        // ENTER
		// Accept input only when the required number of digits has been entered.
        else if(key == '=')
        {
            if(idx == digits)
            {
				//store last value as null
                buf[idx] = '\0';
                return 1;
            }
        }
    }
}

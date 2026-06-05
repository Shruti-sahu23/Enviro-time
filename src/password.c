/******************************************************************************
 * File Name    : password.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file implements the password protection subsystem of
 * the EnviroTime project.
 *
 * Features:
 * - Password verification
 * - Password change functionality
 * - Password masking using '*'
 * - Failed login attempt tracking
 * - System lockout after multiple failures
 * - Audible buzzer indication
 *
 * Default Password:
 * 1111
 *
 * Security Features:
 * - Maximum 3 incorrect attempts allowed
 * - System lockout for 10 seconds after repeated failures
 * - Password confirmation during update
 *
 ******************************************************************************/

#include "input2.h"
#include <LPC21xx.h>

#include "password.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "keypad.h"
#include "delays.h"
#include "alarm.h"
#include "pin_connections.h"

// --------------------------------------------------
// Password storage and security variables.
// --------------------------------------------------
static u8 stored_pass[PASS_LENGTH] = {'1','1','1','1'};

static u8 entered_pass[PASS_LENGTH];

static u8 attempts = 0;

// --------------------------------------------------
// Password_Init()
//
// Initializes password subsystem and resets
// failed login attempt counter.
// --------------------------------------------------

void Password_Init(void)
{
    attempts = 0;
}

// --------------------------------------------------
// GetPassword()
//
// Reads password input from keypad.
//
// Features:
// - Accepts numeric digits only
// - Displays '*' instead of actual digits
// - Supports backspace using 'C'
// - Completes input using '='
//
// Password characters are stored in the
// supplied buffer.
// --------------------------------------------------
// --------------------------------------------------
static void GetPassword(u8 *pass)
{
    u8 idx = 0;

    char key;

    CmdLCD(CLEAR_LCD);

    LCD_Print(GOTO_LINE1_POS0,"ENTER PASS");

    CmdLCD(GOTO_LINE2_POS0);

    while(1)
    {
        key = GetKey();

        // DIGIT
				// Store entered digit and display masked character.
        if((key >= '0') && (key <= '9'))
        {
            if(idx < PASS_LENGTH)
            {
                pass[idx++] = key;

                CharLCD('*');
            }
        }

        // CLEAR LAST DIGIT
				// Delete previously entered digit and update display.
        else if(key == 'C')
        {
            if(idx > 0)
            {
                idx--;

                CmdLCD(GOTO_LINE2_POS0);

                LCD_ShowStars(idx);

                CharLCD(' ');

                CmdLCD(GOTO_LINE2_POS0 + idx);
            }
        }

        // SUBMIT
				// Accept password only when required number of
				// digits has been entered.
        else if(key == '=')
        {
            if(idx == PASS_LENGTH)
            {
                return;
            }
        }
    }
}


// --------------------------------------------------
// Password_Verify()
//
// Verifies user password against stored password.
//
// Operation:
// 1. Prompt user for password.
// 2. Compare entered password with stored password.
// 3. Allow maximum of 3 attempts.
// 4. Lock system after repeated failures.
//
// Returns:
// 1 -> Password correct
// 0 -> Access denied
// --------------------------------------------------
u8 Password_Verify(void)
{
    u8 i;
    u8 remaining;

		// Allow user up to three password attempts.
    while(attempts < 3)
    {
        GetPassword(entered_pass);

        for(i = 0; i < PASS_LENGTH; i++)
        {
            if(entered_pass[i] != stored_pass[i])
            {
                break;
            }
        }

        // CORRECT PASSWORD
				// Password matched successfully.
        if(i == PASS_LENGTH)
        {
            attempts = 0;

            CmdLCD(CLEAR_LCD);

            StrLCD("ACCESS GRANTED");

            delay_ms(500);

            return 1;
        }

        // WRONG PASSWORD
        // Increment failed attempt counter.
				attempts++;

        remaining = 3 - attempts;

        CmdLCD(CLEAR_LCD);

        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("WRONG PASS");

        CmdLCD(GOTO_LINE2_POS0);

        if(remaining > 0)
        {
            StrLCD("LEFT:");

            U32LCD(remaining);
        }

				// Generate short buzzer indication for invalid password.
        IOSET0 = BUZZER;

        delay_ms(150);

        IOCLR0 = BUZZER;

        delay_ms(250);
    }

    // LOCK SYSTEM
		// --------------------------------------------------
		// Security Lockout
		//
		// Activated after three consecutive incorrect
		// password attempts.
		//
		// User must wait for 10 seconds before another
		// password verification attempt can be made.
		// --------------------------------------------------
    {
        u8 sec;

        CmdLCD(CLEAR_LCD);

        CmdLCD(GOTO_LINE1_POS0);
        StrLCD("SYSTEM LOCKED");

        IOSET0 = BUZZER;

			// Display countdown timer during lock period.
        for(sec = 10; sec > 0; sec--)
        {
            CmdLCD(GOTO_LINE2_POS0);

            StrLCD("WAIT ");

            if(sec < 10)
                CharLCD('0');

            U32LCD(sec);

            StrLCD(" SEC ");

            delay_ms(1000);
        }

        IOCLR0 = BUZZER;
    }

    attempts = 0;

    CmdLCD(CLEAR_LCD);

    StrLCD("TRY AGAIN");

    delay_ms(500);

    return 0;
}

// --------------------------------------------------
// Password_Change()
//
// Allows the user to update the system password.
//
// Procedure:
// 1. Verify current password.
// 2. Enter new password.
// 3. Re-enter password for confirmation.
// 4. Update stored password if both entries match.
//
// If confirmation fails, password remains unchanged.
// --------------------------------------------------
void Password_Change(void)
{
    u8 new_pass[PASS_LENGTH];
    u8 confirm_pass[PASS_LENGTH];

    u8 i;

		// Current password must be verified before
		// allowing password modification.
    if(!Password_Verify())
    {
        return;
    }

    CmdLCD(CLEAR_LCD);

    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("NEW PASS:");

    CmdLCD(GOTO_LINE2_POS0);

		// Capture new password from keypad.
    for(i = 0; i < PASS_LENGTH; i++)
    {
        char key = 0;

        while(key == 0)
        {
            Alarm_Task();

            key = KeyScan();
        }

        delay_ms(20);

        while(KeyScan() != 0)
        {
            Alarm_Task();
        }

        new_pass[i] = key;

        CharLCD('*');
    }

    CmdLCD(CLEAR_LCD);

    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("CONFIRM:");

    CmdLCD(GOTO_LINE2_POS0);

		// Re-enter password for confirmation.
    for(i = 0; i < PASS_LENGTH; i++)
    {
        char key = 0;

        while(key == 0)
        {
            Alarm_Task();

            key = KeyScan();
        }

        delay_ms(20);

        while(KeyScan() != 0)
        {
            Alarm_Task();
        }

        confirm_pass[i] = key;

        CharLCD('*');
    }

    for(i = 0; i < PASS_LENGTH; i++)
    {
				// Password confirmation failed.
				// New password will not be saved.
        if(new_pass[i] != confirm_pass[i])
        {
            CmdLCD(CLEAR_LCD);

            StrLCD("NOT MATCH");

            IOSET0 = BUZZER;

            delay_ms(400);

            IOCLR0 = BUZZER;

            return;
        }
    }

		// Store the new verified password.
    for(i = 0; i < PASS_LENGTH; i++)
    {
        stored_pass[i] = new_pass[i];
    }

    CmdLCD(CLEAR_LCD);

		// Notify user that password update was successful.
    StrLCD("PASS UPDATED");

    delay_ms(700);
}

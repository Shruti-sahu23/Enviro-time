/******************************************************************************
 * File Name    : keypad.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file implements the 4x4 matrix keypad interface used in
 * the EnviroTime project.
 *
 * Features:
 * - Keypad initialization
 * - Matrix keypad scanning
 * - Key press detection
 * - Key release detection
 * - Software debouncing
 *
 * Hardware Connections:
 * Rows    -> P1.16 to P1.19
 * Columns -> P1.20 to P1.23
 *
 * Keypad Layout:
 *
 *  7   8   9   /
 *  4   5   6   *
 *  1   2   3   -
 *  C   0   =   +
 *
 ******************************************************************************/

#include <LPC21xx.h>
#include "types.h"
#include "delays.h"
#include "alarm.h"
#include "pin_connections.h"


// Keypad LUT
// --------------------------------------------------
// Lookup table used to map row-column positions
// to corresponding keypad characters.
// --------------------------------------------------
char kpmLUT[4][4] = {
    {'7','8','9','/'},
    {'4','5','6','*'},
    {'1','2','3','-'},
    {'C','0','=','+'}
};

// --------------------------------------------------
// InitKPM()
//
// Configures keypad interface.
//
// Rows:
// Configured as outputs and driven HIGH during
// idle condition.
//
// Columns:
// Configured as inputs for key detection.
// --------------------------------------------------
void InitKPM(void)
{
    // Rows as OUTPUT
    IODIR1 |= ROW_MASK;

    // Columns as INPUT
    IODIR1 &= ~COL_MASK;

    // Set all rows HIGH (idle state)
    IOSET1 = ROW_MASK;
}

// --------------------------------------------------
// KeyScan()
//
// Scans all rows and columns of the keypad to
// determine whether a key is pressed.
//
// Working Principle:
// 1. Drive one row LOW at a time.
// 2. Read all column inputs.
// 3. If a column becomes LOW, determine the
//    corresponding key using the lookup table.
// 4. Apply software debounce before confirming.
//
// Returns:
// Key character if pressed.
// 0 if no key is detected.
// --------------------------------------------------
char KeyScan(void)
{
    u8 row, col;
		
	// Activate each row sequentially and check all column lines for a key press.
    for(row = 0; row < 4; row++)
    {
		//Keep all rows HIGH before scanning
        IOSET1 = ROW_MASK;

		//Drive current row LOW
        IOCLR1 = (1 << (16 + row));

		//Check all columns for key press
        for(col = 0; col < 4; col++)
        {
          	// Active LOW key detection.
			// A pressed key connects the active row to the corresponding column.  
			if(!(IOPIN1 & (1 << (20 + col))))
            	{
                	delay_ms(20);

				//confirm key still pressed after debounce
                if(!(IOPIN1 & (1 << (20 + col))))
                {
					//return corresponding char from LUT
                    return kpmLUT[row][col];
                }
            }
        }
    }
	//No key detected
    return 0;
}

// --------------------------------------------------
// GetKey()
//
// Waits for a complete key press event.
//
// Features:
// 1. Waits until a key is pressed.
// 2. Performs debounce delay.
// 3. Waits until the key is released.
// 4. Returns the detected key.
//
// Alarm_Task() is continuously executed while
// waiting so that alarm functionality remains
// responsive even during keypad operations.
// --------------------------------------------------
char GetKey(void)
{
    char key = 0;

    // WAIT FOR PRESS
	// Wait until the user presses a key.
    while(key == 0)
    {
		//Continue alarm monitoring while waiting
        Alarm_Task();
		//scan if key detected
        key = KeyScan();
    }
	//debounce delay after key press
    delay_ms(20);

    // WAIT FOR RELEASE
	// Wait until the user releases the key.
	// Prevents multiple detections of a single press.
    while(KeyScan() != 0)
    {
		//Continue alarm monitoring 
        Alarm_Task();
    }

    delay_ms(20);
	//return the key value
    return key;
}


/******************************************************************************
 * File Name    : main.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This is the main application file of the EnviroTime system.
 * It integrates all project modules including:
 *
 * 1. Real Time Clock (RTC)
 * 2. Alarm Management System
 * 3. Password Protection
 * 4. LM35 Temperature Monitoring
 * 5. LCD Display Interface
 * 6. Matrix Keypad Interface
 *
 * The application continuously displays the current time, date,
 * day and temperature. Users can access a menu-driven interface
 * to modify RTC settings, configure alarms and change passwords.
 *
 * Author : Shruti sahu
 ******************************************************************************/
#include <LPC21xx.h>
#include "lcd.h"
#include "lcd_defines.h"
#include "delays.h"
#include "rtc.h"
#include "keypad.h"
#include "alarm.h"
#include "password.h"
#include "pin_connections.h"
#include "adc2.h"
// --------------------------------------------------

int i,j;
// --------------------------------------------------
// Define Modes for different operations 
// --------------------------------------------------
#define MODE_RTC      0
#define MODE_MENU     1
#define MODE_ALARM    2
#define MODE_PWD      3


// --------------------------------------------------
// Global system state variables
// Used for mode management, RTC display,
// temperature monitoring and alarm handling.
// --------------------------------------------------

// Stores current operating mode of the system
u8 mode = MODE_RTC;

// Used to detect mode changes and avoid LCD refresh flicker
u8 prev_mode = 255;

// Stores keypad input
u8 key = 0;

/ ADC raw value from LM35 sensor
u16 adc_val = 0;

// Converted sensor voltage
f32 voltage = 0.0;

// Calculated temperature in degree Celsius
f32 current_temp = 0.0;

// RTC time variables
s32 hour, min, sec;

// RTC date variables
s32 date, month, year;

// RTC day variable (0-6)
s32 day;

// Stores previous second value to update LCD once per second
s32 prev_sec = -1;

// Variables used for switch edge detection
u8 prev_sw = 0;
u8 curr_sw = 0;

// Stores Alarm1 and Alarm2 time values
u32 alarm1 = 0;
u32 alarm2 = 0;

// GLOBAL ALARM FLAG
extern u8 g_alarm_active;

// Stores menu entry time for timeout feature
u8 menu_start_sec = 0;

// Indicates whether menu timer has started
u8 menu_timer_started = 0;


/******************************************************************************
 * Function Name : main
 *
 * Description:
 * Entry point of the EnviroTime application.
 *
 * Flow:
 * 1. Initialize all peripherals.
 * 2. Display splash screen.
 * 3. Continuously execute alarm monitoring.
 * 4. Display RTC information.
 * 5. Handle menu navigation and user inputs.
 *
 * Return Value:
 * Never returns.
 ******************************************************************************/

int main(void)
{	// Configure menu switch and alarm acknowledge switch
	// as input pins.
    IODIR0 &= ~SW1;
    IODIR0 &= ~SW_ALARM;

	// Initialize all hardware peripherals required by the application.
	InitLCD();
    RTC_Init();
    InitKPM();
    Alarm_Init();
    Password_Init();
    Init_ADC();

  
    // Display startup screen while peripherals are being initialized.
	CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("EnviroTime");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("Initializing");
    CmdLCD(GOTO_LINE2_POS0+13);

	//Displaying the three ... with the interval of 500ms each
    for(i=0;i<3;i++)
    {
			for(j=0;j<i;j++)
         {    
                CharLCD('.');
                delay_ms(500);
         }
     }
    delay_ms(500);
    CmdLCD(CLEAR_LCD);

	// Infinite loop implementing the main application state machine.
		while(1)
    {
     	// Continuously monitor alarms irrespective of current mode
		//To check whether the current time matches with alarm time or snooze time
		Alarm_Task();
		
       	// Suspend normal system operation while alarm is active
        if(g_alarm_active)
        {
            continue;
        }
       
		/* Detect rising edge of menu switch.
		   Pressing the switch toggles between RTC display
		   mode and menu mode.
		*/

		// Read current switch state
        curr_sw = (IOPIN0 & SW1) ? 1 : 0;

		// Detect rising edge (new button press)
        if(curr_sw && !prev_sw)
        {
            delay_ms(20); // Debounce delay to eliminate switch bouncing

            if(IOPIN0 & SW1)
            {
                //wait for release
                while(IOPIN0&SW1);
				
                if(mode == MODE_RTC)
                {
					// Enter menu from RTC display
                    mode = MODE_MENU;
                   
                }
                else
                {        
					menu_timer_started=0;
					
					// Return to RTC display from any menu
                    mode = MODE_RTC;

					// Force RTC screen refresh
                    prev_sec=-1;
                    
                }
					// Force screen redraw after mode chang		
                    prev_mode=255;
                    CmdLCD(CLEAR_LCD);
			}
        }

        prev_sw = curr_sw;

        // ==========================================
        // MODE : RTC DISPLAY
        // ==========================================
				/* RTC display mode.
				Updates LCD only when second value changes in	
				order to minimize unnecessary LCD refreshes. 
				*/
        
		if(mode == MODE_RTC)
        {
			// Read current RTC time
            GetRTCTimeInfo(&hour, &min, &sec);

			// Read current day of week
            GetRTCDay(&day);

			// Update LCD only when second changes
            if((sec != prev_sec)||(prev_sec==-1))
                    {
             			// Store current second value
						prev_sec = sec;

                        // TIME
                        DisplayRTCTime(hour,min,sec);

            
                        
						/* 
						Read temperature from LM35 sensor connected to
						ADC channel AD0.1 (P0.28).
											
						LM35 output:
						10mV = 1°C
											
						Temperature calculation:
						Temperature = Voltage × 100
						*/

						// Read LM35 sensor using ADC channel 1
                        Read_ADC(1, &adc_val, &voltage);

						// LM35 provides 10mV per degree Celsius
                        current_temp = voltage * 100.0;

                        CmdLCD(0x80 + 9);

						// Display temperature on LCD
                        F32LCD(current_temp,1);

                        CharLCD(0xDF); // degree symbol
                        CharLCD('C');
                                            
                        // Read RTC date information
                        GetRTCDateInfo(&date,&month,&year);

						// Display date in DD/MM/YYYY format
                        DisplayRTCDate(date,month,year);

                        // Display current day of week
                        CmdLCD(GOTO_LINE2_POS0 + 12);

                    DisplayRTCDay(day);
}
        }

        // ==========================================
        // MODE : MENU
        // ==========================================
		/* 
		Main menu mode.
		 
		Allows user to:
		1. Edit RTC
		2. Configure alarms
		3. Change password
		4. Exit to RTC display
		*/
        else if(mode == MODE_MENU)
        {
                       if(menu_timer_started==0)
                       {
						   		// Start menu inactivity timer
                                menu_start_sec=SEC;
                                menu_timer_started=1;
                        }

                          // 10 sec timeout //
							/* 	
							Automatic menu timeout.
							If no key is pressed for 10 seconds,
						 	return to RTC display mode.
							*/
			
						// Check for 10-second inactivity timeout
                       if(((SEC-menu_start_sec+60)%60)>=10)
                       {
						   		// Automatically return to RTC display
                                mode=MODE_RTC;
                                prev_mode=255;
                                menu_timer_started=0;
                                CmdLCD(CLEAR_LCD);
                                prev_sec=-1;
                                continue;
                        }
						// Execute menu initialization only once
						if(prev_mode!=MODE_MENU)
						{
							CmdLCD(CLEAR_LCD);

							// Display menu options
                            LCD_Print(GOTO_LINE1_POS0, "1:RTC 2:ALARM");

                            LCD_Print(GOTO_LINE2_POS0, "3:PWD 4:EXIT");

                prev_mode = MODE_MENU;
            }

			// Scan keypad for menu selection
            key = KeyScan();

            if(key != 0)
            {   
				// Reset inactivity timer whenever key is pressed
				menu_start_sec=SEC;
                delay_ms(20);

                while(KeyScan() != 0);

                // RTC EDIT
                if(key == '1')
                {
					// Password protected RTC modification.
                    if(Password_Verify())
                    {
						// Open RTC editing menu after password verification
                        RTC_Edit();
                        menu_timer_started=0;
                        mode = MODE_RTC;
                        prev_mode = 255;

                        CmdLCD(CLEAR_LCD);
                    }
                }

                // ALARM MENU
                else if(key == '2')
                {
					// Password protected alarm configuration.
                    if(Password_Verify())
                    {
						// Open alarm configuration menu
                        mode = MODE_ALARM;
                        prev_mode = 255;

                        CmdLCD(CLEAR_LCD);
                    }
                }

                // PASSWORD CHANGE
                else if(key == '3')
                {
					// Allows user to update system password.
                    Password_Change();
                    prev_mode = 255;
                    CmdLCD(CLEAR_LCD);
                }

                // EXIT
				// Return to normal RTC display mode.
                else if(key == '4')
                {   
					menu_timer_started=0;
					// Exit menu and return to RTC display
                    mode = MODE_RTC;
                    prev_mode = 255;

                    CmdLCD(CLEAR_LCD);
                }
            }
        }

        // ==========================================
        // MODE : ALARM
        // ==========================================
        // Invokes the alarm setup menu where the user
        // can configure Alarm 1 and Alarm 2 timings.
        // After exiting the alarm menu, control returns
        // to the main menu.
        // ==========================================
				
        else if(mode == MODE_ALARM)
        {
			// Execute alarm configuration menu
            Alarm_Menu(&alarm1, &alarm2);

			// Return to main menu after alarm configuration
            mode = MODE_MENU;

            prev_mode = 255;

            CmdLCD(CLEAR_LCD);
        }

        // ==========================================
        // MODE : PASSWORD
        // ==========================================
        // Displays password-related information.
        // This mode can be extended for future
        // password management features.
        //
        // SW1 can be used to return to the
        // previous screen.
        // ==========================================
				
        else if(mode == MODE_PWD)
        {
			// Display password menu screen
            if(prev_mode != MODE_PWD)
            {
                CmdLCD(CLEAR_LCD);

                CmdLCD(GOTO_LINE1_POS0);
				// Show password menu heading
                StrLCD("PASSWORD");

                CmdLCD(GOTO_LINE2_POS0);
				// Display return instruction
                StrLCD("SW:BACK");

                prev_mode = MODE_PWD;
            }
        }
    }
}

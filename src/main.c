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
// MODES
// --------------------------------------------------
#define MODE_RTC      0
#define MODE_MENU     1
#define MODE_ALARM    2
#define MODE_PWD      3

/* --------------------------------------------------
GLOBAL VARIABLES
// --------------------------------------------------
// Global system state variables
// Used for mode management, RTC display,
// temperature monitoring and alarm handling.
// --------------------------------------------------
// --------------------------------------------------
*/

u8 mode = MODE_RTC;
u8 prev_mode = 255;

u8 key = 0;

//ADC VARIABLES
u16 adc_val = 0;
f32 voltage = 0.0;
f32 current_temp = 0.0;

// RTC VARIABLES
s32 hour, min, sec;
s32 date, month, year;
s32 day;

s32 prev_sec = -1;

// SWITCH VARIABLES
u8 prev_sw = 0;
u8 curr_sw = 0;

// ALARMS
u32 alarm1 = 0;
u32 alarm2 = 0;

// GLOBAL ALARM FLAG
extern u8 g_alarm_active;

//menu variables
u8 menu_start_sec=0;
u8 menu_timer_started=0;

// --------------------------------------------------
// MAIN
// --------------------------------------------------
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
{
    // ----------------------------------------------
    // SWITCH INPUTS
    // ----------------------------------------------
		// Configure menu switch and alarm acknowledge switch
		// as input pins.
    IODIR0 &= ~SW1;
    IODIR0 &= ~SW_ALARM;

    // ----------------------------------------------
    // INITIALIZE MODULES
    // ----------------------------------------------
		// Initialize all hardware peripherals required by
		// the application.
    
		InitLCD();
    RTC_Init();
    InitKPM();
    Alarm_Init();
    Password_Init();
    Init_ADC();

    // ----------------------------------------------
    // SPLASH SCREEN
    // ----------------------------------------------
    // Display startup screen while peripherals are being
		// initialized.
		
		CmdLCD(CLEAR_LCD);
    CmdLCD(GOTO_LINE1_POS0);
    StrLCD("EnviroTime");
    CmdLCD(GOTO_LINE2_POS0);
    StrLCD("Initializing");
    CmdLCD(GOTO_LINE2_POS0+13);
        
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

   

    // ----------------------------------------------
    // MAIN LOOP
    // ----------------------------------------------
		// Infinite loop implementing the main application
    // state machine.
    
		while(1)
    {
        // ==========================================
        // ALWAYS RUN ALARM TASK
        // ==========================================
        Alarm_Task();

        // ==========================================
        // IF ALARM ACTIVE
        // ==========================================
        if(g_alarm_active)
        {
            continue;
        }
        // ==========================================
        // SWITCH EDGE DETECTION
        // ==========================================
				/* Detect rising edge of menu switch.
					 Pressing the switch toggles between RTC display
				   mode and menu mode.
				*/
        curr_sw = (IOPIN0 & SW1) ? 1 : 0;

        if(curr_sw && !prev_sw)
        {
            delay_ms(20);

            if(IOPIN0 & SW1)
            {
                //wait for release
                while(IOPIN0&SW1);
                if(mode == MODE_RTC)
                {
                    mode = MODE_MENU;
                   
                }
                else
                {        
									  menu_timer_started=0;
                    mode = MODE_RTC;
                    prev_sec=-1;
                    
                }
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
            GetRTCTimeInfo(&hour, &min, &sec);

            GetRTCDay(&day);

            if((sec != prev_sec)||(prev_sec==-1))
                    {
                        prev_sec = sec;

                        // TIME
                        DisplayRTCTime(hour,min,sec);

            
                        // -------- TEMPERATURE --------
												/* Read temperature from LM35 sensor connected to
													 ADC channel AD0.1 (P0.28).
											
													 LM35 output:
													 10mV = 1°C
											
													 Temperature calculation:
													 Temperature = Voltage × 100
												*/
                        Read_ADC(1, &adc_val, &voltage);

                        current_temp = voltage * 100.0;

                        CmdLCD(0x80 + 9);

                        F32LCD(current_temp,1);

                        CharLCD(0xDF); // degree symbol
                        CharLCD('C');
                                            
                        // DATE
                        GetRTCDateInfo(&date,&month,&year);

                        DisplayRTCDate(date,month,year);

                        // DAY
                        CmdLCD(GOTO_LINE2_POS0 + 12);

                    DisplayRTCDay(day);
}
        }

        // ==========================================
        // MODE : MENU
        // ==========================================
				/* Main menu mode.
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
                                menu_start_sec=SEC;
                                menu_timer_started=1;
                        }

                          // 10 sec timeout //
													/* Automatic menu timeout.
														 If no key is pressed for 10 seconds,
														 return to RTC display mode.
													*/
                       if(((SEC-menu_start_sec+60)%60)>=10)
                       {
                                mode=MODE_RTC;
                                prev_mode=255;
                                menu_timer_started=0;
                                CmdLCD(CLEAR_LCD);
                                prev_sec=-1;
                                continue;
                        }
						if(prev_mode!=MODE_MENU)
						{
														CmdLCD(CLEAR_LCD);

                            LCD_Print(GOTO_LINE1_POS0, "1:RTC 2:ALARM");

                            LCD_Print(GOTO_LINE2_POS0, "3:PWD 4:EXIT");

                prev_mode = MODE_MENU;
            }

            key = KeyScan();

            if(key != 0)
            {   menu_start_sec=SEC;
                delay_ms(20);

                while(KeyScan() != 0);

                // RTC EDIT
                if(key == '1')
                {
										// Password protected RTC modification.
                    if(Password_Verify())
                    {
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
                    mode = MODE_RTC;
                    prev_mode = 255;

                    CmdLCD(CLEAR_LCD);
                }
            }
        }

        // ==========================================
        // MODE : ALARM
        // ==========================================
				// ==========================================
        // Invokes the alarm setup menu where the user
        // can configure Alarm 1 and Alarm 2 timings.
        // After exiting the alarm menu, control returns
        // to the main menu.
        // ==========================================
				
        else if(mode == MODE_ALARM)
        {
            Alarm_Menu(&alarm1, &alarm2);

            mode = MODE_MENU;

            prev_mode = 255;

            CmdLCD(CLEAR_LCD);
        }

        // ==========================================
        // MODE : PASSWORD
        // ==========================================
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
            if(prev_mode != MODE_PWD)
            {
                CmdLCD(CLEAR_LCD);

                CmdLCD(GOTO_LINE1_POS0);
                StrLCD("PASSWORD");

                CmdLCD(GOTO_LINE2_POS0);
                StrLCD("SW:BACK");

                prev_mode = MODE_PWD;
            }
        }
    }
}

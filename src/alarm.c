/******************************************************************************
 * File Name    : alarm.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file implements the alarm management subsystem of the
 * EnviroTime project.
 *
 * Features:
 * - Alarm 1 configuration
 * - Alarm 2 configuration
 * - Alarm triggering
 * - Alarm acknowledgement
 * - Automatic snooze operation
 * - Buzzer control
 *
 * Alarm Behaviour:
 * 1. User sets alarm time through keypad.
 * 2. Alarm triggers when RTC time matches alarm time.
 * 3. Buzzer sounds continuously.
 * 4. User can acknowledge alarm using alarm switch.
 * 5. If no acknowledgement is received within 10 seconds,
 *    the alarm automatically snoozes for 1 minute.
 *
 ******************************************************************************/

#include <LPC21xx.h>
#include<stdio.h>
#include "alarm.h"
#include "lcd.h"
#include "rtc.h"
#include "delays.h"
#include "lcd_defines.h"
#include "keypad.h"
#include<string.h>
#include "pin_connections.h"

#define MODE_RTC      0

u8 g_alarm_active = 0;

u8 pos = 0;

u32 snooze_time=0XFFFF;
u8 snooze_active=0;
u8 snooze_count=0;

extern u32 alarm1;
extern u32 alarm2;

extern u8 mode;
extern u8 prev_mode;

extern s32 prev_sec;


// --------------------------------------------------
 /*
 Alarm_Init()
 Configures alarm related hardware including:
 SW_ALARM -> Alarm acknowledge switch
 BUZZER   -> Alarm indication output
 Initializes buzzer to OFF state.
 */
// --------------------------------------------------
void Alarm_Init(void)
{
		PINSEL0&= ~(3<<2);
	
    PINSEL1&=~(3<<18);
    
    IODIR0 |= BUZZER;

    IODIR0 &= ~SW_ALARM;

    IOCLR0 = BUZZER;
}

// --------------------------------------------------
 /*
	Alarm_Menu()

 Allows the user to:
 1. View Alarm 1 and Alarm 2 timings.
 2. Configure alarm hours and minutes.
 3. Validate entered alarm values.
 4. Save alarm settings.

 Alarm values are internally stored as
 total minutes from midnight.
 
 Example:
 08:30 -> 510 minutes
 */
// --------------------------------------------------
void Alarm_Menu(u32 *a1, u32 *a2)
{
    char key;
    char line1[17];
                u8 a1h,a1m,a2h,a2m;

    while(1)
    {
        ClearLineLCD(1);
        ClearLineLCD(2);
        a1h=(*a1)/60;
        a1m=(*a1)%60;
                
        a2h=(*a2)/60;
        a2m=(*a2)%60;
                
			// Display currently configured alarm timings.
        sprintf(line1,"A1%02d:%02d A2%02d:%02d",a1h,a1m,a2h,a2m);
                
        LCD_Print(GOTO_LINE1_POS0,line1);
            
        LCD_Print(GOTO_LINE2_POS0,"1:A1 2:A2");

        LCD_Print(GOTO_LINE2_POS0+10,"3:BACK");

        key = GetKey();

        if(key == '3')
        {
            CmdLCD(CLEAR_LCD);
            return;
        }
				
				// User selected Alarm 1 or Alarm 2 for editing.
        if(key == '1' || key == '2')
        {
            char sel=key;
            char buf[6];
            strcpy(buf,"00:00");
            CmdLCD(CLEAR_LCD);
            LCD_Print(GOTO_LINE1_POS0,(sel=='1') ? "SET ALARM1     ": "SET ALARM     ");

            LCD_Print(GOTO_LINE2_POS0,buf);
            CmdLCD(GOTO_LINE2_POS0+pos);
            CmdLCD(0X0E);
                    
            while(1)
            {
               
                key = GetKey();
							
					// Move cursor forward while skipping ':' position.
                if(key == '+')
                {
                    if(pos < 4)
                    {
                        pos++;

                        if(pos == 2)
                            pos++;
                    }
                CmdLCD(GOTO_LINE2_POS0+pos);
                }
								
					// Move cursor backward while skipping ':' position.
                else if(key == '-')
                {
                    if(pos > 0)
                    {
                        pos--;

                        if(pos == 2)
                            pos--;
                    }
                 CmdLCD(GOTO_LINE2_POS0+pos);
                }

					// Reset alarm entry field to default value.
                else if(key == 'C')
                {
                    strcpy(buf,"00:00");
                                        LCD_Print(GOTO_LINE2_POS0,buf);
                                    pos=0;
                                    CmdLCD(GOTO_LINE2_POS0+pos);
                }
								
				 // Validate entered alarm time and save if valid.
                else if(key == '=')
                {
                    u8 hh;
                    u8 mm;

                    hh=((buf[0]-'0')*10)+(buf[1]-'0');

                    mm=((buf[3]-'0')*10)+(buf[4]-'0');
										
										// Reject invalid alarm times.
                    if(hh > 23 || mm > 59)
                    {
                        CmdLCD(CLEAR_LCD);
                        LCD_Print(GOTO_LINE1_POS0,
                        "INVALID TIME");
                        delay_ms(1000);
                        CmdLCD(CLEAR_LCD);
                        LCD_Print(GOTO_LINE1_POS0,(sel=='1')?"SET ALARM1":"SET ALARM2");
                        LCD_Print(GOTO_LINE2_POS0,buf);
                        CmdLCD(GOTO_LINE2_POS0+pos);
                        continue;
                    }

                    if(sel=='1')
                        *a1 = (hh*60)+mm;
                    else
                        *a2 = (hh*60)+mm;

                    CmdLCD(0x0C);

                    CmdLCD(CLEAR_LCD);

                    LCD_Print(GOTO_LINE1_POS0,
                    "ALARM SAVED");

                    delay_ms(1000);

                    break;
                }

                else if(key >= '0' && key <= '9')
                {
                    buf[pos] = key;
                    buf[5]='\0';
                    LCD_Print(GOTO_LINE2_POS0,buf);
                    CmdLCD(GOTO_LINE2_POS0+pos);

                }
            }
        }
    }
}

// --------------------------------------------------
 /*
 Alarm_Task()

 Background alarm monitoring task.

 Responsibilities:
 1. Read current RTC time.
 2. Compare RTC time with configured alarms.
 3. Trigger alarm when match occurs.
 4. Monitor snooze events.
 5. Handle alarm acknowledgement.
 6. Perform automatic snooze after timeout.

 This function is executed continuously from
 the main application loop.
 */
// --------------------------------------------------

void Alarm_Task(void)
{
    s32 hr,min,sec;

    static u8 alarm_ack = 0;
    static u8 alarm_screen=0;

    static u32 last_alarm_time = 0xFFFF;

    u32 cur;

    GetRTCTimeInfo(&hr,&min,&sec);
		
		// Convert current time into total minutes from midnight.
		// This simplifies alarm time comparison.
    cur = (hr*60)+min;//works for minutes snooze

		// Reset alarm acknowledgement when minute changes.
    // Allows future alarms to trigger normally.
    if(cur != last_alarm_time)
    {
        alarm_ack = 0;
        
    }

		 /*
			Check whether:

		 1. Current time matches Alarm 1.
		 2. Current time matches Alarm 2.
		 3. Snooze timer has expired.

		 If any condition becomes true,
		 activate alarm state.
		 */
    if((g_alarm_active == 0)&&((((cur == alarm1) || (cur == alarm2))&&(alarm_ack == 0))||
		((cur == snooze_time)&&(snooze_active)&&(sec == 0)&&(alarm_ack == 0)&&(g_alarm_active==0))))
		{
				g_alarm_active = 1;
        alarm_screen=0;
        snooze_active = 0;
				last_alarm_time = cur;

		}

		// Alarm active state.
		//
		// Displays alarm information and
		// continuously drives buzzer output.
    if(g_alarm_active)
    {
            
                char abuf[17];
                u32 atime;
                u8 ah,am;
                
                static u8 alarm_counter=0;
                static s32 last_sec=-1;
                
                if(cur==alarm1)
                    atime=alarm1;
                else if(cur==alarm2)
                    atime=alarm2;
                else
                    atime=snooze_time;
                
                ah=atime/60;
                am=atime%60;
								
								// Turn ON buzzer while alarm remains active.
								IOSET0 = BUZZER;
								
								// Display alarm screen only once
								// when alarm is first activated.
                if(alarm_screen==0)
                {
                
                CmdLCD(CLEAR_LCD);
								LCD_Print(GOTO_LINE1_POS0,"**** ALARM ****");
                
                sprintf(abuf,"ALARM %02d:%02d",ah,am);
								LCD_Print(GOTO_LINE2_POS0,abuf);
                alarm_screen=1;
                }
								
								// Count elapsed seconds while alarm is active.
								// Used for automatic snooze timing.
                
								if(last_sec!=sec)
                {
                last_sec=sec;
                alarm_counter++;
                }
								
								// Auto Snooze:
								//
								// If alarm remains unattended for 10 seconds,
								// stop buzzer and postpone alarm by 1 minute.
                
								if(alarm_counter>=10)
                {
                g_alarm_active=0;
                alarm_counter=0;
                snooze_time=cur+1;

                if(snooze_time>=1440)
                {
                snooze_time-=1440;
                }
                snooze_active=1;
                snooze_count++;
                alarm_ack=1;
                IOCLR0=BUZZER;
                CmdLCD(CLEAR_LCD);

                LCD_Print(GOTO_LINE1_POS0,"AUTO SNOOZE");
                delay_ms(500);
                CmdLCD(CLEAR_LCD);
                prev_sec=-1;
                prev_mode=255;
                mode=MODE_RTC;
                return;
                }
                
			// User acknowledgement.
			//
			// Pressing alarm switch stops buzzer and
			// prevents re-triggering during current minute.					
        
			if(IOPIN0 & SW_ALARM)
        {
            delay_ms(20);

            if(IOPIN0 & SW_ALARM)
            {
                IOCLR0 = BUZZER;

                g_alarm_active = 0;
                                alarm_screen=0;
                            
                alarm_ack = 1;

                

                CmdLCD(CLEAR_LCD);

                prev_sec = -1;
            }
        }
    }
    else
    {
        IOCLR0 = BUZZER;
    }
}

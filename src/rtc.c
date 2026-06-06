/******************************************************************************
 * File Name    : rtc.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file implements all Real Time Clock (RTC) related
 * functionality used in the EnviroTime project.
 *
 * Features:
 * - RTC initialization
 * - Time display (HH:MM:SS)
 * - Date display (DD/MM/YYYY)
 * - Day display (SUN-SAT)
 * - RTC time/date update
 * - Input validation for RTC settings
 *
 * RTC Hardware:
 * - Uses LPC2148 internal RTC peripheral.
 * - RTC runs continuously using backup battery.
 * - Time is retained even during power failure.
 *
 ******************************************************************************/

#include "rtc.h"
#include "lcd.h"
#include "lcd_defines.h"
#include "keypad.h"
#include "delays.h"
#include "alarm.h"
#include "input2.h"
#include "pin_connections.h"

// --------------------------------------------------
// RTC MACROS
// RTC Control Register Bit Definitions
// --------------------------------------------------
// --------------------------------------------------
#define RTC_ENABLE (1<<0)
#define RTC_RESET  (1<<1)
//#define RTC_CLKSRC (1<<4)

// --------------------------------------------------
// Day Lookup Table
//
// DOW Register Mapping:
// 0 -> SUN
// 1 -> MON
// 2 -> TUE
// 3 -> WED
// 4 -> THU
// 5 -> FRI
// 6 -> SAT
// --------------------------------------------------

char week[][4] =
{
    "SUN",
    "MON",
    "TUE",
    "WED",
    "THU",
    "FRI",
    "SAT"
};



// --------------------------------------------------
// RTC_Init()
//
// Initializes the LPC2148 RTC peripheral.
//
// Steps:
// 1. Enable RTC power.
// 2. Reset RTC.
// 3. Configure RTC prescaler values.
// 4. Load default time/date values.
// 5. Start RTC.
//
// Note:
// Default values should only be programmed
// during first-time setup. If backup battery
// is connected, RTC data is retained and
// should not be reinitialized.
// --------------------------------------------------
void RTC_Init(void)
{
		// Enable power to RTC peripheral.
        PCONP|=(1<<9);
        
		// Configure RTC prescaler for 32.768 kHz clock.
		// These values generate accurate 1-second timing.
        CCR=0X02;//RESET RTC
        
		PREINT=0X000001C8;
        PREFRAC=0X000061C0;

        // SET ONLY FIRST TIME
		//if CMOS battery connections provided no need to set again
        HOUR  = 10;
        MIN   = 48;
        SEC   = 45;

        DOM   = 21;
        MONTH = 5;
        YEAR  = 2026;

        DOW   = 0;
        CCR=0X01;

	}

// --------------------------------------------------
// GetRTCTimeInfo()
//
// Reads current RTC time registers and returns
// hour, minute and second values.
// --------------------------------------------------
void GetRTCTimeInfo(s32 *h, s32 *m, s32 *s)
{
    *h = HOUR;
    *m = MIN;
    *s = SEC;
}

// --------------------------------------------------
// DisplayRTCTime()
//
// Displays current time in HH:MM:SS format
// on LCD Line 1.
// --------------------------------------------------
void DisplayRTCTime(u32 h, u32 m, u32 s)
{
    CmdLCD(GOTO_LINE1_POS0);

    CharLCD((h/10)+'0');
    CharLCD((h%10)+'0');
    CharLCD(':');

    CharLCD((m/10)+'0');
    CharLCD((m%10)+'0');
    CharLCD(':');

    CharLCD((s/10)+'0');
    CharLCD((s%10)+'0');
}

// --------------------------------------------------
// GetRTCDateInfo()
//
// Reads current RTC date registers and returns
// day, month and year values.
// --------------------------------------------------
void GetRTCDateInfo(s32 *d, s32 *m, s32 *y)
{
    *d = DOM;
    *m = MONTH;
    *y = YEAR;
}

// --------------------------------------------------
// DisplayRTCDate()
//
// Displays current date in DD/MM/YYYY format
// on LCD Line 2.
// --------------------------------------------------
void DisplayRTCDate(u32 d, u32 m, u32 y)
{
    CmdLCD(GOTO_LINE2_POS0);

    CharLCD((d/10)+'0');
    CharLCD((d%10)+'0');
    CharLCD('/');

    CharLCD((m/10)+'0');
    CharLCD((m%10)+'0');
    CharLCD('/');

    U32LCD(y);
}

// --------------------------------------------------
// SetRTCTimeInfo()
//
// Updates RTC hour, minute and second registers.
// --------------------------------------------------
void SetRTCTimeInfo(u32 h, u32 m, u32 s)
{
    HOUR = h;
    MIN  = m;
    SEC  = s;
}

// --------------------------------------------------
// SetRTCDateInfo()
//
// Updates RTC date registers.
//
// Parameters:
// d -> Day of month
// m -> Month
// y -> Year
// --------------------------------------------------
void SetRTCDateInfo(u32 d, u32 m, u32 y)
{
    DOM   = d;
    MONTH = m;
    YEAR  = y;
}

// --------------------------------------------------
// GetRTCDay()
//
// Reads current Day Of Week value from RTC.
// --------------------------------------------------
void GetRTCDay(s32 *d)
{
    *d = DOW;
}

// --------------------------------------------------
// DisplayRTCDay()
//
// Displays abbreviated weekday name
// (SUN to SAT) on LCD.
// --------------------------------------------------
void DisplayRTCDay(u32 d)
{
    CmdLCD(GOTO_LINE2_POS0 + 11);

    StrLCD(week[d]);
}

// --------------------------------------------------
// SetRTCDay()
//
// Updates RTC Day Of Week register.
//
// Valid Values:
// 0 -> SUN
// 1 -> MON
// 2 -> TUE
// 3 -> WED
// 4 -> THU
// 5 -> FRI
// 6 -> SAT
// --------------------------------------------------
void SetRTCDay(u32 d)
{
    DOW = d;
}


// --------------------------------------------------
// RTC_Edit()
//
// Interactive RTC configuration screen.
//
// User can modify:
// - Hour
// - Minute
// - Second
// - Date
// - Month
// - Year
// - Day Of Week
//
// Key Functions:
// + -> Move cursor forward
// - -> Move cursor backward
// C -> Clear all fields
// = -> Save updated values
//
// All entered values are validated before
// updating RTC registers.
// --------------------------------------------------
void RTC_Edit(void)
{
    char key;

    char timeStr[9];
    char dateStr[11];
    char dayStr[2];

	// Editable character positions within
	// time/date entry screen.
	// Cursor skips ':' and '/' characters.
    u8 positions[] =
    {
        0,1,3,4,6,7,
        8,9,11,12,14,15,16,17,
        19
    };

    u8 index = 0;

    timeStr[0]=(HOUR/10)+'0';
    timeStr[1]=(HOUR%10)+'0';
    timeStr[2]=':';
    timeStr[3]=(MIN/10)+'0';
    timeStr[4]=(MIN%10)+'0';
    timeStr[5]=':';
    timeStr[6]=(SEC/10)+'0';
    timeStr[7]=(SEC%10)+'0';
    timeStr[8]='\0';

    dateStr[0]=(DOM/10)+'0';
    dateStr[1]=(DOM%10)+'0';
    dateStr[2]='/';
    dateStr[3]=(MONTH/10)+'0';
    dateStr[4]=(MONTH%10)+'0';
    dateStr[5]='/';

    dateStr[6]=((YEAR/1000)%10)+'0';
    dateStr[7]=((YEAR/100)%10)+'0';
    dateStr[8]=((YEAR/10)%10)+'0';
    dateStr[9]=(YEAR%10)+'0';

    dateStr[10]='\0';

    dayStr[0]=DOW+'0';
    dayStr[1]='\0';

    CmdLCD(CLEAR_LCD);

    LCD_Print(GOTO_LINE1_POS0,timeStr);

    LCD_Print(GOTO_LINE2_POS0,dateStr);

    LCD_Print(GOTO_LINE2_POS0+12,"D:");

    LCD_Print(GOTO_LINE2_POS0+14,dayStr);

    CmdLCD(0x0F);

    while(1)
    {
        u8 p = positions[index];

        if(p <= 7)
            CmdLCD(GOTO_LINE1_POS0+p);
        else if(p <= 17)
            CmdLCD(GOTO_LINE2_POS0+(p-8));
        else
            CmdLCD(GOTO_LINE2_POS0+14);

        key = GetKey();

		// Move editing cursor to next editable field.
        if(key == '+')
        {
            if(index < 14)
                index++;
        }
				
		// Move editing cursor to previous editable field.
        else if(key == '-')
        {
            if(index > 0)
                index--;
        }
		// Clear all editable fields and reset cursor.
        else if(key == 'C')
        {
            timeStr[0]='0';
            timeStr[1]='0';
            timeStr[3]='0';
            timeStr[4]='0';
            timeStr[6]='0';
            timeStr[7]='0';

            dateStr[0]='0';
            dateStr[1]='0';
            dateStr[3]='0';
            dateStr[4]='0';
            dateStr[6]='0';
            dateStr[7]='0';
            dateStr[8]='0';
            dateStr[9]='0';

            dayStr[0]='0';

            LCD_Print(GOTO_LINE1_POS0,timeStr);
            LCD_Print(GOTO_LINE2_POS0,dateStr);
            LCD_Print(GOTO_LINE2_POS0+14,dayStr);

            index = 0;
        }
				
		// Convert entered ASCII characters into
		// numeric time/date values and validate.
        else if(key == '=')
        {
            u8 hh,mm,ss,dd,mon,day;
            u16 yy;

            hh=((timeStr[0]-'0')*10)+(timeStr[1]-'0');
            mm=((timeStr[3]-'0')*10)+(timeStr[4]-'0');
            ss=((timeStr[6]-'0')*10)+(timeStr[7]-'0');

            dd=((dateStr[0]-'0')*10)+(dateStr[1]-'0');
            mon=((dateStr[3]-'0')*10)+(dateStr[4]-'0');

            yy=((dateStr[6]-'0')*1000)+((dateStr[7]-'0')*100)+ ((dateStr[8]-'0')*10)+ (dateStr[9]-'0');

            day=dayStr[0]-'0';

			// Validate entered RTC parameters.
			//
			// Time:
			// HH -> 00-23
			// MM -> 00-59
			// SS -> 00-59
			//
			// Date:
			// DD -> 01-31
			// MM -> 01-12
			//
			// Day:
			// 0-6
			//
			// Year:
			// 2000-2099
            if((hh>23 || mm>59||ss>59|| (dd<1||dd>31)||(mon<1||mon>12)||(day>6))||(yy<2000 ||yy>2099))
				{
					IOSET0=BUZZER;
					CmdLCD(CLEAR_LCD);
					LCD_Print(GOTO_LINE1_POS0,"INVALID DATA");
					delay_ms(1000);
					IOCLR0=BUZZER;
					CmdLCD(CLEAR_LCD);
							
					LCD_Print(GOTO_LINE1_POS0,timeStr);
					LCD_Print(GOTO_LINE2_POS0,dateStr);
					LCD_Print(GOTO_LINE2_POS0+12,"D:");
					LCD_Print(GOTO_LINE2_POS0+14,dayStr);
					continue;
				}
			
			// Save validated values into RTC registers.

            SetRTCTimeInfo(hh,mm,ss);

            SetRTCDateInfo(dd,mon,yy);

            SetRTCDay(day);

            CmdLCD(0x0C);

            CmdLCD(CLEAR_LCD);

			// Notify user that RTC update completed successfully.
            LCD_Print(GOTO_LINE1_POS0,"RTC UPDATED");

            delay_ms(1000);

            return;
        }

        else if(key>='0' && key<='9')
        {
            u8 p = positions[index];

            if(p <= 7)
            {
                timeStr[p]=key;

                //LCD_Print(GOTO_LINE1_POS0,timeStr);
                CmdLCD(GOTO_LINE1_POS0+p);
                CharLCD(key);
            }
            else if(p <= 17)
            {
                dateStr[p-8]=key;
                //LCD_Print(GOTO_LINE2_POS0,dateStr);
                CmdLCD(GOTO_LINE2_POS0+(p-8));
                CharLCD(key);
            }
            else
            {
                
                dayStr[0]=key;

                //LCD_Print(GOTO_LINE2_POS0+14,dayStr);
                CmdLCD(GOTO_LINE2_POS0+14);
                CharLCD(key);
                
                
            }
        }
    }
}

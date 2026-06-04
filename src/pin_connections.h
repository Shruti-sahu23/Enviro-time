#define LCD_DATA 8//@p0.8 to p0.15
#define LCD_RS 16//@p0.16
#define LCD_RW 17//@p0.17
#define LCD_EN 18//@p0.18

#define ROW_MASK (0xF << 16)
#define COL_MASK (0xF << 20)

#define BUZZER   (1<<19)
#define SW_ALARM (1<<1)

#define SW1 (1 << 7)

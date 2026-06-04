# ⏰ EnviroTime

An LPC2148-based embedded system that combines a Real-Time Clock (RTC), dual alarms, password-protected settings, and LM35 temperature monitoring with a 16x2 LCD and keypad interface.

## ✨ Features

* 🕒 Real-Time Clock (RTC)
* ⏰ Dual Alarm Support
* 🔔 Auto-Snooze Functionality
* 🔐 Password-Protected Settings
* 🌡️ LM35 Temperature Monitoring
* 📟 LCD-Based User Interface
* ⌨️ Keypad-Based Navigation
* 🚨 Buzzer Notifications

## 🛠️ Hardware Used

* LPC2148 Development Board
* 🌡️ LM35 Temperature Sensor
* 📟 16x2 LCD Display
* ⌨️ 4x4 Matrix Keypad
* 🔊 Buzzer
* 🔘 Push Buttons

## 🔌 Pin Connections

### 📟 LCD (16x2)

| LCD Pin | LPC2148 Pin |
| ------- | ----------- |
| D0      | P0.8        |
| D1      | P0.9        |
| D2      | P0.10       |
| D3      | P0.11       |
| D4      | P0.12       |
| D5      | P0.13       |
| D6      | P0.14       |
| D7      | P0.15       |
| RS      | P0.16       |
| EN      | P0.17       |
| RW      | GND       

### ⌨️ 4x4 Matrix Keypad

| Keypad Pin | LPC2148 Pin |
| ---------- | ----------- |
| Row 1      | P1.16       |
| Row 2      | P1.17       |
| Row 3      | P1.18       |
| Row 4      | P1.19       |
| Column 1   | P1.20       |
| Column 2   | P1.21       |
| Column 3   | P1.22       |
| Column 4   | P1.23       |

### 🌡️ LM35 Temperature Sensor

| LM35 Pin | LPC2148 Pin   |
| -------- | ------------- |
| VCC      | +5V           |
| VOUT     | P0.28 (AD0.1) |
| GND      | GND           |

### 🔔 Alarm & Control

| Device                   | LPC2148 Pin |
| ------------------------ | ----------- |
| Buzzer                   | P0.19       |
| Menu Switch              | P0.7        |
| Alarm Stop/Snooze Switch | P0.1        |


## 💻 Software Tools

* Keil µVision
* Proteus 8 Professional
* Flash Magic

## 📂 Project Modules

* 🕒 RTC Management
* ⏰ Alarm Management
* 🔐 Password Verification
* 🌡️ Temperature Monitoring
* 📟 LCD Driver
* ⌨️ Keypad Driver
* 📊 ADC Driver
  

### 🔄 Working Principle

### System Flow

```text
Power ON
   │
   ▼
Initialize LCD, RTC, Keypad, ADC, Alarm & Password Modules
   │
   ▼
Display Current Time, Date and Temperature
   │
   ▼
Press Menu Switch (P0.7)?
   │
 ┌─┴─┐
 │No │────────────► Continue Displaying RTC Screen
 └─┬─┘
   │Yes
   ▼
Display Main Menu
   │
   ▼
1: RTC Edit
2: Alarm Settings
3: Password Change
4: Exit
```

### 📟 Default RTC Screen

After power-up, the system initializes all peripherals and displays:

* Current Time
* Current Date
* Current Day
* Temperature from LM35 Sensor

Example:

```text
10:48:25 29.5°C
04/06/2026 THU
```

---

### 🔘 Menu Navigation

Press the Menu Switch connected to P0.7.

The LCD displays:

```text
1:RTC 2:ALARM
3:PWD 4:EXIT
```

If no key is pressed for 10 seconds, the system automatically returns to the RTC display.

---

### 🕒 RTC Edit Mode

Select Option 1.

The user is prompted for password verification.

After successful authentication:

* Time can be edited
* Date can be edited
* Day can be edited

Validation checks:

* Hours: 0–23
* Minutes: 0–59
* Seconds: 0–59
* Date: 1–31
* Month: 1–12
* Year: 2000–2099
* Day: 0–6

Invalid entries trigger:

* LCD message: INVALID DATA
* Buzzer indication

---

### ⏰ Alarm Configuration

Select Option 2.

The user can configure:

* Alarm 1
* Alarm 2

Alarm time is entered using the keypad.

When RTC time matches alarm time:

* Buzzer activates
* Alarm screen is displayed

```text
**** ALARM ****
ALARM HH:MM
```

---

### 🔔 Auto Snooze Function

If the alarm is not acknowledged:

* Alarm rings for 10 seconds
* Alarm automatically snoozes
* Alarm is rescheduled for 1 minute later

The LCD displays:

```text
AUTO SNOOZE
```

---

### 🛑 Alarm Stop

Press the Alarm Switch connected to P0.1.

Actions performed:

* Buzzer OFF
* Alarm acknowledged
* System returns to RTC display

---

### 🔐 Password Protection

Select Option 3 to change password.

Features:

* Default password: 1111
* Password required before modifying RTC or alarms
* Maximum 3 incorrect attempts

After 3 wrong attempts:

```text
SYSTEM LOCKED
WAIT 60 SEC
```

The buzzer remains active during lockout.

---

### 🌡️ Temperature Monitoring

The LM35 sensor is connected to ADC channel AD0.1 (P0.28).

Process:

1. ADC samples LM35 output voltage.
2. Voltage is converted to temperature.
3. Temperature is displayed on LCD.

Formula used:

```text
Temperature (°C) = Voltage × 100
```

Example:

```text
0.30V → 30°C
0.35V → 35°C
```

---

## 📁 Folder Structure

```text
Source Files
├── main.c
├── rtc.c
├── alarm.c
├── keypad.c
├── password.c
├── input.c
├── adc.c
```

## 🚀 Future Enhancements

* 💾 EEPROM-Based Password Storage
* ⏰ Multiple Alarm Profiles
* 🌡️ Temperature Alert System
* 📡 Serial Communication Logging
* ☁️ IoT Connectivity


## 👨‍💻 Author

Shruti Sahu


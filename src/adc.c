/******************************************************************************
 * File Name    : adc.c
 * Project      : EnviroTime
 * Target MCU   : LPC2148
 *
 * Description:
 * This file implements Analog-to-Digital Converter (ADC)
 * functions used in the EnviroTime project.
 *
 * Features:
 * - ADC initialization
 * - ADC channel selection
 * - Analog voltage measurement
 * - ADC result conversion
 *
 * Application:
 * LM35 Temperature Sensor Interface
 *
 * Hardware Connection:
 * LM35 Output -> P0.28 (AD0.1)
 *
 * Conversion:
 * ADC Value -> Voltage -> Temperature
 *
 * LM35 Output:
 * 10mV / °C
 *
 * Example:
 * 250mV -> 25°C
 * 300mV -> 30°C
 *
 ******************************************************************************/

#include "types.h"
#include <LPC21xx.h>
#include "adc_defines.h"
#include "delays.h"
#include "adc.h"


// --------------------------------------------------
// Init_ADC()
//
// Configures LPC2148 ADC peripheral.
//
// Configuration:
// - P0.28 configured as AD0.1
// - ADC powered ON
// - ADC clock divider configured
//
// This function must be called once during
// system initialization.
// --------------------------------------------------
void Init_ADC(void)
{
    // P0.28 -> AD0.1
	// Configure P0.28 pin for ADC channel AD0.1.
    PINSEL1 &= ~(3 << 24);
    PINSEL1 |=  (1 << 24);

    // ADC configuration
	// Enable ADC and configure ADC clock.
    ADCR =
        (1 << PDN_BIT) |
        (CLKDIV << CLKDIV_BITS);
}

// ------------------------------------------------
// Read_ADC()
//
// Performs ADC conversion on the selected channel.
//
// Parameters:
// chNO -> ADC channel number
// dVal -> Raw 10-bit ADC result
// eAR  -> Equivalent analog voltage
//
// Operation:
// 1. Select ADC channel.
// 2. Start conversion.
// 3. Wait for conversion completion.
// 4. Read ADC result.
// 5. Convert ADC count into voltage.
//
// Voltage Formula:
//
// Voltage = (ADC_Count × VREF) / 1023
//
// Returns:
// dVal -> Raw ADC count
// eAR  -> Measured voltage
// --------------------------------------------------
void Read_ADC(u8 chNO, u16 *dVal, f32 *eAR)
{
    // Select requested ADC input channel.
    ADCR &= ~(0xFF);
    ADCR |= (1 << chNO);

    // Start ADC conversion.
    ADCR |= (1 << 24);
		
    // Wait until conversion completes.
	// DONE bit becomes 1 after conversion.
    while(((ADDR >> 31) & 1) == 0);

    // Extract 10-bit ADC conversion result.
    *dVal = (ADDR >> 6) & 0x3FF;

    // Convert ADC count into equivalent voltage.
    *eAR = (*dVal * 3.3f) / 1023.0f;

	// Stop conversion and clear START bits.
    ADCR &= ~(7 << 24);
}

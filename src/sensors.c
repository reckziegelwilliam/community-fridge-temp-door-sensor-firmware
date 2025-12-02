/**
 * @file sensors.c
 * @brief Temperature sensor implementation using the RP2040 ADC
 * 
 * This module configures and reads from the RP2040's built-in 12-bit ADC
 * to measure temperature from an analog sensor (TMP36 or similar).
 * 
 * RP2040 ADC Overview:
 * - 12-bit resolution (0-4095)
 * - 500 kS/s maximum sampling rate
 * - 4 external channels (ADC0-ADC3 on GPIO26-29) + 1 internal temp sensor
 * - Fixed 3.3V reference voltage (connected to the Pico's 3.3V rail)
 * 
 * Common Mistakes to Avoid:
 * 1. Forgetting to call adc_init() - the ADC is disabled by default
 * 2. Not configuring the GPIO for ADC use with adc_gpio_init()
 * 3. Reading from the wrong channel (channels are 0-indexed, not by GPIO number)
 */

#include "sensors.h"
#include "config.h"

// Pico SDK headers for hardware access
#include "hardware/adc.h"      // ADC peripheral functions
#include "hardware/gpio.h"     // GPIO configuration (used internally by adc_gpio_init)

/**
 * @brief Initialize the ADC for temperature sensing
 * 
 * The RP2040's ADC must be explicitly initialized before use.
 * This saves power when the ADC isn't needed.
 */
void sensors_init(void) {
    // Step 1: Initialize the ADC peripheral
    // This powers on the ADC and resets it to a known state.
    // Must be called before any other ADC functions.
    adc_init();
    
    // Step 2: Configure the GPIO pin for ADC use
    // This does several things:
    //   - Disables digital input (to reduce noise and power)
    //   - Disables pull-up/pull-down resistors
    //   - Connects the pin to the ADC input multiplexer
    // 
    // Note: GPIO26 = ADC0, GPIO27 = ADC1, GPIO28 = ADC2, GPIO29 = ADC3
    adc_gpio_init(TEMP_SENSOR_PIN);
    
    // Step 3: Select the ADC input channel
    // The ADC can only read one channel at a time. We select our
    // temperature sensor channel here. If you have multiple sensors,
    // you'd call adc_select_input() before each read.
    adc_select_input(TEMP_SENSOR_ADC_CHANNEL);
}

/**
 * @brief Read temperature from the TMP36 sensor
 * 
 * Conversion process:
 * 
 * 1. ADC raw value (0-4095) represents 0V to 3.3V
 *    voltage = raw_value * (3.3V / 4096)
 * 
 * 2. TMP36 outputs voltage linearly proportional to temperature:
 *    - 0.5V at 0°C
 *    - Increases by 10mV per °C (0.01V/°C)
 *    - So: temperature = (voltage - 0.5) / 0.01
 *    - Simplified: temperature = (voltage - 0.5) * 100
 * 
 * Example:
 *    ADC reads 620 → voltage = 620 * (3.3/4096) = 0.5V → temp = 0°C
 *    ADC reads 775 → voltage = 775 * (3.3/4096) = 0.625V → temp = 12.5°C
 */
float sensors_read_temperature_c(void) {
    // Ensure we're reading from the correct channel
    // (In case another part of the code changed it)
    adc_select_input(TEMP_SENSOR_ADC_CHANNEL);
    
    // Read the raw 12-bit ADC value (0-4095)
    // This is a blocking call but only takes ~2 microseconds
    uint16_t raw = adc_read();
    
    // Convert raw ADC value to voltage
    // The ADC reference is 3.3V and resolution is 12 bits (4096 levels)
    float voltage = (float)raw * (ADC_VREF / (float)ADC_RESOLUTION);
    
    // Convert voltage to temperature using TMP36 formula
    // TMP36 outputs 0.5V at 0°C and increases by 10mV/°C
    float temp_c = (voltage - TMP36_OFFSET_V) * TMP36_SCALE;
    
    return temp_c;
}

/**
 * @brief Validate a temperature reading
 * 
 * The TMP36 sensor has a specified operating range of -40°C to +125°C.
 * Readings outside this range indicate:
 *   - Sensor disconnected (usually reads near 0V or 3.3V → extreme temps)
 *   - Sensor damaged
 *   - Wrong sensor type connected
 *   - Wiring problem
 */
bool sensors_is_reading_valid(float temp_c) {
    return (temp_c >= TEMP_VALID_MIN_C) && (temp_c <= TEMP_VALID_MAX_C);
}


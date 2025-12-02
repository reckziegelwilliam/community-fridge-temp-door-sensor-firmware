/**
 * @file sensors.h
 * @brief Temperature sensor interface for the Community Fridge Probe
 * 
 * This module handles the ADC-based temperature sensor (TMP36 or similar).
 * It provides a clean API for initializing the ADC and reading temperature.
 * 
 * The implementation details (ADC configuration, voltage conversion, sensor
 * characteristics) are hidden from the rest of the application.
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>

/**
 * @brief Initialize the temperature sensor subsystem
 * 
 * This function must be called once at startup before any sensor reads.
 * It configures the ADC hardware and sets up the appropriate GPIO pin
 * for analog input.
 * 
 * What this does internally:
 *   1. Enables the ADC peripheral (it's off by default to save power)
 *   2. Configures the GPIO pin as ADC input (disables digital functions)
 *   3. Selects the correct ADC channel
 */
void sensors_init(void);

/**
 * @brief Read the current temperature from the sensor
 * 
 * Reads the ADC, converts to voltage, then converts to temperature using
 * the TMP36 formula: Temperature = (Voltage - 0.5V) * 100
 * 
 * @return Temperature in degrees Celsius as a float.
 *         Returns values in approximately the range -40°C to +125°C.
 *         Out-of-range values may indicate a sensor problem.
 * 
 * @note This function performs a blocking ADC read. On the RP2040,
 *       this takes approximately 2 microseconds, which is negligible.
 * 
 * @note For more accurate readings in noisy environments, consider
 *       averaging multiple samples. The app_logic module handles this
 *       by maintaining a rolling average of readings over time.
 */
float sensors_read_temperature_c(void);

/**
 * @brief Check if a temperature reading is valid
 * 
 * Validates that a temperature reading is within the expected range
 * for the sensor. Readings outside this range indicate a hardware problem.
 * 
 * @param temp_c Temperature reading to validate
 * @return true if the reading is plausible, false if it indicates an error
 */
bool sensors_is_reading_valid(float temp_c);

#endif // SENSORS_H


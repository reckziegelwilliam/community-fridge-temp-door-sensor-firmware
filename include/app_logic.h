/**
 * @file app_logic.h
 * @brief High-level application logic for the Community Fridge Probe
 * 
 * This module is the "brain" of the firmware. It coordinates:
 *   - Periodic sensor sampling
 *   - Temperature history and averaging
 *   - Status determination (OK, DOOR_OPEN, TOO_WARM, ERROR)
 *   - Serial telemetry output
 * 
 * Design Philosophy:
 * ------------------
 * This module owns the application state and makes decisions, while
 * delegating hardware access to the sensor/LED modules. This separation
 * makes the code easier to test and modify:
 * 
 *   - Want to change the temperature sensor? Modify sensors.c
 *   - Want to change the status thresholds? Modify config.h
 *   - Want to add networking? Add a new module and call it from here
 *   - Want to change the averaging algorithm? Modify this file
 */

#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "led_status.h"  // For status_t enum

/**
 * @brief Initialize the application logic module
 * 
 * Sets up internal state (history buffer, timing, etc.).
 * Does NOT initialize hardware - that's done by the individual
 * sensor modules which should be initialized before calling this.
 */
void app_init(void);

/**
 * @brief Main application update function
 * 
 * This function should be called regularly from the main loop.
 * It handles:
 *   1. Checking if it's time to sample sensors
 *   2. Reading sensors and updating history
 *   3. Computing rolling average
 *   4. Determining system status
 *   5. Updating LED status
 *   6. Printing telemetry (at configured intervals)
 * 
 * @param millis_since_boot Current time in milliseconds.
 *                          Used for timing sample intervals.
 * 
 * @note This function manages its own timing. It will only
 *       perform sensor reads at SAMPLE_INTERVAL_MS intervals,
 *       so it's safe to call every iteration of the main loop.
 */
void app_update(uint32_t millis_since_boot);

/**
 * @brief Get the current temperature reading
 * 
 * @return Most recent temperature in degrees Celsius
 */
float app_get_current_temp(void);

/**
 * @brief Get the rolling average temperature
 * 
 * @return Average of the last N samples (N = HISTORY_BUFFER_SIZE)
 */
float app_get_average_temp(void);

/**
 * @brief Get the current door state
 * 
 * @return true if door is open, false if closed
 */
bool app_get_door_open(void);

/**
 * @brief Get the current system status
 * 
 * @return Current status_t value
 */
status_t app_get_status(void);

/**
 * @brief Get the number of samples in the history buffer
 * 
 * During startup, this will be less than HISTORY_BUFFER_SIZE
 * until the buffer fills up.
 * 
 * @return Number of valid samples in history
 */
int app_get_sample_count(void);

#endif // APP_LOGIC_H


/**
 * @file door_sensor.h
 * @brief Door sensor interface for the Community Fridge Probe
 * 
 * This module handles a magnetic reed switch used to detect whether
 * the fridge door is open or closed.
 * 
 * Reed Switch Basics:
 * A reed switch contains two ferromagnetic contacts in a sealed glass tube.
 * When a magnet is nearby, the contacts are pulled together (closed circuit).
 * When the magnet is removed, the contacts spring apart (open circuit).
 * 
 * Typical installation:
 *   - Reed switch mounted on the fridge frame
 *   - Magnet mounted on the door
 *   - When door is closed, magnet is near switch → switch closed
 *   - When door opens, magnet moves away → switch opens
 * 
 * Non-blocking Debouncing:
 * This module uses a state machine approach for debouncing that does NOT
 * block the main loop. Call door_sensor_update() from the main loop, and
 * door_sensor_is_open() returns the last confirmed debounced state.
 */

#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initialize the door sensor GPIO
 * 
 * Configures the GPIO pin as an input with internal pull-up resistor.
 * Must be called once at startup before reading the door state.
 * 
 * Wiring requirements:
 *   - One terminal of the reed switch to the GPIO pin
 *   - Other terminal to GND
 *   - No external resistors needed (uses internal pull-up)
 */
void door_sensor_init(void);

/**
 * @brief Update the door sensor debounce state machine (non-blocking)
 * 
 * This function should be called from the main loop on every iteration.
 * It handles the timing and sampling for debouncing without blocking.
 * 
 * @param millis_since_boot Current time in milliseconds (from get_absolute_time)
 * 
 * The debounce logic:
 *   - Samples the GPIO at DEBOUNCE_INTERVAL_MS intervals
 *   - Tracks consecutive samples that match
 *   - Updates the confirmed state after DEBOUNCE_SAMPLES consistent readings
 */
void door_sensor_update(uint32_t millis_since_boot);

/**
 * @brief Check if the door is currently open
 * 
 * Returns the debounced state of the door sensor.
 * This function returns immediately without blocking.
 * 
 * @return true if door is open, false if door is closed
 * 
 * @note The returned state is updated by door_sensor_update(). State changes
 *       are confirmed after DEBOUNCE_SAMPLES consistent readings at
 *       DEBOUNCE_INTERVAL_MS intervals (~50ms with default settings).
 */
bool door_sensor_is_open(void);

/**
 * @brief Get the raw (non-debounced) door sensor state
 * 
 * For debugging purposes. Returns the instantaneous GPIO state
 * without any debouncing.
 * 
 * @return true if door appears open (GPIO high), false if closed (GPIO low)
 */
bool door_sensor_raw_state(void);

#endif // DOOR_SENSOR_H


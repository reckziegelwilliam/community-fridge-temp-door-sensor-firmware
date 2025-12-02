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
 */

#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <stdbool.h>

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
 * @brief Check if the door is currently open
 * 
 * Returns the debounced state of the door sensor.
 * 
 * @return true if door is open, false if door is closed
 * 
 * @note This function uses software debouncing to filter out
 *       electrical noise and mechanical bounce. The first call
 *       after a state change may take up to ~50ms to return
 *       the new state (configurable via DEBOUNCE_* in config.h).
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


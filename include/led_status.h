/**
 * @file led_status.h
 * @brief LED status indicator interface for the Community Fridge Probe
 * 
 * This module drives an LED with different blink patterns to indicate
 * the current system status. It's designed as a non-blocking state machine
 * that can be updated from the main loop without using delays.
 * 
 * Pattern Summary:
 *   STATUS_OK        → Solid ON (everything is fine)
 *   STATUS_DOOR_OPEN → Slow blink (1s on, 1s off)
 *   STATUS_TOO_WARM  → Fast blink (200ms on, 200ms off) 
 *   STATUS_ERROR     → Triple flash pattern (attention-getting)
 */

#ifndef LED_STATUS_H
#define LED_STATUS_H

#include <stdint.h>

/**
 * @brief System status values
 * 
 * These represent the possible states of the fridge probe.
 * The LED will display a different pattern for each state.
 * 
 * Priority order (highest to lowest):
 *   STATUS_ERROR > STATUS_DOOR_OPEN > STATUS_TOO_WARM > STATUS_OK
 */
typedef enum {
    STATUS_OK,          // All systems normal
    STATUS_DOOR_OPEN,   // Fridge door is open
    STATUS_TOO_WARM,    // Temperature exceeds threshold
    STATUS_ERROR        // Sensor or system error
} status_t;

/**
 * @brief Initialize the LED status subsystem
 * 
 * Configures the LED GPIO pin as an output and sets initial state.
 * Must be called once at startup.
 */
void led_status_init(void);

/**
 * @brief Set the current system status
 * 
 * Updates the internal status state. The LED pattern will change
 * on the next call to led_status_update().
 * 
 * @param status The new system status to display
 * 
 * @note This function only updates internal state. Call led_status_update()
 *       regularly to actually drive the LED patterns.
 */
void led_status_set(status_t status);

/**
 * @brief Get the current system status
 * 
 * @return The currently set status value
 */
status_t led_status_get(void);

/**
 * @brief Update the LED based on current status and time
 * 
 * This function implements a non-blocking state machine that handles
 * all the LED blink patterns. It should be called regularly from
 * the main loop (every 10-50ms is fine).
 * 
 * @param millis_since_boot Current time in milliseconds since boot.
 *                          Used for timing the blink patterns.
 *                          Get this from to_ms_since_boot(get_absolute_time())
 * 
 * How it works:
 *   - Compares current time to last state change time
 *   - Toggles LED when enough time has passed for current pattern
 *   - Different patterns use different timing values
 * 
 * This approach avoids blocking delays, allowing the main loop
 * to remain responsive for other tasks.
 */
void led_status_update(uint32_t millis_since_boot);

/**
 * @brief Convert status enum to string for debugging
 * 
 * @param status Status value to convert
 * @return Pointer to static string (do not free)
 */
const char* led_status_to_string(status_t status);

#endif // LED_STATUS_H


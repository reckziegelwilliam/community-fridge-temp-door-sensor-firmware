/**
 * @file door_sensor.c
 * @brief Door sensor implementation with software debouncing
 * 
 * Why Debouncing is Needed:
 * -------------------------
 * Mechanical switches (including reed switches) don't make clean transitions.
 * When the contacts open or close, they can "bounce" - rapidly making and
 * breaking contact several times over a few milliseconds. Without debouncing,
 * a single door open/close could register as multiple events.
 * 
 * Additionally, electrical noise on long wires can cause brief glitches
 * that look like state changes.
 * 
 * Debouncing Approach Used Here:
 * ------------------------------
 * We use a simple "N consistent samples" approach:
 *   1. Read the GPIO multiple times with short delays between
 *   2. Only accept a state change if all samples agree
 *   3. This filters out both mechanical bounce and electrical noise
 * 
 * Tradeoffs:
 *   - Simple and deterministic
 *   - Small delay in detecting state changes (~50ms with default settings)
 *   - Good enough for door monitoring (we don't need sub-millisecond response)
 * 
 * Alternative approaches (not used here):
 *   - Timer-based: Start a timer on first edge, ignore changes until timer expires
 *   - Interrupt-based: Use GPIO interrupt + timer for lower CPU usage
 *   - Hardware: Add capacitor across switch (but we want a software solution)
 */

#include "door_sensor.h"
#include "config.h"

// Pico SDK headers
#include "pico/stdlib.h"       // For sleep_ms(), includes gpio.h

/**
 * @brief Initialize the door sensor GPIO with internal pull-up
 * 
 * GPIO Configuration:
 *   - Direction: Input (we're reading from the switch)
 *   - Pull: Internal pull-up enabled
 * 
 * With pull-up and switch to GND:
 *   - Switch closed (magnet near) → GPIO pulled to GND → reads LOW (0)
 *   - Switch open (magnet away) → Pull-up pulls to 3.3V → reads HIGH (1)
 * 
 * So: HIGH = door open, LOW = door closed
 */
void door_sensor_init(void) {
    // Initialize GPIO pin
    gpio_init(DOOR_SENSOR_PIN);
    
    // Set as input (not output)
    gpio_set_dir(DOOR_SENSOR_PIN, GPIO_IN);
    
    // Enable internal pull-up resistor
    // The Pico has ~50kΩ internal pull-ups, which is suitable for most switches
    gpio_pull_up(DOOR_SENSOR_PIN);
    
    // Note: The pull-up means:
    // - When nothing is connected, the pin reads HIGH
    // - When the switch closes and connects to GND, the pin reads LOW
}

/**
 * @brief Read raw GPIO state without debouncing
 * 
 * Useful for debugging to see the actual hardware state.
 */
bool door_sensor_raw_state(void) {
    // gpio_get() returns true (1) if pin is HIGH, false (0) if LOW
    // HIGH = door open (switch open, pull-up active)
    // LOW = door closed (switch closed to GND)
    return gpio_get(DOOR_SENSOR_PIN);
}

/**
 * @brief Read debounced door state
 * 
 * Implementation:
 *   1. Take DEBOUNCE_SAMPLES readings
 *   2. Wait DEBOUNCE_INTERVAL_MS between each reading
 *   3. Count how many readings show "open"
 *   4. If all readings agree, that's the state
 *   5. If readings disagree, return the majority (or current raw state)
 * 
 * This approach:
 *   - Takes ~50ms (5 samples × 10ms) to confirm a state change
 *   - Filters out brief glitches (need consistent readings)
 *   - Simple to understand and debug
 * 
 * Note: This is a blocking function due to the sleep calls.
 * For a more advanced approach, you could make this non-blocking
 * by tracking state over multiple calls from the main loop.
 */
bool door_sensor_is_open(void) {
    int open_count = 0;
    
    // Take multiple samples
    for (int i = 0; i < DEBOUNCE_SAMPLES; i++) {
        if (gpio_get(DOOR_SENSOR_PIN)) {
            open_count++;
        }
        
        // Don't sleep after the last sample (unnecessary delay)
        if (i < DEBOUNCE_SAMPLES - 1) {
            sleep_ms(DEBOUNCE_INTERVAL_MS);
        }
    }
    
    // Require all samples to agree for a definitive state
    // If all samples show open, return true (open)
    // If all samples show closed, return false (closed)
    // If mixed, return the majority
    
    if (open_count == DEBOUNCE_SAMPLES) {
        // All samples show door is open
        return true;
    } else if (open_count == 0) {
        // All samples show door is closed
        return false;
    } else {
        // Mixed readings - return majority
        // This provides some hysteresis in noisy conditions
        return (open_count > (DEBOUNCE_SAMPLES / 2));
    }
}


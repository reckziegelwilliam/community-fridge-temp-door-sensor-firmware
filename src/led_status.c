/**
 * @file led_status.c
 * @brief LED status indicator with non-blocking blink patterns
 * 
 * State Machine Design:
 * ---------------------
 * Instead of using blocking delays (sleep_ms) to blink the LED, we use
 * a state machine that tracks:
 *   - Current status (what pattern to show)
 *   - Current LED state (on or off)
 *   - Last toggle time (when did we last change the LED)
 *   - Pattern-specific state (for complex patterns like triple-flash)
 * 
 * On each update() call, we check if enough time has passed to toggle
 * the LED. This allows the main loop to do other things while the LED
 * blinks "in the background".
 * 
 * Why Non-Blocking Matters:
 * -------------------------
 * If we used sleep_ms() to blink the LED, we couldn't:
 *   - Respond to button presses during the delay
 *   - Read sensors on schedule
 *   - Handle network events (in future versions)
 * 
 * The cooperative multitasking approach (polling in main loop) is simpler
 * than using an RTOS but requires all code to be non-blocking.
 */

#include "led_status.h"
#include "config.h"

// Pico SDK headers
#include "pico/stdlib.h"  // Includes GPIO functions

// =============================================================================
// Internal state
// =============================================================================

// Current status being displayed
static status_t current_status = STATUS_OK;

// Current physical LED state (true = on, false = off)
static bool led_on = false;

// Timestamp of last LED state change (for timing patterns)
static uint32_t last_toggle_ms = 0;

// For the ERROR triple-flash pattern: which flash are we on? (0-5)
// Pattern: ON-off-ON-off-ON-off-pause, so 6 states before repeat
static int error_flash_state = 0;

// =============================================================================
// Initialization
// =============================================================================

/**
 * @brief Initialize LED GPIO as output
 */
void led_status_init(void) {
    gpio_init(STATUS_LED_PIN);
    gpio_set_dir(STATUS_LED_PIN, GPIO_OUT);
    
    // Start with LED on (STATUS_OK = solid on)
    gpio_put(STATUS_LED_PIN, true);
    led_on = true;
}

// =============================================================================
// Status management
// =============================================================================

void led_status_set(status_t status) {
    if (status != current_status) {
        current_status = status;
        // Reset pattern state when status changes
        // This ensures patterns start from the beginning
        error_flash_state = 0;
        last_toggle_ms = 0;  // Will be set on next update
    }
}

status_t led_status_get(void) {
    return current_status;
}

// =============================================================================
// LED update state machine
// =============================================================================

/**
 * @brief Set the physical LED state
 */
static void set_led(bool on) {
    led_on = on;
    gpio_put(STATUS_LED_PIN, on);
}

/**
 * @brief Handle STATUS_OK pattern (solid on)
 */
static void update_ok(uint32_t millis) {
    (void)millis;  // Unused for solid-on pattern
    if (!led_on) {
        set_led(true);
    }
}

/**
 * @brief Handle simple blink pattern (used for DOOR_OPEN and TOO_WARM)
 * 
 * @param millis Current time
 * @param period_ms Time for one on+off cycle
 */
static void update_blink(uint32_t millis, uint32_t period_ms) {
    // Initialize timing on first call or after status change
    if (last_toggle_ms == 0) {
        last_toggle_ms = millis;
        set_led(true);
        return;
    }
    
    // Check if it's time to toggle
    uint32_t half_period = period_ms / 2;
    if ((millis - last_toggle_ms) >= half_period) {
        set_led(!led_on);
        last_toggle_ms = millis;
    }
}

/**
 * @brief Handle STATUS_DOOR_OPEN pattern (slow blink)
 */
static void update_door_open(uint32_t millis) {
    // Full cycle = 2 seconds (1s on, 1s off)
    update_blink(millis, LED_SLOW_BLINK_MS * 2);
}

/**
 * @brief Handle STATUS_TOO_WARM pattern (fast blink)
 */
static void update_too_warm(uint32_t millis) {
    // Full cycle = 400ms (200ms on, 200ms off)
    update_blink(millis, LED_FAST_BLINK_MS * 2);
}

/**
 * @brief Handle STATUS_ERROR pattern (triple flash)
 * 
 * Pattern: flash-gap-flash-gap-flash-gap-pause-repeat
 * 
 * States:
 *   0: LED on (flash 1)
 *   1: LED off (gap 1)
 *   2: LED on (flash 2)
 *   3: LED off (gap 2)
 *   4: LED on (flash 3)
 *   5: LED off (long pause)
 *   (then repeat from 0)
 */
static void update_error(uint32_t millis) {
    // Initialize timing on first call
    if (last_toggle_ms == 0) {
        last_toggle_ms = millis;
        set_led(true);
        error_flash_state = 0;
        return;
    }
    
    // Determine how long the current state should last
    uint32_t state_duration;
    if (error_flash_state == 5) {
        // Long pause after the 3 flashes
        state_duration = LED_ERROR_PAUSE_MS;
    } else {
        // Short flash or gap
        state_duration = LED_ERROR_FLASH_MS;
    }
    
    // Check if it's time to advance to next state
    if ((millis - last_toggle_ms) >= state_duration) {
        error_flash_state = (error_flash_state + 1) % 6;
        last_toggle_ms = millis;
        
        // Even states (0, 2, 4) = LED on, odd states (1, 3, 5) = LED off
        set_led((error_flash_state % 2) == 0);
    }
}

/**
 * @brief Main update function - dispatches to pattern handlers
 */
void led_status_update(uint32_t millis_since_boot) {
    switch (current_status) {
        case STATUS_OK:
            update_ok(millis_since_boot);
            break;
            
        case STATUS_DOOR_OPEN:
            update_door_open(millis_since_boot);
            break;
            
        case STATUS_TOO_WARM:
            update_too_warm(millis_since_boot);
            break;
            
        case STATUS_ERROR:
            update_error(millis_since_boot);
            break;
    }
}

// =============================================================================
// Utility functions
// =============================================================================

const char* led_status_to_string(status_t status) {
    switch (status) {
        case STATUS_OK:        return "OK";
        case STATUS_DOOR_OPEN: return "DOOR_OPEN";
        case STATUS_TOO_WARM:  return "TOO_WARM";
        case STATUS_ERROR:     return "ERROR";
        default:               return "UNKNOWN";
    }
}


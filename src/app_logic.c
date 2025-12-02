/**
 * @file app_logic.c
 * @brief Application logic implementation with circular buffer and status determination
 * 
 * Circular Buffer Explained:
 * --------------------------
 * We store temperature history in a circular (ring) buffer. This is a fixed-size
 * array where we overwrite the oldest entry when adding new data.
 * 
 * Example with buffer size 4:
 *   Initial:    [_, _, _, _]  head=0, count=0
 *   Add 10.0:   [10, _, _, _] head=1, count=1
 *   Add 11.0:   [10, 11, _, _] head=2, count=2
 *   Add 12.0:   [10, 11, 12, _] head=3, count=3
 *   Add 13.0:   [10, 11, 12, 13] head=0, count=4 (wrapped!)
 *   Add 14.0:   [14, 11, 12, 13] head=1, count=4 (overwrote oldest)
 * 
 * Benefits:
 *   - Fixed memory usage (no malloc/free)
 *   - O(1) add operation
 *   - Automatically discards old data
 *   - Perfect for rolling averages
 * 
 * Status Priority:
 * ----------------
 * When multiple conditions are true, we report the highest priority status:
 * 
 *   1. ERROR      - Something is wrong with the hardware (highest priority)
 *   2. DOOR_OPEN  - Door is open (important but not an error)
 *   3. TOO_WARM   - Temperature exceeds threshold
 *   4. OK         - Everything is normal (lowest priority)
 * 
 * This ensures that error conditions are always visible even if other
 * conditions would also apply.
 */

#include "app_logic.h"
#include "config.h"
#include "sensors.h"
#include "door_sensor.h"
#include "led_status.h"

#include <stdio.h>   // For printf (serial output)
#include <string.h>  // For memset

// =============================================================================
// Internal state
// =============================================================================

// Circular buffer for temperature history
static float temp_history[HISTORY_BUFFER_SIZE];
static int history_head = 0;   // Next position to write
static int history_count = 0;  // Number of valid entries (max = HISTORY_BUFFER_SIZE)

// Cached computed values
static float current_temp = 0.0f;
static float average_temp = 0.0f;
static bool door_open = false;
static status_t current_status = STATUS_OK;

// Timing state
static uint32_t last_sample_ms = 0;
static uint32_t last_telemetry_ms = 0;
static bool first_update = true;

// =============================================================================
// Internal helper functions
// =============================================================================

/**
 * @brief Add a temperature reading to the history buffer
 * 
 * Uses modulo arithmetic to wrap around when the buffer is full.
 * The oldest reading is automatically overwritten.
 */
static void add_to_history(float temp) {
    temp_history[history_head] = temp;
    history_head = (history_head + 1) % HISTORY_BUFFER_SIZE;
    
    if (history_count < HISTORY_BUFFER_SIZE) {
        history_count++;
    }
}

/**
 * @brief Calculate the average of all readings in the history buffer
 * 
 * @return Average temperature, or 0.0 if buffer is empty
 */
static float calculate_average(void) {
    if (history_count == 0) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (int i = 0; i < history_count; i++) {
        sum += temp_history[i];
    }
    
    return sum / (float)history_count;
}

/**
 * @brief Determine system status based on current readings
 * 
 * Priority order: ERROR > DOOR_OPEN > TOO_WARM > OK
 * 
 * The logic here is deliberately simple and easy to follow.
 * Each condition is checked independently, then combined by priority.
 */
static status_t determine_status(void) {
    // Check for sensor error first (highest priority)
    if (!sensors_is_reading_valid(current_temp)) {
        return STATUS_ERROR;
    }
    
    // Check door state (second priority)
    if (door_open) {
        return STATUS_DOOR_OPEN;
    }
    
    // Check temperature threshold (third priority)
    // Use the rolling average for stability (avoid flickering from noise)
    if (average_temp > TEMP_OK_MAX_C) {
        return STATUS_TOO_WARM;
    }
    
    // Everything is OK
    return STATUS_OK;
}

/**
 * @brief Print telemetry line to serial output
 * 
 * Format: t=4.3C, avg=4.1C, door=open, status=OK
 * 
 * This is designed to be easily parseable by both humans and scripts.
 * One reading per line, comma-separated fields.
 */
static void print_telemetry(void) {
    printf("t=%.1fC, avg=%.1fC, door=%s, status=%s\n",
           current_temp,
           average_temp,
           door_open ? "open" : "closed",
           led_status_to_string(current_status));
}

// =============================================================================
// Public API implementation
// =============================================================================

void app_init(void) {
    // Clear the history buffer
    memset(temp_history, 0, sizeof(temp_history));
    history_head = 0;
    history_count = 0;
    
    // Reset state
    current_temp = 0.0f;
    average_temp = 0.0f;
    door_open = false;
    current_status = STATUS_OK;
    
    // Reset timing
    last_sample_ms = 0;
    last_telemetry_ms = 0;
    first_update = true;
}

void app_update(uint32_t millis_since_boot) {
    // On first update, initialize timing and take first sample immediately
    if (first_update) {
        last_sample_ms = millis_since_boot;
        last_telemetry_ms = millis_since_boot;
        first_update = false;
        
        // Take initial readings
        current_temp = sensors_read_temperature_c();
        add_to_history(current_temp);
        average_temp = calculate_average();
        door_open = door_sensor_is_open();
        current_status = determine_status();
        led_status_set(current_status);
        
        // Print initial telemetry
        printf("=== Fridge Probe Started ===\n");
        print_telemetry();
        return;
    }
    
    // Check if it's time to sample sensors
    if ((millis_since_boot - last_sample_ms) >= SAMPLE_INTERVAL_MS) {
        last_sample_ms = millis_since_boot;
        
        // Read sensors
        current_temp = sensors_read_temperature_c();
        door_open = door_sensor_is_open();
        
        // Update history and compute average
        add_to_history(current_temp);
        average_temp = calculate_average();
        
        // Determine and set status
        current_status = determine_status();
        led_status_set(current_status);
    }
    
    // Check if it's time to print telemetry
    if ((millis_since_boot - last_telemetry_ms) >= TELEMETRY_INTERVAL_MS) {
        last_telemetry_ms = millis_since_boot;
        print_telemetry();
    }
}

float app_get_current_temp(void) {
    return current_temp;
}

float app_get_average_temp(void) {
    return average_temp;
}

bool app_get_door_open(void) {
    return door_open;
}

status_t app_get_status(void) {
    return current_status;
}

int app_get_sample_count(void) {
    return history_count;
}


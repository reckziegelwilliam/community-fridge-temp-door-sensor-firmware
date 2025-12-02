/**
 * @file main.c
 * @brief Entry point for the Community Fridge Probe firmware
 * 
 * This file contains the main() function and the cooperative main loop.
 * It initializes all hardware modules and repeatedly calls the update
 * functions that drive the application.
 * 
 * Cooperative Multitasking:
 * -------------------------
 * This firmware uses a simple cooperative multitasking approach:
 *   - No RTOS (Real-Time Operating System)
 *   - Single main loop that runs forever
 *   - Each module manages its own timing internally
 *   - No module should block for extended periods
 * 
 * The main loop calls update functions rapidly (~10ms intervals).
 * Each update function checks if it's time to do work, does it if so,
 * and returns immediately. This keeps everything responsive.
 * 
 * Timing:
 * -------
 * The Pico SDK provides get_absolute_time() which returns a high-resolution
 * timestamp. We convert this to milliseconds for easier timing logic.
 * 
 * Future Extensions:
 * ------------------
 * This structure makes it easy to add new features:
 * 
 *   // Add a network module
 *   #include "network.h"
 *   network_init();
 *   // In main loop:
 *   network_update(millis);
 * 
 *   // Add a serial command parser
 *   #include "serial_cmd.h"
 *   serial_cmd_init();
 *   // In main loop:
 *   serial_cmd_process();
 * 
 *   // Add SD card logging
 *   #include "sd_logger.h"
 *   sd_logger_init();
 *   // In main loop:
 *   sd_logger_update(millis);
 */

#include <stdio.h>

// Pico SDK headers
#include "pico/stdlib.h"      // Standard library (stdio, gpio, time)
#include "pico/time.h"        // For get_absolute_time(), to_ms_since_boot()

// Application modules
#include "config.h"
#include "sensors.h"
#include "door_sensor.h"
#include "led_status.h"
#include "app_logic.h"

/**
 * @brief Get current time in milliseconds since boot
 * 
 * Helper function to convert Pico's absolute_time_t to a simple uint32_t.
 * 
 * Note: uint32_t can hold ~49 days worth of milliseconds before wrapping.
 * For a fridge monitor that might run for months, this could be an issue.
 * 
 * The app modules use relative timing (elapsed time since last event)
 * rather than absolute timestamps, which handles wraparound correctly
 * as long as the interval between checks is less than ~24 days.
 * 
 * For production code running indefinitely, consider using 64-bit timestamps.
 */
static inline uint32_t get_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

/**
 * @brief Main entry point
 * 
 * This function:
 *   1. Initializes the Pico's stdio (USB serial)
 *   2. Initializes all hardware modules
 *   3. Initializes application logic
 *   4. Runs the main loop forever
 */
int main(void) {
    // =========================================================================
    // STEP 1: Initialize stdio (USB serial output)
    // =========================================================================
    // 
    // This enables printf() to send output over USB.
    // The Pico will enumerate as a USB CDC (serial) device.
    // On the host, it appears as /dev/ttyACM0 (Linux) or a COM port (Windows).
    // 
    // Note: stdio_init_all() initializes both USB and UART stdio based on
    // the CMakeLists.txt configuration. We enabled USB and disabled UART.
    //
    stdio_init_all();
    
    // Give USB time to enumerate (optional but helps capture early output)
    // Without this, the first few printf()s might be lost if you connect
    // a serial monitor right after plugging in the Pico.
    sleep_ms(2000);
    
    // =========================================================================
    // STEP 2: Print startup banner
    // =========================================================================
    printf("\n");
    printf("========================================\n");
    printf("  Community Fridge Probe Firmware\n");
    printf("  v1.0 - Raspberry Pi Pico (RP2040)\n");
    printf("========================================\n");
    printf("\n");
    printf("Pin assignments:\n");
    printf("  Temperature (ADC): GPIO%d\n", TEMP_SENSOR_PIN);
    printf("  Door sensor:       GPIO%d\n", DOOR_SENSOR_PIN);
    printf("  Status LED:        GPIO%d\n", STATUS_LED_PIN);
    printf("\n");
    printf("Configuration:\n");
    printf("  Sample interval:   %d ms\n", SAMPLE_INTERVAL_MS);
    printf("  Telemetry interval: %d ms\n", TELEMETRY_INTERVAL_MS);
    printf("  History buffer:    %d samples\n", HISTORY_BUFFER_SIZE);
    printf("  Temp threshold:    %.1f C\n", TEMP_OK_MAX_C);
    printf("\n");
    
    // =========================================================================
    // STEP 3: Initialize hardware modules
    // =========================================================================
    // 
    // Order matters here! Some modules might depend on others being
    // initialized first. In our case, the order doesn't matter much,
    // but it's good practice to initialize low-level modules first.
    //
    printf("Initializing hardware...\n");
    
    // Initialize temperature sensor (ADC)
    printf("  - ADC (temperature sensor)... ");
    sensors_init();
    printf("OK\n");
    
    // Initialize door sensor (GPIO input)
    printf("  - GPIO (door sensor)... ");
    door_sensor_init();
    printf("OK\n");
    
    // Initialize LED (GPIO output)
    printf("  - GPIO (status LED)... ");
    led_status_init();
    printf("OK\n");
    
    // =========================================================================
    // STEP 4: Initialize application logic
    // =========================================================================
    printf("  - Application logic... ");
    app_init();
    printf("OK\n");
    
    printf("\n");
    printf("Initialization complete. Starting main loop.\n");
    printf("\n");
    
    // =========================================================================
    // STEP 5: Main loop (runs forever)
    // =========================================================================
    //
    // This is a simple cooperative loop:
    //   - Get current time
    //   - Update LED patterns (handles blinking)
    //   - Update application logic (handles sensor reading, status, telemetry)
    //   - Small sleep to prevent tight spinning
    //
    // The sleep_ms(10) creates roughly 10ms between iterations.
    // This is fast enough for responsive LED patterns and sensor reading,
    // but slow enough to not waste CPU cycles.
    //
    // If you add interrupt-driven features later, you might remove the sleep
    // and use tight_loop_contents() instead, or switch to an RTOS.
    //
    while (true) {
        // Get current time in milliseconds
        uint32_t millis = get_millis();
        
        // Update LED patterns (non-blocking)
        // This handles the blink timing for different status patterns
        led_status_update(millis);
        
        // Update application logic (non-blocking)
        // This handles sensor sampling, status determination, and telemetry
        app_update(millis);
        
        // Small delay to prevent tight spinning
        // 10ms gives us:
        //   - 100 iterations per second
        //   - Plenty fast for LED patterns (fastest is 100ms on/off)
        //   - Low CPU usage
        //   - Responsive feel
        //
        // Note: sleep_ms() puts the CPU in a low-power state briefly
        sleep_ms(10);
    }
    
    // Never reached, but good practice to include
    return 0;
}


/**
 * @file config.h
 * @brief Configuration constants for the Community Fridge Probe firmware
 * 
 * This file contains all configurable parameters for the firmware.
 * Adjust these values to match your hardware setup and requirements.
 * 
 * HARDWARE SETUP SUMMARY:
 * - Temperature sensor (TMP36 or similar) connected to GPIO26 (ADC0)
 * - Door reed switch connected to GPIO15 (uses internal pull-up)
 * - Status LED uses the on-board LED on GPIO25
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PIN ASSIGNMENTS
// =============================================================================

/**
 * Temperature Sensor Pin (ADC)
 * 
 * The Pico has 3 ADC-capable pins: GPIO26 (ADC0), GPIO27 (ADC1), GPIO28 (ADC2)
 * We use GPIO26 which corresponds to ADC channel 0.
 * 
 * Wiring for TMP36:
 *   - TMP36 pin 1 (left, flat side facing you): 3.3V
 *   - TMP36 pin 2 (middle): GPIO26
 *   - TMP36 pin 3 (right): GND
 */
#define TEMP_SENSOR_PIN         26
#define TEMP_SENSOR_ADC_CHANNEL 0   // GPIO26 = ADC0, GPIO27 = ADC1, GPIO28 = ADC2

/**
 * Door Reed Switch Pin
 * 
 * Any GPIO can be used. We use GPIO15.
 * The internal pull-up resistor is enabled, so:
 *   - When door is CLOSED: magnet keeps reed switch closed, pin reads LOW (0)
 *   - When door is OPEN: reed switch opens, pin reads HIGH (1) via pull-up
 * 
 * Wiring:
 *   - Reed switch terminal 1: GPIO15
 *   - Reed switch terminal 2: GND
 */
#define DOOR_SENSOR_PIN         15

/**
 * Status LED Pin
 * 
 * The Raspberry Pi Pico has an on-board LED connected to GPIO25.
 * No external wiring needed.
 */
#define STATUS_LED_PIN          25

// =============================================================================
// TIMING CONFIGURATION
// =============================================================================

/**
 * How often to sample sensors (in milliseconds)
 * 
 * 2000ms (2 seconds) is a reasonable interval for fridge monitoring.
 * Faster sampling uses more CPU but gives quicker response.
 * Slower sampling saves power if battery-operated.
 */
#define SAMPLE_INTERVAL_MS      2000

/**
 * How often to print telemetry to serial (in milliseconds)
 * 
 * We print every N samples. With 2000ms sample interval and 5000ms print interval,
 * we print roughly every 2-3 samples.
 */
#define TELEMETRY_INTERVAL_MS   5000

// =============================================================================
// HISTORY BUFFER CONFIGURATION
// =============================================================================

/**
 * Number of temperature samples to keep in the rolling buffer
 * 
 * With 2-second sampling, 32 samples = ~64 seconds of history.
 * The rolling average smooths out noise and brief temperature spikes.
 * 
 * Power of 2 makes modulo operations efficient (compiler can use bitmask).
 */
#define HISTORY_BUFFER_SIZE     32

// =============================================================================
// TEMPERATURE THRESHOLDS
// =============================================================================

/**
 * Maximum acceptable temperature (Celsius)
 * 
 * Refrigerators should typically maintain 0-4°C (32-40°F).
 * We set the threshold slightly higher to avoid false alarms
 * during normal door-open events.
 * 
 * If the rolling average exceeds this, status becomes TOO_WARM.
 */
#define TEMP_OK_MAX_C           7.0f

/**
 * Temperature range for sensor validation
 * 
 * Readings outside this range indicate a sensor problem (disconnected,
 * damaged, or wrong sensor type). This triggers STATUS_ERROR.
 */
#define TEMP_VALID_MIN_C        (-40.0f)    // TMP36 minimum
#define TEMP_VALID_MAX_C        (125.0f)    // TMP36 maximum

// =============================================================================
// DOOR SENSOR DEBOUNCING
// =============================================================================

/**
 * Number of consistent samples required to confirm door state change
 * 
 * Debouncing prevents false triggers from electrical noise or
 * mechanical bounce when the reed switch opens/closes.
 * 
 * With 10ms between samples and 5 required samples, state changes
 * are confirmed after ~50ms of consistent readings.
 */
#define DEBOUNCE_SAMPLES        5

/**
 * Time between debounce samples (in milliseconds)
 * 
 * Shorter intervals give faster response but may not filter
 * all mechanical bounce. 10ms is a good balance.
 */
#define DEBOUNCE_INTERVAL_MS    10

// =============================================================================
// LED BLINK PATTERNS (in milliseconds)
// =============================================================================

/**
 * LED pattern timing for different status states
 * 
 * STATUS_OK:        Solid ON (no blinking)
 * STATUS_DOOR_OPEN: Slow blink (1 second on, 1 second off)
 * STATUS_TOO_WARM:  Fast blink (200ms on, 200ms off)
 * STATUS_ERROR:     Triple flash (100ms on, 100ms off, repeat 3x, then 700ms off)
 */
#define LED_SLOW_BLINK_MS       1000    // Door open pattern
#define LED_FAST_BLINK_MS       200     // Too warm pattern
#define LED_ERROR_FLASH_MS      100     // Error pattern flash duration
#define LED_ERROR_PAUSE_MS      700     // Error pattern pause after 3 flashes

// =============================================================================
// ADC CONFIGURATION (for reference in sensors.c)
// =============================================================================

/**
 * ADC Reference Voltage
 * 
 * The Pico's ADC uses 3.3V as its reference voltage.
 * This is fixed by the hardware and cannot be changed.
 */
#define ADC_VREF                3.3f

/**
 * ADC Resolution
 * 
 * The RP2040's ADC is 12-bit, giving values from 0 to 4095.
 * Full scale (4095) corresponds to the reference voltage (3.3V).
 */
#define ADC_RESOLUTION          4096

/**
 * TMP36 Sensor Characteristics
 * 
 * The TMP36 outputs 0.5V at 0°C and increases by 10mV per °C.
 * Formula: Temperature = (Voltage - 0.5) * 100
 * 
 * Example calculations:
 *   - 0.5V   → (0.5 - 0.5) * 100 =   0°C
 *   - 0.75V  → (0.75 - 0.5) * 100 = 25°C
 *   - 1.0V   → (1.0 - 0.5) * 100 =  50°C
 *   - 0.1V   → (0.1 - 0.5) * 100 = -40°C (minimum)
 */
#define TMP36_OFFSET_V          0.5f    // Voltage at 0°C
#define TMP36_SCALE             100.0f  // °C per volt (1 / 10mV)

#endif // CONFIG_H


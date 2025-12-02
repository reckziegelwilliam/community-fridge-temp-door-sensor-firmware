# Community Fridge Probe Firmware

A temperature and door monitoring system for community fridges, running on the Raspberry Pi Pico (RP2040).

## Features

- **Temperature monitoring** via analog sensor (TMP36 or thermistor)
- **Door state detection** via magnetic reed switch with debouncing
- **Rolling average** calculation for stable readings
- **Visual status indication** via LED patterns
- **Serial telemetry** output over USB

## Hardware Requirements

| Component | Connection | Notes |
|-----------|------------|-------|
| Raspberry Pi Pico | - | RP2040 microcontroller |
| TMP36 temperature sensor | GPIO26 (ADC0) | Or any analog temp sensor |
| Magnetic reed switch | GPIO15 | Uses internal pull-up |
| Status LED | GPIO25 | On-board Pico LED |

### Wiring Diagram

```
                    Raspberry Pi Pico
                   ┌─────────────────┐
                   │                 │
     TMP36         │  GPIO26 (ADC0) ◄──── Temp sensor output
     ┌───┐         │                 │
     │ 1 ├─────────│  3V3           ──── Temp sensor VCC
     │ 2 ├─────────│  (see above)    │
     │ 3 ├─────────│  GND           ──── Temp sensor GND
     └───┘         │                 │
                   │  GPIO15        ◄──── Reed switch terminal 1
                   │  GND           ──── Reed switch terminal 2
                   │                 │
                   │  GPIO25        ──── On-board LED (built-in)
                   │                 │
                   │  USB           ──── Serial output to host
                   └─────────────────┘
```

**TMP36 Pinout** (flat side facing you):
- Pin 1 (left): VCC (3.3V)
- Pin 2 (middle): Signal output → GPIO26
- Pin 3 (right): GND

**Reed Switch**: Either terminal to GPIO15, other to GND. No polarity.

## Status LED Patterns

| Status | LED Pattern | Meaning |
|--------|-------------|---------|
| OK | Solid on | Temperature normal, door closed |
| DOOR_OPEN | Slow blink (1s on/off) | Fridge door is open |
| TOO_WARM | Fast blink (200ms on/off) | Temperature exceeds threshold |
| ERROR | Triple flash | Sensor error or disconnected |

## Serial Output Format

Telemetry is printed every 5 seconds (configurable):

```
t=4.3C, avg=4.1C, door=closed, status=OK
t=4.5C, avg=4.2C, door=open, status=DOOR_OPEN
t=8.1C, avg=7.3C, door=closed, status=TOO_WARM
```

## Building the Firmware

### Prerequisites

1. **Pico SDK** - The official Raspberry Pi Pico C/C++ SDK
2. **ARM toolchain** - `arm-none-eabi-gcc`
3. **CMake** - Build system (3.13+)
4. **Make or Ninja** - Build tool

### macOS Setup

```bash
# Install toolchain via Homebrew
brew install cmake
brew install --cask gcc-arm-embedded

# Clone the Pico SDK
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable (add to ~/.zshrc for persistence)
export PICO_SDK_PATH=~/pico-sdk
```

### Linux (Ubuntu/Debian) Setup

```bash
# Install toolchain
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential

# Clone the Pico SDK
cd ~
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Set environment variable (add to ~/.bashrc for persistence)
export PICO_SDK_PATH=~/pico-sdk
```

### Build Commands

```bash
# From the project root directory
mkdir build
cd build
cmake ..
make -j4

# Or using Ninja (faster):
cmake -G Ninja ..
ninja
```

The output file will be `build/fridge_probe.uf2`.

## Flashing the Pico

1. **Hold the BOOTSEL button** on the Pico
2. **Connect the Pico to USB** while holding BOOTSEL
3. **Release BOOTSEL** - The Pico appears as a USB mass storage device named `RPI-RP2`
4. **Copy the UF2 file** to the drive:

```bash
# macOS
cp build/fridge_probe.uf2 /Volumes/RPI-RP2/

# Linux
cp build/fridge_probe.uf2 /media/$USER/RPI-RP2/
```

The Pico will automatically reboot and start running the firmware.

## Viewing Serial Output

### macOS

```bash
# Find the device
ls /dev/tty.usbmodem*

# Connect (example - your device name may differ)
screen /dev/tty.usbmodem14201 115200
# Press Ctrl+A then K to exit screen
```

### Linux

```bash
# Find the device
ls /dev/ttyACM*

# Connect
screen /dev/ttyACM0 115200
# Or use minicom:
minicom -D /dev/ttyACM0 -b 115200
```

### Alternative: Python

```python
import serial
ser = serial.Serial('/dev/ttyACM0', 115200)
while True:
    print(ser.readline().decode().strip())
```

## Configuration

All configurable values are in `include/config.h`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `SAMPLE_INTERVAL_MS` | 2000 | Time between sensor reads |
| `TELEMETRY_INTERVAL_MS` | 5000 | Time between serial output |
| `HISTORY_BUFFER_SIZE` | 32 | Rolling average window size |
| `TEMP_OK_MAX_C` | 7.0 | Temperature alarm threshold |
| `DEBOUNCE_SAMPLES` | 5 | Door sensor debounce reads |

## Project Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.c                                   │
│  - Initialize all modules                                        │
│  - Run cooperative main loop                                     │
│  - Call update functions every ~10ms                             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                       app_logic.c                                │
│  - Owns application state (history, status)                      │
│  - Coordinates sensor reading                                    │
│  - Determines status priority                                    │
│  - Outputs serial telemetry                                      │
└─────────────────────────────────────────────────────────────────┘
           │                   │                    │
           ▼                   ▼                    ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│   sensors.c      │ │  door_sensor.c   │ │  led_status.c    │
│  - ADC init      │ │  - GPIO input    │ │  - GPIO output   │
│  - Read temp     │ │  - Debouncing    │ │  - Blink patterns│
│  - Convert °C    │ │  - Pull-up       │ │  - State machine │
└──────────────────┘ └──────────────────┘ └──────────────────┘
           │                   │                    │
           ▼                   ▼                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Pico SDK / Hardware                          │
│  hardware_adc.h    hardware_gpio.h    pico/stdlib.h             │
└─────────────────────────────────────────────────────────────────┘
```

### Module Responsibilities

| Module | Responsibility |
|--------|----------------|
| `main.c` | Entry point, init sequence, main loop |
| `app_logic.c` | Business logic, state management, telemetry |
| `sensors.c` | ADC configuration, temperature conversion |
| `door_sensor.c` | GPIO input, software debouncing |
| `led_status.c` | LED control, non-blocking blink patterns |
| `config.h` | All configurable constants |

## Extending the Firmware

### Adding Wi-Fi (Pico W)

1. Create `network.c` / `network.h`
2. Add `pico_cyw43_arch_lwip_poll` to CMake libraries
3. Call `network_init()` from main
4. Call `network_update(millis)` from main loop
5. Send data in `app_logic.c` or via new module

### Adding Calibration

1. Add calibration offset to `config.h`
2. Parse serial commands in main loop
3. Store calibration in flash (see Pico SDK flash API)

### Adding SD Card Logging

1. Add `sd_card.c` module using SPI
2. Log readings periodically from `app_logic.c`
3. Implement circular file buffer to avoid filling card

## Troubleshooting

### No Serial Output

- Wait 2-3 seconds after plugging in (USB enumeration)
- Check correct serial port (`ls /dev/tty*`)
- Verify USB cable supports data (not charge-only)

### Temperature Readings Wrong

- Check TMP36 wiring (VCC, signal, GND order)
- Verify 3.3V supply (not 5V - will damage ADC!)
- Check conversion formula for your sensor type

### Door Always Shows Open/Closed

- Test reed switch with multimeter
- Verify magnet is close enough when door closed
- Check GPIO pin assignment in `config.h`

### LED Not Working

- GPIO25 is the on-board LED - should work without wiring
- Check for bad status determination logic
- Add debug prints to see current status

## License

MIT License - See LICENSE file

## Version History

- **v1.0** - Initial release (sensors + LED + serial)

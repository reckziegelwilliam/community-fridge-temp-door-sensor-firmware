# ==============================================================================
# Community Fridge Probe Firmware
# CMakeLists.txt for Raspberry Pi Pico (RP2040) using the official Pico C SDK
# ==============================================================================

cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK (must be done before project())
# This file should be copied from $PICO_SDK_PATH/external/pico_sdk_import.cmake
include(pico_sdk_import.cmake)

# Define the project
project(fridge_probe C CXX ASM)

# Set C/C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
# This must be called after project() but before any target definitions
pico_sdk_init()

# ==============================================================================
# Define the executable target
# ==============================================================================

add_executable(fridge_probe
    src/main.c
    src/sensors.c
    src/door_sensor.c
    src/led_status.c
    src/app_logic.c
)

# Add the include directory for our header files
target_include_directories(fridge_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ==============================================================================
# Link required Pico SDK libraries
# ==============================================================================

target_link_libraries(fridge_probe
    pico_stdlib          # Standard library (GPIO, time, stdio)
    hardware_adc         # ADC hardware support for temperature sensor
    hardware_gpio        # GPIO hardware support
)

# ==============================================================================
# Configure stdio output
# ==============================================================================

# Enable USB serial output (appears as /dev/ttyACM0 or similar on host)
pico_enable_stdio_usb(fridge_probe 1)

# Disable UART serial output (we're using USB for simplicity)
# Set to 1 if you want to use the UART pins (GP0/GP1) instead
pico_enable_stdio_uart(fridge_probe 0)

# ==============================================================================
# Generate additional output files
# ==============================================================================

# Create map/bin/hex/uf2 files in addition to ELF
# The .uf2 file is what you drag-and-drop onto the Pico for flashing
pico_add_extra_outputs(fridge_probe)

# ==============================================================================
# Build configuration notes:
# ==============================================================================
#
# To build this project:
#   1. Set PICO_SDK_PATH environment variable to your Pico SDK location
#   2. mkdir build && cd build
#   3. cmake ..
#   4. make -j4 (or ninja if using Ninja generator)
#
# The output will be in build/fridge_probe.uf2
#
# To flash:
#   1. Hold BOOTSEL button on Pico
#   2. Connect Pico to USB
#   3. Release BOOTSEL (Pico appears as mass storage device)
#   4. Copy fridge_probe.uf2 to the RPI-RP2 drive
#
# ==============================================================================


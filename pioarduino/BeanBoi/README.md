# BeanBoi Cave Surveying Device

A high-precision ESP32-based cave surveying instrument that measures heading, inclination, roll, and distance using advanced sensor fusion and calibration algorithms.

## Quick Start

```bash
# Clone and navigate
git clone https://github.com/christiankuhlmann/SurveyingDevice.git
cd SurveyingDevice/pioarduino/BeanBoi

# Install Python dependencies for calibration analysis
pip install numpy matplotlib scipy

# Build and upload firmware (PlatformIO will auto-install ~500MB platform on first run)
pio run -t upload ; pio device monitor
```

**⚠️ Critical**: This project requires **Arduino 3.x** (included in custom platform). The custom ESP32 platform is strongly recommended but see "Build System" section for compatibility details.

## Overview

BeanBoi is a handheld surveying device designed for cave mapping, similar to commercial solutions like SAP5 and DistoX. It combines three sensor types to provide accurate spatial measurements:

- **Magnetometer (RM3100)**: High-precision magnetic field measurement for heading determination
- **Accelerometer (SCA3300)**: Precision inclination and roll measurement
- **Laser Rangefinder (LDK_2M)**: Time-of-flight distance measurement up to 2 meters

The device features sophisticated calibration algorithms using Eigen linear algebra library, real-time data display on a Waveshare OLED screen, and wireless data transmission via Bluetooth Low Energy (BLE).

## Features

### Measurement Capabilities
- **Heading**: Magnetic azimuth with calibration compensation
- **Inclination**: Vertical angle measurement
- **Roll**: Device rotation around the sighting axis
- **Distance**: Laser rangefinder measurements
- **Coordinate Output**: Automatic conversion to Cartesian coordinates (East, North, Up)

### Calibration System
- **Static Calibration**: Multi-position magnetometer and accelerometer alignment (375 sample positions)
- **Dynamic Laser Alignment**: Corrects for laser-sensor misalignment (8-position calibration)
- **Persistent Storage**: Calibration data saved to ESP32 NVS (non-volatile storage)
- **Quality Analysis**: Python scripts for calibration validation and error characterization

### User Interface
- **OLED Display**: Real-time measurement readout and status information
- **Button Navigation**: 5-button interface for menu navigation and measurement control
- **Visual Feedback**: Stabilization indicators and calibration progress
- **Shot History**: On-device storage and review of measurements

### Wireless Communication
- **BLE Interface**: Transmits measurements to companion app
- **Real-time Updates**: Streaming sensor data during measurements
- **Shot Transfer**: Wireless retrieval of stored survey data

## Hardware Requirements

### Components
- **ESP32 Development Board**: Feather ESP32 or compatible (dual-core, 240MHz, 520KB RAM)
- **RM3100 Magnetometer**: High-precision 3-axis magnetometer (SPI interface, ~0.01° heading resolution)
- **SCA3300 Inclinometer**: MEMS 3-axis accelerometer (SPI interface, ±0.001g accuracy)
- **LDK_2M Laser Rangefinder**: Time-of-flight distance sensor (up to 2 meters range)
- **Waveshare 2.42" OLED Display**: 128x64 pixel monochrome display (SSD1305 driver)
- **5 Tactile Buttons**: For user input and menu navigation
- **Power Supply**: 3.3V regulated (battery or USB, ~200mA typical operation)

### Pin Connections

**SPI Bus (Shared):**
- MOSI, MISO, SCK connected to both RM3100 and SCA3300
- Separate CS (chip select) pins for each sensor
- SPI configured for high-speed communication (up to 8MHz)

**GPIO Assignments:**
- LDK_2M laser module: TX/RX UART or GPIO trigger (see `LDK2MSensorConnection.h`)
- Button inputs: GPIO with internal pull-ups, interrupt-driven
- OLED display: I2C (SDA, SCL) or SPI depending on configuration

**Power Considerations:**
- Laser module draws ~100mA during measurement
- OLED ~20mA average
- Magnetometer and accelerometer ~10mA combined
- ESP32 ~80mA during active processing

*Note: See `src/*SensorConnection.cpp` files for specific pin definitions used in your build.*

## Software Architecture

### Core Components

#### Sensor Abstraction Layer
- Generic interfaces (`Accelerometer`, `Magnetometer`, `Laser`) in `lib/CaveSurveyDevice/Sensors.h`
- Hardware-specific implementations in `src/*SensorConnection.h`
- Modular design allows easy sensor replacement

#### Sensor Handler
- Central processing class (`SensorHandler`) orchestrates all sensor operations
- Applies calibration matrices and bias corrections
- Manages multi-step calibration workflows
- Implements measurement stabilization and averaging

#### Numerical Methods Library
- **Inertial Alignment**: Magnetometer-accelerometer calibration using least-squares optimization
- **Laser Alignment**: Corrects for mechanical misalignment between sensors
- **Curve Fitting**: Ellipsoid fitting and matrix decomposition algorithms
- Built on Eigen library for efficient linear algebra

#### FreeRTOS Task System
- **Core 0 Tasks**: Sensor processing, button handling, display updates
- **Core 1 Tasks**: BLE communication
- Task synchronization via queues and semaphores
- Interrupt-driven button handling

#### State Machine
- Device states: Idle, Measuring, Calibrating, History Review
- Display modes: Measurement, Calibration Progress, Menu Navigation
- Button-driven state transitions

### Data Flow

```
Raw Sensors → SensorHandler.update() → Apply Calibration → Stabilize → takeShot()
                                              ↓
                                    Display + BLE Transmission
                                              ↓
                                        NVS Storage
```

## Getting Started

### System Requirements Summary

| Component | Requirement | Purpose |
|-----------|-------------|---------|
| **Platform** | Custom ESP32 (pioarduino fork) | Arduino 3.x + ESP-IDF 5.x support |
| **Framework** | Arduino 3.x | Modern timer API, performance improvements |
| **ESP-IDF** | v5.x (or 4.x compatible) | Enhanced FreeRTOS features |
| **FreeRTOS** | 1000 Hz tick rate | Precise timing for sensor sampling |
| **Compiler** | GCC 11.2+ (or 8.4+) | C++17, Eigen optimizations |
| **Libraries** | ArduinoEigen 0.3.2+ | Linear algebra (calibration algorithms) |
| | ArduinoJson 7.2.1+ | Calibration data serialization |
| | NimBLE-Arduino 1.3.0+ | Bluetooth Low Energy communication |
| **Python** | 3.x + numpy, matplotlib, scipy | Calibration quality analysis |
| **RAM** | 520KB (ESP32 standard) | 400KB for compute task stack |
| **Flash** | 4MB minimum | Firmware + NVS storage |

### Prerequisites

**Required Software:**
- [PlatformIO](https://platformio.org/) IDE or CLI (recommended: VS Code with PlatformIO extension)
- Python 3.x with the following packages (for calibration analysis scripts):
  - `numpy`
  - `matplotlib`
  - `scipy`
- USB cable for ESP32 programming

**Platform Requirements:**
- **CRITICAL**: This project uses a custom ESP32 platform fork that provides Arduino 3.x framework support
- The custom platform includes ESP-IDF 5.x with enhanced FreeRTOS features
- **DO NOT** replace the platform URL in `platformio.ini` with standard `espressif32`

### Installation

1. Clone the repository:
```bash
git clone https://github.com/christiankuhlmann/SurveyingDevice.git
cd SurveyingDevice/pioarduino/BeanBoi
```

2. Install Python dependencies (for calibration analysis):
```bash
pip install numpy matplotlib scipy
```

3. Build the firmware:
```bash
pio run -e featheresp32
```

4. Upload to device:
```bash
pio run -t upload
```

5. Monitor serial output:
```bash
pio device monitor
```

Or combine upload and monitor:
```bash
pio run -t upload ; pio device monitor
```

**First-time setup notes:**
- PlatformIO will automatically download the custom ESP32 platform (~500MB)
- Library dependencies (ArduinoEigen, ArduinoJson, NimBLE-Arduino) are auto-installed
- Build time: ~2-3 minutes on first compile

### Configuration

#### Platform Configuration (`platformio.ini`)

```ini
[env:featheresp32]
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
board = featheresp32
framework = arduino
lib_deps = 
    hideakitai/ArduinoEigen@^0.3.2
    bblanchon/ArduinoJson@^7.2.1
    h2zero/NimBLE-Arduino@^1.3.0
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
```

**Key Configuration Details:**
- **Platform**: Custom fork provides Arduino 3.x (based on ESP-IDF 5.x)
- **Libraries**: 
  - ArduinoEigen v0.3.2+ for linear algebra operations
  - ArduinoJson v7.2.1+ for calibration data serialization
  - NimBLE-Arduino v1.3.0+ for Bluetooth Low Energy
- **Monitor filters**: Exception decoder enabled for better crash debugging

#### FreeRTOS Configuration (`sdkconfig.defaults`)

Critical FreeRTOS settings required for this project:

```
CONFIG_FREERTOS_HZ=1000                              # 1ms tick rate for precise timing
CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES=6   # (NOT currently required - see note below)
CONFIG_AUTOSTART_ARDUINO=y                           # Auto-start Arduino framework
```

These settings enable:
- High-resolution task notifications for interrupt handling
- Dual-core task pinning (`xTaskCreatePinnedToCore`)
- Inter-task communication via task notifications and queues
- Hardware timer interrupts for 5Hz display refresh

**Note on Task Notification Arrays:**

⚠️ **Current Implementation**: The code currently uses **only the default notification index (0)** via standard `xTaskNotifyWait()` and `xTaskNotifyFromISR()` functions. The `CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES=6` setting is **NOT strictly required** for the current codebase to function.

**Why it's configured anyway:**
- The custom ESP-IDF 5.x platform includes this by default
- Provides future expansion capability for indexed notifications
- No performance penalty when not used
- If you need to use standard `espressif32` platform (ESP-IDF 4.x), you can remove this config line from `sdkconfig.defaults` without affecting current functionality

**What are indexed notifications?** (FreeRTOS 10.4.0+ / ESP-IDF 5.x feature)
- Allows up to 32 notification values per task (configured to 6 here)
- Accessed via `xTaskNotifyWaitIndexed()`, `xTaskNotifyFromISRIndexed()`
- Current code doesn't use indexed versions - only uses default index 0

#### Task Architecture

FreeRTOS tasks are distributed across both ESP32 cores:

**Core 0 (Protocol & Sensors):**
- `computehandler`: Sensor processing, calibration (Priority 1, 400KB stack)
- `inputhandler`: Button interrupt handling (Priority 2, 10KB stack)
- `displayhandler`: 5Hz OLED refresh (Priority 3, 10KB stack)

**Core 1 (Wireless):**
- `bleTask`: BLE communication (Priority 1, 16KB stack)

**Stack Size Rationale:**
- Large compute stack (400KB) required for Eigen matrix operations during calibration
- `fitEllipsoid()` function uses ~30KB static buffers for least-squares solving
- Smaller stacks sufficient for I/O and display tasks

#### Library Dependencies

**ArduinoEigen (v0.3.2+)**
- Provides C++ template library for linear algebra operations
- Used for: Matrix multiplications, SVD decomposition, quaternion rotations
- Critical functions: `Eigen::JacobiSVD`, `Eigen::Vector3f`, `Eigen::Matrix3f`
- Why required: All calibration algorithms use least-squares matrix operations

**ArduinoJson (v7.2.1+)**
- JSON serialization/deserialization library
- Used for: Calibration data export, debugging output
- Functions: `dumpCalibToSerial()` outputs calibration data for Python analysis
- Why required: Enables calibration quality validation with external tools

**NimBLE-Arduino (v1.3.0+)**
- Lightweight Bluetooth Low Energy stack for ESP32
- Used for: Wireless measurement transmission, shot data retrieval
- Advantages over classic BLE: Lower memory footprint, better performance
- Why required: App communication protocol based on GATT characteristics

All dependencies are automatically installed by PlatformIO based on `platformio.ini` configuration.

## Calibration Procedure

### Static Calibration (Magnetometer + Accelerometer)

1. Enter calibration mode via device menu
2. Rotate device through 375 predetermined positions:
   - 25 different headings (every 14.4°)
   - 15 different inclinations (0° to 90°)
3. Device collects sensor readings at each position
4. Algorithm computes:
   - Calibration matrices (Ra_cal, Rm_cal)
   - Bias vectors (ba_cal, bm_cal)
   - Alignment rotation matrix (Rm_align)
5. Calibration automatically saved to NVS

### Laser Alignment

1. Enter laser calibration mode
2. Point device at fixed target at known distance
3. Rotate device through 8 positions
4. Algorithm computes laser-sensor alignment matrices (Ra_las, Rm_las)

### Calibration Analysis

Use Python scripts to validate calibration quality:

```bash
# Evaluate calibration matrices
python calibration_evaluation.py

# Analyze radial offset errors
python radial_offset_analysis.py

# Check calibration symmetry
python symmetry_analysis.py
```

These scripts consume JSON calibration data dumped from the device via `SensorHandler::dumpCalibToSerial()`.

## Usage

### Taking a Measurement

1. Power on device (enters idle mode)
2. Aim at target point
3. Wait for stabilization indicator
4. Press measurement button
5. Device averages 100 samples and displays results
6. Measurement automatically saved to device memory

### Reviewing Measurements

1. Enter history mode via menu
2. Navigate through stored shots
3. View heading, inclination, roll, distance
4. Optional: Delete individual shots

### BLE Data Transfer

1. Enable BLE on companion device
2. Pair with BeanBoi device
3. Measurements automatically transmitted
4. Retrieve shot history wirelessly

## Development

### Project Structure

```
BeanBoi/
├── lib/                          # Libraries
│   ├── CaveSurveyDevice/        # Core device logic
│   ├── NumericalMethods/        # Calibration algorithms
│   ├── ESPFilesystem/           # NVS persistence
│   ├── BLE/                     # Bluetooth manager
│   ├── Eigen/                   # Linear algebra library
│   └── [Sensor Libraries]       # RM3100, SCA3300, LDK_2M, OLED
├── src/                         # Main application code
│   ├── main.cpp                 # Entry point
│   ├── FreeRTOS2.cpp           # Task implementation & state machine
│   ├── programflow.cpp         # High-level control flow
│   ├── display_funcs.cpp       # OLED display rendering
│   └── [Sensor Connections]    # Hardware implementations
├── platformio.ini              # Build configuration
└── [Analysis Scripts]          # Python calibration tools
```

### Key Files

- `SensorHandler.cpp` (743 lines): Core calibration and measurement logic
- `InertialAlignment.cpp`: Magnetometer-accelerometer calibration
- `LaserAlignment.cpp`: Laser-sensor alignment
- `FreeRTOS2.cpp`: State machine and button handling
- `ble_manager.cpp`: BLE communication

### Debugging

Enable debug output in `lib/CaveSurveyDevice/debug_csd.h`:

```cpp
#define DEBUG_SENSOR_ENA    // Sensor readings
#define DEBUG_MAIN_ENA      // Main program flow
#define DEBUG_BLE_ENA       // BLE operations
#define DEBUG_HEAP_ENA      // Memory usage
```

Use debug macros in code:
```cpp
Debug_csd::debug(DEBUG_SENSOR, "Sensor initialized");
Debug_csd::debugf(DEBUG_MAIN, "Heading: %.2f", heading);
```

### Build System

Uses custom ESP32 platform fork for Arduino 3.x and ESP-IDF 5.x compatibility:
```ini
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
```

**Platform Requirements (Recommended vs Required):**

The custom platform is **recommended** but not strictly required. Here's what it provides:

| Feature | Custom Platform | Standard Platform | Status |
|---------|----------------|-------------------|--------|
| **Arduino Core** | 3.x | 2.x | **REQUIRED** - Code uses Arduino 3.x timer API |
| **ESP-IDF** | 5.x | 4.x | Recommended (current code works with 4.x) |
| **FreeRTOS** | 10.4.0+ | 10.2.x | Compatible (uses standard API, not indexed) |
| **Compiler** | GCC 11.2+ | GCC 8.4 | Either works (GCC 8.4+ sufficient) |
| **Arduino API** | `timerBegin(freq)` | `timerBegin(timer, prescaler, countUp)` | **CRITICAL** - See note below |

**Critical Dependencies:**

✅ **Arduino 3.x is REQUIRED** - The code uses `timerBegin(DISPLAY_TMR_HZ)` which is the Arduino 3.x API
- Arduino 2.x uses `timerBegin(timer, prescaler, countUp)` - **will cause compilation errors**
- This is why the custom platform is recommended - it provides Arduino 3.x

⚠️ **ESP-IDF 4.x may work** - If you remove `CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES` from `sdkconfig.defaults`
- Current code doesn't use indexed notifications
- Only requires standard FreeRTOS 10.x features (available in ESP-IDF 4.x)
- However, untested - custom platform is recommended

## Calibration Constants

Defined in `lib/CaveSurveyDevice/SensorHandler.h`:

```cpp
N_ALIGN_MAG_ACC = 375    // Static calibration samples (25×15 positions)
N_LASER_CAL = 8          // Laser alignment samples
N_SHOT_SAMPLES = 100     // Measurement averaging samples
STDEV_LIMIT = 0.05       // Stabilization threshold
```

## Performance Characteristics

- **Measurement Rate**: Up to 5 Hz (limited by display refresh)
- **Stabilization Time**: Typically 1-2 seconds
- **Calibration Time**: ~10 minutes for full static calibration
- **Shot Storage**: Limited by ESP32 NVS capacity (~500KB available)
- **BLE Range**: Typical Bluetooth range (10-30 meters)

## Troubleshooting

### Build Issues

**Problem**: "Platform espressif32 not found" or download fails
- **Solution**: Check internet connection. PlatformIO will download ~500MB on first build.

**Problem**: "undefined reference to xTaskCreatePinnedToCore"
- **Solution**: Very unlikely - this function exists in both ESP-IDF 4.x and 5.x. Check your platform installation.

**Problem**: Stack overflow crashes or WDT resets
- **Solution**: Check FreeRTOS configuration in `sdkconfig.defaults`. Ensure `CONFIG_FREERTOS_HZ=1000` is set.

### Runtime Issues

**Problem**: Device resets during calibration
- **Solution**: Increase `TaskStackSizes::COMPUTE` in `FreeRTOS2.h` if you modified Eigen code

**Problem**: Compilation fails with "CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES undeclared"
- **Solution**: Remove this line from `sdkconfig.defaults` - it's not required for current code. Only needed if using indexed notifications.
- **Alternative**: Use the custom platform which includes ESP-IDF 5.x where this option exists.

**Problem**: Timer compilation errors (`timerBegin` wrong number of arguments)
- **Solution**: **CRITICAL** - Arduino 3.x changed the timer API. You must use Arduino 3.x (custom platform) or modify timer code for Arduino 2.x.
- **Root cause**: Code uses `timerBegin(freq)` (Arduino 3.x), but Arduino 2.x expects `timerBegin(timer, prescaler, countUp)`

**Problem**: "undefined reference to xTaskCreatePinnedToCore"
- **Solution**: Very unlikely - this function exists in both ESP-IDF 4.x and 5.x. Check your platform installation.
- **Solution**: Increase `TaskStackSizes::COMPUTE` in `FreeRTOS2.h` if you modified Eigen code

**Problem**: BLE not connecting
- **Solution**: Check NimBLE is enabled and Core 1 task is running. Monitor serial output for BLE initialization messages.

**Problem**: Sensor initialization fails
- **Solution**: Check SPI wiring. Verify sensors are powered. Enable `DEBUG_SENSOR_ENA` in `debug_csd.h`.

**Problem**: Heap allocation failures
- **Solution**: Enable `DEBUG_HEAP_ENA` to monitor memory usage. Large calibration matrices may fragment heap.

### Upload Issues

**Problem**: "Could not open port"
- **Solution**: Ensure no other programs (including other monitor instances) have the serial port open

**Problem**: "Timed out waiting for packet header"
- **Solution**: Put ESP32 into bootloader mode manually (hold BOOT button while pressing RESET)

### Calibration Quality

**Problem**: Large residuals in calibration_evaluation.py
- **Solution**: Repeat calibration with more precise positioning. Ensure no magnetic interference.

**Problem**: Non-orthogonal calibration matrices
- **Solution**: Check sensor mechanical alignment. Verify rotation matrix computations in `InertialAlignment.cpp`.

## Contributing

Contributions welcome! Areas for improvement:

- Additional sensor support
- Enhanced calibration algorithms
- Power optimization
- Companion app development
- Documentation improvements

## License

[License information to be added]

## Acknowledgments

- Inspired by DistoX and SAP5 cave surveying devices
- Built with PlatformIO and Arduino framework
- Uses Eigen library for numerical computation
- Waveshare OLED driver library

## References

- [RM3100 Magnetometer Datasheet](https://www.pnicorp.com/rm3100/)
- [SCA3300 Inclinometer Datasheet](https://www.murata.com/products/sensor/inclinometer/sca3300)
- [Eigen Linear Algebra Library](https://eigen.tuxfamily.org/)
- [ESP32 Technical Reference](https://www.espressif.com/en/products/socs/esp32)

## Contact

[Contact information to be added]

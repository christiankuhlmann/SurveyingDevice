# BeanBoi Cave Surveying Device - AI Agent Instructions

## Project Overview
Embedded ESP32 firmware for a precision cave surveying instrument that measures heading, inclination, roll, and distance using magnetometer (RM3100), accelerometer (SCA3300), and laser rangefinder (LDK_2M). The device performs sophisticated sensor calibration and alignment using Eigen linear algebra, displays data on a Waveshare OLED, and transmits measurements via BLE.

## Architecture

### Core Components
- **Hardware Abstraction**: Sensor interfaces (`Accelerometer`, `Magnetometer`, `Laser`) in `lib/CaveSurveyDevice/Sensors.h` with concrete implementations in `src/*SensorConnection.h` (e.g., `RM3100SensorConnection`, `SCA3300SensorConnection`, `LDK2MSensorConnection`)
- **Sensor Processing**: `SensorHandler` class (`lib/CaveSurveyDevice/SensorHandler.h`) orchestrates calibration, alignment, and measurement correction
- **Numerical Methods**: `lib/NumericalMethods/` contains Eigen-based algorithms for inertial alignment (`InertialAlignment.h`), laser alignment (`LaserAlignment.h`), and curve fitting (`FittingFuncs.h`)
- **State Machine**: FreeRTOS tasks manage device states (`DeviceStateEnum`) and display modes (`DisplayModeEnum`) defined in `src/FreeRTOS2.h`
- **Persistence**: `lib/ESPFilesystem/FileFuncs.h` uses ESP32 Preferences (NVS) for calibration data and shot storage

### Data Flow
1. Raw sensor readings → `SensorHandler::update()` → apply calibration matrices (Ra_cal, Rm_cal) and bias vectors (ba_cal, bm_cal)
2. Static calibration: Collect N_ALIGN_MAG_ACC samples → `alignMagAcc()` → compute rotation matrix (Rm_align) and inclination angle
3. Laser calibration: Collect N_LASER_CAL samples → `alignLaser()` → compute alignment matrices (Ra_las, Rm_las)
4. Measurement: `takeShot()` → stabilize → read sensors → apply corrections → convert to Cardan angles or Cartesian coordinates

### FreeRTOS Task Structure
- **Core 0**: `computehandler` (sensor processing), `inputhandler` (button interrupts), `displayhandler` (5Hz OLED refresh)
- **Core 1**: `BLETask` (Bluetooth measurement transmission)
- Button actions trigger state transitions via `executeAction()` in `src/FreeRTOS2.cpp`

## Development Workflow

### Build & Upload
```powershell
# PlatformIO commands (uses custom ESP32 platform fork)
pio run -e featheresp32          # Build firmware
pio run -t upload                 # Upload to device
pio device monitor                # Serial monitor (115200 baud)
pio run -t upload ; pio device monitor  # Upload + monitor
```

### Debugging
- Serial debugging controlled by `lib/CaveSurveyDevice/debug_csd.h` flags:
  - `DEBUG_SENSOR_ENA`, `DEBUG_MAIN_ENA`, `DEBUG_BLE_ENA`, etc.
  - Use `Debug_csd::debug(DEBUG_SENSOR, "message")` or `Debug_csd::debugf()` for formatted output
- Exception decoder enabled in `platformio.ini` via `monitor_filters = esp32_exception_decoder`
- Heap monitoring: `DEBUG_HEAP_ENA` tracks memory usage

### Calibration Analysis
Python scripts in root directory analyze calibration quality:
- `calibration_evaluation.py`: Validates calibration matrices (orthogonality, determinant, residuals)
- `radial_offset_analysis.py`, `symmetry_analysis.py`: Sensor error characterization
- These consume JSON data dumped from `SensorHandler::dumpCalibToSerial()`

## Critical Conventions

### Eigen Linear Algebra
- All vector/matrix types use `Eigen::Vector3f`, `Eigen::Matrix3f` (float precision for ESP32)
- Include `<ArduinoEigen.h>` or specific headers like `<ArduinoEigenDense.h>`
- Common operations: `.normalized()`, `.cross()`, `.transpose()`, Quaternion rotations via `quatRot()`
- Calibration correction: `corrected = R * (raw - bias)` pattern throughout `SensorHandler`

### File I/O Patterns
- Namespace-based storage: `FileFuncs::writeToFile("SD001", "heading", value)`
- Shot data stored sequentially: `saveShotData()` increments counter, `readShotData()` retrieves by ID
- Calibration persistence: `SensorHandler::saveCalibration()` writes all matrices/biases to NVS

### State Management
- Device states: `MODE_IDLE`, `MODE_LASER_ON`, `MODE_CALIB`, `MODE_HISTORY`, etc. (see `DeviceStateEnum`)
- Display modes: `DISP_IDLE`, `DISP_SHOT_TAKEN`, `DISP_CALIBRATION`, etc. (see `DisplayModeEnum`)
- Transitions in `executeAction()` based on button IDs (ID_B1-B5) and long/short press detection
- Menu system: `OLED::MenuEnum` defines navigation tree in `src/display_funcs.h`

### Calibration Constants
- `N_ALIGN_MAG_ACC = 375`: Static calibration samples (25 heading × 15 inclination positions)
- `N_LASER_CAL = 8`: Laser alignment samples
- `N_SHOT_SAMPLES = 100`: Measurement averaging
- `STDEV_LIMIT = 0.05`: Stabilization threshold
- Defined in `lib/CaveSurveyDevice/SensorHandler.h`

## Common Tasks

### Adding a Sensor
1. Create interface implementation extending `Accelerometer`/`Magnetometer`/`Laser` in `src/`
2. Implement `getMeasurement()` and `init()` methods
3. Instantiate in `src/programflow.cpp` global scope
4. Pass to `SensorHandler` constructor

### Modifying Calibration Algorithm
1. Edit `lib/NumericalMethods/InertialAlignment.cpp` or `LaserAlignment.cpp`
2. Algorithms use least-squares fitting (`Eigen::JacobiSVD`) to solve overdetermined systems
3. Test with Python scripts by dumping calibration data to serial

### Adding Display Elements
1. Use `OLED::DisplayHandler` in `src/display_funcs.h` (wraps Waveshare library)
2. Drawing primitives: `drawPixel()`, `drawLine()`, `drawString()`, `fillRect()`
3. Top bar reserved: `TOP_BAR_HEIGHT = 16` pixels for status icons
4. Update in `displayhandler` task or specific display functions in `src/programflow.cpp`

### BLE Communication
- Measurements sent via `sendBLEData(MeasurementData)` to queue
- BLE task consumes queue and notifies characteristics (heading, inclination, roll, distance)
- NimBLE library configured in `lib/BLE/ble_manager.cpp`

## Key Files Reference
- `src/main.cpp`: Entry point, FreeRTOS task creation
- `src/FreeRTOS2.cpp`: State machine, button handling, task implementations
- `lib/CaveSurveyDevice/SensorHandler.cpp`: Core calibration/measurement logic (743 lines)
- `lib/NumericalMethods/utils.h`: Rotation matrices, quaternions, ENU frame transformations
- `platformio.ini`: Uses custom ESP32 platform from `github.com/pioarduino/platform-espressif32`

## Gotchas
- **Custom Platform**: Don't replace PlatformIO platform URL with standard `espressif32` - requires Arduino 3.x features
- **Float Precision**: ESP32 FPU handles floats efficiently; use `float` not `double` in Eigen types
- **Stack Sizes**: Tasks have generous stacks (100k for compute, 2.5k for input/display) due to Eigen allocations
- **NVS Limits**: Preference namespace names limited to 15 chars, key names to 15 chars
- **Sensor Init Order**: Call `sh.init()` after sensor instantiation but before calibration loading
